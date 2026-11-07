// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

#pragma once

// Max-independent part of ossia.client for a device mirrored over OSCQuery:
// how the mirror is polled, and how the connection is kept.

#include <ossia/network/base/device.hpp>
#include <ossia/network/oscquery/oscquery_mirror.hpp>

#include <ossia/network/generic/generic_device.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

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

// The device the Minuit listener of the package builds for an instance it saw
// announced under _minuit._tcp, at ip:remote_port; it listens on local_port.
// It does not announce itself. Throws when local_port cannot be opened.
std::unique_ptr<ossia::net::generic_device> make_minuit_listener_device(
    const std::string& instance, const std::string& ip, uint16_t remote_port,
    uint16_t local_port);

// True when something accepts a TCP connection at the host and port of a
// ws:// URL, within timeout_ms. Building a mirror on a server that is down
// blocks the caller for 1.5 s before it fails; this costs timeout_ms at most.
bool oscquery_server_reachable(std::string_view ws_url, int timeout_ms);

// One attempt to connect a client to an OSCQuery server by URL, on a thread of
// its own: building the mirror waits for the WebSocket (up to 1.5 s), reading
// the tree waits for the server's answer (up to 3 s). Done on the main thread
// of Max, these waits freeze Max, and a server of the same Max (an
// ossia.device, whose network context the main thread polls) cannot answer
// at all. The host checks done() from the main thread and takes the device.
class oscquery_connect_attempt
{
public:
  oscquery_connect_attempt(std::string ws_url, std::string device_name);
  // Waits for the thread: at most the time of one attempt.
  ~oscquery_connect_attempt();
  oscquery_connect_attempt(const oscquery_connect_attempt&) = delete;
  oscquery_connect_attempt& operator=(const oscquery_connect_attempt&) = delete;

  bool done() const { return m_done; }
  // After done(): the device, connected and its tree read, or nullptr.
  std::shared_ptr<ossia::net::generic_device> take();
  // After done(), when take() gives nullptr: why.
  const std::string& error() const { return m_error; }

private:
  std::shared_ptr<ossia::net::generic_device> m_device;
  std::string m_error;
  std::atomic_bool m_done{false};
  std::thread m_thread;
};

// The OSCQuery devices announced over mDNS (_oscjson._tcp), each mirrored:
// what ossia.client finds by name, and what ossia.fuzzysearch searches.
// Every mirror is built on a thread of its own (oscquery_connect_attempt),
// never on the thread that calls poll: built there, a mirror of a server of
// the same Max, whose network context that thread polls, could never
// connect. A build that fails is tried again every retry_ms while the device
// is announced, and a mirror whose WebSocket goes down is dropped, reported,
// and built again the same way.
class oscquery_directory
{
public:
  oscquery_directory();
  ~oscquery_directory();
  oscquery_directory(const oscquery_directory&) = delete;
  oscquery_directory& operator=(const oscquery_directory&) = delete;

  struct changes
  {
    // No longer announced.
    std::vector<std::string> removed;
    // Still announced, but the WebSocket of their mirror went down.
    std::vector<std::string> lost;
  };

  // Reads what mDNS reported, starts and ends the builds, follows the
  // WebSocket of each mirror. Called regularly, always on the same thread.
  changes poll();

  // Any thread. The mirror of the device announced under that name, nullptr
  // while it is not built.
  std::shared_ptr<ossia::net::generic_device> find(const std::string& name) const;
  std::vector<std::shared_ptr<ossia::net::generic_device>> devices() const;

  // Any thread. The instance name under which this machine announces the
  // server whose WebSocket listens on ws_port: the name it was given, or the
  // one the system chose because it was taken on the network ("name (2)").
  // nullopt until the announcement is seen.
  std::optional<std::string> announced_name(int ws_port) const;

  static constexpr int retry_ms = 1000;

private:
  struct impl;
  std::unique_ptr<impl> m_impl;
};

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

  // tick runs on Max's scheduler thread, watch and unwatch on the main thread
  // (connect, disconnect): each waits for a tick in progress, so that the
  // device can be destroyed once unwatch returns.
  //
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
  // Recursive: a tick that reconnects calls connect, which calls unwatch.
  std::recursive_mutex m_mutex;
  ossia::oscquery::oscquery_mirror_protocol* m_mirror{};
  std::string m_url;
  int m_poll_ms{10};
  bool m_reconnect{true};
  bool m_attempting{false};
  state m_state{state::idle};
};

}
}
