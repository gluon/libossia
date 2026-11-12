// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// Stopping the OSCQuery server of an ossia.device while a client is
// connected: oscquery_asio::oscquery_server_protocol under a multiplex,
// removed with stop_expose_to, as ossia-max does on "stop". Build with
// OSSIA_DNSSD=OFF to keep these tests off mDNS. Loopback.
#include "ReproCommon.hpp"

#include <ossia/network/oscquery/oscquery_mirror.hpp>

#include "include_catch.hpp"

#include <optional>

using namespace repro;

// Max polls the network context of a device from its scheduler and does not
// catch what the poll throws: an exception there ends Max. Here the context is
// polled the same way by a thread, which records the first exception.
TEST_CASE("Server stop: no exception out of the poll afterwards", "[stop-polled]")
{
  const bool mirror_client = GENERATE(false, true);
  auto ctx = ossia::net::create_network_context();
  ossia::net::generic_device dev{std::make_unique<ossia::net::multiplex_protocol>(), "stop_polled"};
  add_float(dev.get_root_node(), "/foo/bar", 0.f);
  auto& multiplex = static_cast<ossia::net::multiplex_protocol&>(dev.get_protocol());
  const uint16_t ws = free_port(true);
  auto server = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, free_port(false), ws, false);
  auto* exposed = server.get();
  multiplex.expose_to(std::move(server));

  std::atomic_bool stop{false};
  std::mutex error_mutex;
  std::string error;
  std::thread scheduler([&] {
    while(!stop)
    {
      try
      {
        ctx->context.restart();
        ctx->context.poll();
      }
      catch(const std::exception& e)
      {
        std::lock_guard lock{error_mutex};
        if(error.empty())
          error = e.what();
      }
      std::this_thread::sleep_for(10ms);
    }
  });

  // The client: a raw WebSocket connection, or an ossia mirror (built away
  // from the polling thread, as ossia.client does).
  std::optional<ws_client> raw;
  std::unique_ptr<ossia::net::generic_device> mirror;
  if(mirror_client)
  {
    std::thread connect([&] {
      try
      {
        mirror = std::make_unique<ossia::net::generic_device>(
            std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
                "ws://127.0.0.1:" + std::to_string(ws), free_port(false)),
            "mirror");
      }
      catch(...)
      {
      }
    });
    connect.join();
  }
  else
  {
    raw.emplace(ws);
  }
  const bool connected = mirror_client ? bool(mirror) : raw->connected();
  REQUIRE(connected);

  multiplex.stop_expose_to(*exposed);
  std::this_thread::sleep_for(2000ms);
  stop = true;
  scheduler.join();
  mirror.reset();
  raw.reset();

  std::lock_guard lock{error_mutex};
  std::printf("stop-polled: client %s; exception out of the poll after the stop: \"%s\"\n",
              mirror_client ? "ossia mirror" : "raw WebSocket", error.c_str());
  CHECK(error.empty());
}

// stop() alone, the server kept alive in the multiplex: the client's
// connection, closed by the stop or by the client leaving afterwards, must not
// call back into the stopped server. The stop runs on the polling thread, as
// in Max, where the scheduler both polls and stops.
TEST_CASE("Server stop: a stopped server kept alive, a client that leaves", "[stop-kept]")
{
  auto ctx = ossia::net::create_network_context();
  ossia::net::generic_device dev{std::make_unique<ossia::net::multiplex_protocol>(), "stop_kept"};
  add_float(dev.get_root_node(), "/foo/bar", 0.f);
  auto& multiplex = static_cast<ossia::net::multiplex_protocol&>(dev.get_protocol());
  const uint16_t ws = free_port(true);
  auto server = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, free_port(false), ws, false);
  auto* exposed = server.get();
  multiplex.expose_to(std::move(server));

  std::atomic_bool stop{false};
  std::mutex error_mutex;
  std::string error;
  std::thread scheduler([&] {
    while(!stop)
    {
      try
      {
        ctx->context.restart();
        ctx->context.poll();
      }
      catch(const std::exception& e)
      {
        std::lock_guard lock{error_mutex};
        if(error.empty())
          error = e.what();
      }
      std::this_thread::sleep_for(10ms);
    }
  });

  std::optional<ws_client> raw;
  raw.emplace(ws);
  REQUIRE(raw->connected());
  std::this_thread::sleep_for(300ms);

  boost::asio::post(ctx->context, [exposed] { exposed->stop(); });
  std::this_thread::sleep_for(500ms);
  raw.reset();
  std::this_thread::sleep_for(1000ms);
  stop = true;
  scheduler.join();

  std::lock_guard lock{error_mutex};
  std::printf("stop-kept: exception out of the poll after stop() and the client's leaving: \"%s\"\n",
              error.c_str());
  CHECK(error.empty());
}

// After a server is removed from a running network context, a server exposed
// again on that context, on the same ports or others, must answer HTTP.
TEST_CASE("Server stop: a server exposed again answers HTTP", "[reexpose]")
{
  const bool with_client = GENERATE(true, false);
  const bool same_ports = GENERATE(true, false);
  auto ctx = ossia::net::create_network_context();
  ossia::net::generic_device dev{std::make_unique<ossia::net::multiplex_protocol>(), "reexpose"};
  add_float(dev.get_root_node(), "/value", 0.f);
  auto& multiplex = static_cast<ossia::net::multiplex_protocol&>(dev.get_protocol());
  const uint16_t osc = free_port(false), ws = free_port(true), ws2 = free_port(true);
  auto first = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, osc, ws, false);
  auto* exposed = first.get();
  multiplex.expose_to(std::move(first));
  std::thread net([&] { ctx->run(); });
  std::this_thread::sleep_for(200ms);
  const int before = http_get(ws, "/", 3000).status;

  std::optional<ws_client> client;
  if(with_client)
    client.emplace(ws);
  const bool connected = with_client ? client->connected() : true;

  multiplex.stop_expose_to(*exposed);
  std::this_thread::sleep_for(1000ms);
  std::string error;
  try
  {
    multiplex.expose_to(std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(
        ctx, same_ports ? osc : free_port(false), same_ports ? ws : ws2, false));
  }
  catch(const std::exception& e)
  {
    error = e.what();
  }
  std::this_thread::sleep_for(500ms);
  const int after = http_get(same_ports ? ws : ws2, "/", 3000).status;
  std::printf("reexpose: client %d, same ports %d: HTTP before %d, after exposing again %d%s%s\n",
              int(with_client), int(same_ports), before, after, error.empty() ? "" : ", expose_to threw ",
              error.c_str());
  client.reset();
  ossia::net::stop_network_context(*ctx);
  net.join();

  CHECK(before == 200);
  CHECK(connected);
  CHECK(error.empty());
  CHECK(after == 200);
}
