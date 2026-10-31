// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

#include "client_mirror.hpp"

#include <servus/listener.h>
#include <servus/servus.h>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <map>

#include <unistd.h>

namespace ossia
{
namespace max_binding
{

bool oscquery_server_reachable(std::string_view ws_url, int timeout_ms)
{
  // ws://host:port, with or without the scheme
  std::string_view rest = ws_url;
  if(auto scheme = rest.find("://"); scheme != std::string_view::npos)
    rest.remove_prefix(scheme + 3);
  if(auto slash = rest.find('/'); slash != std::string_view::npos)
    rest.remove_suffix(rest.size() - slash);
  std::string host{rest}, port{"80"};
  if(auto colon = rest.find_last_of(':'); colon != std::string_view::npos)
  {
    host = std::string(rest.substr(0, colon));
    port = std::string(rest.substr(colon + 1));
  }
  if(host.empty())
    return false;

  try
  {
    boost::asio::io_context ctx;
    boost::asio::ip::tcp::resolver resolver{ctx};
    boost::asio::ip::tcp::socket socket{ctx};
    bool connected = false;
    resolver.async_resolve(
        host, port,
        [&](const boost::system::error_code& ec,
            boost::asio::ip::tcp::resolver::results_type results) {
      if(ec)
        return;
      boost::asio::async_connect(
          socket, results,
          [&](const boost::system::error_code& ec, const boost::asio::ip::tcp::endpoint&) {
        connected = !ec;
          });
        });
    ctx.run_for(std::chrono::milliseconds(timeout_ms));
    if(socket.is_open())
    {
      boost::system::error_code ignored;
      socket.close(ignored);
    }
    return connected;
  }
  catch(...)
  {
    return false;
  }
}

oscquery_connect_attempt::oscquery_connect_attempt(
    std::string ws_url, std::string device_name)
{
  m_thread = std::thread([this, url = std::move(ws_url), name = std::move(device_name)] {
    try
    {
      auto dev = std::make_shared<ossia::net::generic_device>(
          std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(url), name);
      if(dev->get_protocol().update(dev->get_root_node()))
        m_device = std::move(dev);
      else
        m_error = "no answer to the tree request of " + url;
    }
    catch(const std::exception& e)
    {
      m_error = e.what();
    }
    catch(...)
    {
      m_error = "could not connect to " + url;
    }
    m_done = true;
  });
}

oscquery_connect_attempt::~oscquery_connect_attempt()
{
  if(m_thread.joinable())
    m_thread.join();
}

std::shared_ptr<ossia::net::generic_device> oscquery_connect_attempt::take()
{
  if(!m_done)
    return {};
  if(m_thread.joinable())
    m_thread.join();
  return std::move(m_device);
}

struct oscquery_directory::impl final : servus::Listener
{
  using clock = std::chrono::steady_clock;
  struct entry
  {
    std::string host;
    std::string port;
    std::string url;
    std::shared_ptr<ossia::net::generic_device> device;
    ossia::oscquery::oscquery_mirror_protocol* mirror{};
    std::unique_ptr<oscquery_connect_attempt> attempt;
    clock::time_point next_try{};
  };

  servus::Servus service{"_oscjson._tcp"};
  // Filled by Servus during browse, read by poll.
  std::mutex events_mutex;
  std::vector<std::pair<bool, std::string>> events;
  // The mirrors, read by find and devices from any thread.
  mutable std::mutex mutex;
  std::map<std::string, entry> entries;
  // Builds that ended their use (device removed or announced elsewhere):
  // released once their thread is done.
  std::vector<std::unique_ptr<oscquery_connect_attempt>> abandoned;

  impl()
  {
    for(const auto& i : service.getInstances())
      instanceAdded(i);
    service.addListener(this);
    service.beginBrowsing(servus::Interface::IF_ALL);
  }

  ~impl() override
  {
    service.removeListener(this);
    service.endBrowsing();
  }

  void instanceAdded(const std::string& instance) override
  {
    std::lock_guard lock{events_mutex};
    events.push_back({true, instance});
  }

  void instanceRemoved(const std::string& instance) override
  {
    std::lock_guard lock{events_mutex};
    events.push_back({false, instance});
  }

  void abandon(entry& e)
  {
    if(e.attempt)
      abandoned.push_back(std::move(e.attempt));
  }
};

oscquery_directory::oscquery_directory()
    : m_impl{std::make_unique<impl>()}
{
}

oscquery_directory::~oscquery_directory() = default;

oscquery_directory::changes oscquery_directory::poll()
{
  auto& d = *m_impl;
  changes out;
  try
  {
    d.service.browse(0);
  }
  catch(...)
  {
  }

  std::vector<std::pair<bool, std::string>> events;
  {
    std::lock_guard lock{d.events_mutex};
    events.swap(d.events);
  }

  const auto now = impl::clock::now();
  std::lock_guard lock{d.mutex};
  for(const auto& [added, name] : events)
  {
    if(added)
    {
      // The address of the SRV record, resolved by the system when the
      // WebSocket connects, as the mirror of ossia.client always did.
      std::string host = d.service.get(name, "servus_ip");
      if(host.empty())
        host = d.service.get(name, "servus_host");
      const std::string port = d.service.get(name, "servus_port");
      if(host.empty() || port.empty())
        continue;
      const std::string url = "ws://" + host + ":" + port;
      auto& e = d.entries[name];
      if(e.url != url)
      {
        d.abandon(e);
        e = impl::entry{};
        e.host = host;
        e.port = port;
        e.url = url;
        e.next_try = now;
      }
    }
    else if(auto it = d.entries.find(name); it != d.entries.end())
    {
      d.abandon(it->second);
      d.entries.erase(it);
      out.removed.push_back(name);
    }
  }

  for(auto& [name, e] : d.entries)
  {
    if(e.attempt)
    {
      if(!e.attempt->done())
        continue;
      auto dev = e.attempt->take();
      e.attempt.reset();
      if(dev)
      {
        e.mirror = polled_oscquery_mirror(*dev);
        e.device = std::move(dev);
      }
      else
      {
        e.next_try = now + std::chrono::milliseconds(retry_ms);
      }
    }
    else if(e.device)
    {
      // The WebSocket went down: the server stopped, or its process died,
      // still announced. Dropped now, built again when it answers.
      if(e.mirror && !e.mirror->connected())
      {
        e.device.reset();
        e.mirror = nullptr;
        e.next_try = now + std::chrono::milliseconds(retry_ms);
        out.lost.push_back(name);
      }
    }
    else if(now >= e.next_try)
    {
      e.attempt = std::make_unique<oscquery_connect_attempt>(e.url, name);
    }
  }

  std::erase_if(d.abandoned, [](const auto& a) { return a->done(); });
  return out;
}

std::shared_ptr<ossia::net::generic_device>
oscquery_directory::find(const std::string& name) const
{
  std::lock_guard lock{m_impl->mutex};
  auto it = m_impl->entries.find(name);
  return it != m_impl->entries.end() ? it->second.device : nullptr;
}

namespace
{
// The first label of a host name, in lower case: "Julien4" for the SRV target
// "julien4.local.", as for gethostname's "Julien4" or "Julien4.local".
std::string host_label(std::string_view host)
{
  std::string out{host.substr(0, host.find('.'))};
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
  return out;
}
}

std::optional<std::string> oscquery_directory::announced_name(int ws_port) const
{
  char buf[256]{};
  if(::gethostname(buf, sizeof(buf) - 1) != 0)
    return std::nullopt;
  const auto self = host_label(buf);
  const auto port = std::to_string(ws_port);
  std::lock_guard lock{m_impl->mutex};
  for(const auto& [name, e] : m_impl->entries)
    if(e.port == port && host_label(e.host) == self)
      return name;
  return std::nullopt;
}

std::vector<std::shared_ptr<ossia::net::generic_device>> oscquery_directory::devices() const
{
  std::lock_guard lock{m_impl->mutex};
  std::vector<std::shared_ptr<ossia::net::generic_device>> out;
  for(const auto& [name, e] : m_impl->entries)
    if(e.device)
      out.push_back(e.device);
  return out;
}

oscquery_link::oscquery_link(host h)
    : m_host{std::move(h)}
{
}

oscquery_link::~oscquery_link()
{
  std::lock_guard lock{m_mutex};
  m_attempting = false;
  unwatch();
}

void oscquery_link::watch(ossia::net::device_base& dev, std::string ws_url, int poll_ms)
{
  std::lock_guard lock{m_mutex};
  m_mirror = polled_oscquery_mirror(dev);
  m_url = std::move(ws_url);
  m_poll_ms = poll_ms > 0 ? poll_ms : 1;
  if(!m_mirror)
  {
    m_state = state::idle;
    return;
  }
  m_state = state::connected;
  if(m_host.cancel)
    m_host.cancel();
  if(m_host.schedule)
    m_host.schedule(1);
}

void oscquery_link::unwatch()
{
  // Waits for a tick running on another thread.
  std::lock_guard lock{m_mutex};
  if(m_attempting)
    return;
  m_mirror = nullptr;
  m_state = state::idle;
  if(m_host.cancel)
    m_host.cancel();
}

void oscquery_link::tick()
{
  std::lock_guard lock{m_mutex};
  switch(m_state)
  {
    case state::idle:
      return;

    case state::connected:
      if(m_mirror->connected())
      {
        m_mirror->run_commands();
        if(m_host.schedule)
          m_host.schedule(m_poll_ms);
        return;
      }
      // The WebSocket went down: the device stays until the first attempt
      // replaces it, one second from now.
      m_state = state::lost;
      m_mirror = nullptr;
      if(m_host.lost)
        m_host.lost();
      if(!m_reconnect)
      {
        m_state = state::idle;
        return;
      }
      if(m_host.schedule)
        m_host.schedule(retry_ms);
      return;

    case state::lost: {
      // An attempt. Building a mirror on a server that is down blocks for
      // 1.5 s: the port is probed first, for 100 ms at most.
      m_attempting = true;
      bool ok = oscquery_server_reachable(m_url, probe_ms) && m_host.reconnect
                && m_host.reconnect();
      m_attempting = false;
      // On success the host called watch(), which scheduled the poll.
      if(!ok || m_state != state::connected)
      {
        m_state = state::lost;
        if(m_host.schedule)
          m_host.schedule(retry_ms);
      }
      return;
    }
  }
}

}
}
