// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "oscquery_server_asio.hpp"

#include <ossia/detail/algorithms.hpp>
#include <ossia/detail/mutex.hpp>
#include <ossia/detail/string_map.hpp>
#include <ossia/network/base/node_functions.hpp>
#include <ossia/network/base/osc_address.hpp>
#include <ossia/network/common/network_logger.hpp>
#include <ossia/network/common/node_visitor.hpp>
#include <ossia/network/context.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/generic/generic_node.hpp>
#include <ossia/network/generic/generic_parameter.hpp>
#include <ossia/network/osc/detail/osc.hpp>
#include <ossia/network/osc/detail/osc_1_1_extended_policy.hpp>
#include <ossia/network/osc/detail/osc_packet_processor.hpp>
#include <ossia/network/osc/detail/osc_receive.hpp>
#include <ossia/network/osc/detail/osc_value_write_visitor.hpp>
#include <ossia/network/osc/detail/receiver.hpp>
#include <ossia/network/osc/detail/sender.hpp>
#include <ossia/network/oscquery/detail/get_query_parser.hpp>
#include <ossia/network/oscquery/detail/json_query_parser.hpp>
#include <ossia/network/oscquery/detail/json_writer.hpp>
#include <ossia/network/oscquery/detail/osc_writer.hpp>
#include <ossia/network/oscquery/detail/oscquery_protocol_common.hpp>
#include <ossia/network/oscquery/detail/outbound_visitor.hpp>
#include <ossia/network/oscquery/detail/query_parser.hpp>
#include <ossia/network/sockets/udp_socket.hpp>
#include <ossia/network/sockets/websocket_server_beast.hpp>
#include <ossia/protocols/oscquery/oscquery_client_asio.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
namespace ossia::oscquery_asio
{

struct oscquery_server_protocol::osc_receiver_impl : ossia::net::udp_receive_socket
{
  using udp_receive_socket::udp_receive_socket;
};

static uintptr_t
client_identifier(const clients& clts, const oscpack::IpEndpointName& ip)
{
  for(const auto& client_p : clts)
  {
    auto& c = *client_p;
    if(c.remote_sender_port == ip.port)
    {
      return (uintptr_t)c.connection.lock().get();
    }
  }
  return {};
}

static bool
is_same(const oscquery_client& clt, const ossia::net::message_origin_identifier& id)
{
  return (uintptr_t)clt.connection.lock().get() == id.identifier;
}

oscquery_server_protocol_base::oscquery_server_protocol_base(
    ossia::net::network_context_ptr ctx,
    const std::vector<ossia::net::osc_server_configuration>& conf, uint16_t ws_port,
    bool forceWS, std::string bind)
    : protocol_base{flags{SupportsMultiplex}}
    , m_context{std::move(ctx)}
    , m_websocketServer{std::make_unique<ossia::net::websocket_server_beast>(m_context)}
    , m_oscConf{conf}
    , m_wsPort{ws_port}
    , m_bind{std::move(bind)}
    , m_forceWS{forceWS}
{
  m_clients.reserve(2);
  m_websocketServer->set_open_handler(
      [&](ossia::net::ws_connection_handle hdl) { on_connectionOpen(hdl); });
  m_websocketServer->set_close_handler(
      [&](ossia::net::ws_connection_handle hdl) { on_connectionClosed(hdl); });
  m_websocketServer->set_message_handler(
      [&](const ossia::net::ws_connection_handle& hdl, ossia::net::ws_opcode op,
          const std::string& str) {
    switch(op)
    {
      case ossia::net::ws_opcode::text: {
        auto res = on_text_ws_message(hdl, str);

        if(!res.data.empty() && res.type != ossia::net::server_reply::data_type::binary
           && m_logger.outbound_logger)
          m_logger.outbound_logger->info("OSCQuery WS Out: {}", res.data);

        return res;
      }
      case ossia::net::ws_opcode::binary: {
        return on_binary_ws_message(hdl, str);
      }
      default:
        return ossia::net::server_reply{};
    }
  });
}

oscquery_server_protocol_base::~oscquery_server_protocol_base()
{
  if(m_device)
  {
    auto& dev = *m_device;
    dev.on_node_created.disconnect<&oscquery_server_protocol_base::on_nodeCreated>(this);
    dev.on_node_removing.disconnect<&oscquery_server_protocol_base::on_nodeRemoved>(
        this);
    dev.on_parameter_created
        .disconnect<&oscquery_server_protocol_base::on_parameterChanged>(this);
    dev.on_parameter_removing
        .disconnect<&oscquery_server_protocol_base::on_parameterChanged>(this);
    dev.on_attribute_modified
        .disconnect<&oscquery_server_protocol_base::on_attributeChanged>(this);
    dev.on_node_renamed.disconnect<&oscquery_server_protocol_base::on_nodeRenamed>(this);
  }
  stop();
}

bool oscquery_server_protocol_base::pull(net::parameter_base&)
{
  //! The server cannot pull because it can have multiple clients.
  return false;
}

std::future<void> oscquery_server_protocol_base::pull_async(net::parameter_base&)
{
  // Do nothing
  return {};
}

void oscquery_server_protocol_base::request(net::parameter_base&)
{
  // Do nothing
}

struct ws_client_socket
{
  ossia::net::websocket_server_interface& server;
  ossia::net::ws_connection_handle& connection;

  void send_binary_message(std::string_view s)
  {
    server.send_binary_message(connection, s);
  }
};

void oscquery_server_protocol_base::write_to(
    oscquery_client& client, std::string_view data, bool critical)
{
  if(!critical && client.osc_socket)
    client.osc_socket->write(data.data(), data.size());
  else
    m_websocketServer->send_binary_message(client.connection, data);
}

// A bundle holds several addresses, and each client gets those it listens to:
// the clients that get them all share one bundle.
template <typename Elements, typename AddressOf, typename Send>
bool oscquery_server_protocol_base::write_to_listeners(
    Elements& elements, AddressOf address_of, Send send)
{
  using element = std::decay_t<decltype(*std::begin(elements))>;
  lock_t lock(m_clientsMutex);
  bool ok = true;
  std::vector<oscquery_client*> whole;
  for(auto& client_p : m_clients)
  {
    auto& client = *client_p;
    std::vector<element> subset;
    for(auto& e : elements)
      if(client.listens_to(address_of(e)))
        subset.push_back(e);

    if(subset.size() == std::size(elements))
      whole.push_back(&client);
    else if(!subset.empty())
      ok &= send(subset, [&](std::string_view data, bool critical) {
        write_to(client, data, critical);
        return true;
      });
  }
  if(!whole.empty())
    ok &= send(elements, [&](std::string_view data, bool critical) {
      for(auto* client : whole)
        write_to(*client, data, critical);
      return true;
    });
  return ok;
}

template <typename T>
bool oscquery_server_protocol_base::push_impl(const T& addr, const ossia::value& v)
{
  using namespace ossia::net;
  using namespace ossia::oscquery;
  using proto = oscquery_protocol_common<osc_extended_policy>;

  // TODO if we have multiple clients, likely better to generate the message
  // once....
  auto val = bound_value(addr, v);
  if(val.valid())
  {
    const std::string_view address = ossia::net::osc_address(addr);
    record_sent(std::string{address}, val);
    // Push to the clients that listen to it
    auto critical = m_forceWS || addr.get_critical();
    if(!critical)
    {
      lock_t lock(m_clientsMutex);
      for(auto& client_p : m_clients)
      {
        auto& client = *client_p;
        if(!client.listens_to(address))
          continue;
        if(client.osc_socket)
        {
          if(m_logger.outbound_logger)
          {
            m_logger.outbound_logger->info(
                "Out: {} {}", ossia::net::osc_address(addr), val);
          }

          proto::osc_send_message(*this, *client.osc_socket, addr, val);
        }
        else
        {
          if(m_logger.outbound_logger)
          {
            m_logger.outbound_logger->info(
                "Out: {} {}", ossia::net::osc_address(addr), val);
          }

          proto::ws_send_binary_message(
              *this, ws_client_socket{*m_websocketServer, client.connection}, addr, val);
        }
      }
    }
    else
    {
      lock_t lock(m_clientsMutex);
      for(auto& client_p : m_clients)
      {
        auto& client = *client_p;
        if(!client.listens_to(address))
          continue;
        if(m_logger.outbound_logger)
        {
          m_logger.outbound_logger->info(
              "Out: {} {}", ossia::net::osc_address(addr), val);
        }
        proto::ws_send_binary_message(
            *this, ws_client_socket{*m_websocketServer, client.connection}, addr, val);
      }
    }

    return true;
  }
  return false;
}

bool oscquery_server_protocol_base::push(
    const net::parameter_base& addr, const ossia::value& v)
{
  return push_impl(addr, v);
}

bool oscquery_server_protocol_base::push_raw(const net::full_parameter_data& addr)
{
  return push_impl(addr, addr.value());
}

bool oscquery_server_protocol_base::push_bundle(
    const std::vector<const ossia::net::parameter_base*>& addresses)
{
  using namespace ossia::net;
  using OscVersion = osc_extended_policy;

  for(auto* p : addresses)
    record_sent(ossia::net::osc_address(*p), p->value());
  return write_to_listeners(
      addresses, [](auto* p) -> std::string_view { return ossia::net::osc_address(*p); },
      [](const auto& list, auto write) {
    if(auto bundle
       = ossia::net::make_bundle(ossia::net::bundle_client_policy<OscVersion>{}, list))
      return write(
          std::string_view{bundle->data.data(), bundle->data.size()}, bundle->critical);
    return false;
      });
}

bool oscquery_server_protocol_base::push_raw_bundle(
    const std::vector<ossia::net::full_parameter_data>& addresses)
{
  using namespace ossia::net;
  using OscVersion = osc_extended_policy;

  for(auto& p : addresses)
    record_sent(ossia::net::osc_address(p), p.value());
  return write_to_listeners(
      addresses, [](auto& p) -> std::string_view { return ossia::net::osc_address(p); },
      [](const auto& list, auto write) {
    if(auto bundle
       = ossia::net::make_bundle(ossia::net::bundle_client_policy<OscVersion>{}, list))
      return write(
          std::string_view{bundle->data.data(), bundle->data.size()}, bundle->critical);
    return false;
      });
}

// A bundle of values, each element with its own value (a parameter may come
// several times). Sent to the clients in OSC bundles of at most 1400 bytes:
// over UDP they stay under a usual Ethernet MTU.
template <typename Policy>
static bool push_values_bundle(
    std::span<ossia::bundle_element> elements, Policy policy,
    const std::function<bool(std::string_view, bool)>& write)
{
  constexpr std::size_t header = 16; // "#bundle\0" and the time tag
  constexpr std::size_t limit = 1400;
  bool ok = !elements.empty();
  std::size_t begin = 0, size = header;
  auto send = [&](std::size_t end) {
    if(end == begin)
      return;
    ossia::net::make_bundle_bounded(
        policy, std::span<ossia::bundle_element>(elements.data() + begin, end - begin),
        [&](const ossia::net::bundle& b) {
      ok &= write(std::string_view{b.data.data(), b.data.size()}, b.critical);
        });
    begin = end;
    size = header;
  };
  for(std::size_t i = 0; i < elements.size(); i++)
  {
    auto& [param, value] = elements[i];
    const std::size_t message
        = 4 + ossia::oscquery::osc_writer::to_message(*param, value).size();
    if(i > begin && size + message > limit)
      send(i);
    size += message;
  }
  send(elements.size());
  return ok;
}

bool oscquery_server_protocol_base::push_bundle(std::span<ossia::bundle_element> elements)
{
  using OscVersion = ossia::net::osc_extended_policy;
  for(auto& [param, value] : elements)
    record_sent(ossia::net::osc_address(*param), value);
  return write_to_listeners(
      elements,
      [](auto& e) -> std::string_view { return ossia::net::osc_address(*e.parameter); },
      [](auto& list, auto write) {
    return push_values_bundle(
        std::span<ossia::bundle_element>(list), ossia::net::bundle_server_policy<OscVersion>{}, write);
      });
}

bool oscquery_server_protocol_base::push_bundle_bounded(
    std::span<ossia::bundle_element> elements)
{
  using OscVersion = ossia::net::osc_extended_policy;
  for(auto& [param, value] : elements)
    record_sent(ossia::net::osc_address(*param), value);
  return write_to_listeners(
      elements,
      [](auto& e) -> std::string_view { return ossia::net::osc_address(*e.parameter); },
      [](auto& list, auto write) {
    return push_values_bundle(
        std::span<ossia::bundle_element>(list), ossia::net::bundle_bounded_server_policy<OscVersion>{}, write);
      });
}

bool oscquery_server_protocol_base::echo_incoming_message(
    const net::message_origin_identifier& id, const net::parameter_base& addr,
    const ossia::value& val)
{
  using namespace ossia::net;

  using send_visitor = osc_value_send_visitor<
      net::parameter_base, osc_extended_policy, socket_writer<udp_send_socket>>;

  bool not_this_protocol = &id.protocol != this;
  const std::string_view address = ossia::net::osc_address(addr);
  record_sent(std::string{address}, val, not_this_protocol ? 0 : id.identifier);
  // we know that the value is valid
  // Push to all clients except ours
  auto critical = addr.get_critical();
  if(!critical)
  {
    lock_t lock(m_clientsMutex);
    // No need to echo if we just have one client, the most common case
    for(auto& client_p : m_clients)
    {
      auto& client = *client_p;
      if(!client.listens_to(address))
        continue;
      if(not_this_protocol || !is_same(client, id))
      {
        if(client.osc_socket)
        {
          if(m_logger.outbound_logger)
          {
            m_logger.outbound_logger->info(
                "Out: {} {}", addr.get_node().osc_address(), val);
          }
          send_visitor vis{addr, ossia::net::osc_address(addr), {*client.osc_socket}};
          val.apply(vis);
        }
        else
        {
          if(m_logger.outbound_logger)
          {
            m_logger.outbound_logger->info(
                "Out: {} {}", addr.get_node().osc_address(), val);
          }
          m_websocketServer->send_binary_message(
              client.connection, ossia::oscquery::osc_writer::to_message(addr, val));
        }
      }
    }
  }
  else
  {
    lock_t lock(m_clientsMutex);

    for(auto& client_p : m_clients)
    {
      auto& client = *client_p;
      if(!client.listens_to(address))
        continue;
      if(not_this_protocol || !is_same(client, id))
      {

        if(m_logger.outbound_logger)
        {
          m_logger.outbound_logger->info(
              "Out: {} {}", addr.get_node().osc_address(), val);
        }

        m_websocketServer->send_binary_message(
            client.connection, ossia::oscquery::osc_writer::to_message(addr, val));
      }
    }
  }

  return true;
}

bool oscquery_server_protocol_base::observe(net::parameter_base& address, bool enable)
{
  if(enable)
  {
    m_listening.insert(std::make_pair(address.get_node().osc_address(), &address));
  }
  else
  {
    m_listening.erase(address.get_node().osc_address());
  }

  return true;
}

bool oscquery_server_protocol_base::observe_quietly(net::parameter_base& addr, bool b)
{
  return observe(addr, b);
}

bool oscquery_server_protocol_base::update(net::node_base&)
{
  return false;
}

void oscquery_server_protocol_base::set_device(net::device_base& dev)
{
  if(m_device)
  {
    auto& old = *m_device;
    old.on_node_created.disconnect<&oscquery_server_protocol_base::on_nodeCreated>(this);
    old.on_node_removing.disconnect<&oscquery_server_protocol_base::on_nodeRemoved>(
        this);
    dev.on_parameter_created
        .disconnect<&oscquery_server_protocol_base::on_parameterChanged>(this);
    dev.on_parameter_removing
        .disconnect<&oscquery_server_protocol_base::on_parameterChanged>(this);
    old.on_attribute_modified
        .disconnect<&oscquery_server_protocol_base::on_attributeChanged>(this);
    old.on_node_renamed.disconnect<&oscquery_server_protocol_base::on_nodeRenamed>(this);

    ossia::net::visit_parameters(
        old.get_root_node(),
        [&](ossia::net::node_base& n, ossia::net::parameter_base& p) {
      if(p.callback_count() > 0)
        observe(p, false);
        });
  }
  m_device = &dev;

  dev.on_node_created.connect<&oscquery_server_protocol_base::on_nodeCreated>(this);
  dev.on_node_removing.connect<&oscquery_server_protocol_base::on_nodeRemoved>(this);
  dev.on_parameter_created.connect<&oscquery_server_protocol_base::on_parameterChanged>(
      this);
  dev.on_parameter_removing.connect<&oscquery_server_protocol_base::on_parameterChanged>(
      this);
  dev.on_attribute_modified.connect<&oscquery_server_protocol_base::on_attributeChanged>(
      this);
  dev.on_node_renamed.connect<&oscquery_server_protocol_base::on_nodeRenamed>(this);

  update_zeroconf();

  ossia::net::visit_parameters(
      dev.get_root_node(), [&](ossia::net::node_base& n, ossia::net::parameter_base& p) {
        if(p.callback_count() > 0)
          observe(p, true);
      });

  if(bound_to_every_interface())
    m_websocketServer->listen(m_wsPort);
  else
    m_websocketServer->listen(m_bind, m_wsPort);
}

void oscquery_server_protocol_base::stop()
{
  // Withdraw the announcement first: clients that look the server up by name
  // stop finding it before its connections close, and a server stopped but
  // not destroyed is no longer announced.
  m_zeroconfServerWS = {};

  if(m_websocketServer)
  {
    try
    {
      lock_t lock(m_clientsMutex);
      // close client-connections before stopping
      auto it = m_clients.begin();
      while(it != m_clients.end())
      {
        m_websocketServer->close((*it)->connection);
        it = m_clients.erase(it);
      }
      m_clientCount = 0;
    }
    catch(...)
    {
      logger().error("Error when freeing connections");
    }

    try
    {
      m_websocketServer->stop();
      m_websocketServer.reset();
    }

    catch(...)
    {
      logger().error("Error when stopping WS server");
    }
  }
}

void oscquery_server_protocol_base::set_force_ws(bool forceWS) noexcept
{
  m_forceWS.store(forceWS, std::memory_order_relaxed);
}

oscquery_client*
oscquery_server_protocol_base::find_client(const connection_handler& hdl)
{
  lock_t lock(m_clientsMutex);

  auto it = ossia::find_if(m_clients, [&](auto& clt) { return *clt == hdl; });
  if(it != m_clients.end())
    return it->get();
  return nullptr;
}

using map_setter_fun = void (*)(
    const std::pair<const std::string, std::string>& str, ossia::net::parameter_data&);
template <typename Attr>
static auto make_setter_pair(Attr)
{
  return std::make_pair(
      ossia::oscquery::detail::metadata<Attr>::key(),
      [](const std::pair<const std::string, std::string>& str,
         ossia::net::parameter_data& addr) {
    ossia::oscquery::query_parser::parse(Attr{}, str.second, addr);
      });
}

using query_reader_map_type = string_view_map<map_setter_fun>;

static auto& querySetterMap()
{
  static const query_reader_map_type map{[] {
    query_reader_map_type attr_impl;
    using namespace ossia::net;

    // Remaining metadata
    ossia::for_each_tagged(ossia::oscquery::detail::all_attributes{}, [&](auto attr) {
      using type = typename decltype(attr)::type;
      attr_impl.insert(make_setter_pair(type{}));
    });

    return attr_impl;
  }()};

  return map;
}

void oscquery_server_protocol_base::add_node(
    std::string_view parent_path, const string_map<std::string>& parameters)
{
  // /foo/bar/baz?ADD_NODE=tutu&VALUE="[hi]"
  net::parameter_data address;
  auto& map = querySetterMap();

  const auto& name_it = parameters.find(ossia::oscquery::detail::add_node());
  if(name_it != parameters.end())
  {
    address.name = name_it->second;
  }

  for(const auto& e : parameters)
  {
    auto it = map.find(e.first);
    if(it != map.end())
    {
      it->second(e, address);
    }
    else
    {
      // TODO add as an extended attribute.
    }
  }

  m_device->on_add_node_requested(std::string(parent_path), address);
}

void oscquery_server_protocol_base::remove_node(
    std::string_view path, const std::string& node)
{
  m_device->on_remove_node_requested(std::string(path), node);
}

void oscquery_server_protocol_base::rename_node(
    std::string_view path, const std::string& new_name)
{
  m_device->on_rename_node_requested(std::string(path), new_name);
}

void oscquery_server_protocol_base::process_raw_osc_data(
    const char* data, std::size_t sz, uintptr_t origin)
{
  if(origin)
  {
    lock_t lock(m_clientsMutex);
    for(auto& client_p : m_clients)
    {
      if((uintptr_t)client_p->connection.lock().get() == origin)
      {
        client_p->packets++;
        break;
      }
    }
  }
  auto on_message = [this, origin](auto&& msg) { this->on_osc_message(msg, origin); };
  ossia::net::osc_packet_processor<decltype(on_message)>{on_message}(data, sz);
}

void oscquery_server_protocol_base::record_sender(const std::string& ip, uint16_t port)
{
  std::lock_guard lock{m_sendersMutex};
  auto& st = m_senders[ip + ":" + std::to_string(port)];
  st.ip = ip;
  st.port = port;
  st.packets++;
  st.last = std::chrono::steady_clock::now();
}

bool oscquery_server_protocol_base::on_prism_name(
    const connection_handler& hdl, const rapidjson::Document& doc)
{
  if(!doc.IsObject())
    return false;
  auto cmd = doc.FindMember("COMMAND");
  if(cmd == doc.MemberEnd() || !cmd->value.IsString()
     || std::string_view{cmd->value.GetString()} != "PRISM_NAME")
    return false;
  auto data = doc.FindMember("DATA");
  if(data == doc.MemberEnd() || !data->value.IsString())
    return true;
  lock_t lock(m_clientsMutex);
  for(auto& client_p : m_clients)
  {
    if(*client_p == hdl)
    {
      client_p->name = data->value.GetString();
      break;
    }
  }
  return true;
}

std::string oscquery_server_protocol_base::query_connections()
{
  rapidjson::StringBuffer buf;
  rapidjson::Writer<rapidjson::StringBuffer> wr{buf};
  wr.StartObject();
  wr.Key("CLIENTS");
  wr.StartArray();
  {
    lock_t lock(m_clientsMutex);
    for(auto& client_p : m_clients)
    {
      auto& c = *client_p;
      wr.StartObject();
      wr.Key("IP");
      wr.String(c.client_ip.c_str());
      wr.Key("PORT");
      wr.Int(c.client_port);
      if(!c.name.empty())
      {
        wr.Key("NAME");
        wr.String(c.name.c_str());
      }
      if(c.osc_port)
      {
        wr.Key("OSC_PORT");
        wr.Int(c.osc_port);
      }
      if(c.remote_sender_port)
      {
        wr.Key("SENDER_PORT");
        wr.Int(c.remote_sender_port);
      }
      wr.Key("LISTEN");
      wr.StartArray();
      {
        std::lock_guard lck{c.listeningMutex};
        std::vector<std::string> paths;
        for(auto& [path, param] : c.listening)
          paths.push_back(path);
        std::sort(paths.begin(), paths.end());
        for(auto& path : paths)
          wr.String(path.c_str());
      }
      wr.EndArray();
      wr.Key("PACKETS");
      wr.Int64(c.packets.load());
      wr.EndObject();
    }
  }
  wr.EndArray();
  wr.Key("SENDERS");
  wr.StartArray();
  {
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard lock{m_sendersMutex};
    for(auto& [key, st] : m_senders)
    {
      wr.StartObject();
      wr.Key("IP");
      wr.String(st.ip.c_str());
      wr.Key("PORT");
      wr.Int(st.port);
      wr.Key("PACKETS");
      wr.Int64(st.packets);
      wr.Key("LAST_MS");
      wr.Int64(
          std::chrono::duration_cast<std::chrono::milliseconds>(now - st.last).count());
      wr.EndObject();
    }
  }
  wr.EndArray();
  wr.EndObject();
  return std::string(buf.GetString(), buf.GetSize());
}

void oscquery_server_protocol_base::record_sent(
    std::string address, const ossia::value& v, uintptr_t skipped)
{
  std::lock_guard lock{m_sentMutex};
  m_sent.insert_or_assign(
      std::move(address), sent_value{std::chrono::steady_clock::now(), v, skipped});
}

std::optional<ossia::value>
oscquery_server_protocol_base::sent_within_window(std::string_view address, uintptr_t origin)
{
  const auto window = crossing_window();
  if(window.count() <= 0)
    return std::nullopt;
  std::lock_guard lock{m_sentMutex};
  auto it = m_sent.find(std::string(address));
  if(it == m_sent.end() || it->second.skipped == origin
     || std::chrono::steady_clock::now() - it->second.time > window)
    return std::nullopt;
  return it->second.value;
}

void oscquery_server_protocol_base::answer_crossing(
    std::string_view address, uintptr_t origin, const ossia::value& sent)
{
  auto* node = ossia::net::find_node(m_device->get_root_node(), address);
  auto* param = node ? node->get_parameter() : nullptr;
  if(!param)
    return;
  // The client's value differs from what it was sent: it applied the
  // server's value after its own, and keeps it unless it gets the final one
  const auto value = param->value();
  if(value == sent)
    return;

  using namespace ossia::net;
  using send_visitor = osc_value_send_visitor<
      net::parameter_base, osc_extended_policy, socket_writer<udp_send_socket>>;
  lock_t lock(m_clientsMutex);
  for(auto& client_p : m_clients)
  {
    auto& client = *client_p;
    if((uintptr_t)client.connection.lock().get() != origin)
      continue;
    // It was sent nothing there: its write cannot have crossed anything
    if(!client.listens_to(address))
      return;
    if(m_logger.outbound_logger)
      m_logger.outbound_logger->info("Out: {} {}", address, value);
    if(client.osc_socket && !param->get_critical())
    {
      send_visitor vis{*param, ossia::net::osc_address(*param), {*client.osc_socket}};
      value.apply(vis);
    }
    else
    {
      m_websocketServer->send_binary_message(
          client.connection, ossia::oscquery::osc_writer::to_message(*param, value));
    }
    break;
  }
}

uintptr_t oscquery_server_protocol_base::client_of_sender_port(int port)
{
  oscpack::IpEndpointName ip;
  ip.port = port;
  lock_t lock(m_clientsMutex);
  return client_identifier(m_clients, ip);
}

void oscquery_server_protocol_base::on_osc_message(
    const oscpack::ReceivedMessage& m, uintptr_t origin)
try
{
  auto id = ossia::net::message_origin_identifier{*this, origin};
  // Read before the value is applied: forwarding it to the other clients
  // replaces what was sent
  auto sent = origin ? sent_within_window(m.AddressPattern(), origin) : std::nullopt;
  ossia::net::on_input_message<true>(
      m.AddressPattern(), ossia::net::osc_client_message_applier{{id, m}}, m_listening, *m_device,
      m_logger);
  if(sent)
    answer_crossing(m.AddressPattern(), origin, *sent);
}
catch(const std::exception& e)
{
  logger().error("oscquery_server_protocol::on_OSCMessage: {}", e.what());
}
catch(...)
{
  logger().error("oscquery_server_protocol::on_OSCMessage: error.");
}

void oscquery_server_protocol_base::on_connectionOpen(const connection_handler& hdl)
try
{
  auto ip = m_websocketServer->get_remote_ip(hdl);
  if(ip.substr(0, 7) == "::ffff:")
    ip = ip.substr(7);
  // Its TCP port, after the last ':' of address:port (the address may be
  // IPv6); 0 when the client has already left
  const auto endpoint = m_websocketServer->get_remote_endpoint(hdl);
  const auto colon = endpoint.rfind(':');
  const int port = colon == std::string::npos ? 0 : std::atoi(endpoint.c_str() + colon + 1);

  {
    lock_t lock(m_clientsMutex);

    m_clients.emplace_back(std::make_unique<oscquery_client>(hdl));
    m_clients.back()->client_ip = std::move(ip);
    m_clients.back()->client_port = port;
    m_clientCount++;
  }

  onClientConnected(m_websocketServer->get_remote_endpoint(hdl));
}
catch(const std::exception& e)
{
  logger().error("oscquery_server_protocol::on_connectionOpen: {}", e.what());
}
catch(...)
{
  logger().error("oscquery_server_protocol::on_connectionOpen: error.");
}

void oscquery_server_protocol_base::on_connectionClosed(const connection_handler& hdl)
{
  {
    lock_t lock(m_clientsMutex);
    auto it = ossia::find_if(m_clients, [&](auto& clt) { return *clt == hdl; });
    if(it != m_clients.end())
    {
      --m_clientCount;
      m_clients.erase(it);
    }
  }

  onClientDisconnected(m_websocketServer->get_remote_endpoint(hdl));
}

void oscquery_server_protocol_base::on_nodeCreated(const net::node_base& n)
try
{
  const auto mess = ossia::oscquery::json_writer::path_added(n);

  lock_t lock(m_clientsMutex);
  for(auto& client : m_clients)
  {
    m_websocketServer->send_message(client->connection, mess);
  }
}
catch(const std::exception& e)
{
  logger().error("oscquery_server_protocol::on_nodeCreated: {}", e.what());
}
catch(...)
{
  logger().error("oscquery_server_protocol::on_nodeCreated: error.");
}

void oscquery_server_protocol_base::on_nodeRemoved(const net::node_base& n)
try
{
  const auto mess = ossia::oscquery::json_writer::path_removed(n.osc_address());

  lock_t lock(m_clientsMutex);
  for(auto& client : m_clients)
  {
    m_websocketServer->send_message(client->connection, mess);
  }
}
catch(const std::exception& e)
{
  logger().error("oscquery_server_protocol::on_nodeRemoved: {}", e.what());
}
catch(...)
{
  logger().error("oscquery_server_protocol::on_nodeRemoved: error.");
}

void oscquery_server_protocol_base::on_parameterChanged(
    const ossia::net::parameter_base& p)
{
  // Created, or removed from a node that stays: for a node taken out of the
  // tree, its PATH_REMOVED says it all
  if(p.get_node().is_removed())
    return;

  on_attributeChanged(p.get_node(), ossia::net::text_value_type());
}

void oscquery_server_protocol_base::on_attributeChanged(
    const net::node_base& n, std::string_view attr)
try
{
  const auto mess = ossia::oscquery::json_writer::attributes_changed(n, attr);
  lock_t lock(m_clientsMutex);
  for(auto& client : m_clients)
  {
    m_websocketServer->send_message(client->connection, mess);
  }
}
catch(const std::exception& e)
{
  logger().error("oscquery_server_protocol::on_attributeChanged: {}", e.what());
}
catch(...)
{
  logger().error("oscquery_server_protocol::on_attributeChanged: error.");
}

void oscquery_server_protocol_base::on_nodeRenamed(
    const net::node_base& n, std::string oldname)
try
{
  auto old_addr = n.osc_address();
  auto it = old_addr.find_last_of('/');
  old_addr.resize(it + 1);
  old_addr += oldname;
  {
    // Local listening
    m_listening.rename(old_addr, n.osc_address());
  }

  {
    // Remote listening
    lock_t lock(m_clientsMutex);
    for(auto& client_p : m_clients)
    {
      auto& client = *client_p;
      std::lock_guard lck{client.listeningMutex};
      auto it = client.listening.find(old_addr);
      if(it != client.listening.end())
      {
        auto v = it->second;
        client.listening.erase(it);
        client.listening.insert({n.osc_address(), v});
      }
    }
  }
  const auto mess
      = ossia::oscquery::json_writer::path_renamed(old_addr, n.osc_address());
  lock_t lock(m_clientsMutex);
  for(auto& client : m_clients)
  {
    m_websocketServer->send_message(client->connection, mess);
  }
}
catch(const std::exception& e)
{
  logger().error("oscquery_server_protocol::on_nodeRenamed: {}", e.what());
}
catch(...)
{
  logger().error("oscquery_server_protocol::on_nodeRenamed: error.");
}

void oscquery_server_protocol_base::update_zeroconf()
{
  // The announcement leaves on every interface: a server listening on one
  // address only is not announced, its clients use that address
  if(!bound_to_every_interface())
  {
    m_zeroconfServerWS = {};
    return;
  }
  try
  {
    m_zeroconfServerWS = net::make_zeroconf_server(
        get_device().get_name(), "_oscjson._tcp", "", m_wsPort, 0);
  }
  catch(const std::exception& e)
  {
    logger().error("oscquery_server_protocol::update_zeroconf: {}", e.what());
  }
  catch(...)
  {
    logger().error("oscquery_server_protocol::update_zeroconf: error.");
  }
}

ossia::net::server_reply oscquery_server_protocol_base::on_text_ws_message(
    const connection_handler& hdl, const std::string& message)
{
  if(m_logger.inbound_logger)
    m_logger.inbound_logger->info("WS In: {}", message);

  if(message.empty())
  {
    return {};
  }
  else if(message[0] == '/')
  {
    return ossia::oscquery::query_parser::parse_http_request(
        message, ossia::oscquery::get_query_answerer{}(*this, hdl));
  }
  else
  {
    rapidjson::Document doc;
    doc.Parse(message); // TODO ParseInsitu

    if(!doc.HasParseError())
    {
      if(on_prism_name(hdl, doc))
        return {};
      auto s = ossia::oscquery::json_query_answerer{}(*this, hdl, doc);
      return std::string{};
    }
  }
  // Exceptions are catched in the caller.

  return {};
}

ossia::net::server_reply oscquery_server_protocol_base::on_binary_ws_message(
    const oscquery_server_protocol_base::connection_handler& hdl,
    const std::string& message)
{
  // The connection identifies the client, as for its other messages
  process_raw_osc_data(message.data(), message.size(), (uintptr_t)hdl.lock().get());
  return {};
}

oscquery_server_protocol::oscquery_server_protocol(
    ossia::net::network_context_ptr ctx, uint16_t osc_port, uint16_t ws_port,
    bool forceWS, std::string bind)
    : oscquery_server_protocol_base{ctx, {ossia::net::udp_server_configuration{{{bind.empty() ? "0.0.0.0" : bind}, osc_port}}}, ws_port, forceWS, bind}
    , m_oscPort{osc_port}
    , m_oscServer{std::make_unique<osc_receiver_impl>(
          ossia::net::inbound_socket_configuration{bind.empty() ? "0.0.0.0" : bind, osc_port},
          m_context->context)}

{
}

void oscquery_server_protocol::set_device(ossia::net::device_base& dev)
{
  oscquery_server_protocol_base::set_device(dev);

  m_oscServer->open();
  // The client a datagram comes from, by its UDP source port, as
  // ossia::oscquery::oscquery_server_protocol does
  m_oscServer->receive_from(
      [this](const char* data, std::size_t sz, const boost::asio::ip::udp::endpoint& from) {
    const auto origin = client_of_sender_port(from.port());
    if(!origin)
    {
      auto ip = from.address().to_string();
      if(ip.substr(0, 7) == "::ffff:")
        ip = ip.substr(7);
      record_sender(ip, from.port());
    }
    process_raw_osc_data(data, sz, origin);
  });
}

void oscquery_server_protocol::update_zeroconf()
{
  try
  {
    int port = ossia::get<ossia::net::udp_server_configuration>(m_oscConf[0]).port;
    m_zeroconfServerOSC
        = net::make_zeroconf_server(get_device().get_name(), "_osc._udp", "", port, 0);
  }
  catch(const std::exception& e)
  {
    logger().error("oscquery_server_protocol::update_zeroconf: {}", e.what());
  }
  catch(...)
  {
    logger().error("oscquery_server_protocol::update_zeroconf: error.");
  }
}

void oscquery_server_protocol::stop()
{
  try
  {
    m_oscServer->close();
  }
  catch(...)
  {
    logger().error("Error when stopping osc server");
  }
  oscquery_server_protocol_base::stop();
}

oscquery_server_protocol::~oscquery_server_protocol()
{
  if(m_device)
  {
    auto& dev = *m_device;
    dev.on_node_created.disconnect<&oscquery_server_protocol::on_nodeCreated>(this);
    dev.on_node_removing.disconnect<&oscquery_server_protocol::on_nodeRemoved>(this);
    dev.on_parameter_created.disconnect<&oscquery_server_protocol::on_parameterChanged>(
        this);
    dev.on_parameter_removing.disconnect<&oscquery_server_protocol::on_parameterChanged>(
        this);
    dev.on_attribute_modified.disconnect<&oscquery_server_protocol::on_attributeChanged>(
        this);
    dev.on_node_renamed.disconnect<&oscquery_server_protocol::on_nodeRenamed>(this);
  }
  stop();
}
}
