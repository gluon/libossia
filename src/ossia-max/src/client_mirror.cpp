// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

#include "client_mirror.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <chrono>

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
