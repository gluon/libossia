// The beast WebSocket and HTTP server: HTTP statuses, stop, clients that
// leave as they connect, IPv4 by default, and a mirror on an IPv6 URL
#include "include_catch.hpp"

#include <ossia/detail/config.hpp>

#include <ossia/detail/logger.hpp>
#include <ossia/network/context.hpp>
#include <ossia/network/exceptions.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/oscquery/oscquery_mirror.hpp>
#include <ossia/network/sockets/websocket_server_beast.hpp>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <spdlog/sinks/ostream_sink.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace
{
ossia::net::server_reply json(const std::string& s)
{
  ossia::net::server_reply r;
  r.type = ossia::net::server_reply::data_type::json;
  r.data = s;
  return r;
}

// A beast server of the test, on its own context and thread
struct beast_server
{
  ossia::net::network_context_ptr ctx = std::make_shared<ossia::net::network_context>();
  std::unique_ptr<ossia::net::websocket_server_beast> server
      = std::make_unique<ossia::net::websocket_server_beast>(ctx);
  std::thread thread;

  explicit beast_server(bool dual_stack = false)
  {
    server->set_dual_stack(dual_stack);
    server->set_open_handler([](auto) { });
    server->set_close_handler([](auto) { });
    server->set_message_handler(
        [](auto, auto, const std::string& resource) -> ossia::net::server_reply {
      if(resource == "/nothing")
        throw ossia::node_not_found_error{"/nothing"};
      if(resource == "/bad")
        throw ossia::bad_request_error{"/bad"};
      if(resource == "/boom")
        throw std::runtime_error{"boom"};
      if(resource.find("HOST_INFO") != std::string::npos)
        return json(R"({"NAME":"beast","EXTENSIONS":{"VALUE":true}})");
      if(resource == "/")
        return json(
            R"({"FULL_PATH":"/","CONTENTS":{"a":{"FULL_PATH":"/a","TYPE":"i","VALUE":[3]}}})");
      return json("{}");
    });
  }
  void start(int port)
  {
    server->listen(port);
    thread = std::thread{[c = ctx] { c->run(); }};
  }
  ~beast_server()
  {
    if(server)
      server->stop();
    server.reset();
    ctx->context.stop();
    if(thread.joinable())
      thread.join();
  }
};

int http_status(const std::string& address, int port, const std::string& target)
{
  try
  {
    boost::asio::io_context ctx;
    boost::asio::ip::tcp::socket sock{ctx};
    sock.connect({boost::asio::ip::make_address(address), (unsigned short)port});
    const std::string req
        = "GET " + target + " HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n";
    boost::asio::write(sock, boost::asio::buffer(req));
    std::string reply;
    boost::system::error_code ec;
    char buf[4096];
    for(std::size_t n; (n = sock.read_some(boost::asio::buffer(buf), ec)) > 0 && !ec;)
      reply.append(buf, n);
    int code = 0;
    std::sscanf(reply.c_str(), "HTTP/1.1 %d", &code);
    return code;
  }
  catch(...)
  {
    return 0;
  }
}

bool tcp_connects(const std::string& address, int port)
{
  boost::asio::io_context ctx;
  boost::asio::ip::tcp::socket sock{ctx};
  boost::system::error_code ec;
  sock.connect({boost::asio::ip::make_address(address), (unsigned short)port}, ec);
  return !ec;
}

// What the logger writes while it lives
struct log_capture
{
  std::ostringstream out;
  std::shared_ptr<spdlog::sinks::ostream_sink_mt> sink
      = std::make_shared<spdlog::sinks::ostream_sink_mt>(out);
  log_capture() { ossia::logger().sinks().push_back(sink); }
  ~log_capture()
  {
    auto& s = ossia::logger().sinks();
    s.erase(std::remove(s.begin(), s.end(), sink), s.end());
  }
  int count(const std::string& what)
  {
    ossia::logger().flush();
    const auto text = out.str();
    int n = 0;
    for(auto p = text.find(what); p != std::string::npos; p = text.find(what, p + 1))
      n++;
    return n;
  }
};
}

TEST_CASE("beast server: an HTTP error keeps its status", "[beast]")
{
  beast_server s;
  s.start(5840);
  CHECK(http_status("127.0.0.1", 5840, "/nothing") == 404);
  CHECK(http_status("127.0.0.1", 5840, "/bad") == 400);
  CHECK(http_status("127.0.0.1", 5840, "/boom") == 500);
  CHECK(http_status("127.0.0.1", 5840, "/") == 200);
}

TEST_CASE("beast server: no handler runs once it is stopped", "[beast]")
{
  beast_server s;
  std::atomic_bool stopped{false};
  std::atomic_int late{0};
  s.server->set_close_handler([&](auto) {
    if(stopped)
      late++;
  });
  s.start(5841);

  // Clients connected while the server stops and is destroyed; their context
  // outlives them
  boost::asio::io_context cctx;
  std::vector<
      std::unique_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>>
      clients;
  for(int i = 0; i < 4; i++)
  {
    auto ws = std::make_unique<
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>(cctx);
    ws->next_layer().connect({boost::asio::ip::make_address("127.0.0.1"), 5841});
    ws->handshake("127.0.0.1", "/");
    clients.push_back(std::move(ws));
  }
  std::this_thread::sleep_for(100ms);

  stopped = true;
  s.server->stop();

  // The clients leave after: the closings that complete once the server is
  // stopped run no handler
  for(auto& ws : clients)
  {
    boost::system::error_code ec;
    ws->next_layer().close(ec);
  }
  std::this_thread::sleep_for(300ms);
  CHECK(late == 0);

  // Nor once it is destroyed
  s.server.reset();
  std::this_thread::sleep_for(100ms);
  CHECK(late == 0);
}

TEST_CASE("beast server: a client that leaves as it connects is no error", "[beast]")
{
  log_capture log;
  beast_server s;
  s.start(5842);

  // Clients that reset at once, before the server accepts them
  for(int i = 0; i < 200; i++)
  {
    boost::asio::io_context ctx;
    boost::asio::ip::tcp::socket sock{ctx};
    boost::system::error_code ec;
    sock.connect({boost::asio::ip::make_address("127.0.0.1"), 5842}, ec);
    if(ec)
      continue;
    sock.set_option(boost::asio::socket_base::linger(true, 0), ec);
    sock.close(ec);
  }
  std::this_thread::sleep_for(300ms);

  CHECK(log.count("WS accept error") == 0);
  // And the server still serves
  CHECK(http_status("127.0.0.1", 5842, "/") == 200);
}

TEST_CASE("beast server: IPv4 by default, dual stack on demand", "[beast]")
{
  {
    beast_server s;
    CHECK(!s.server->dual_stack());
    s.start(5843);
    CHECK(tcp_connects("127.0.0.1", 5843));
    CHECK(!tcp_connects("::1", 5843));
  }
  {
    beast_server s{true};
    s.start(5844);
    CHECK(tcp_connects("127.0.0.1", 5844));
    if(tcp_connects("::1", 5844))
      SUCCEED("dual stack");
    else
      WARN("no IPv6 loopback on this machine");
  }
}

TEST_CASE("mirror: a URL with an IPv6 address between brackets", "[beast]")
{
  beast_server s{true};
  s.start(5845);
  if(!tcp_connects("::1", 5845))
  {
    WARN("no IPv6 loopback on this machine: skipped");
    return;
  }

  auto proto = new ossia::oscquery::oscquery_mirror_protocol("ws://[::1]:5845", 10845);
  ossia::net::generic_device mirror{
      std::unique_ptr<ossia::net::protocol_base>(proto), "mirror"};
  CHECK(proto->update(mirror.get_root_node()));
  CHECK(ossia::net::find_node(mirror.get_root_node(), "/a"));
}
