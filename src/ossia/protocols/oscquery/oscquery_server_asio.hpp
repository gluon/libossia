#pragma once
#include <ossia/detail/hash_map.hpp>
#include <ossia/detail/lockfree_queue.hpp>
#include <ossia/detail/mutex.hpp>
#include <ossia/network/base/listening.hpp>
#include <ossia/network/base/protocol.hpp>
#include <ossia/network/context_functions.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/sockets/websocket_common.hpp>
#include <ossia/network/sockets/websocket_reply.hpp>
#include <ossia/network/zeroconf/zeroconf.hpp>
#include <ossia/protocols/osc/osc_factory.hpp>

#include <nano_signal_slot.hpp>

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
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
class query_answerer;
class get_query_answerer;
struct json_query_answerer;
}
namespace oscquery_asio
{
struct oscquery_client;
using clients = std::vector<std::unique_ptr<oscquery_client>>;
//! Implementation of an oscquery server.
class OSSIA_EXPORT oscquery_server_protocol_base : public ossia::net::protocol_base
{
  friend struct oscquery_client;
  friend class ossia::oscquery::query_answerer;
  friend class ossia::oscquery::get_query_answerer;
  friend struct ossia::oscquery::json_query_answerer;

public:
  using connection_handler = std::weak_ptr<void>;

  // Use oscquery_server_protocol_with_osc
  explicit oscquery_server_protocol_base(
      ossia::net::network_context_ptr ctx, uint16_t, uint16_t)
      = delete;

  //! bind: the address the WebSocket server listens on, every interface by
  //! default; on another one, the server is not announced over mDNS
  oscquery_server_protocol_base(
      ossia::net::network_context_ptr ctx,
      const std::vector<ossia::net::osc_server_configuration>& conf, uint16_t ws_port,
      bool forceWS, std::string bind = "0.0.0.0");
  ~oscquery_server_protocol_base() override;

  bool pull(net::parameter_base&) override;
  std::future<void> pull_async(net::parameter_base&) override;
  void request(net::parameter_base&) override;
  bool push(const net::parameter_base&, const ossia::value& v) override;
  bool push_raw(const ossia::net::full_parameter_data& parameter_base) override;
  bool push_bundle(const std::vector<const ossia::net::parameter_base*>&) override;
  bool push_raw_bundle(const std::vector<ossia::net::full_parameter_data>&) override;
  bool push_bundle(std::span<ossia::bundle_element>) override;
  bool push_bundle_bounded(std::span<ossia::bundle_element>) override;
  bool echo_incoming_message(
      const ossia::net::message_origin_identifier&, const ossia::net::parameter_base&,
      const ossia::value& v) override;
  bool observe(net::parameter_base&, bool) override;
  bool observe_quietly(net::parameter_base&, bool) override;
  bool update(net::node_base& b) override;

  void set_device(net::device_base& dev) override;
  void stop() override;
  ossia::net::device_base& get_device() const noexcept { return *m_device; }

  int get_ws_port() const noexcept { return m_wsPort; }
  //! The address the server listens on, "0.0.0.0" for every interface
  const std::string& get_bind() const noexcept { return m_bind; }
  bool bound_to_every_interface() const noexcept
  {
    return m_bind.empty() || m_bind == "0.0.0.0";
  }

  bool force_ws() const noexcept { return m_forceWS.load(std::memory_order_relaxed); }
  void set_force_ws(bool forceWS) noexcept;

  //! A value a client writes less than this after the server sent it a
  //! different value on the same address crossed that value: the client
  //! applied the server's value after it sent its own, which the server
  //! applied last. The server then sends that client its final value, so
  //! that both hold it. 0 turns it off.
  static constexpr std::chrono::milliseconds default_crossing_window{100};
  void set_crossing_window(std::chrono::milliseconds window) noexcept
  {
    m_crossingWindow.store(window.count(), std::memory_order_relaxed);
  }
  std::chrono::milliseconds crossing_window() const noexcept
  {
    return std::chrono::milliseconds{m_crossingWindow.load(std::memory_order_relaxed)};
  }

  const std::vector<ossia::net::osc_server_configuration>&
  get_transports() const noexcept
  {
    return m_oscConf;
  }

  //! The answer to ?PRISM_CONNECTIONS, a PRISM extension declared in the
  //! EXTENSIONS of HOST_INFO: {"CLIENTS": [...], "SENDERS": [...]}. A client:
  //! IP, PORT (TCP), NAME (given by the client, if any), OSC_PORT and
  //! SENDER_PORT (its UDP ports, if it streams), LISTEN (the addresses it
  //! asked for), PACKETS (the OSC packets it sent). A sender, a UDP source
  //! that is none of the clients, the last 256 to send: IP, PORT, PACKETS,
  //! LAST_MS (since its last packet).
  std::string query_connections();

  Nano::Signal<void(const std::string&)> onClientConnected;
  Nano::Signal<void(const std::string&)> onClientDisconnected;

protected:
  explicit oscquery_server_protocol_base(ossia::net::network_context_ptr ctx) = delete;

  // OSC callback. origin: the client the message comes from, 0 when unknown;
  // a value its device echoes is not sent back to it.
  void on_osc_message(const oscpack::ReceivedMessage& m, uintptr_t origin);
  void process_raw_osc_data(const char* data, std::size_t sz, uintptr_t origin);
  // The client connected from that IPv4 address (in host order) whose OSC
  // sender has that UDP port (LOCAL_SENDER_PORT of its START_OSC_STREAMING),
  // 0 if none.
  uintptr_t client_of_sender(unsigned long address, int port);
  // A UDP packet of a source that is none of the clients
  void record_sender(const std::string& ip, uint16_t port);
  // PRISM_NAME, a text command a client sends on the WebSocket to give its
  // name; true if the message was one
  bool on_prism_name(const connection_handler& hdl, const rapidjson::Document& doc);

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

  void write_to(oscquery_client& client, std::string_view data, bool critical);
  template <typename Elements, typename AddressOf, typename Send>
  bool write_to_listeners(Elements& elements, AddressOf address_of, Send send);

  void update_zeroconf();
  // Exceptions here will be catched by the server
  // which will set appropriate error codes.
  ossia::net::server_reply
  on_text_ws_message(const connection_handler& hdl, const std::string& message);
  ossia::net::server_reply
  on_binary_ws_message(const connection_handler& hdl, const std::string& message);

  ossia::net::network_context_ptr m_context;

  std::unique_ptr<ossia::net::websocket_server_interface> m_websocketServer;

  net::zeroconf_server m_zeroconfServerWS;

  // What we expose over OSC
  std::vector<ossia::net::osc_server_configuration> m_oscConf;

  // Listening status of the local software
  net::listened_parameters m_listening;

  // The clients connected to this server
  clients m_clients;
  std::atomic_int m_clientCount{};

  ossia::net::device_base* m_device{};

  // To lock m_clients
  mutex_t m_clientsMutex;

  // The local ports
  uint16_t m_wsPort{};
  std::string m_bind;

  // Will only send changes through WS
  std::atomic_bool m_forceWS{};

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

  struct sender_stats
  {
    std::string ip;
    uint16_t port{};
    int64_t packets{};
    std::chrono::steady_clock::time_point last;
  };
  // The UDP sources that are none of the clients, by "ip:port": past
  // max_senders, the one silent for the longest goes
  static constexpr std::size_t max_senders = 256;
  std::mutex m_sendersMutex;
  std::unordered_map<std::string, sender_stats> m_senders;
};

class OSSIA_EXPORT oscquery_server_protocol : public oscquery_server_protocol_base
{
public:
  struct osc_receiver_impl;
  //! bind: the address both ports listen on, every interface by default
  explicit oscquery_server_protocol(
      ossia::net::network_context_ptr ctx, uint16_t osc_port = 1234,
      uint16_t ws_port = 5678, bool forceWS = false, std::string bind = "0.0.0.0");
  ~oscquery_server_protocol();

  void update_zeroconf();
  void set_device(net::device_base& dev) override;
  void stop() override;
  int get_osc_port() const noexcept { return m_oscPort; }

private:
  uint16_t m_oscPort{};
  std::unique_ptr<osc_receiver_impl> m_oscServer;

  net::zeroconf_server m_zeroconfServerOSC;
};
}
}
