#pragma once

#include <ossia/network/local/local.hpp>
#include <ossia/network/oscquery/oscquery_mirror.hpp>
#include <ossia/network/zeroconf/zeroconf.hpp>

#include <ossia-max/src/client_mirror.hpp>
#include <ossia-max/src/device_base.hpp>

#include <memory>
#include <string>

namespace ossia
{
namespace max_binding
{

#pragma mark -
#pragma mark t_client structure declaration

class client : public device_base
{
public:
  using is_client = std::true_type;

  static void register_children(client*);
  void unregister_children();
  static void loadbang(client*);

  static void print_protocol_help()
  {
    post("connect <protocol> <args> ...");
    post("Available protocols (case sensitive): Minuit, oscquery");
    post("Protocols parameters :");
    post(
        "Minuit <remoteip> <remoteport> <localport> :\n"
        "\tremoteip (symbol): ip of target device\n"
        "\tremoteport (float): port on which packet should be send\n"
        "\tlocalport (float): port to which this device is listening");
    post(
        "oscquery <oscport> <wsurl> :\n"
        "\twsurl (symbol) : url to connect to (default : "
        "ws://127.0.0.1:5678)\n");
    post(
        "osc <remoteip> <remoteport> <localport> :\n"
        "\tremoteip (symbol): ip of target device\n"
        "\tremoteport (float): port on which packet should be send\n"
        "\tlocalport (float): port to which this device is listening");
  }

  std::vector<ossia::net::minuit_connection_data> m_minuit_devices;
  std::vector<ossia::net::oscquery_connection_data> m_oscq_devices;

  void* m_poll_clock;
  // The poll itself, on Max's main thread: the clock only sets it
  void* m_poll_qelem{};
  void* m_connect_qelem{};
  void* m_attempt_qelem{};
  static void connect_clock(client* x);
  static void attempt_clock(client* x);
  // The tree read again by update, applied by the poll: values output then
  std::future<void> m_tree_read;

  ossia::oscquery::oscquery_mirror_protocol* m_oscq_protocol{};
  // Connection by URL: polls the mirror, reconnects when the server goes away.
  std::unique_ptr<oscquery_link> m_link;
  std::string m_wsurl;
  long m_reconnect{1};
  // Connection by URL in progress, on a thread of its own; checked by
  // m_attempt_clock. Attempts left by a disconnect or a new connect end on
  // their own and are released when done.
  std::unique_ptr<oscquery_connect_attempt> m_attempt;
  std::vector<std::unique_ptr<oscquery_connect_attempt>> m_abandoned;
  std::vector<t_atom> m_attempt_status;
  void* m_attempt_clock{};
  static void attempt_check(client* x);
  static void attempt_check_impl(client* x);
  static void connect_impl(client* x);
  static void disconnect_impl(client* x);
  static void poll_message_impl(client* x);
  static void finish_connect(client* x, int count, t_atom* status, bool read_tree);

  bool m_zeroconf{true};
  bool is_zeroconf() const { return m_zeroconf; }
  std::string get_name() const { return m_name ? std::string(m_name->s_name) : ""; }

  void set_feedback(bool b);

  static t_max_err do_notify(client *x, t_symbol *, t_symbol *msg, void *, void *data);

  static void refresh(client*);
  static void connect(client*);
  static void connect_mess_cb(client* x, t_symbol*, int argc, t_atom* argv);
  static void disconnect(client*);
  static void get_devices(client* x);
  static void check_thread_status(client* x);
  static void update(client* x);
  static void stats(client* x);
  static void poll_message(client* x);
  static void poll_main(client* x);
  // static void destroy(client* x);
  // static void* create(t_symbol* name, int argc, t_atom* argv);
  static void find_devices_async(client* x);
  static void assist(client*, void*, long, long, char*);
  static void get_mess_cb(client*, t_symbol*);

  static void* create(t_symbol*, long, t_atom*);
  static void destroy(ossia::max_binding::client*);

  int m_argc{};
  t_atom* m_argv{};
  long m_feedback{1};
};

} // max namespace
} // ossia namespace
