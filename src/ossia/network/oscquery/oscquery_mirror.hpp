#pragma once
#include <ossia/network/base/bundle.hpp>

#include <ossia/detail/json_fwd.hpp>
#include <ossia/detail/lockfree_queue.hpp>
#include <ossia/network/base/listening.hpp>
#include <ossia/network/base/protocol.hpp>
#include <ossia/network/oscquery/host_info.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <mutex>
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
struct parameter_data;
class websocket_client;
}
namespace oscquery
{
struct osc_outbound_visitor;
struct http_client_context;
struct http_responder;

class OSSIA_EXPORT oscquery_mirror_protocol final : public ossia::net::protocol_base
{
public:
  oscquery_mirror_protocol(std::string host, uint16_t local_osc_port = 10203);
  ~oscquery_mirror_protocol() override;

  bool pull(net::parameter_base&) override;
  std::future<void> pull_async(net::parameter_base&) override;
  void request(net::parameter_base&) override;
  bool push(const net::parameter_base&, const ossia::value& v) override;
  bool push_raw(const ossia::net::full_parameter_data& parameter_base) override;
  bool push_bundle(const std::vector<const ossia::net::parameter_base*>&) override;
  bool push_raw_bundle(const std::vector<ossia::net::full_parameter_data>&) override;
  bool push_bundle(std::span<ossia::bundle_element>) override;
  bool push_bundle_bounded(std::span<ossia::bundle_element>) override;
  bool observe(net::parameter_base&, bool) override;
  bool observe_quietly(net::parameter_base&, bool) override;
  bool update(net::node_base& b) override;
  bool echo_incoming_message(
      const ossia::net::message_origin_identifier& id,
      const ossia::net::parameter_base& addr, const value& val) override;

  std::future<void> update_async(net::node_base& b) override;

  void stop() override;
  void set_device(net::device_base& dev) override;
  ossia::net::device_base& get_device() const { return *m_device; }

  //! The name this client gives a server that lists its clients (PRISM_NAME);
  //! the name of the device when empty. Set before the device.
  void set_client_name(std::string name) { m_clientName = std::move(name); }

  /**
   * @brief Run the commands registered in th event queue
   *
   * The data structures are not protected by locks since it may hurt
   * the performance on a tree, and hard to get right.
   * Instead, all the edition operations on a device should happen on a single
   * thread.
   * When edition operations are received from the network, they are put in a
   * queue.
   *
   * Run this function regularly in order to get the update of the device, for
   * instance in the event loop of your application.
   */
  void run_commands();

  /**
   * @brief Provide a callback for whenever an edition command is received
   *
   * This is an alternative way to calling runCommands() : a function
   * can be provided; it will be called every time a new command is received,
   * after it has been pushed.
   * This can be used to put the command in the application's main loop.
   */
  void set_command_callback(std::function<void()>);

  /**
   * @brief Request a new node from the server
   * @param Parent of the new node
   * @param Data of the new node
   */
  void request_add_node(net::node_base&, const ossia::net::parameter_data&);

  /**
   * @brief Request a node removal from the server
   * @param Node to be removed
   */
  void request_remove_node(net::node_base&);

  /**
   * @brief Request a node renaming from the server
   * @param Node to be removed and new name
   */
  void request_rename_node(net::node_base& node, const std::string& new_name);

  /**
   * @brief Define behavior when a node is removed : mark it as zombie if true
   * (default), remove it otherwise.
   * @param zombie_on_removed
   */
  //! Whether this client wrote that address within the last moments
  bool written_recently(const std::string& address);
  bool written_since(
      const std::string& address, std::chrono::steady_clock::time_point t);

  //! Asks the server for the value of a parameter nothing follows, without
  //! waiting for it: at most one request in flight for an address, and one
  //! per refresh_interval. A reader gets the last value known meanwhile.
  void refresh_unfollowed(const std::string& address);
  static constexpr std::chrono::milliseconds refresh_interval{100};

  void set_zombie_on_remove(bool zombie_on_remove)
  {
    m_zombie_on_remove = zombie_on_remove;
  }

  /**
   * @brief Get zombie on removed move
   * @return
   */
  bool get_zombie_on_remove() const noexcept { return m_zombie_on_remove; }

  host_info get_host_info() const noexcept;

  bool connected() const noexcept override { return m_hasWS; }
  void connect() override;

private:
  friend struct http_answer;
  friend struct http_path_answer;
  friend struct http_path_error;

  void init();
  using connection_handler = std::weak_ptr<void>;
  bool on_WSMessage(connection_handler hdl, const std::string& message);
  // The structure of an address that changed, and what comes of it
  void read_path(const std::string& path);
  void on_path_read(const std::string& path, const std::string& message);
  void on_path_gone(const std::string& path);
  bool on_BinaryWSMessage(connection_handler hdl, const std::string& message);
  void
  on_OSCMessage(const oscpack::ReceivedMessage& m, const oscpack::IpEndpointName& ip);

  void cleanup_connections();

  void http_send_message(const std::string& str);
  void http_send_message(const rapidjson::StringBuffer& str);

  void ws_send_message(const std::string& str);
  void ws_send_binary_message(const std::string& str);
  void ws_send_message(const rapidjson::StringBuffer& str);
  bool query_connected();
  void query_stop();

  void on_nodeRenamed(const ossia::net::node_base& n, std::string oldname);

  void start_http();

  void on_ws_disconnected() { m_hasWS = false; }

  template <typename Parameters>
  bool push_bundle_impl(const Parameters& parameters, bool bounded = false);

  // A server that does not answer crossings leaves the client with its own
  // value after the server's crossed it: a value received less than
  // crossing_window after a write of ours, and different from it, is read
  // again from the server settle_delay later.
  struct crossing_state;
  std::shared_ptr<crossing_state> m_crossing;
  void record_sent(const std::string& address, const ossia::value& v);
  void check_crossing(std::string_view address);

  std::unique_ptr<osc::sender<oscquery::osc_outbound_visitor>> m_oscSender;
  std::unique_ptr<osc::receiver> m_oscServer;
  std::unique_ptr<ossia::net::websocket_client> m_websocketClient;
  std::atomic_bool m_hasWS{};

  // Listening status of the local software
  net::listened_parameters m_listening;

  ossia::net::device_base* m_device{};
  std::string m_clientName;

  std::promise<void> m_namespacePromise;

  struct get_osc_promise
  {
    std::promise<void> promise;
    ossia::net::parameter_base* address{};
  };
  struct get_ws_promise
  {
    get_ws_promise() = default;
    get_ws_promise(const get_ws_promise&) = delete;
    get_ws_promise(get_ws_promise&&) noexcept = default;
    get_ws_promise& operator=(const get_ws_promise&) = delete;
    get_ws_promise& operator=(get_ws_promise&&) noexcept = default;

    get_ws_promise(
        std::promise<void>&& p, const std::string& addr, bool only_changes = false)
        : promise{std::move(p)}
        , address{addr}
        , only_if_changed{only_changes}
        , asked{only_changes ? std::chrono::steady_clock::now()
                             : std::chrono::steady_clock::time_point{}}
    {
    }
    std::promise<void> promise;
    std::string address{};
    // Read again after a crossing: the same value is not a new one
    bool only_if_changed{};
    // When a read of our own was asked: a value this client writes after
    // is newer than the answer
    std::chrono::steady_clock::time_point asked{};
  };

  ossia::spsc_queue<get_ws_promise> m_getWSPromises;
  // The reads of a parameter nothing follows, by address: the last request
  // and its answer to come
  struct unfollowed_read
  {
    std::chrono::steady_clock::time_point asked{};
    std::future<void> answer;
  };
  std::mutex m_unfollowedMutex;
  std::unordered_map<std::string, unfollowed_read> m_unfollowed;

  // The values read again after an ATTRIBUTES_CHANGED, by address, until the
  // answer comes (or for 1 s): one read at a time for an address.
  std::mutex m_attributeReadsMutex;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_attributeReads;
  ossia::spsc_queue<std::function<void()>> m_functionQueue;
  std::function<void()> m_commandCallback;

  std::thread m_wsThread;
  std::string m_queryHost;
  std::string m_queryPort;
  std::string m_httpHost;
  int m_osc_port{};

  std::unique_ptr<http_client_context> m_http;
  host_info m_host_info;
  mutable std::mutex m_host_info_mutex;

  ossia::net::message_origin_identifier m_id;

  void set_feedback(bool fb) override;
  std::atomic<bool> m_feedback{true};
  bool m_zombie_on_remove{true};
};

//! Use this function to load a device preset in the OSCQuery format.
//! The value to give someone who reads a parameter. On a mirror, a parameter
//! nothing follows gets no value from the server, which sends only what is
//! listened to: it is asked for, without waiting, at most every
//! refresh_interval, and the last value known is given meanwhile. To follow a
//! value, put a callback on the parameter (ossia.remote, Ossia.Remote): the
//! server then sends each value as it changes.
OSSIA_EXPORT ossia::value read_followed_value(ossia::net::parameter_base& param);

OSSIA_EXPORT void load_oscquery_device(ossia::net::device_base& dev, std::string json);
}
}
