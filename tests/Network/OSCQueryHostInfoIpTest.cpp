// HOST_INFO asked over HTTP gives the address the server is reached on: a
// client that takes WS_IP for the address subscribes there, and an empty one
// leaves it unsubscribed. On both servers: the one of the C API and the asio one.
#include "include_catch.hpp"

#include <ossia/detail/config.hpp>

#include <ossia/detail/json.hpp>
#include <ossia/network/context.hpp>
#include <ossia/network/context_functions.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/local/local.hpp>
#include <ossia/network/oscquery/oscquery_server.hpp>
#include <ossia/protocols/oscquery/oscquery_server_asio.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>

#include <chrono>
#include <string>
#include <thread>

using namespace std::chrono_literals;

namespace
{
std::string http_body(int port, const std::string& target)
{
  try
  {
    boost::asio::io_context ctx;
    boost::asio::ip::tcp::socket sock{ctx};
    sock.connect({boost::asio::ip::make_address("127.0.0.1"), (unsigned short)port});
    const std::string req
        = "GET " + target + " HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
    boost::asio::write(sock, boost::asio::buffer(req));
    std::string reply;
    boost::system::error_code ec;
    char buf[4096];
    for(std::size_t n; (n = sock.read_some(boost::asio::buffer(buf), ec)) > 0 && !ec;)
      reply.append(buf, n);
    auto p = reply.find("\r\n\r\n");
    return p == std::string::npos ? std::string{} : reply.substr(p + 4);
  }
  catch(...)
  {
    return {};
  }
}

void check_ws_ip(const std::string& body)
{
  INFO(body);
  rapidjson::Document d;
  d.Parse(body.c_str());
  REQUIRE(!d.HasParseError());
  REQUIRE(d.IsObject());
  REQUIRE(d.HasMember("WS_IP"));
  REQUIRE(d["WS_IP"].IsString());
  CHECK(std::string(d["WS_IP"].GetString()) == "127.0.0.1");
}
}

TEST_CASE("HOST_INFO over HTTP: WS_IP of the server of the C API", "[hostinfo-ws-ip]")
{
  auto proto = std::make_unique<ossia::oscquery::oscquery_server_protocol>(1852, 5852);
  proto->disable_zeroconf();
  ossia::net::generic_device dev{std::move(proto), "ws_ip"};
  std::this_thread::sleep_for(200ms);
  check_ws_ip(http_body(5852, "/?HOST_INFO"));
}

TEST_CASE("HOST_INFO over HTTP: WS_IP of the asio server", "[hostinfo-ws-ip]")
{
  auto ctx = ossia::net::create_network_context();
  {
    auto mp = std::make_unique<ossia::net::multiplex_protocol>();
    auto* multiplex = mp.get();
    ossia::net::generic_device dev{std::move(mp), "ws_ip_asio"};
    multiplex->expose_to(
        std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, 1853, 5853));
    std::thread net{[ctx] { ctx->run(); }};
    std::this_thread::sleep_for(200ms);
    check_ws_ip(http_body(5853, "/?HOST_INFO"));
    ossia::net::stop_network_context(*ctx);
    net.join();
  }
}
