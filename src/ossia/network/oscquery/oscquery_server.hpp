#pragma once
#include <ossia/detail/lockfree_queue.hpp>
#include <ossia/detail/mutex.hpp>
#include <ossia/network/base/listening.hpp>
#include <ossia/network/base/protocol.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/sockets/websocket_reply.hpp>
#include <ossia/network/zeroconf/zeroconf.hpp>

#include <ossia/detail/hash_map.hpp>

#include <nano_signal_slot.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
namespace osc
{
template <typename T>
class sender;
class receiver;
}
namespace oscpack
{
class ReceivedMessage;
class IpEndpointName;
}
namespace ossia
{
namespace net
{
class websocket_server_interface;
}
namespace oscquery
{
struct oscquery_client;
using clients = std::vector<std::unique_ptr<oscquery_client>>;
//! Implementation of an oscquery server.
class OSSIA_EXPORT oscquery_server_protocol final : public ossia::net::protocol_base
{
  friend class query_answerer;
  friend class get_query_answerer;
  friend struct json_query_answerer;

public:
  using connection_handler = std::weak_ptr<void>;
  oscquery_server_protocol(uint16_t osc_port = 1234, uint16_t ws_port = 5678);
  ~oscquery_server_protocol() override;

  bool pull(net::parameter_base&) override;
  std::future<void> pull_async(net::parameter_base&) override;
  void request(net::parameter_base&) override;
  bool push(const net::parameter_base&, const ossia::value& v) override;
  bool push_raw(const ossia::net::full_parameter_data& parameter_base) override;
  bool push_bundle(const std::vector<const ossia::net::parameter_base*>&) override;
  bool push_raw_bundle(const std::vector<ossia::net::full_parameter_data>&) override;
  bool echo_incoming_message(
      const ossia::net::message_origin_identifier&, const ossia::net::parameter_base&,
      const ossia::value& v) override;
  bool observe(net::parameter_base&, bool) override;
  bool observe_quietly(net::parameter_base&, bool) override;
  bool update(net::node_base& b) override;
  void run_commands();
  void set_device(net::device_base& dev) override;
  void stop() override;
  ossia::net::device_base& get_device() const { return *m_device; }

  int get_osc_port() const { return m_oscPort; }

  int get_ws_port() const { return m_wsPort; }

  template <typename F>
  void for_each_client(F&& f) const
      noexcept(noexcept(f(std::declval<const oscquery_client&>())))
  {
    ossia::lock_t lck{m_clientsMutex};
    for(auto& clt : m_clients)
    {
      f(*clt);
    }
  }

  Nano::Signal<void(const std::string&)> onClientConnected;
  Nano::Signal<void(const std::string&)> onClientDisconnected;

  void disable_zeroconf();
  void set_zeroconf_servers(
      net::zeroconf_server oscquery_server, net::zeroconf_server osc_server);

  //! A client's write that comes less than this after the server sent it a
  //! different value on that address gets the server's final value back, to
  //! that client only: crossed writes end on the same value. 0 turns it off.
  static constexpr std::chrono::milliseconds default_crossing_window{100};
  void set_crossing_window(std::chrono::milliseconds window) noexcept
  {
    m_crossingWindow.store(window.count(), std::memory_order_relaxed);
  }
  std::chrono::milliseconds crossing_window() const noexcept
  {
    return std::chrono::milliseconds{m_crossingWindow.load(std::memory_order_relaxed)};
  }

private:
  // A value the server sent on its own (a local change, another client's
  // value it forwards), to every client but skipped
  void record_sent(std::string address, const ossia::value& v, uintptr_t skipped = 0);
  // Before a write of origin to address is applied: the value the server
  // sent it there within the crossing window, if any
  std::optional<ossia::value> sent_within_window(std::string_view address, uintptr_t origin);
  // After the write: its answer, when it left the address with another value
  // than sent. The answer opens no window.
  void answer_crossing(std::string_view address, uintptr_t origin, const ossia::value& sent);

  // List of connected clients
  oscquery_client* find_client(const connection_handler& hdl);

  void add_node(std::string_view path, const string_map<std::string>& parameters);
  void remove_node(std::string_view path, const std::string& node);
  void rename_node(std::string_view node, const std::string& new_name);

  // OSC callback
  void on_OSCMessage(const oscpack::ReceivedMessage& m, oscpack::IpEndpointName ip);
  // A message from a client: origin is its connection, 0 when unknown; a
  // value its device echoes is not sent back to it
  void on_osc_message_from(const oscpack::ReceivedMessage& m, uintptr_t origin);

  // Websocket callbacks
  void on_connectionOpen(const connection_handler& hdl);
  void on_connectionClosed(const connection_handler& hdl);

  // Local device callback
  void on_nodeCreated(const ossia::net::node_base&);
  void on_nodeRemoved(const ossia::net::node_base&);
  void on_parameterChanged(const ossia::net::parameter_base&);
  void on_attributeChanged(const ossia::net::node_base&, std::string_view attr);
  void on_nodeRenamed(const ossia::net::node_base& n, std::string oldname);

  template <typename T>
  bool push_impl(const T& addr, const ossia::value& v);

  void update_zeroconf();
  // Exceptions here will be catched by the server
  // which will set appropriate error codes.
  ossia::net::server_reply
  on_WSrequest(const connection_handler& hdl, const std::string& message);
  ossia::net::server_reply
  on_BinaryWSrequest(const connection_handler& hdl, const std::string& message);

  std::unique_ptr<osc::receiver> m_oscServer;
  std::unique_ptr<ossia::net::websocket_server_interface> m_websocketServer;

  net::zeroconf_server m_zeroconfServerWS;
  net::zeroconf_server m_zeroconfServerOSC;

  // Listening status of the local software
  net::listened_parameters m_listening;

  // The clients connected to this server
  clients m_clients TS_GUARDED_BY(m_clientsMutex);

  ossia::net::device_base* m_device{};

  // Where the websocket server lives
  std::thread m_serverThread;

  // To lock m_clients
  mutable mutex_t m_clientsMutex;

  // The local ports
  uint16_t m_oscPort{};
  uint16_t m_wsPort{};

  bool m_echo{};
  bool m_disableZeroconf{};

  // TODO could we make an intermediate base class for oscquery_{server,mirror}
  // that hold that function queue and other shared members/methods ?

  // function queue to hold ws callback
  // and avoid tree to be modified on another thread
  ossia::spsc_queue<std::function<void()>> m_functionQueue;

  struct sent_value
  {
    std::chrono::steady_clock::time_point time;
    ossia::value value;
    uintptr_t skipped{};
  };
  // By address, the last value the server sent on its own
  std::mutex m_sentMutex;
  std::unordered_map<std::string, sent_value> m_sent;
  std::atomic<int64_t> m_crossingWindow{default_crossing_window.count()};
};
}

class OSSIA_EXPORT oscquery_device
{
public:
  oscquery_device(
      uint16_t osc_port = 1234, uint16_t ws_port = 5678, std::string name = "oscquery");

  ~oscquery_device();

  ossia::net::generic_device device;
  ossia::oscquery::oscquery_server_protocol& protocol;
};
}
