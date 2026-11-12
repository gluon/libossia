// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// What the OSCQuery servers send and accept over the WebSocket. Both servers
// where it applies: the one of the C API (oscquery::oscquery_server_protocol,
// zeroconf disabled) and the one of an ossia.device in Max
// (oscquery_asio::oscquery_server_protocol under a multiplex; build with
// OSSIA_DNSSD=OFF to keep it off mDNS). Loopback.
#include "ReproCommon.hpp"

#include <ossia/network/oscquery/oscquery_mirror.hpp>
#include <ossia/network/value/value_conversion.hpp>

#include "include_catch.hpp"

#include <functional>

using namespace repro;

namespace
{
struct server_under_test
{
  std::unique_ptr<api_server> api;
  std::unique_ptr<asio_server> asio;

  server_under_test(bool use_asio, const std::function<void(ossia::net::node_base&)>& build)
  {
    if(use_asio)
    {
      asio = std::make_unique<asio_server>();
      build(asio->root());
      asio->start();
    }
    else
    {
      api = std::make_unique<api_server>();
      build(api->root());
    }
  }
  uint16_t ws() const { return api ? api->ws : asio->ws; }
  ossia::net::generic_device& dev() { return api ? *api->dev : *asio->dev; }
  ossia::net::parameter_base* param(const std::string& path)
  {
    return ossia::net::find_node(dev().get_root_node(), path)->get_parameter();
  }
  const char* name() const { return api ? "C API server" : "asio server"; }
};

float as_float(const ossia::value& v)
{
  return ossia::convert<float>(v);
}
}

// Proposal, lines 149-154: a client receives the values of what it LISTENs
// to, and stops receiving them after IGNORE.
TEST_CASE("WebSocket: LISTEN and IGNORE decide what a client receives", "[listen-ignore]")
{
  const bool use_asio = GENERATE(false, true);
  server_under_test s{use_asio, [](auto& root) { add_float(root, "/f", 0.f); }};
  ws_client quiet{s.ws()}, follower{s.ws()};
  REQUIRE(quiet.connected());
  REQUIRE(follower.connected());
  follower.command("LISTEN", "/f");
  std::this_thread::sleep_for(300ms);
  quiet.take(), follower.take();

  s.param("/f")->push_value(1.f);
  std::this_thread::sleep_for(300ms);
  const auto to_quiet = ws_client::messages(quiet.take(), "/f").size();
  const auto to_follower = ws_client::messages(follower.take(), "/f").size();

  follower.command("IGNORE", "/f");
  std::this_thread::sleep_for(300ms);
  follower.take();
  s.param("/f")->push_value(2.f);
  std::this_thread::sleep_for(300ms);
  const auto after_ignore = ws_client::messages(follower.take(), "/f").size();

  std::printf("listen-ignore: %s: client without LISTEN got %zu, client with LISTEN got %zu, after IGNORE %zu\n",
              s.name(), to_quiet, to_follower, after_ignore);
  CHECK(to_follower == 1);
  CHECK(to_quiet == 0);
  CHECK(after_ignore == 0);
}

// Proposal, line 212: a write to a read-only parameter is ignored. Line 101:
// a write-only parameter has no value to give.
TEST_CASE("WebSocket: ACCESS is applied to network writes and reads", "[access]")
{
  const bool use_asio = GENERATE(false, true);
  server_under_test s{use_asio, [](auto& root) {
    add_float(root, "/getonly", 2.5f)->set_access(ossia::access_mode::GET);
    add_float(root, "/setonly", 1.5f)->set_access(ossia::access_mode::SET);
  }};
  ws_client c{s.ws()};
  REQUIRE(c.connected());
  c.send_binary(osc_float("/getonly", 99.f));
  std::this_thread::sleep_for(300ms);
  const float after = as_float(s.param("/getonly")->value());
  const auto read = http_get(s.ws(), "/setonly?VALUE");
  std::printf("access: %s: read-only parameter after a network write of 99: %g; GET /setonly?VALUE -> %d %s\n",
              s.name(), after, read.status, read.body.c_str());
  CHECK(after == 2.5f);
  CHECK(read.status == 204);
}

// With the device's echo on, a value written by a client goes to the other
// clients that listen, not back to the client that wrote it.
static void check_echo_spares_sender(bool use_asio)
{
  server_under_test s{use_asio, [](auto& root) { add_float(root, "/f", 0.f); }};
  s.dev().set_echo(true);
  ws_client writer{s.ws()}, other{s.ws()};
  REQUIRE(writer.connected());
  REQUIRE(other.connected());
  writer.command("LISTEN", "/f");
  other.command("LISTEN", "/f");
  std::this_thread::sleep_for(300ms);
  writer.take(), other.take();
  for(int i = 0; i < 20; i++)
  {
    writer.send_binary(osc_float("/f", float(i)));
    std::this_thread::sleep_for(20ms);
  }
  std::this_thread::sleep_for(500ms);
  const auto back = ws_client::messages(writer.take(), "/f").size();
  const auto forwarded = ws_client::messages(other.take(), "/f").size();
  std::printf("echo-sender: %s, echo on: 20 writes; %zu came back to the writer, %zu reached the other client\n",
              s.name(), back, forwarded);
  CHECK(forwarded == 20);
  CHECK(back == 0);
}

TEST_CASE("WebSocket: a client's write is not sent back to it, asio server", "[echo-sender]")
{
  check_echo_spares_sender(true);
}

TEST_CASE(
    "WebSocket: a client's write is not sent back to it, C API server",
    "[echo-sender]")
{
  check_echo_spares_sender(false);
}

// The server sets a value while a client writes another one to the same
// parameter. Without arbitration each gets the other's value, and they keep
// different values. The proposal says nothing about this case: the server
// makes the decision, and sends its final value back to the client whose
// write crossed one of its values (set_crossing_window).
static void check_crossing_converges(bool use_asio)
{
  server_under_test s{use_asio, [](auto& root) { add_float(root, "/f", 0.f); }};
  ws_client c{s.ws()};
  REQUIRE(c.connected());
  c.command("LISTEN", "/f");
  std::this_thread::sleep_for(300ms);
  c.take();
  int diverged = 0;
  for(int i = 0; i < 20; i++)
  {
    const float mine = 100.f + i, theirs = 200.f + i;
    s.param("/f")->push_value(mine);     // the server sets a value
    c.send_binary(osc_float("/f", theirs)); // the client writes before it reads
    std::this_thread::sleep_for(300ms);
    float client = theirs;
    for(auto& m : ws_client::messages(c.take(), "/f"))
      client = m.value; // the client applies what it receives after its write
    const float server = as_float(s.param("/f")->value());
    if(client != server)
      diverged++;
  }
  std::printf("crossing: %s: %d of 20 crossing writes end with different values\n", s.name(), diverged);
  CHECK(diverged == 0);
}

TEST_CASE("WebSocket: crossing writes end on the same value, asio server", "[crossing]")
{
  check_crossing_converges(true);
}

TEST_CASE(
    "WebSocket: crossing writes end on the same value, C API server",
    "[crossing]")
{
  check_crossing_converges(false);
}

// ATTRIBUTES_CHANGED is not in the proposal, nor declared in EXTENSIONS. It
// should not come twice for one new parameter, nor after the node is removed.
TEST_CASE("WebSocket: ATTRIBUTES_CHANGED once on add, none on removal", "[attributes-changed]")
{
  const bool use_asio = GENERATE(false, true);
  server_under_test s{use_asio, [](auto& root) { add_float(root, "/f", 0.f); }};
  ws_client c{s.ws()};
  REQUIRE(c.connected());
  std::this_thread::sleep_for(300ms);
  c.take();
  add_float(s.dev().get_root_node(), "/added", 0.25f);
  std::this_thread::sleep_for(500ms);
  auto on_add = c.take();
  s.dev().get_root_node().remove_child("added");
  std::this_thread::sleep_for(500ms);
  auto on_remove = c.take();
  for(auto& f : on_add)
    if(f.text)
      std::printf("attributes-changed: %s: on add: %s\n", s.name(), f.data.c_str());
  for(auto& f : on_remove)
    if(f.text)
      std::printf("attributes-changed: %s: on removal: %s\n", s.name(), f.data.c_str());
  const int added = ws_client::count_text(on_add, "ATTRIBUTES_CHANGED");
  const int removed = ws_client::count_text(on_remove, "ATTRIBUTES_CHANGED");
  std::printf("attributes-changed: %s: %d ATTRIBUTES_CHANGED on add, %d after PATH_REMOVED\n", s.name(), added, removed);
  CHECK(added <= 1);
  CHECK(removed == 0);
}

// The client side: a mirror that follows a parameter added on the server gets
// its value once, not once per notification.
TEST_CASE("Mirror: the value of an added parameter arrives once", "[mirror-added-value]")
{
  const float initial = GENERATE(0.5f, 0.f);
  asio_server s;
  add_float(s.root(), "/node/first", 0.f);
  s.start();
  auto client = std::make_shared<ossia::net::generic_device>(
      std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
          "ws://127.0.0.1:" + std::to_string(s.ws), free_port(false)),
      "mirror");
  REQUIRE(client->get_protocol().update(client->get_root_node()));
  auto* mirror = dynamic_cast<ossia::oscquery::oscquery_mirror_protocol*>(&client->get_protocol());
  REQUIRE(mirror);

  std::mutex m;
  std::vector<ossia::value> received;
  struct follower
  {
    std::mutex* m;
    std::vector<ossia::value>* received;
    void on_created(const ossia::net::parameter_base& p)
    {
      auto* mm = m;
      auto* r = received;
      const_cast<ossia::net::parameter_base&>(p).add_callback([mm, r](const ossia::value& v) {
        std::lock_guard l{*mm};
        r->push_back(v);
      });
    }
  } f{&m, &received};
  client->on_parameter_created.connect<&follower::on_created>(f);
  auto poll = [&](int ms) {
    for(int t = 0; t < ms; t += 5)
    {
      mirror->run_commands();
      std::this_thread::sleep_for(5ms);
    }
  };

  add_float(s.root(), "/node/late", initial);
  poll(1000);
  std::size_t when_added;
  {
    std::lock_guard l{m};
    when_added = received.size();
    received.clear();
  }
  ossia::net::find_node(s.root(), "/node/late")->get_parameter()->push_value(0.75f);
  poll(1000);
  std::size_t after_change;
  {
    std::lock_guard l{m};
    after_change = received.size();
  }
  client->on_parameter_created.disconnect<&follower::on_created>(f);
  client.reset();
  std::printf("mirror-added-value: initial %g: %zu value(s) when added, %zu after one change\n", initial,
              when_added, after_change);
  CHECK(when_added == 1);
  CHECK(after_change == 1);
}
