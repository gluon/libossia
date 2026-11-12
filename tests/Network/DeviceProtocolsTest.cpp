// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// What the getprotocols and getoscqclients messages of ossia.device report,
// without Max: device_protocols.cpp of ossia-max, against the server an
// ossia.device exposes (oscquery_asio::oscquery_server_protocol under a
// multiplex) and against the server of the C API
// (oscquery::oscquery_server_protocol). Loopback.
#include "ReproCommon.hpp"
#include "device_protocols.hpp"

#include "include_catch.hpp"

using namespace repro;
using ossia::max_binding::describe_protocol;
using ossia::max_binding::oscquery_clients;

namespace
{
// The server of the C API under a multiplex, zeroconf disabled
struct multiplexed_api_server
{
  uint16_t osc = free_port(false);
  uint16_t ws = free_port(true);
  std::unique_ptr<ossia::net::generic_device> dev;
  ossia::net::multiplex_protocol* multiplex{};

  multiplexed_api_server()
  {
    auto mp = std::make_unique<ossia::net::multiplex_protocol>();
    multiplex = mp.get();
    dev = std::make_unique<ossia::net::generic_device>(std::move(mp), "protocols");
    auto p = std::make_unique<ossia::oscquery::oscquery_server_protocol>(osc, ws);
    p->disable_zeroconf();
    multiplex->expose_to(std::move(p));
    std::this_thread::sleep_for(200ms);
  }
};

std::string start_streaming(uint16_t server_port, uint16_t sender_port)
{
  return R"({"COMMAND":"START_OSC_STREAMING","DATA":{"LOCAL_SERVER_PORT":)"
         + std::to_string(server_port) + R"(,"LOCAL_SENDER_PORT":)" + std::to_string(sender_port)
         + "}}";
}

// Two clients, one of which streams over UDP to udp_port
template <typename Server>
void check_clients(Server& s, uint16_t ws, const char* name)
{
  const uint16_t udp_port = free_port(false);
  ws_client by_ws{ws}, by_udp{ws};
  REQUIRE(by_ws.connected());
  REQUIRE(by_udp.connected());
  by_udp.send_text(start_streaming(udp_port, free_port(false)));
  std::this_thread::sleep_for(500ms);

  const auto clients = oscquery_clients(*s.multiplex);
  int streaming = 0, by_websocket = 0;
  for(const auto& c : clients)
  {
    std::printf("getoscqclients: %s: %s %d\n", name, c.ip.c_str(), c.osc_port);
    CHECK(c.ip == "127.0.0.1");
    if(c.osc_port == udp_port)
      streaming++;
    else if(c.osc_port == -1)
      by_websocket++;
  }
  std::printf("getoscqclients: %s: %zu client(s)\n", name, clients.size());
  CHECK(clients.size() == 2);
  CHECK(streaming == 1);
  CHECK(by_websocket == 1);
}
}

TEST_CASE("getprotocols lists the OSCQuery server of an ossia.device", "[device-protocols]")
{
  asio_server s;
  s.start();
  REQUIRE(s.multiplex->get_protocols().size() == 1);
  const auto e = describe_protocol(*s.multiplex->get_protocols().front());
  std::printf("getprotocols: asio server: \"%s\" %d %d\n", e.kind.c_str(), e.first_port, e.second_port);
  CHECK(e.kind == "oscquery");
  CHECK(e.first_port == s.osc);
  CHECK(e.second_port == s.ws);
}

TEST_CASE("getprotocols lists the OSCQuery server of the C API", "[device-protocols]")
{
  multiplexed_api_server s;
  REQUIRE(s.multiplex->get_protocols().size() == 1);
  const auto e = describe_protocol(*s.multiplex->get_protocols().front());
  std::printf("getprotocols: C API server: \"%s\" %d %d\n", e.kind.c_str(), e.first_port, e.second_port);
  CHECK(e.kind == "oscquery");
  CHECK(e.first_port == s.osc);
  CHECK(e.second_port == s.ws);
}

TEST_CASE("getoscqclients lists the clients of the OSCQuery server of an ossia.device", "[device-protocols]")
{
  asio_server s;
  s.start();
  check_clients(s, s.ws, "asio server");
}

TEST_CASE("getoscqclients lists the clients of the OSCQuery server of the C API", "[device-protocols]")
{
  multiplexed_api_server s;
  check_clients(s, s.ws, "C API server");
}
