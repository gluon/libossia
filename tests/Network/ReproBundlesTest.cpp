// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// OSC bundles on the way out of an OSCQuery client (mirror) and of an
// OSCQuery server exposed through a multiplex, as ossia.device does.
// The server below gives its UDP port to a sink: what a mirror sends over UDP
// is read datagram by datagram. Build with OSSIA_DNSSD=OFF to keep the asio
// server off mDNS. Loopback.
#include "ReproCommon.hpp"

#include <ossia/network/base/bundle.hpp>
#include <ossia/network/domain/domain.hpp>
#include <ossia/network/oscquery/detail/json_writer.hpp>
#include <ossia/network/oscquery/oscquery_mirror.hpp>

#include "include_catch.hpp"

#include <span>

using namespace repro;

namespace
{
// An asio OSCQuery server under a multiplex, with `leaves` float parameters
// /load/p<i>; its OSC port is the port of `sink`.
struct sink_server
{
  ossia::net::network_context_ptr ctx = ossia::net::create_network_context();
  ossia::net::multiplex_protocol* multiplex{};
  std::unique_ptr<ossia::net::generic_device> dev;
  uint16_t osc = free_port(false);
  uint16_t ws = free_port(true);
  udp_sink sink{osc};
  std::thread net;

  explicit sink_server(int leaves)
  {
    auto mp = std::make_unique<ossia::net::multiplex_protocol>();
    multiplex = mp.get();
    dev = std::make_unique<ossia::net::generic_device>(std::move(mp), "bundles");
    for(int i = 0; i < leaves; i++)
    {
      auto& n = ossia::net::create_node(dev->get_root_node(), "/load/p" + std::to_string(i));
      ossia::try_setup_parameter("float", n)->set_domain(ossia::make_domain(0.f, 1000000.f));
    }
    multiplex->expose_to(std::make_unique<ossia::oscquery_asio::oscquery_server_protocol_base>(
        ctx,
        std::vector<ossia::net::osc_server_configuration>{
            ossia::net::udp_server_configuration{{{"0.0.0.0"}, osc}}},
        ws, false));
    net = std::thread([this] { ctx->run(); });
    std::this_thread::sleep_for(200ms);
  }
  ~sink_server()
  {
    ossia::net::stop_network_context(*ctx);
    net.join();
  }
  std::string url() const { return "ws://127.0.0.1:" + std::to_string(ws); }
  ossia::net::parameter_base* param(int i)
  {
    return ossia::net::find_node(dev->get_root_node(), "/load/p" + std::to_string(i))->get_parameter();
  }
};
}

// The mirror's push_bundle sends the values of many parameters in OSC
// bundles: over UDP, no datagram above 1400 bytes.
TEST_CASE("Mirror: push_bundle sends the values", "[mirror-push-bundle]")
{
  const int n = 3000;
  sink_server server{n};
  auto client = std::make_shared<ossia::net::generic_device>(
      std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(server.url(), free_port(false)), "bundles");
  REQUIRE(client->get_protocol().update(client->get_root_node()));
  std::vector<const ossia::net::parameter_base*> params;
  for(int i = 0; i < n; i++)
  {
    auto* p = ossia::net::find_node(client->get_root_node(), "/load/p" + std::to_string(i))->get_parameter();
    p->set_value(ossia::value(float(i)));
    params.push_back(p);
  }
  server.sink.clear();
  const bool sent = client->get_protocol().push_bundle(params);
  const bool all = server.sink.wait_messages(n, 3000);
  std::lock_guard lock{server.sink.mutex};
  std::size_t largest = 0;
  for(auto size : server.sink.datagrams)
    largest = std::max(largest, size);
  std::printf("mirror-push-bundle: push_bundle returned %d; %zu message(s) in %zu datagram(s), largest %zu bytes\n",
              int(sent), server.sink.messages.size(), server.sink.datagrams.size(), largest);
  CHECK(sent);
  CHECK(all);
  CHECK(largest <= 1400);
}

// A server exposed through a multiplex (ossia.device) sends a bundle of
// values as one OSC bundle to a client, not one datagram per value.
TEST_CASE("Multiplex: push_bundle reaches the exposed server", "[multiplex-push-bundle]")
{
  const int n = 40;
  sink_server server{n};
  udp_sink client_sink{free_port(false)};
  ws_client client{server.ws};
  REQUIRE(client.connected());
  auto stream = ossia::oscquery::json_writer::start_osc_streaming(client_sink.port(), 0);
  client.send_text(std::string(stream.GetString(), stream.GetSize()));
  for(int i = 0; i < n; i++)
    client.command("LISTEN", "/load/p" + std::to_string(i));
  std::this_thread::sleep_for(500ms);

  std::vector<const ossia::net::parameter_base*> params;
  for(int i = 0; i < n; i++)
  {
    server.param(i)->set_value(ossia::value(float(i)));
    params.push_back(server.param(i));
  }
  client_sink.clear();
  server.dev->get_protocol().push_bundle(params);
  const bool all = client_sink.wait_messages(n, 3000);
  std::lock_guard lock{client_sink.mutex};
  std::printf("multiplex-push-bundle: %zu message(s) in %zu datagram(s)\n", client_sink.messages.size(),
              client_sink.datagrams.size());
  CHECK(all);
  CHECK(client_sink.datagrams.size() == 1);
}

// Three values of one parameter in one push_bundle of bundle_element: one
// bundle, the three messages in order.
TEST_CASE("Mirror: values of one parameter in one bundle", "[repeated-parameter-bundle]")
{
  sink_server server{1};
  auto client = std::make_shared<ossia::net::generic_device>(
      std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(server.url(), free_port(false)), "bundles");
  REQUIRE(client->get_protocol().update(client->get_root_node()));
  auto* p0 = ossia::net::find_node(client->get_root_node(), "/load/p0")->get_parameter();
  std::vector<ossia::bundle_element> elements{{p0, 1.f}, {p0, 2.f}, {p0, 3.f}};
  server.sink.clear();
  const bool sent = client->get_protocol().push_bundle(std::span<ossia::bundle_element>(elements));
  server.sink.wait_messages(3, 3000);
  std::this_thread::sleep_for(200ms);
  std::lock_guard lock{server.sink.mutex};
  std::string order;
  for(auto& m : server.sink.messages)
    order += std::to_string(int(m.value)) + " ";
  std::printf("repeated-parameter-bundle: push_bundle returned %d; %zu message(s) (%s) in %zu datagram(s)\n",
              int(sent), server.sink.messages.size(), order.c_str(), server.sink.datagrams.size());
  CHECK(server.sink.messages.size() == 3);
  CHECK(order == "1 2 3 ");
  CHECK(server.sink.datagrams.size() == 1);
}

// The same through a server, as the C API and ossia.device send the values of
// a tick:
// three values of one parameter in one push_bundle of bundle_element reach a
// client that listens to it in one bundle, in order, and none reaches a
// client that does not.
TEST_CASE("Server: values of one parameter in one bundle", "[server-values-bundle]")
{
  sink_server server{1};
  udp_sink listening{free_port(false)}, quiet{free_port(false)};
  ws_client listener{server.ws}, other{server.ws};
  REQUIRE(listener.connected());
  REQUIRE(other.connected());
  auto stream = [](ws_client& c, udp_sink& s) {
    auto text = ossia::oscquery::json_writer::start_osc_streaming(s.port(), 0);
    c.send_text(std::string(text.GetString(), text.GetSize()));
  };
  stream(listener, listening);
  stream(other, quiet);
  listener.command("LISTEN", "/load/p0");
  std::this_thread::sleep_for(500ms);

  std::vector<ossia::bundle_element> elements{
      {server.param(0), 1.f}, {server.param(0), 2.f}, {server.param(0), 3.f}};
  listening.clear();
  quiet.clear();
  ossia::net::protocol_base* asio{};
  server.multiplex->for_each_protocol([&](ossia::net::protocol_base& p) {
    if(dynamic_cast<ossia::oscquery_asio::oscquery_server_protocol_base*>(&p))
      asio = &p;
  });
  REQUIRE(asio);
  const bool sent = asio->push_bundle(std::span<ossia::bundle_element>(elements));
  listening.wait_messages(3, 3000);
  std::this_thread::sleep_for(200ms);
  std::lock_guard lock{listening.mutex};
  std::lock_guard lock_quiet{quiet.mutex};
  std::string order;
  for(auto& m : listening.messages)
    order += std::to_string(int(m.value)) + " ";
  std::printf("server-values-bundle: push_bundle returned %d; the listening client got %zu message(s) (%s) in %zu datagram(s), the other %zu\n",
              int(sent), listening.messages.size(), order.c_str(), listening.datagrams.size(),
              quiet.messages.size());
  CHECK(sent);
  CHECK(order == "1 2 3 ");
  CHECK(listening.datagrams.size() == 1);
  CHECK(quiet.messages.empty());
}
