// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

#pragma once

// Max-independent part of ossia.client for a device mirrored over OSCQuery:
// how the mirror is polled, and how the connection is kept.

#include <ossia/network/base/device.hpp>
#include <ossia/network/oscquery/oscquery_mirror.hpp>

#include <functional>
#include <string>
#include <string_view>

namespace ossia
{
namespace max_binding
{

// The OSCQuery mirror of a client's device, whether the client connected by
// URL or by name, set up as ossia-max sets up a connection by URL: a path the
// server removes is removed, not kept as a zombie. The tree changes the server
// sends (PATH_ADDED, PATH_REMOVED, ATTRIBUTES_CHANGED) wait in the mirror's
// command queue until run_commands is called, which the client does when it
// polls. Returns nullptr for a device of another protocol.
inline ossia::oscquery::oscquery_mirror_protocol*
polled_oscquery_mirror(ossia::net::device_base& dev)
{
  auto* mirror = dynamic_cast<ossia::oscquery::oscquery_mirror_protocol*>(&dev.get_protocol());
  if(mirror)
    mirror->set_zombie_on_remove(false);
  return mirror;
}

// True when something accepts a TCP connection at the host and port of a
// ws:// URL, within timeout_ms. Building a mirror on a server that is down
// blocks the caller for 1.5 s before it fails; this costs timeout_ms at most.
bool oscquery_server_reachable(std::string_view ws_url, int timeout_ms);

// The link of an ossia.client to an OSCQuery server reached by URL.
//
// The mirror's WebSocket is the only way the server reaches the client: when
// it goes down (the server stopped), the client receives nothing more and
// nothing reconnects it. The link polls the mirror from the main thread, and
// when the WebSocket is down it reports the loss once, then tries again every
// second, as long as the host wants it to, until the server answers on its
// port and the host builds the device again. Everything runs on the thread
// that calls tick, never on a network thread.
class oscquery_link
{
public:
  struct host
  {
    // Call tick() after ms, on the main thread; a new call replaces the
    // previous one.
    std::function<void(int ms)> schedule;
    // Forget the scheduled tick.
    std::function<void()> cancel;
    // The WebSocket went down.
    std::function<void()> lost;
    // Build the device again, as the first connection built it, and call
    // watch() with it. Returns true when it is connected. The server
    // answered on its port right before this call.
    std::function<bool()> reconnect;
  };

  explicit oscquery_link(host h);
  // No tick after this.
  ~oscquery_link();
  oscquery_link(const oscquery_link&) = delete;
  oscquery_link& operator=(const oscquery_link&) = delete;

  // Follow the device just connected to ws_url; poll_ms is the period of the
  // poll while the WebSocket is up. Not a mirror: nothing is followed.
  void watch(ossia::net::device_base& dev, std::string ws_url, int poll_ms);
  // Explicit disconnect, or a connection made another way: no more ticks.
  // Ignored while reconnect() is being called, which disconnects first.
  void unwatch();
  // Whether to try again after a loss. Default true.
  void set_reconnect(bool on) { m_reconnect = on; }
  bool reconnect() const { return m_reconnect; }
  bool watching() const { return m_state != state::idle; }
  bool lost() const { return m_state == state::lost; }

  // Main thread, when scheduled. Connected: applies the tree changes the
  // server sent, polls again. Loss: reports it, then tries again every second.
  void tick();

  static constexpr int retry_ms = 1000;
  static constexpr int probe_ms = 100;

private:
  enum class state
  {
    idle,
    connected,
    lost
  };

  host m_host;
  ossia::oscquery::oscquery_mirror_protocol* m_mirror{};
  std::string m_url;
  int m_poll_ms{10};
  bool m_reconnect{true};
  bool m_attempting{false};
  state m_state{state::idle};
};

}
}
