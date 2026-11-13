// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// What the OSCQuery servers send and accept over the WebSocket. Both servers
// where it applies: the one of the C API (oscquery::oscquery_server_protocol,
// zeroconf disabled) and the one of an ossia.device in Max
// (oscquery_asio::oscquery_server_protocol under a multiplex; build with
// OSSIA_DNSSD=OFF to keep it off mDNS). Loopback.
#include "ReproCommon.hpp"

#include <ossia/network/base/node_attributes.hpp>
#include <ossia/network/exceptions.hpp>
#include <ossia/network/oscquery/oscquery_mirror.hpp>
#include <ossia/network/sockets/websocket_server_beast.hpp>
#include <ossia/protocols/oscquery/oscquery_mirror_asio.hpp>
#include <ossia/network/value/value_conversion.hpp>

#include "include_catch.hpp"

#include <functional>
#include <map>

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

namespace
{
// An OSCQuery server written by hand, as another implementation may be: it
// signals a change of its tree by PATH_CHANGED, which the servers of this
// library do not send. Its tree: /f and the nodes the test sets, each a
// float.
struct changing_server
{
  ossia::net::network_context_ptr ctx = ossia::net::create_network_context();
  ossia::net::websocket_server_beast ws{ctx};
  uint16_t port = free_port(true);
  uint16_t osc = free_port(false);
  std::mutex mutex;
  std::vector<ossia::net::ws_connection_handle> clients;
  std::map<std::string, float> nodes{{"f", 0.5f}};
  std::thread thread;

  changing_server()
  {
    ws.set_open_handler([this](ossia::net::ws_connection_handle hdl) {
      std::lock_guard lock{mutex};
      clients.push_back(hdl);
    });
    ws.set_close_handler([](ossia::net::ws_connection_handle) {});
    ws.set_message_handler(
        [this](const ossia::net::ws_connection_handle&, ossia::net::ws_opcode,
               const std::string& text) { return answer(text); });
    ws.listen(port);
    thread = std::thread([this] { ws.run(); });
    std::this_thread::sleep_for(100ms);
  }
  ~changing_server()
  {
    ws.stop();
    ossia::net::stop_network_context(*ctx);
    thread.join();
  }
  std::string url() const { return "ws://127.0.0.1:" + std::to_string(port); }

  static std::string node_json(const std::string& name, float v)
  {
    return "{\"FULL_PATH\":\"/" + name + "\",\"TYPE\":\"f\",\"ACCESS\":3,\"VALUE\":"
           + std::to_string(v) + "}";
  }
  ossia::net::server_reply answer(const std::string& text)
  {
    ossia::net::server_reply reply;
    reply.type = ossia::net::server_reply::data_type::json;
    if(text.empty() || text[0] != '/')
      return reply; // a command: LISTEN, START_OSC_STREAMING
    if(text.find("HOST_INFO") != std::string::npos)
    {
      reply.data = "{\"NAME\":\"changing\",\"OSC_PORT\":" + std::to_string(osc)
                   + ",\"OSC_TRANSPORT\":\"UDP\",\"EXTENSIONS\":{\"LISTEN\":true,"
                     "\"PATH_CHANGED\":true,\"VALUE\":true}}";
      return reply;
    }
    const auto query = text.find('?');
    const std::string path = text.substr(0, query);
    std::lock_guard lock{mutex};
    if(path == "/")
    {
      std::string contents;
      for(auto& [name, v] : nodes)
        contents += (contents.empty() ? "\"" : ",\"") + name + "\":" + node_json(name, v);
      reply.data = "{\"FULL_PATH\":\"/\",\"ACCESS\":0,\"CONTENTS\":{" + contents + "}}";
      return reply;
    }
    auto it = nodes.find(path.substr(1));
    if(it == nodes.end())
      throw ossia::node_not_found_error{path};
    reply.data = query == std::string::npos
                     ? node_json(it->first, it->second)
                     : "{\"VALUE\":" + std::to_string(it->second) + "}";
    return reply;
  }
  void set(const std::string& name, float v)
  {
    std::lock_guard lock{mutex};
    nodes[name] = v;
  }
  void remove(const std::string& name)
  {
    std::lock_guard lock{mutex};
    nodes.erase(name);
  }
  void path_changed(const std::string& path)
  {
    std::vector<ossia::net::ws_connection_handle> to;
    {
      std::lock_guard lock{mutex};
      to = clients;
    }
    for(auto& c : to)
      ws.send_message(c, "{\"COMMAND\":\"PATH_CHANGED\",\"DATA\":\"" + path + "\"}");
  }
};

// The mirror of an ossia client (oscquery_mirror_protocol, which applies the
// tree on the thread that runs its commands) or the asio mirror (which applies
// it on the thread of its context).
struct mirror_under_test
{
  bool asio;
  ossia::net::network_context_ptr ctx;
  std::unique_ptr<ossia::net::generic_device> dev;
  ossia::oscquery::oscquery_mirror_protocol* mirror{};
  std::atomic_bool polling{true};
  std::thread poller;
  bool updated{};

  mirror_under_test(bool use_asio, const std::string& url)
      : asio{use_asio}
  {
    if(asio)
    {
      ctx = ossia::net::create_network_context();
      auto p = std::make_unique<ossia::oscquery_asio::oscquery_mirror_asio_protocol>(ctx, url);
      auto* proto = p.get();
      dev = std::make_unique<ossia::net::generic_device>(std::move(p), "changing_asio");
      updated = proto->update(dev->get_root_node());
      poller = std::thread([this] {
        while(polling)
        {
          ctx->context.poll();
          std::this_thread::sleep_for(1ms);
        }
      });
    }
    else
    {
      auto p = std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(url, free_port(false));
      mirror = p.get();
      dev = std::make_unique<ossia::net::generic_device>(std::move(p), "changing");
      updated = mirror->update(dev->get_root_node());
    }
  }
  ~mirror_under_test()
  {
    polling = false;
    if(poller.joinable())
      poller.join();
    dev.reset();
  }
  const char* name() const { return asio ? "asio mirror" : "mirror"; }
  bool wait(const std::function<bool()>& done, int ms = 3000)
  {
    for(int t = 0; t < ms; t += 10)
    {
      if(mirror)
        mirror->run_commands();
      if(done())
        return true;
      std::this_thread::sleep_for(10ms);
    }
    return false;
  }
  ossia::net::node_base* node(const std::string& path)
  {
    return ossia::net::find_node(dev->get_root_node(), path);
  }
  // A node that is there and not a zombie
  bool alive(const std::string& path)
  {
    auto* n = node(path);
    return n && !ossia::net::get_zombie(n->get_extended_attributes());
  }
  float value(const std::string& path)
  {
    auto* n = node(path);
    auto* p = n ? n->get_parameter() : nullptr;
    return p ? ossia::convert<float>(p->value()) : -1.f;
  }
};
}

// Proposal, line 158: PATH_CHANGED says the structure of an address changed,
// and the client asks for it: a node that appears, changes or goes, or the
// whole tree.
TEST_CASE("Mirror: PATH_CHANGED reads the address again", "[path-changed]")
{
  const bool use_asio = GENERATE(false, true);
  changing_server s;
  mirror_under_test m{use_asio, s.url()};
  REQUIRE(m.updated);
  REQUIRE(m.wait([&] { return m.alive("/f"); }));
  std::this_thread::sleep_for(300ms); // the WebSocket is up

  s.set("new", 1.5f);
  s.path_changed("/new");
  const bool appeared = m.wait([&] { return m.alive("/new"); });
  const float first = m.value("/new");

  s.set("new", 2.5f);
  s.path_changed("/new");
  const bool changed = m.wait([&] { return m.value("/new") == 2.5f; });

  s.remove("new");
  s.path_changed("/new");
  const bool gone = m.wait([&] { return !m.alive("/new"); });

  s.set("late", 0.25f);
  s.path_changed("/");
  const bool whole = m.wait([&] { return m.alive("/late"); });

  std::printf("path-changed: %s: appeared %d (value %g), changed %d, gone %d, whole tree %d, /f kept %d\n",
              m.name(), int(appeared), first, int(changed), int(gone), int(whole),
              int(m.alive("/f")));
  CHECK(appeared);
  CHECK(first == 1.5f);
  CHECK(changed);
  CHECK(gone);
  CHECK(whole);
  CHECK(m.alive("/f"));
}

namespace
{
// The server of an ossia.device on every interface, as expose oscquery leaves
// it: a client may then reach it over IPv6, which the dual-stack listener of
// this base accepts.
struct open_asio_server
{
  ossia::net::network_context_ptr ctx = ossia::net::create_network_context();
  std::unique_ptr<ossia::net::generic_device> dev;
  ossia::net::multiplex_protocol* multiplex{};
  uint16_t osc = free_port(false);
  uint16_t ws = free_port(true);
  std::thread net;

  open_asio_server()
  {
    auto mp = std::make_unique<ossia::net::multiplex_protocol>();
    multiplex = mp.get();
    dev = std::make_unique<ossia::net::generic_device>(std::move(mp), "open");
    add_float(dev->get_root_node(), "/f", 0.f);
    multiplex->expose_to(std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(
        ctx, osc, ws, false));
    net = std::thread([this] { ctx->run(); });
    std::this_thread::sleep_for(300ms);
  }
  ~open_asio_server()
  {
    ossia::net::stop_network_context(*ctx);
    if(net.joinable())
      net.join();
  }
  ossia::net::parameter_base* param()
  {
    return ossia::net::find_node(dev->get_root_node(), "/f")->get_parameter();
  }
};
}

// A client that reaches the server by a name, as ossia.client connect <name>
// does, resolves ::1 before 127.0.0.1. The OSC side of OSCQuery is IPv4 only
// (oscpack): a client that arrived over IPv6 would be streamed its values at
// an address it cannot receive on, and would get the tree and no value. The
// server listens on IPv4, so such a client falls back to 127.0.0.1, which the
// server sees, and the values reach it.
TEST_CASE("Mirror: a client that resolves ::1 first falls back to IPv4 and gets the values",
          "[mirror-ipv6]")
{
  const bool over_ipv6 = GENERATE(false, true);
  open_asio_server s;
  // localhost resolves to ::1 first, as the name an mDNS announcement gives does
  const std::string host = over_ipv6 ? "localhost" : "127.0.0.1";
  auto client = std::make_shared<ossia::net::generic_device>(
      std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
          "ws://" + host + ":" + std::to_string(s.ws), free_port(false)),
      "mirror");
  auto* mirror = dynamic_cast<ossia::oscquery::oscquery_mirror_protocol*>(&client->get_protocol());
  REQUIRE(mirror);
  REQUIRE(mirror->update(client->get_root_node()));

  auto* node = ossia::net::find_node(client->get_root_node(), "/f");
  REQUIRE(node);
  auto* p = node->get_parameter();
  REQUIRE(p);
  std::mutex m;
  std::vector<float> got;
  p->add_callback([&](const ossia::value& v) {
    std::lock_guard l{m};
    got.push_back(ossia::convert<float>(v));
  });
  // The LISTEN of the callback, and the client's own port, reach the server
  for(int t = 0; t < 100; t++)
  {
    mirror->run_commands();
    std::this_thread::sleep_for(10ms);
  }

  s.param()->push_value(0.75f);
  for(int t = 0; t < 200; t++)
  {
    mirror->run_commands();
    std::this_thread::sleep_for(10ms);
    std::lock_guard l{m};
    if(!got.empty())
      break;
  }
  // The address the server sees: a client that fell back is at 127.0.0.1
  const auto peers = http_get(s.ws, "/?PEERS");
  std::lock_guard l{m};
  std::printf("mirror-ipv6: client at %s: %zu value(s) after a push of 0.75: %s; the server sees %s\n",
              host.c_str(), got.size(), got.empty() ? "none" : std::to_string(got.back()).c_str(),
              peers.body.find("127.0.0.1") != std::string::npos ? "127.0.0.1"
                  : (peers.body.find("::1") != std::string::npos ? "::1" : "another address"));
  CHECK(!got.empty());
  if(!got.empty())
    CHECK(got.back() == 0.75f);
  CHECK(peers.body.find("127.0.0.1") != std::string::npos);
  CHECK(peers.body.find("::1") == std::string::npos);
}
