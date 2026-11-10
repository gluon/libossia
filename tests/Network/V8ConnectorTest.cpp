// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// Headless test of the Max-independent part of ossia.v8connector
// (src/ossia-max/src/v8connector_branch.cpp).
//
// The device is built the way ossia-max builds it (generic_device over a
// multiplex protocol, oscquery_asio server on a network context). A parameter
// is created by hand, as an ossia.parameter does; two branches with the same
// name are created, as two ossia.v8connector instances do; the tree is then
// read back through an OSCQuery mirror client.

#include <ossia/detail/config.hpp>

#include <ossia/network/base/node_attributes.hpp>
#include <ossia/network/base/node_functions.hpp>
#include <ossia/network/common/complex_type.hpp>
#include <ossia/network/context.hpp>
#include <ossia/network/context_functions.hpp>
#include <ossia/network/dataspace/dataspace_visitors.hpp>
#include <ossia/network/domain/domain.hpp>
#include <ossia/network/domain/domain_functions.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/local/local.hpp>
#include <ossia/network/oscquery/oscquery_mirror.hpp>
#include <ossia/network/value/vec.hpp>
#include <ossia/protocols/oscquery/oscquery_server_asio.hpp>

#include "include_catch.hpp"
#include <ossia/network/oscquery/detail/json_writer.hpp>

#include <client_mirror.hpp>
#include <device_bundles.hpp>
#include <device_network.hpp>
#include <v8connector_branch.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <set>
#include <string>
#include <thread>

using namespace ossia::max_binding::v8connector_detail;

namespace
{
// Drain a branch after a refused call: the queue holds exactly that refusal.
void expect_refused(branch& b, const char* address)
{
  auto in = b.drain();
  REQUIRE(in.size() == 1);
  REQUIRE(in[0].address == "refused");
  REQUIRE(in[0].value.get<std::vector<ossia::value>>()[0] == ossia::value(std::string(address)));
}
}

namespace
{

// A free port, chosen by the system; with OSSIA_TEST_PORTS=<first>-<last>,
// the next free one in that range, in turn, so that the tests of a machine
// shared with other benches stay in their own ports.
uint16_t free_port_in_range(bool tcp)
{
  const char* range = std::getenv("OSSIA_TEST_PORTS");
  if(!range)
    return 0;
  int first = 0, last = 0;
  if(std::sscanf(range, "%d-%d", &first, &last) != 2 || first <= 0 || last < first || last > 65535)
    return 0;
  static int next = 0;
  for(int i = 0; i <= last - first; i++)
  {
    const int port = first + (next++ % (last - first + 1));
    int fd = ::socket(AF_INET, tcp ? SOCK_STREAM : SOCK_DGRAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    const bool ok = ::bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0;
    ::close(fd);
    if(ok)
      return static_cast<uint16_t>(port);
  }
  return 0;
}

uint16_t free_port(bool tcp)
{
  if(const auto port = free_port_in_range(tcp))
    return port;
  int fd = ::socket(AF_INET, tcp ? SOCK_STREAM : SOCK_DGRAM, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;
  ::bind(fd, (sockaddr*)&addr, sizeof(addr));
  socklen_t len = sizeof(addr);
  ::getsockname(fd, (sockaddr*)&addr, &len);
  ::close(fd);
  return ntohs(addr.sin_port);
}

std::vector<attribute>
attrs(std::initializer_list<std::pair<const char*, scalars>> l)
{
  std::vector<attribute> out;
  for(auto& [n, v] : l)
    out.push_back({n, v});
  return out;
}

leaf_spec spec_of(const char* address, std::vector<attribute> a)
{
  leaf_spec spec;
  auto err = make_leaf_spec(address, a, spec);
  REQUIRE(err.empty());
  return spec;
}

std::string S(const char* s)
{
  return s;
}

}

TEST_CASE("v8connector_declarations", "v8connector_declarations")
{
  leaf_spec spec;

  SECTION("numeric types require min and max")
  {
    auto err = make_leaf_spec("cutoff", attrs({{"type", {S("float")}}}), spec);
    REQUIRE(!err.empty());
    REQUIRE(err.find("@min and @max") != std::string::npos);

    REQUIRE(make_leaf_spec("cutoff", attrs({{"type", {S("float")}}, {"min", {20}}, {"max", {20000}}}), spec).empty());
    REQUIRE(make_leaf_spec("cutoff", attrs({{"type", {S("float")}}, {"range", {20, 20000}}}), spec).empty());
    REQUIRE(spec.min == std::vector<float>{20.f});
    REQUIRE(spec.max == std::vector<float>{20000.f});

    REQUIRE(make_leaf_spec("pos", attrs({{"type", {S("vec2f")}}, {"range", {-1, 1}}}), spec).empty());
    REQUIRE(spec.min == std::vector<float>{-1.f, -1.f});
    REQUIRE(spec.max == std::vector<float>{1.f, 1.f});

    REQUIRE(!make_leaf_spec("pos", attrs({{"type", {S("vec2f")}}, {"range", {-1, 1, 0}}}), spec).empty());
    REQUIRE(!make_leaf_spec("x", attrs({{"type", {S("float")}}, {"min", {1}}, {"max", {0}}}), spec).empty());
    REQUIRE(!make_leaf_spec("x", attrs({{"type", {S("float")}}, {"range", {0, 1}}, {"min", {0}}}), spec).empty());
  }

  SECTION("non numeric types take no range, strings take an enumeration")
  {
    REQUIRE(make_leaf_spec("s", attrs({{"type", {S("string")}}}), spec).empty());
    REQUIRE(make_leaf_spec("s", attrs({{"type", {S("symbol")}}, {"range", {S("a"), S("b")}}}), spec).empty());
    REQUIRE(spec.type == "string");
    REQUIRE(spec.enumeration == std::vector<std::string>{"a", "b"});
    REQUIRE(!make_leaf_spec("b", attrs({{"type", {S("bool")}}, {"min", {0}}, {"max", {1}}}), spec).empty());
    REQUIRE(make_leaf_spec("i", attrs({{"type", {S("impulse")}}, {"critical", {1}}}), spec).empty());
    REQUIRE(spec.critical);
  }

  SECTION("unknown things are refused")
  {
    REQUIRE(!make_leaf_spec("x", attrs({{"min", {0}}, {"max", {1}}}), spec).empty());
    REQUIRE(!make_leaf_spec("x", attrs({{"type", {S("flt")}}, {"min", {0}}, {"max", {1}}}), spec).empty());
    REQUIRE(!make_leaf_spec("x", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}, {"colour", {S("red")}}}), spec).empty());
    REQUIRE(!make_leaf_spec("x", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}, {"unit", {S("gain.decibelz")}}}), spec).empty());
    REQUIRE(!make_leaf_spec("x", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}, {"clip", {S("sometimes")}}}), spec).empty());
    REQUIRE(!make_leaf_spec("", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}}), spec).empty());
    REQUIRE(!make_leaf_spec("a//b", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}}), spec).empty());
  }

  SECTION("addresses are normalized")
  {
    REQUIRE(normalize_relative_address("/gain/") == "gain");
    REQUIRE(normalize_relative_address(" engine/seed ") == "engine/seed");
    REQUIRE(normalize_relative_address("dev:/x").empty());
  }
}

TEST_CASE("v8connector_tree", "v8connector_tree")
{
  auto ctx = ossia::net::create_network_context();
  auto proto = std::make_unique<ossia::net::multiplex_protocol>();
  auto* multiplex = proto.get();
  ossia::net::generic_device dev{std::move(proto), "v8_headless"};
  auto& root = dev.get_root_node();

  // A parameter created by hand, as ossia.parameter does
  {
    auto& n = ossia::net::create_node(root, "/manual/gain");
    auto* p = ossia::try_setup_parameter("float", n);
    p->set_domain(ossia::make_domain(0.f, 1.f));
    p->push_value(0.25f);
  }

  // Two branch nodes with the same name, as two instances do
  auto& bn1 = ossia::net::create_node(root, "/synth");
  auto& bn2 = ossia::net::create_node(root, "/synth");
  REQUIRE(bn1.osc_address() == "/synth");
  REQUIRE(bn2.osc_address() == "/synth.1");

  branch b1, b2;
  int lost = 0;
  b1.set_node_lost_callback([&] { ++lost; });
  b1.attach(bn1);
  b2.attach(bn2);

  const auto gain = spec_of(
      "gain", attrs({{"type", {S("float")}}, {"min", {-60}}, {"max", {6}}, {"unit", {S("gain.db")}},
                     {"default", {-12}}, {"description", {S("main gain")}}, {"clip", {S("both")}}}));
  REQUIRE(b1.add(gain).empty());
  REQUIRE(b2.add(gain).empty());
  REQUIRE(b1.add(spec_of("cutoff", attrs({{"type", {S("float")}}, {"range", {20, 20000}}, {"unit", {S("time.hz")}}}))).empty());
  REQUIRE(b1.add(spec_of("voices", attrs({{"type", {S("int")}}, {"min", {1}}, {"max", {16}}, {"default", {4}}}))).empty());
  REQUIRE(b1.add(spec_of("pos", attrs({{"type", {S("vec2f")}}, {"range", {-1, 1}}}))).empty());
  REQUIRE(b1.add(spec_of("mode", attrs({{"type", {S("string")}}, {"range", {S("sine"), S("saw")}}, {"default", {S("sine")}}}))).empty());
  REQUIRE(b1.add(spec_of("engine/reset", attrs({{"type", {S("impulse")}}, {"critical", {1}}}))).empty());
  REQUIRE(b1.add(gain).empty()); // the same declaration again is accepted and does nothing
  REQUIRE(b1.size() == 6);
  {
    auto other = gain;
    other.max = {12.f};
    REQUIRE(!b1.add(other).empty()); // other attributes at a declared address: refused
  }

  SECTION("the default of a leaf comes out of its own branch once it is created")
  {
    // as ossia-max outputs the defaults of its parameters after they registered
    auto in = b1.drain();
    REQUIRE(in.size() == 4); // the three defaults, then the refusal of the second gain
    REQUIRE(in[3].address == "refused");
    in.pop_back();
    REQUIRE(in[0].address == "gain");
    REQUIRE(in[0].value == ossia::value(-12.f));
    REQUIRE(in[1].address == "voices");
    REQUIRE(in[1].value == ossia::value(4));
    REQUIRE(in[2].address == "mode");
    REQUIRE(in[2].value == ossia::value(std::string("sine")));
    REQUIRE(b1.drain().empty());
  }

  auto* gain1 = ossia::net::find_node(root, "/synth/gain")->get_parameter();
  auto* gain2 = ossia::net::find_node(root, "/synth.1/gain")->get_parameter();
  REQUIRE(gain1);
  REQUIRE(gain2);

  SECTION("set: no echo to the pushing branch, clipping, validation")
  {
    int other_listener = 0;
    gain1->add_callback([&](const ossia::value&) { ++other_listener; });
    b1.drain();

    REQUIRE(b1.set("gain", {-6}).empty());
    REQUIRE(b1.drain().empty());
    REQUIRE(other_listener == 1);
    REQUIRE(gain1->value() == ossia::value(-6.f));

    REQUIRE(b1.set("gain", {100}).empty());
    // stored as sent, as ossia.parameter does; bounded when it leaves
    REQUIRE(gain1->value() == ossia::value(100.f));
    // the value that leaves differs from the value sent: it comes back to the
    // instance that sent it, as if it came from another source
    auto corrected = b1.drain();
    REQUIRE(corrected.size() == 1);
    REQUIRE(corrected[0].address == "gain");
    REQUIRE(corrected[0].value == ossia::value(6.f));
    REQUIRE(b1.set("gain", {-116}).empty());
    corrected = b1.drain();
    REQUIRE(corrected.size() == 1);
    REQUIRE(corrected[0].value == ossia::value(-60.f));
    REQUIRE(b1.set("gain", {-6}).empty()); // in range: nothing comes back
    REQUIRE(b1.drain().empty());
    REQUIRE(b1.set("voices", {3.7f}).empty()); // converted to int: the effective value comes back
    corrected = b1.drain();
    REQUIRE(corrected.size() == 1);
    REQUIRE(corrected[0].address == "voices");
    REQUIRE(corrected[0].value == ossia::value(4));
    REQUIRE(b1.set("voices", {4}).empty());
    REQUIRE(b1.drain().empty());

    REQUIRE(!b1.set("nothere", {1}).empty());
    expect_refused(b1, "nothere");
    REQUIRE(!b1.set("pos", {1}).empty());
    expect_refused(b1, "pos");
    REQUIRE(b1.set("pos", {0.5f, -0.5f}).empty());

    int wakes = 0;
    b1.set_wake_callback([&] { ++wakes; });
    gain1->push_value(0.5f);
    auto in = b1.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "gain");
    REQUIRE(in[0].value == ossia::value(0.5f));
    REQUIRE(wakes == 1);

    // another source pushes out of range straight on the parameter, as an
    // ossia.remote does: the parameter stores it, the branch outputs it bounded
    gain1->push_value(-116.f);
    REQUIRE(gain1->value() == ossia::value(-116.f));
    in = b1.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].value == ossia::value(-60.f));
    auto* voices1 = ossia::net::find_node(root, "/synth/voices")->get_parameter();
    voices1->set_bounding(ossia::bounding_mode::CLIP);
    voices1->push_value(40);
    in = b1.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "voices");
    REQUIRE(in[0].value == ossia::value(16));
  }

  SECTION("the declared leaves are served by the oscquery server next to the hand made parameter")
  {
    const uint16_t osc = free_port(false);
    const uint16_t ws = free_port(true);
    multiplex->expose_to(
        std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, osc, ws, false));
    std::thread net([&] { ctx->run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    {
      ossia::net::generic_device client{
          std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
              "ws://127.0.0.1:" + std::to_string(ws), free_port(false)),
          "client"};
      REQUIRE(client.get_protocol().update(client.get_root_node()));
      auto& croot = client.get_root_node();

      REQUIRE(ossia::net::find_node(croot, "/manual/gain") != nullptr);
      auto* sg = ossia::net::find_node(croot, "/synth/gain");
      REQUIRE(sg != nullptr);
      REQUIRE(ossia::net::find_node(croot, "/synth.1/gain") != nullptr);
      REQUIRE(ossia::net::find_node(croot, "/synth/engine/reset") != nullptr);

      auto* p = sg->get_parameter();
      REQUIRE(p != nullptr);
      REQUIRE(p->get_value_type() == ossia::val_type::FLOAT);
      REQUIRE(p->get_domain().get_min() == ossia::value(-60.f));
      REQUIRE(p->get_domain().get_max() == ossia::value(6.f));
      REQUIRE(p->get_bounding() == ossia::bounding_mode::CLIP);
      REQUIRE(ossia::get_pretty_unit_text(p->get_unit()) == "gain.db");
      REQUIRE(ossia::net::get_description(*sg) == std::optional<std::string>{"main gain"});

      auto* pos = ossia::net::find_node(croot, "/synth/pos")->get_parameter();
      REQUIRE(pos->get_value_type() == ossia::val_type::VEC2F);
      REQUIRE(pos->get_domain().get_min() == ossia::value(ossia::make_vec(-1.f, -1.f)));
      REQUIRE(pos->get_domain().get_max() == ossia::value(ossia::make_vec(1.f, 1.f)));

      auto* voices = ossia::net::find_node(croot, "/synth/voices")->get_parameter();
      REQUIRE(voices->value() == ossia::value(4));

      auto* mode = ossia::net::find_node(croot, "/synth/mode")->get_parameter();
      REQUIRE(mode->get_value_type() == ossia::val_type::STRING);
      REQUIRE(ossia::get_values(mode->get_domain()).size() == 2);
    }

    ossia::net::stop_network_context(*ctx);
    net.join();
  }

  SECTION("the output is filtered by the parameter, as at the outlet of every ossia object")
  {
    // @repetitions 0: the repetition filter is on, two identical values give one output
    REQUIRE(b1.add(spec_of("rep", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}, {"repetitions", {0}}}))).empty());
    // @repetitions 1 (the default): two identical values give two outputs
    REQUIRE(b1.add(spec_of("norep", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}}))).empty());
    // @mute 1: nothing leaves
    REQUIRE(b1.add(spec_of("muted", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}, {"mute", {1}}}))).empty());
    // @enable 0: nothing leaves either
    REQUIRE(b1.add(spec_of("off", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}, {"enable", {0}}}))).empty());
    b1.drain();

    auto* rep = ossia::net::find_node(root, "/synth/rep")->get_parameter();
    auto* norep = ossia::net::find_node(root, "/synth/norep")->get_parameter();
    auto* muted = ossia::net::find_node(root, "/synth/muted")->get_parameter();
    auto* off = ossia::net::find_node(root, "/synth/off")->get_parameter();
    REQUIRE(rep->get_repetition_filter() == ossia::repetition_filter::ON);
    REQUIRE(norep->get_repetition_filter() == ossia::repetition_filter::OFF);
    REQUIRE(muted->get_muted());
    REQUIRE(off->get_disabled());

    // another source pushes the same value twice
    rep->push_value(0.3f);
    rep->push_value(0.3f);
    auto in = b1.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "rep");
    REQUIRE(in[0].value == ossia::value(0.3f));
    rep->push_value(0.4f);
    REQUIRE(b1.drain().size() == 1);

    norep->push_value(0.3f);
    norep->push_value(0.3f);
    in = b1.drain();
    REQUIRE(in.size() == 2);
    REQUIRE(in[0].address == "norep");
    REQUIRE(in[1].value == ossia::value(0.3f));

    muted->push_value(0.5f);
    off->push_value(0.5f);
    REQUIRE(b1.drain().empty());
    REQUIRE(muted->value() == ossia::value(0.5f)); // stored, not output

    // the same filter applies to a bound address: the follower of rep sees one value
    branch f;
    f.set_resolution_context({&root}, {&dev});
    REQUIRE(f.bind("synth/rep").empty());
    f.drain();
    rep->push_value(0.7f);
    rep->push_value(0.7f);
    in = f.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "synth/rep");
    REQUIRE(f.bind("synth/muted").empty());
    REQUIRE(f.drain().empty()); // the value found on bind is filtered too
  }

  SECTION("remove and clear")
  {
    REQUIRE(b1.remove("engine/reset"));
    REQUIRE(ossia::net::find_node(root, "/synth/engine") == nullptr); // empty container pruned
    REQUIRE(!b1.remove("engine/reset"));
    REQUIRE(b1.size() == 5);

    b2.clear();
    REQUIRE(ossia::net::find_node(root, "/synth.1") != nullptr);
    REQUIRE(bn2.children().empty());
  }

  SECTION("the enclosing node disappears: declarations survive and come back")
  {
    b1.drain();
    root.remove_child(bn1);
    REQUIRE(lost == 1);
    REQUIRE(!b1.attached());
    REQUIRE(!b1.set("gain", {0}).empty());
    expect_refused(b1, "gain");

    auto& bn3 = ossia::net::create_node(root, "/synth");
    REQUIRE(bn3.osc_address() == "/synth");
    b1.attach(bn3);
    REQUIRE(ossia::net::find_node(root, "/synth/cutoff") != nullptr);
    REQUIRE(ossia::net::find_node(root, "/synth/engine/reset") != nullptr);
    // the defaults come out again after the new registration
    auto in = b1.drain();
    REQUIRE(in.size() == 3);
    REQUIRE(in[0].address == "gain");
    REQUIRE(in[0].value == ossia::value(-12.f));
    REQUIRE(b1.set("gain", {-3}).empty());
  }
}

TEST_CASE("v8connector_bind", "v8connector_bind")
{
  auto proto = std::make_unique<ossia::net::multiplex_protocol>();
  ossia::net::generic_device dev{std::move(proto), "v8_bind"};
  auto& root = dev.get_root_node();

  // A parameter created by hand, as ossia.parameter does
  {
    auto& n = ossia::net::create_node(root, "/manual/gain");
    auto* p = ossia::try_setup_parameter("float", n);
    p->set_domain(ossia::make_domain(0.f, 1.f));
    p->push_value(0.25f);
  }

  // Instance A declares, instance B follows. Both resolve like an ossia.remote
  // placed in a patcher whose device is dev.
  auto& bn_a = ossia::net::create_node(root, "/synth");
  auto& bn_b = ossia::net::create_node(root, "/mirror");
  branch a, b;
  a.attach(bn_a);
  b.attach(bn_b);
  std::vector<std::pair<std::string, std::size_t>> changes;
  b.set_binding_changed_callback(
      [&](const std::string& addr, std::size_t n) { changes.emplace_back(addr, n); });
  a.set_resolution_context({&root}, {&dev});
  b.set_resolution_context({&root}, {&dev});

  const auto gain = spec_of(
      "gain", attrs({{"type", {S("float")}}, {"min", {-60}}, {"max", {6}}, {"default", {-12}}, {"clip", {S("both")}}}));
  REQUIRE(a.add(gain).empty());
  REQUIRE(a.add(spec_of("voices", attrs({{"type", {S("int")}}, {"min", {1}}, {"max", {16}}}))).empty());

  SECTION("bind follows an existing parameter, set goes both ways, no echo")
  {
    REQUIRE(b.bind("synth/gain").empty());
    REQUIRE(b.bound_count("synth/gain") == 1);
    REQUIRE(changes.back() == std::make_pair(std::string("synth/gain"), std::size_t(1)));
    REQUIRE(b.bind("synth/gain").empty()); // twice: accepted, nothing changes
    REQUIRE(b.bound_count("synth/gain") == 1);
    REQUIRE(ossia::net::find_node(root, "/mirror/synth") == nullptr); // nothing created

    // the current value is delivered on bind
    auto in = b.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "synth/gain");
    REQUIRE(in[0].value == ossia::value(-12.f));
    a.drain();

    // B sets: A receives, B does not
    REQUIRE(b.set("synth/gain", {-20}).empty());
    in = a.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "gain");
    REQUIRE(in[0].value == ossia::value(-20.f));
    REQUIRE(b.drain().empty());

    // A sets: B receives under the bound address, A does not
    REQUIRE(a.set("gain", {-10}).empty());
    in = b.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "synth/gain");
    REQUIRE(in[0].value == ossia::value(-10.f));
    REQUIRE(a.drain().empty());

    // the owner's type and range apply to the follower's pushes, and the
    // effective value comes back to the follower that sent it
    REQUIRE(b.set("synth/gain", {100}).empty());
    auto* gp = ossia::net::find_node(root, "/synth/gain")->get_parameter();
    REQUIRE(gp->value() == ossia::value(100.f));
    in = b.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "synth/gain");
    REQUIRE(in[0].value == ossia::value(6.f));
    in = a.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].value == ossia::value(6.f));
    REQUIRE(!b.set("synth/gain", {S("loud")}).empty());
    expect_refused(b, "synth/gain");

    // another source pushes out of range straight on the parameter, as an
    // ossia.remote does: both instances output the bounded value
    gp->push_value(-116.f);
    in = a.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "gain");
    REQUIRE(in[0].value == ossia::value(-60.f));
    in = b.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "synth/gain");
    REQUIRE(in[0].value == ossia::value(-60.f));

    // unbind: nothing flows any more, the parameter is untouched
    REQUIRE(b.unbind("synth/gain"));
    REQUIRE(!b.unbind("synth/gain"));
    expect_refused(b, "synth/gain");
    REQUIRE(a.set("gain", {0}).empty());
    REQUIRE(b.drain().empty());
    REQUIRE(ossia::net::find_node(root, "/synth/gain") != nullptr);
  }

  SECTION("the followed parameter disappears and comes back")
  {
    REQUIRE(b.bind("synth/gain").empty());
    b.drain();
    a.drain();
    REQUIRE(a.remove("gain"));
    REQUIRE(b.bound_count("synth/gain") == 0);
    REQUIRE(changes.back() == std::make_pair(std::string("synth/gain"), std::size_t(0)));
    REQUIRE(!b.set("synth/gain", {1}).empty());
    expect_refused(b, "synth/gain");

    REQUIRE(a.add(gain).empty());
    REQUIRE(b.bound_count("synth/gain") == 1);
    // the follower outputs the value it finds, once: the quiet default did
    // not fire its callback, the resolution read the final value
    auto in = b.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "synth/gain");
    REQUIRE(in[0].value == ossia::value(-12.f));
    // the owner outputs its default locally
    in = a.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "gain");
    REQUIRE(in[0].value == ossia::value(-12.f));
    REQUIRE(b.set("synth/gain", {-30}).empty());
    in = a.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].value == ossia::value(-30.f));

    // the whole owner node disappears (its model registers again) and comes back
    root.remove_child(bn_a);
    REQUIRE(b.bound_count("synth/gain") == 0);
    auto& bn_a2 = ossia::net::create_node(root, "/synth");
    b.drain();
    a.attach(bn_a2);
    REQUIRE(b.bound_count("synth/gain") == 1);
    in = b.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].value == ossia::value(-12.f));
    REQUIRE(b.set("synth/gain", {-40}).empty());
    REQUIRE(ossia::net::find_node(root, "/synth/gain")->get_parameter()->value() == ossia::value(-40.f));
  }

  SECTION("a bind made before the declaration outputs the default once it exists")
  {
    REQUIRE(b.bind("synth/cutoff").empty());
    REQUIRE(b.bound_count("synth/cutoff") == 0);
    REQUIRE(b.drain().empty());
    REQUIRE(a.add(spec_of("cutoff", attrs({{"type", {S("float")}}, {"range", {20, 20000}}, {"default", {1000}}}))).empty());
    REQUIRE(b.bound_count("synth/cutoff") == 1);
    auto in = b.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "synth/cutoff");
    REQUIRE(in[0].value == ossia::value(1000.f));
  }

  SECTION("a bind to a parameter that does not exist yet waits for it")
  {
    REQUIRE(b.bind("later/x").empty());
    REQUIRE(b.bound_count("later/x") == 0);
    auto& n = ossia::net::create_node(root, "/later/x");
    ossia::try_setup_parameter("int", n);
    REQUIRE(b.bound_count("later/x") == 1);
  }

  SECTION("absolute, global and pattern addresses, as ossia.remote")
  {
    REQUIRE(b.bind("/manual/gain").empty());
    REQUIRE(b.bound_count("/manual/gain") == 1);
    REQUIRE(b.bind("v8_bind:/manual/gain").empty());
    REQUIRE(b.bound_count("v8_bind:/manual/gain") == 1);
    REQUIRE(b.bind("synth/*").empty());
    REQUIRE(b.bound_count("synth/*") == 2);
    b.drain();
    REQUIRE(a.set("voices", {8}).empty());
    auto in = b.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "synth/voices");
    REQUIRE(in[0].value == ossia::value(8));
    // a new match of the pattern is followed when it appears
    REQUIRE(a.add(spec_of("cutoff", attrs({{"type", {S("float")}}, {"range", {20, 20000}}}))).empty());
    REQUIRE(b.bound_count("synth/*") == 3);
  }

  SECTION("bind and add do not share an address inside one instance")
  {
    REQUIRE(!b.bind("").empty());
    REQUIRE(a.bind("manual/gain").empty());
    REQUIRE(!a.bind("gain").empty());  // declared by add in a
    REQUIRE(!a.add(spec_of("manual/gain", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}}))).empty());
  }
}

TEST_CASE("v8connector_node_on_first_add", "v8connector_node_on_first_add")
{
  auto proto = std::make_unique<ossia::net::multiplex_protocol>();
  ossia::net::generic_device dev{std::move(proto), "v8_node"};
  auto& root = dev.get_root_node();
  {
    auto& n = ossia::net::create_node(root, "/manual/gain");
    ossia::try_setup_parameter("float", n);
  }

  // The host side: a factory that creates the node under the device root, as
  // ossia.model does, and counts how many times it is asked.
  int created = 0;
  branch b;
  b.set_node_factory([&]() -> ossia::net::node_base* {
    ++created;
    return &ossia::net::create_node(root, "/mirror");
  });
  b.set_resolution_context({&root}, {&dev});

  SECTION("an instance that only binds creates no node, like an ossia.remote")
  {
    b.reattach(); // what registration does
    REQUIRE(created == 0);
    REQUIRE(b.bind("manual/gain").empty());
    REQUIRE(b.bound_count("manual/gain") == 1);
    REQUIRE(created == 0);
    REQUIRE(!b.attached());
    REQUIRE(ossia::net::find_node(root, "/mirror") == nullptr);
    REQUIRE(b.set("manual/gain", {0.5f}).empty());
    REQUIRE(created == 0);
  }

  SECTION("the first add creates the node, which then stays like an ossia.model")
  {
    b.reattach();
    REQUIRE(created == 0);
    REQUIRE(b.add(spec_of("x", attrs({{"type", {S("int")}}, {"min", {0}}, {"max", {10}}}))).empty());
    REQUIRE(created == 1);
    REQUIRE(b.attached());
    REQUIRE(ossia::net::find_node(root, "/mirror/x") != nullptr);
    REQUIRE(b.add(spec_of("y", attrs({{"type", {S("bool")}}}))).empty());
    REQUIRE(created == 1);

    b.clear();
    REQUIRE(b.attached());
    REQUIRE(ossia::net::find_node(root, "/mirror") != nullptr);
    REQUIRE(ossia::net::find_node(root, "/mirror")->children().empty());
    REQUIRE(created == 1);

    REQUIRE(b.add(spec_of("z", attrs({{"type", {S("string")}}}))).empty());
    REQUIRE(b.remove("z"));
    REQUIRE(b.attached());
    REQUIRE(created == 1);

    // the host detaches and registers again (its node was destroyed): the
    // node is asked for again only because there are leaves
    REQUIRE(b.add(spec_of("w", attrs({{"type", {S("impulse")}}}))).empty());
    b.detach();
    root.remove_child(*ossia::net::find_node(root, "/mirror"));
    b.reattach();
    REQUIRE(created == 2);
    REQUIRE(ossia::net::find_node(root, "/mirror/w") != nullptr);
  }

  SECTION("a factory that cannot create yet is asked again later")
  {
    bool ready = false;
    int asked = 0;
    b.set_node_factory([&]() -> ossia::net::node_base* {
      ++asked;
      return ready ? &ossia::net::create_node(root, "/mirror") : nullptr;
    });
    REQUIRE(b.add(spec_of("x", attrs({{"type", {S("bool")}}}))).empty());
    REQUIRE(asked == 1);
    REQUIRE(!b.attached());
    ready = true;
    b.reattach();
    REQUIRE(asked == 2);
    REQUIRE(b.attached());
    REQUIRE(ossia::net::find_node(root, "/mirror/x") != nullptr);
  }
}

namespace
{
std::string served(const ossia::net::node_base& n)
{
  return ossia::oscquery::json_writer::query_namespace(n).GetString();
}

bool is_refusal(const inbound& in, const std::string& address)
{
  if(in.address != "refused" || in.value.get_type() != ossia::val_type::LIST)
    return false;
  const auto& l = in.value.get<std::vector<ossia::value>>();
  return l.size() == 2 && l[0] == ossia::value(address)
         && l[1].get_type() == ossia::val_type::STRING
         && !l[1].get<std::string>().empty();
}
}

TEST_CASE("v8connector_units_clip_refusals", "v8connector_units_clip_refusals")
{
  auto proto = std::make_unique<ossia::net::multiplex_protocol>();
  ossia::net::generic_device dev{std::move(proto), "v8_units"};
  auto& root = dev.get_root_node();
  branch b;
  b.set_node_factory([&]() -> ossia::net::node_base* {
    return &ossia::net::create_node(root, "/types");
  });
  b.set_resolution_context({&root}, {&dev});

  SECTION("color units are kept by the parameter, after creation and after re-registration")
  {
    // the atoms the helper sends for these two declarations
    REQUIRE(b.declare("color", attrs({{"type", {S("vec3f")}}, {"min", {0}}, {"max", {1}},
                                      {"unit", {S("color.rgb")}}, {"default", {1, 0.5f, 0}}})).empty());
    REQUIRE(b.declare("rgba", attrs({{"type", {S("vec4f")}}, {"min", {0}}, {"max", {1}},
                                     {"unit", {S("color.rgba")}}, {"default", {0, 0.5f, 1, 1}}})).empty());
    auto check = [&] {
      auto* c = ossia::net::find_node(root, "/types/color")->get_parameter();
      auto* r = ossia::net::find_node(root, "/types/rgba")->get_parameter();
      REQUIRE(ossia::get_pretty_unit_text(c->get_unit()) == "color.rgb");
      REQUIRE(c->get_value_type() == ossia::val_type::VEC3F);
      REQUIRE(ossia::get_pretty_unit_text(r->get_unit()) == "color.rgba");
      REQUIRE(r->get_value_type() == ossia::val_type::VEC4F);
      REQUIRE(c->value() == ossia::value(ossia::make_vec(1.f, 0.5f, 0.f)));
    };
    check();
    // the parent registers again: its node is destroyed, then recreated
    b.detach();
    root.remove_child(*ossia::net::find_node(root, "/types"));
    b.reattach();
    check();
  }

  SECTION("a leaf is served with a single EXTENDED_TYPE")
  {
    REQUIRE(b.declare("color", attrs({{"type", {S("vec3f")}}, {"min", {0}}, {"max", {1}},
                                      {"unit", {S("color.rgb")}}, {"default", {1, 0.5f, 0}}})).empty());
    REQUIRE(b.declare("rgba", attrs({{"type", {S("vec4f")}}, {"min", {0}}, {"max", {1}},
                                     {"unit", {S("color.rgba")}}, {"default", {0, 0.5f, 1, 1}}})).empty());
    REQUIRE(b.declare("plain", attrs({{"type", {S("vec3f")}}, {"min", {0}}, {"max", {1}}})).empty());
    // a second EXTENDED_TYPE key would hide the first from clients that keep
    // the last duplicate key
    auto check = [&](const char* address, const char* extended_type) {
      const auto json = served(*ossia::net::find_node(root, address));
      INFO(json);
      const auto first = json.find("\"EXTENDED_TYPE\"");
      REQUIRE(first != std::string::npos);
      REQUIRE(json.find("\"EXTENDED_TYPE\"", first + 1) == std::string::npos);
      REQUIRE(json.find(std::string("\"EXTENDED_TYPE\":") + extended_type) != std::string::npos);
      REQUIRE(json.find("\"UNIT\"") == std::string::npos);
    };
    auto check_all = [&] {
      // a color is described by the array of its components
      check("/types/color", "[\"color.rgb.r\",\"color.rgb.g\",\"color.rgb.b\"]");
      check("/types/rgba", "[\"color.rgb.r\",\"color.rgb.g\",\"color.rgb.b\",\"color.rgb.a\"]");
      // without a unit, the node's own extended type
      check("/types/plain", "\"vecf\"");
    };
    check_all();
    // the parent registers again: same form
    b.detach();
    root.remove_child(*ossia::net::find_node(root, "/types"));
    b.reattach();
    check_all();
  }

  SECTION("a type the unit would change is refused, with the reason")
  {
    REQUIRE(!b.declare("rgb", attrs({{"type", {S("vec4f")}}, {"min", {0}}, {"max", {1}},
                                     {"unit", {S("color.rgb")}}})).empty());
    REQUIRE(!b.declare("g", attrs({{"type", {S("int")}}, {"min", {-60}}, {"max", {6}},
                                   {"unit", {S("gain.db")}}})).empty());
    auto in = b.drain();
    REQUIRE(in.size() == 2);
    REQUIRE(is_refusal(in[0], "rgb"));
    const auto reason = in[0].value.get<std::vector<ossia::value>>()[1].get<std::string>();
    REQUIRE(reason.find("requires type 'vec3f'") != std::string::npos);
    REQUIRE(is_refusal(in[1], "g"));
    REQUIRE(ossia::net::find_node(root, "/types") == nullptr); // nothing created
    REQUIRE(b.size() == 0);
  }

  SECTION("clip none is accepted as the free mode and served CLIPMODE none")
  {
    REQUIRE(b.declare("x", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}},
                                  {"clip", {S("none")}}})).empty());
    auto* p = ossia::net::find_node(root, "/types/x")->get_parameter();
    REQUIRE(p->get_bounding() == ossia::bounding_mode::FREE);
    REQUIRE(served(*ossia::net::find_node(root, "/types/x")).find("\"CLIPMODE\":\"none\"")
            != std::string::npos);
    // replaying the same declaration with "none" or "free" is the same leaf
    REQUIRE(b.declare("x", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}},
                                  {"clip", {S("free")}}})).empty());
    REQUIRE(b.size() == 1);
    REQUIRE(!b.declare("y", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}},
                                   {"clip", {S("sometimes")}}})).empty());
  }

  SECTION("every refusal is queued as refused <address> <reason>, nothing is created")
  {
    b.drain();
    REQUIRE(!b.declare("cutoff", attrs({{"type", {S("float")}}})).empty()); // no range
    auto in = b.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(is_refusal(in[0], "cutoff"));
    REQUIRE(ossia::net::find_node(root, "/types/cutoff") == nullptr);
    REQUIRE(b.size() == 0);

    REQUIRE(b.declare("gain", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}})).empty());
    b.drain();
    REQUIRE(!b.declare("gain", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {2}}})).empty());
    REQUIRE(!b.set("nothere", {1}).empty());
    REQUIRE(!b.set("gain", {S("loud")}).empty());
    REQUIRE(!b.remove("nothere"));
    REQUIRE(!b.unbind("nothere"));
    REQUIRE(!b.bind("gain").empty()); // declared by add in this instance
    in = b.drain();
    REQUIRE(in.size() == 6);
    REQUIRE(is_refusal(in[0], "gain"));
    REQUIRE(is_refusal(in[1], "nothere"));
    REQUIRE(is_refusal(in[2], "gain"));
    REQUIRE(is_refusal(in[3], "nothere"));
    REQUIRE(is_refusal(in[4], "nothere"));
    REQUIRE(is_refusal(in[5], "gain"));
    // a refusal wakes the host like a value does
    int wakes = 0;
    b.set_wake_callback([&] { ++wakes; });
    b.refuse("z", "reason");
    REQUIRE(wakes == 1);
  }
}

// A device that a branch follows disappears and comes back under the same
// name, as an ossia.client connected by name does when the zeroconf
// announcement of its server goes away and comes back. The host gives the
// branch the new device, as it does when a device is created; it does not
// tell the branch that the old one is gone, because ossia-max's client clears
// its device before announcing the removal.
TEST_CASE("v8connector_device_return", "v8connector_device_return")
{
  // The server: leaves under /bench, served over OSCQuery.
  auto ctx = ossia::net::create_network_context();
  auto server_multiplex = std::make_unique<ossia::net::multiplex_protocol>();
  auto* multiplex = server_multiplex.get();
  ossia::net::generic_device server{std::move(server_multiplex), "td_bench"};
  std::vector<ossia::net::parameter_base*> leaves;
  for(int i = 0; i < 4; i++)
  {
    auto& n = ossia::net::create_node(server.get_root_node(), "/bench/p" + std::to_string(i));
    auto* p = ossia::try_setup_parameter("float", n);
    p->set_domain(ossia::make_domain(0.f, 1.f));
    p->push_value(0.f);
    leaves.push_back(p);
  }
  const uint16_t osc = free_port(false);
  const uint16_t ws = free_port(true);
  multiplex->expose_to(
      std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, osc, ws, false));
  std::thread net([&] { ctx->run(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // The global device of Max, where the instance registers.
  ossia::net::generic_device max_dev{std::make_unique<ossia::net::multiplex_protocol>(), "max"};
  auto& max_root = max_dev.get_root_node();

  auto connect = [&] {
    auto d = std::make_shared<ossia::net::generic_device>(
        std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
            "ws://127.0.0.1:" + std::to_string(ws), free_port(false)),
        "td_bench");
    REQUIRE(d->get_protocol().update(d->get_root_node()));
    return d;
  };

  // The server pushes the value until the branch receives it: a client that
  // has just connected may not be known to the server yet.
  auto received = [](branch& b, ossia::net::parameter_base& p, const ossia::value& v) {
    for(int i = 0; i < 400; i++)
    {
      if(i % 20 == 0)
        p.push_value(v);
      for(auto& in : b.drain())
        if(in.address == "td_bench:/bench/p0" && in.value == v)
          return true;
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return false;
  };

  branch b;
  std::shared_ptr<ossia::net::generic_device> client = connect();
  b.set_resolution_context({&max_root}, {&max_dev, client.get()});
  REQUIRE(b.bind("td_bench:/bench/p0").empty());
  REQUIRE(b.bound_count("td_bench:/bench/p0") == 1);

  for(int cycle = 0; cycle < 20; cycle++)
  {
    INFO("cycle " << cycle);
    // The device goes away; the host does not call drop_device.
    client.reset();
    REQUIRE(b.bound_count("td_bench:/bench/p0") == 0);

    // It comes back under the same name: the host registers the instance
    // again with the devices it knows.
    client = connect();
    b.set_resolution_context({&max_root}, {&max_dev, client.get()});
    REQUIRE(b.bound_count("td_bench:/bench/p0") == 1);

    // Values are received again.
    const bool ok = received(b, *leaves[0], ossia::value((cycle + 1) / 32.f));
    if(!ok)
    {
      client.reset();
      ossia::net::stop_network_context(*ctx);
      net.join();
    }
    REQUIRE(ok);
  }

  client.reset();
  ossia::net::stop_network_context(*ctx);
  net.join();
}

// An ossia.client connected by name gets its device from the zeroconf
// listener, which builds an OSCQuery mirror and reads the tree once. The tree
// changes the server sends afterwards wait in the mirror's command queue until
// the client polls it.
TEST_CASE("v8connector_client_by_name", "v8connector_client_by_name")
{
  auto ctx = ossia::net::create_network_context();
  auto server_multiplex = std::make_unique<ossia::net::multiplex_protocol>();
  auto* multiplex = server_multiplex.get();
  ossia::net::generic_device server{std::move(server_multiplex), "td_bench"};
  {
    auto& n = ossia::net::create_node(server.get_root_node(), "/bench/first");
    ossia::try_setup_parameter("float", n)->push_value(0.f);
  }
  const uint16_t osc = free_port(false);
  const uint16_t ws = free_port(true);
  multiplex->expose_to(
      std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, osc, ws, false));
  std::thread net([&] { ctx->run(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // As ZeroconfOscqueryListener::addInstance builds it.
  auto client = std::make_shared<ossia::net::generic_device>(
      std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
          "ws://127.0.0.1:" + std::to_string(ws), free_port(false)),
      "td_bench");
  REQUIRE(client->get_protocol().update(client->get_root_node()));
  auto& croot = client->get_root_node();
  REQUIRE(ossia::net::find_node(croot, "/bench/first") != nullptr);

  ossia::net::generic_device max_dev{std::make_unique<ossia::net::multiplex_protocol>(), "max"};
  branch b;
  b.set_resolution_context({&max_dev.get_root_node()}, {&max_dev, client.get()});
  REQUIRE(b.bind("td_bench:/bench/late").empty());
  REQUIRE(b.bound_count("td_bench:/bench/late") == 0);

  // Without polling, a parameter added on the server never shows up.
  {
    auto& n = ossia::net::create_node(server.get_root_node(), "/bench/late");
    ossia::try_setup_parameter("float", n)->push_value(0.5f);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  REQUIRE(ossia::net::find_node(croot, "/bench/late") == nullptr);

  // The client polls the mirror, as it does for a connection by URL.
  auto* mirror = ossia::max_binding::polled_oscquery_mirror(*client);
  auto poll_until = [&](auto done) {
    for(int i = 0; i < 400 && mirror && !done(); i++)
    {
      mirror->run_commands();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return done();
  };

  bool ok = mirror != nullptr;
  ok = ok && poll_until([&] { return ossia::net::find_node(croot, "/bench/late") != nullptr; });
  // The bind made before the parameter existed resolves when it appears.
  ok = ok && b.bound_count("td_bench:/bench/late") == 1;

  // A path removed on the server is removed on the client.
  if(ok)
  {
    server.get_root_node().find_child("bench")->remove_child("late");
    ok = poll_until([&] { return ossia::net::find_node(croot, "/bench/late") == nullptr; });
    ok = ok && b.bound_count("td_bench:/bench/late") == 0;
  }

  client.reset();
  ossia::net::stop_network_context(*ctx);
  net.join();
  REQUIRE(ok);
}

namespace
{
bool drained_refusal(branch& b, const std::string& address, const std::string& reason)
{
  for(auto& in : b.drain())
    if(in.address == "refused")
    {
      const auto& l = in.value.get<std::vector<ossia::value>>();
      if(l[0] == ossia::value(address) && l[1] == ossia::value(reason))
        return true;
    }
  return false;
}
}

// [ossia.v8connector] without a name: the leaves sit directly under the node
// of the enclosing model, next to the ossia.parameter objects.
TEST_CASE("v8connector_without_name", "v8connector_without_name")
{
  auto proto = std::make_unique<ossia::net::multiplex_protocol>();
  ossia::net::generic_device dev{std::move(proto), "v8_shared"};
  auto& root = dev.get_root_node();

  // The node of an ossia.model, with an ossia.parameter and an empty child
  // node made by another object.
  auto* model = &ossia::net::create_node(root, "/synth");
  {
    auto* p = ossia::try_setup_parameter("float", ossia::net::create_node(*model, "manual"));
    p->push_value(0.25f);
    ossia::net::create_node(*model, "fx");
  }

  auto shared = [&](branch& b) {
    b.set_owns_node(false);
    b.set_node_factory([&]() -> ossia::net::node_base* { return model; });
    b.set_resolution_context({&root}, {&dev});
  };
  branch a, b;
  shared(a);
  shared(b);
  const auto gain = attrs({{"type", {S("float")}}, {"min", {-60}}, {"max", {6}}, {"default", {-12}}});

  REQUIRE(a.declare("gain", gain).empty());
  auto* a_gain = ossia::net::find_node(root, "/synth/gain");
  REQUIRE(a_gain != nullptr);
  REQUIRE(a_gain->get_parameter() != nullptr);
  REQUIRE(a.node() == model);
  a.drain();

  SECTION("an address taken by another instance is refused, never renamed")
  {
    REQUIRE(!b.declare("gain", gain).empty());
    REQUIRE(drained_refusal(b, "gain", "already exists"));
    REQUIRE(b.size() == 0);
    REQUIRE(ossia::net::find_node(root, "/synth/gain") == a_gain);
    REQUIRE(ossia::net::find_node(root, "/synth/gain.1") == nullptr);
    REQUIRE(a_gain->get_parameter()->value() == ossia::value(-12.f));
  }

  SECTION("an address taken by an ossia.parameter is refused")
  {
    REQUIRE(!a.declare("manual", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}})).empty());
    REQUIRE(drained_refusal(a, "manual", "already exists"));
    REQUIRE(ossia::net::find_node(root, "/synth/manual")->get_parameter()->value()
            == ossia::value(0.25f));
  }

  SECTION("for comparison, ossia.parameter renames: a second one becomes gain.1")
  {
    // object_base.cpp, case object_class::param: a node that already holds a
    // parameter is replaced by create_node(parent, name), which picks a free name.
    auto* n = &ossia::net::create_node(*a_gain->get_parent(), a_gain->get_name());
    REQUIRE(n != a_gain);
    REQUIRE(n->get_name() == "gain.1");
  }

  SECTION("remove takes only the leaves of its instance, and only the nodes it created")
  {
    REQUIRE(b.declare("env/attack", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}})).empty());
    REQUIRE(b.declare("fx/mix", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}})).empty());
    REQUIRE(ossia::net::find_node(root, "/synth/env/attack") != nullptr);
    b.drain();

    REQUIRE(!a.remove("env/attack"));
    REQUIRE(drained_refusal(a, "env/attack", "unknown address 'env/attack'"));
    REQUIRE(!b.remove("gain"));
    REQUIRE(ossia::net::find_node(root, "/synth/gain") == a_gain);

    REQUIRE(b.remove("env/attack"));
    REQUIRE(ossia::net::find_node(root, "/synth/env") == nullptr);
    REQUIRE(b.remove("fx/mix"));
    // fx existed before: it stays
    REQUIRE(ossia::net::find_node(root, "/synth/fx") != nullptr);
    REQUIRE(ossia::net::find_node(root, "/synth/manual") != nullptr);
    REQUIRE(ossia::net::find_node(root, "/synth/gain") == a_gain);
  }

  SECTION("the replay on ready does not conflict with itself")
  {
    REQUIRE(a.declare("gain", gain).empty());
    for(auto& in : a.drain())
      REQUIRE(in.address != "refused");
    REQUIRE(ossia::net::find_node(root, "/synth/gain") == a_gain);
    REQUIRE(ossia::net::find_node(root, "/synth/gain.1") == nullptr);
  }

  SECTION("detached, the instance leaves the node as it found it; an address taken meanwhile is refused on return")
  {
    REQUIRE(a.declare("env/attack", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}})).empty());
    a.detach();
    REQUIRE(ossia::net::find_node(root, "/synth/gain") == nullptr);
    REQUIRE(ossia::net::find_node(root, "/synth/env") == nullptr);
    REQUIRE(ossia::net::find_node(root, "/synth/manual") != nullptr);
    REQUIRE(ossia::net::find_node(root, "/synth/fx") != nullptr);
    REQUIRE(ossia::net::find_node(root, "/synth") == model);

    // an ossia.parameter takes gain while the instance is away
    ossia::try_setup_parameter("float", ossia::net::create_node(*model, "gain"));
    a.drain();
    a.reattach();
    REQUIRE(drained_refusal(a, "gain", "already exists"));
    REQUIRE(a.size() == 1);
    REQUIRE(ossia::net::find_node(root, "/synth/env/attack") != nullptr);
  }

  SECTION("the model registers again: the leaves come back under its new node")
  {
    root.remove_child("synth");
    REQUIRE(!a.attached());
    model = &ossia::net::create_node(root, "/synth");
    a.reattach();
    REQUIRE(ossia::net::find_node(root, "/synth/gain") != nullptr);
  }
}

// Without a name and without an enclosing model, the leaves sit at the root of
// the device, which the branch also knows for its bindings. The device goes away.
TEST_CASE("v8connector_without_name_device_root", "v8connector_without_name_device_root")
{
  auto d = std::make_unique<ossia::net::generic_device>(
      std::make_unique<ossia::net::multiplex_protocol>(), "v8_root");
  branch a;
  a.set_owns_node(false);
  a.set_node_factory([&]() -> ossia::net::node_base* { return d ? &d->get_root_node() : nullptr; });
  a.set_resolution_context({&d->get_root_node()}, {d.get()});
  REQUIRE(a.declare("gain", attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1}}})).empty());
  REQUIRE(ossia::net::find_node(d->get_root_node(), "/gain") != nullptr);
  REQUIRE(a.bind("gain.1").empty());

  d.reset();
  REQUIRE(!a.attached());
  // the branch no longer knows the device: a new context does not touch it
  ossia::net::generic_device other{std::make_unique<ossia::net::multiplex_protocol>(), "other"};
  a.set_resolution_context({&other.get_root_node()}, {&other});
  REQUIRE(a.size() == 1);
}

// A server removes a parameter and creates it again at the same address, as
// a server that restarts publishes its tree again. The client receives
// PATH_REMOVED then PATH_ADDED; a bind to that address follows the new
// parameter, values included.
TEST_CASE("v8connector_parameter_recreated", "v8connector_parameter_recreated")
{
  auto ctx = ossia::net::create_network_context();
  auto server_multiplex = std::make_unique<ossia::net::multiplex_protocol>();
  auto* multiplex = server_multiplex.get();
  ossia::net::generic_device server{std::move(server_multiplex), "td_bench"};
  auto make_leaf = [&] {
    auto& n = ossia::net::create_node(server.get_root_node(), "/bench/p0");
    auto* p = ossia::try_setup_parameter("float", n);
    p->set_domain(ossia::make_domain(0.f, 1.f));
    p->push_value(0.f);
    return p;
  };
  auto* leaf = make_leaf();
  const uint16_t osc = free_port(false);
  const uint16_t ws = free_port(true);
  multiplex->expose_to(
      std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, osc, ws, false));
  std::thread net([&] { ctx->run(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // A client connected by URL, polled as ossia.client polls it.
  auto client = std::make_shared<ossia::net::generic_device>(
      std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
          "ws://127.0.0.1:" + std::to_string(ws), free_port(false)),
      "td_bench");
  REQUIRE(client->get_protocol().update(client->get_root_node()));
  auto* mirror = ossia::max_binding::polled_oscquery_mirror(*client);
  auto& croot = client->get_root_node();

  ossia::net::generic_device max_dev{std::make_unique<ossia::net::multiplex_protocol>(), "max"};
  branch b;
  b.set_resolution_context({&max_dev.get_root_node()}, {&max_dev, client.get()});
  REQUIRE(b.bind("td_bench:/bench/p0").empty());
  REQUIRE(b.bound_count("td_bench:/bench/p0") == 1);

  auto poll_until = [&](auto done) {
    for(int i = 0; i < 400 && !done(); i++)
    {
      mirror->run_commands();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return done();
  };
  auto received = [&](ossia::net::parameter_base& p, const ossia::value& v) {
    for(int i = 0; i < 400; i++)
    {
      if(i % 20 == 0)
        p.push_value(v);
      mirror->run_commands();
      for(auto& in : b.drain())
        if(in.address == "td_bench:/bench/p0" && in.value == v)
          return true;
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return false;
  };

  bool ok = received(*leaf, ossia::value(0.25f));
  INFO("before: " << ok);

  // The client reads the whole tree again (update): every parameter is
  // removed, then created again at the same address.
  bool reloaded = false, reload_bound = false, after_reload = false;
  if(ok)
  {
    reloaded = client->get_protocol().update(croot);
    reload_bound = poll_until([&] { return b.bound_count("td_bench:/bench/p0") == 1; });
    after_reload = received(*leaf, ossia::value(0.5f));
  }

  // The server removes the parameter, then creates it again.
  bool removed = false, back = false, bound_again = false, after = false;
  if(ok)
  {
    server.get_root_node().find_child("bench")->remove_child("p0");
    removed = poll_until([&] { return ossia::net::find_node(croot, "/bench/p0") == nullptr; });
    leaf = make_leaf();
    back = poll_until([&] { return ossia::net::find_node(croot, "/bench/p0") != nullptr; });
    bound_again = poll_until([&] { return b.bound_count("td_bench:/bench/p0") == 1; });
    after = received(*leaf, ossia::value(0.75f));
  }

  client.reset();
  ossia::net::stop_network_context(*ctx);
  net.join();
  REQUIRE(ok);
  REQUIRE(reloaded);
  REQUIRE(reload_bound);
  REQUIRE(after_reload);
  REQUIRE(removed);
  REQUIRE(back);
  REQUIRE(bound_again);
  REQUIRE(after);
}

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <cstdlib>
#include <fstream>
#include <sstream>

extern char** environ;

namespace
{
// The clock of the host: one callback, armed for later, fired by the test loop
// on its own thread, as Max fires a clock on its main thread.
struct fake_clock
{
  std::function<void()> fn;
  bool armed{};
  std::chrono::steady_clock::time_point due;
  void set(int ms)
  {
    armed = true;
    due = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
  }
  void unset() { armed = false; }
  void run()
  {
    if(armed && std::chrono::steady_clock::now() >= due)
    {
      armed = false;
      fn();
    }
  }
};

std::string self_path()
{
#ifdef __APPLE__
  char buf[4096];
  uint32_t size = sizeof(buf);
  if(_NSGetExecutablePath(buf, &size) == 0)
    return buf;
#endif
  return {};
}

// An OSCQuery server in another process, as a TouchDesigner component is:
// killed and started again on the same ports, it keeps nothing of its clients.
// It pushes a new value to /bench/p0 every 50 ms, and logs the WebSocket
// messages it receives to a file: a LISTEN logged after a restart proves the
// client subscribed again on the new connection.
struct server_process
{
  uint16_t osc, ws;
  std::string log_path;
  // The name the server announces; empty: td_bench.
  std::string name;
  pid_t pid{};

  server_process(uint16_t osc, uint16_t ws)
      : osc{osc}
      , ws{ws}
      , log_path{"/tmp/ossia_v8connector_server_" + std::to_string(getpid()) + ".log"}
  {
    std::remove(log_path.c_str());
  }
  ~server_process()
  {
    stop();
    std::remove(log_path.c_str());
  }
  bool start()
  {
    const auto exe = self_path();
    if(exe.empty())
      return false;
    setenv("OSSIA_BENCH_OSC", std::to_string(osc).c_str(), 1);
    setenv("OSSIA_BENCH_WS", std::to_string(ws).c_str(), 1);
    setenv("OSSIA_BENCH_LOG", log_path.c_str(), 1);
    setenv("OSSIA_BENCH_NAME", name.c_str(), 1);
    const char* argv[] = {exe.c_str(), "zz_td_bench_server", nullptr};
    if(posix_spawn(&pid, exe.c_str(), nullptr, nullptr, const_cast<char**>(argv), environ) != 0)
      return false;
    const auto url = "ws://127.0.0.1:" + std::to_string(ws);
    for(int i = 0; i < 100; i++)
    {
      if(ossia::max_binding::oscquery_server_reachable(url, 100))
        return true;
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
  }
  void stop()
  {
    if(!pid)
      return;
    kill(pid, SIGKILL);
    int status = 0;
    waitpid(pid, &status, 0);
    pid = 0;
  }
  std::string log() const
  {
    std::ifstream in{log_path};
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
  }
  bool listened_since(std::size_t from, const std::string& address) const
  {
    const auto s = log();
    for(auto pos = s.find("LISTEN", from); pos != std::string::npos; pos = s.find("LISTEN", pos + 1))
    {
      const auto eol = s.find('\n', pos);
      if(s.substr(pos, eol == std::string::npos ? eol : eol - pos).find(address) != std::string::npos)
        return true;
    }
    return false;
  }
};
}

// The server process, run by server_process (hidden from the normal runs).
TEST_CASE("zz_td_bench_server", "[.]")
{
  const char* osc_s = std::getenv("OSSIA_BENCH_OSC");
  const char* ws_s = std::getenv("OSSIA_BENCH_WS");
  const char* log_s = std::getenv("OSSIA_BENCH_LOG");
  REQUIRE(osc_s);
  REQUIRE(ws_s);
  REQUIRE(log_s);
  auto ctx = ossia::net::create_network_context();
  auto mp = std::make_unique<ossia::net::multiplex_protocol>();
  auto* multiplex = mp.get();
  // OSSIA_BENCH_NAME: the name the device is announced under, when set.
  const char* name_s = std::getenv("OSSIA_BENCH_NAME");
  ossia::net::generic_device dev{std::move(mp), name_s && *name_s ? name_s : "td_bench"};
  auto& n = ossia::net::create_node(dev.get_root_node(), "/bench/p0");
  auto* leaf = ossia::try_setup_parameter("float", n);
  leaf->set_domain(ossia::make_domain(0.f, 1.f));
  leaf->push_value(0.f);
  auto server = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(
      ctx, std::atoi(osc_s), std::atoi(ws_s), false);
  ossia::net::network_logger l;
  l.inbound_logger = spdlog::basic_logger_mt("server_in", log_s);
  l.inbound_logger->flush_on(spdlog::level::info);
  server->set_logger(l);
  multiplex->expose_to(std::move(server));
  std::thread net([&] { ctx->run(); });
  // Killed by the test; 10 minutes at most on its own.
  for(int i = 0; i < 12000; i++)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    leaf->push_value(((i % 999) + 1) / 1000.f);
  }
  ossia::net::stop_network_context(*ctx);
  net.join();
}

// The server of an ossia.client connected by URL dies and comes back on the
// same ports, as a TouchDesigner component turned off and on. The link
// reports the loss, builds the device again when the server answers, and the
// bound address receives values again, the server having received LISTEN on
// the new connection. Every wait is bounded and names its step.
TEST_CASE("v8connector_client_reconnects", "v8connector_client_reconnects")
{
  const uint16_t osc = free_port(false);
  const uint16_t ws = free_port(true);
  const std::string url = "ws://127.0.0.1:" + std::to_string(ws);
  server_process server{osc, ws};
  REQUIRE(server.start());

  ossia::net::generic_device max_dev{std::make_unique<ossia::net::multiplex_protocol>(), "max"};
  auto& max_root = max_dev.get_root_node();
  branch b;
  std::shared_ptr<ossia::net::generic_device> client;
  fake_clock clock;
  int losses = 0, attempts = 0;

  // What ossia.client does: build the mirror device by URL, read its tree,
  // register the objects of the patcher again.
  auto connect = [&]() -> bool {
    attempts++;
    try
    {
      client = std::make_shared<ossia::net::generic_device>(
          std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(url, free_port(false)),
          "td_bench");
    }
    catch(...)
    {
      return false;
    }
    if(!client->get_protocol().update(client->get_root_node()))
      return false;
    b.set_resolution_context({&max_root}, {&max_dev, client.get()});
    return true;
  };

  std::unique_ptr<ossia::max_binding::oscquery_link> link;
  link = std::make_unique<ossia::max_binding::oscquery_link>(ossia::max_binding::oscquery_link::host{
      [&](int ms) { clock.set(ms); },
      [&] { clock.unset(); },
      [&] { losses++; },
      [&] {
    if(!connect())
      return false;
    // the host gives the link the device it built, as client::connect does
    link->watch(*client, url, 10);
    return true;
      }});
  clock.fn = [&] { link->tick(); };

  REQUIRE(connect());
  link->watch(*client, url, 10);
  REQUIRE(b.bind("td_bench:/bench/p0").empty());
  REQUIRE(b.bound_count("td_bench:/bench/p0") == 1);

  // Run the host's clock and the test until done() or the time is up.
  auto run_until = [&](auto done, int ms) {
    for(int i = 0; i < ms / 5 && !done(); i++)
    {
      clock.run();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return done();
  };
  // A value pushed by the server (a new one every 50 ms) reaches the bind.
  auto receives = [&] {
    b.drain();
    bool got = false;
    return run_until(
        [&] {
      for(auto& in : b.drain())
        if(in.address == "td_bench:/bench/p0")
          got = true;
      return got;
        },
        3000);
  };

  REQUIRE(receives());

  SECTION("the server dies and comes back: the link reports it and the bind receives again")
  {
    const int cycles = 10;
    bool ok = true;
    std::string step;
    for(int cycle = 0; cycle < cycles && ok; cycle++)
    {
      const int losses_before = losses, attempts_before = attempts;
      server.stop();
      step = "cycle " + std::to_string(cycle) + ": loss reported";
      ok = run_until([&] { return losses > losses_before; }, 3000);
      if(!ok)
        break;
      // the server is down: nothing is built, the link keeps trying
      step = "cycle " + std::to_string(cycle) + ": still lost while the server is down";
      run_until([&] { return false; }, 1200);
      ok = link->lost() && attempts == attempts_before;
      if(!ok)
        break;
      const auto log_from = server.log().size();
      step = "cycle " + std::to_string(cycle) + ": server started again";
      ok = server.start();
      if(!ok)
        break;
      step = "cycle " + std::to_string(cycle) + ": bound again after the return";
      ok = run_until([&] { return b.bound_count("td_bench:/bench/p0") == 1 && !link->lost(); }, 5000);
      if(!ok)
        break;
      step = "cycle " + std::to_string(cycle) + ": value received after the return";
      ok = receives();
      if(!ok)
        break;
      step = "cycle " + std::to_string(cycle) + ": LISTEN received by the new server";
      ok = server.listened_since(log_from, "/bench/p0");
    }
    const int final_losses = losses;
    INFO(step);
    link.reset();
    client.reset();
    REQUIRE(ok);
    REQUIRE(final_losses == cycles);
  }

  SECTION("reconnect off: the loss is reported, nothing is tried")
  {
    link->set_reconnect(false);
    const int attempts_before = attempts;
    server.stop();
    const bool reported = run_until([&] { return losses == 1; }, 3000);
    REQUIRE(server.start());
    run_until([&] { return false; }, 2500);
    const bool tried = attempts != attempts_before;
    const bool watching = link->watching();
    link.reset();
    client.reset();
    REQUIRE(reported);
    REQUIRE(!tried);
    REQUIRE(!watching);
  }

  SECTION("destroyed while lost: no tick fires afterwards")
  {
    server.stop();
    REQUIRE(run_until([&] { return losses == 1; }, 3000));
    // a retry is scheduled; the host is destroyed before it fires. The clock
    // of the host keeps its target, as a Max clock does: a tick that still
    // fired would run on freed memory.
    REQUIRE(clock.armed);
    auto* raw = link.get();
    clock.fn = [raw] { raw->tick(); };
    link.reset();
    const bool cancelled = !clock.armed;
    clock.due = std::chrono::steady_clock::now();
    clock.run();
    REQUIRE(cancelled);
    client.reset();
  }
}

#include <ossia/protocols/osc/osc_factory.hpp>

#include <boost/asio/ip/udp.hpp>
#include <boost/endian/conversion.hpp>

#include <oscpack/osc/OscReceivedElements.h>

#include <atomic>
#include <mutex>

namespace
{
// What an OSCQuery client sends over UDP, as received: every datagram, and
// the messages it holds (a bundle is opened). The server below announces this
// port in HOST_INFO and does not listen to it itself.
struct udp_sink
{
  boost::asio::io_context ctx;
  boost::asio::ip::udp::socket socket{ctx};
  std::thread thread;
  std::mutex mutex;
  std::vector<std::size_t> datagrams;              // size of each datagram
  std::vector<std::pair<std::string, float>> messages; // address, first float
  std::atomic_bool running{true};

  explicit udp_sink(uint16_t port)
  {
    socket.open(boost::asio::ip::udp::v4());
    socket.bind({boost::asio::ip::make_address("0.0.0.0"), port});
    socket.non_blocking(true);
    thread = std::thread([this] {
      std::vector<char> buf(65536);
      while(running)
      {
        boost::system::error_code ec;
        boost::asio::ip::udp::endpoint from;
        auto n = socket.receive_from(boost::asio::buffer(buf), from, 0, ec);
        if(ec)
        {
          std::this_thread::sleep_for(std::chrono::microseconds(200));
          continue;
        }
        std::lock_guard lock{mutex};
        datagrams.push_back(n);
        decode(buf.data(), n);
      }
    });
  }
  ~udp_sink()
  {
    running = false;
    thread.join();
  }
  void decode(const char* data, std::size_t size)
  {
    try
    {
      oscpack::ReceivedPacket p{data, (oscpack::osc_bundle_element_size_t)size};
      if(p.IsBundle())
      {
        oscpack::ReceivedBundle b{p};
        for(auto it = b.ElementsBegin(); it != b.ElementsEnd(); ++it)
          decode(it->Contents(), it->Size());
      }
      else
      {
        oscpack::ReceivedMessage m{p};
        float v = 0.f;
        if(m.ArgumentCount() > 0 && m.ArgumentsBegin()->IsFloat())
          v = m.ArgumentsBegin()->AsFloat();
        messages.emplace_back(m.AddressPattern(), v);
      }
    }
    catch(...)
    {
    }
  }
  void clear()
  {
    std::lock_guard lock{mutex};
    datagrams.clear();
    messages.clear();
  }
  // Waits until n messages arrived, or the time is up.
  bool wait_messages(std::size_t n, int ms)
  {
    for(int i = 0; i < ms / 5; i++)
    {
      {
        std::lock_guard lock{mutex};
        if(messages.size() >= n)
          return true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    std::lock_guard lock{mutex};
    return messages.size() >= n;
  }
};

// An OSCQuery server whose OSC port is read by a udp_sink.
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
    dev = std::make_unique<ossia::net::generic_device>(std::move(mp), "td_bench");
    for(int i = 0; i < leaves; i++)
    {
      auto& n = ossia::net::create_node(dev->get_root_node(), "/load/p" + std::to_string(i));
      auto* p = ossia::try_setup_parameter("float", n);
      p->set_domain(ossia::make_domain(0.f, 1000000.f));
    }
    ossia::try_setup_parameter("impulse", ossia::net::create_node(dev->get_root_node(), "/bang"));
    multiplex->expose_to(std::make_unique<ossia::oscquery_asio::oscquery_server_protocol_base>(
        ctx,
        std::vector<ossia::net::osc_server_configuration>{
            ossia::net::udp_server_configuration{{{"0.0.0.0"}, osc}}},
        ws, false));
    net = std::thread([this] { ctx->run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  ~sink_server()
  {
    ossia::net::stop_network_context(*ctx);
    net.join();
  }
  std::string url() const { return "ws://127.0.0.1:" + std::to_string(ws); }
};
}

// The client's push_bundle sends the values of many parameters in OSC
// bundles: over UDP, no datagram above 1400 bytes.
TEST_CASE("oscquery_mirror_push_bundle", "oscquery_mirror_push_bundle")
{
  const int n = 3000;
  sink_server server{n};
  auto client = std::make_shared<ossia::net::generic_device>(
      std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(server.url(), free_port(false)),
      "td_bench");
  REQUIRE(client->get_protocol().update(client->get_root_node()));

  std::vector<const ossia::net::parameter_base*> params;
  for(int i = 0; i < n; i++)
  {
    auto* node = ossia::net::find_node(client->get_root_node(), "/load/p" + std::to_string(i));
    REQUIRE(node);
    auto* p = node->get_parameter();
    p->set_value(ossia::value(float(i)));
    params.push_back(p);
  }
  server.sink.clear();

  const bool sent = client->get_protocol().push_bundle(params);
  const bool all = server.sink.wait_messages(n, 3000);
  std::lock_guard lock{server.sink.mutex};
  INFO("datagrams " << server.sink.datagrams.size() << ", messages " << server.sink.messages.size());
  REQUIRE(sent);
  REQUIRE(all);
  REQUIRE(server.sink.messages.size() == std::size_t(n));
  for(auto size : server.sink.datagrams)
    REQUIRE(size <= 1400);
  REQUIRE(server.sink.datagrams.size() < std::size_t(n) / 20);
  REQUIRE(server.sink.messages[1234].first == "/load/p1234");
  REQUIRE(server.sink.messages[1234].second == 1234.f);
}

#include <ossia/network/sockets/websocket_client.hpp>

namespace
{
// A client of an OSCQuery server reduced to its WebSocket: it counts the
// binary frames the server sends, and can ask the server to send its values
// over UDP to a port of the test (START_OSC_STREAMING).
struct raw_client
{
  std::atomic_int binary_frames{0};
  ossia::net::websocket_client ws{
      [this](auto, websocketpp::frame::opcode::value op, const std::string&) {
    if(op == websocketpp::frame::opcode::value::BINARY)
      binary_frames++;
      }};
  std::thread thread;

  explicit raw_client(const std::string& url)
  {
    thread = std::thread([this, url] {
      try
      {
        ws.connect_and_run(url);
      }
      catch(...)
      {
      }
    });
    for(int i = 0; i < 400 && !ws.connected(); i++)
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  ~raw_client()
  {
    ws.stop();
    thread.join();
  }
  void stream_to(uint16_t udp_port)
  {
    auto msg = ossia::oscquery::json_writer::start_osc_streaming(udp_port, 0);
    ws.send_message(std::string(msg.GetString(), msg.GetSize()));
  }
  // This client is a peer of the test: it asks for what it wants, as any client
  void listen_load(int n, const std::string& prefix = "/load/p")
  {
    for(int i = 0; i < n; i++)
      ws.send_message(ossia::oscquery::json_writer::listen(prefix + std::to_string(i)));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
};
}

// An OSCQuery server under a multiplex sends a bundle as one OSC bundle to
// each client; before, the multiplex pushed its values one by one.
TEST_CASE("oscquery_multiplex_push_bundle", "oscquery_multiplex_push_bundle")
{
  const int n = 40;
  sink_server server{n};
  udp_sink client_sink{free_port(false)};
  raw_client client{server.url()};
  REQUIRE(client.ws.connected());
  client.stream_to(client_sink.socket.local_endpoint().port());
  client.listen_load(n);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  std::vector<const ossia::net::parameter_base*> params;
  for(int i = 0; i < n; i++)
  {
    auto* p = ossia::net::find_node(server.dev->get_root_node(), "/load/p" + std::to_string(i))
                  ->get_parameter();
    p->set_value(ossia::value(float(i)));
    params.push_back(p);
  }
  client_sink.clear();
  server.dev->get_protocol().push_bundle(params);
  const bool all = client_sink.wait_messages(n, 3000);
  std::lock_guard lock{client_sink.mutex};
  INFO("datagrams " << client_sink.datagrams.size() << ", messages " << client_sink.messages.size());
  REQUIRE(all);
  REQUIRE(client_sink.datagrams.size() == 1);
  REQUIRE(client_sink.messages.size() == std::size_t(n));
}

// The sets of a branch during a tick leave at flush in OSC bundles: through a
// client's mirror to a bound server, and through a served device to its
// clients. Only what changes leaves; with bundles off, one message per value.
TEST_CASE("v8connector_bundles", "v8connector_bundles")
{
  const int n = 3000;
  sink_server server{n};
  auto client = std::make_shared<ossia::net::generic_device>(
      std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(server.url(), free_port(false)),
      "td_bench");
  REQUIRE(client->get_protocol().update(client->get_root_node()));
  ossia::net::generic_device max_dev{std::make_unique<ossia::net::multiplex_protocol>(), "max"};
  branch b;
  b.set_resolution_context({&max_dev.get_root_node()}, {&max_dev, client.get()});
  std::vector<std::string> addresses;
  for(int i = 0; i < n; i++)
  {
    addresses.push_back("td_bench:/load/p" + std::to_string(i));
    REQUIRE(b.bind(addresses.back()).empty());
  }
  REQUIRE(b.bound_count(addresses[2999]) == 1);
  b.drain();
  server.sink.clear();

  auto tick = [&](auto value_of) {
    for(int i = 0; i < n; i++)
      REQUIRE(b.set(addresses[i], {value_of(i)}).empty());
  };
  auto settle = [] { std::this_thread::sleep_for(std::chrono::milliseconds(200)); };

  SECTION("3000 sets of one tick leave at flush, in datagrams of at most 1400 bytes")
  {
    b.set_bundle(true);
    tick([](int i) { return float(i); });
    settle();
    {
      std::lock_guard lock{server.sink.mutex};
      REQUIRE(server.sink.messages.empty());
    }
    REQUIRE(b.flush() == std::size_t(n));
    REQUIRE(server.sink.wait_messages(n, 3000));
    std::lock_guard lock{server.sink.mutex};
    INFO("datagrams " << server.sink.datagrams.size());
    REQUIRE(server.sink.messages.size() == std::size_t(n));
    for(auto size : server.sink.datagrams)
      REQUIRE(size <= 1400);
    REQUIRE(server.sink.datagrams.size() < std::size_t(n) / 20);
  }

  SECTION("only what changes leaves, as the repetition filter says")
  {
    for(int i = 0; i < n; i++)
      ossia::net::find_node(client->get_root_node(), "/load/p" + std::to_string(i))
          ->get_parameter()
          ->set_repetition_filter(ossia::repetition_filter::ON);
    b.set_bundle(true);
    // every value differs from the one the parameter holds
    tick([](int i) { return float(i) + 1000.f; });
    REQUIRE(b.flush() == std::size_t(n));
    REQUIRE(server.sink.wait_messages(n, 3000));
    server.sink.clear();
    // 1 % changes
    tick([](int i) { return i % 100 == 0 ? float(i) + 1000.5f : float(i) + 1000.f; });
    REQUIRE(b.flush() == std::size_t(n / 100));
    REQUIRE(server.sink.wait_messages(n / 100, 3000));
    settle();
    std::lock_guard lock{server.sink.mutex};
    REQUIRE(server.sink.messages.size() == std::size_t(n / 100));
    REQUIRE(server.sink.datagrams.size() == 1);
  }

  SECTION("bundles off: one datagram per value, as before")
  {
    b.set_bundle(false);
    tick([](int i) { return float(i); });
    REQUIRE(b.flush() == 0);
    REQUIRE(server.sink.wait_messages(n, 5000));
    std::lock_guard lock{server.sink.mutex};
    REQUIRE(server.sink.datagrams.size() == std::size_t(n));
  }

  client.reset();
}

// Leaves declared under a served device (ossia.device): the bundle leaves
// through the OSCQuery server, which sends what it is given as one datagram.
// The branch hands it groups of at most 1400 bytes.
TEST_CASE("v8connector_bundles_served_leaves", "v8connector_bundles_served_leaves")
{
  const int n = 3000;
  sink_server server{0};
  udp_sink client_sink{free_port(false)};
  raw_client client{server.url()};
  REQUIRE(client.ws.connected());
  client.stream_to(client_sink.socket.local_endpoint().port());
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  branch b;
  auto& root = server.dev->get_root_node();
  b.set_node_factory([&]() -> ossia::net::node_base* { return &ossia::net::create_node(root, "/synth"); });
  b.set_resolution_context({&root}, {server.dev.get()});
  for(int i = 0; i < n; i++)
    REQUIRE(b.declare("p" + std::to_string(i), attrs({{"type", {S("float")}}, {"min", {0}}, {"max", {1000}}}))
                .empty());
  b.drain();
  b.set_bundle(true);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  // Once the leaves exist: a LISTEN on an unknown address listens to nothing
  client.listen_load(n, "/synth/p");
  client_sink.clear();

  for(int i = 0; i < n; i++)
    REQUIRE(b.set("p" + std::to_string(i), {float(i)}).empty());
  REQUIRE(b.flush() == std::size_t(n));
  const bool all = client_sink.wait_messages(n, 3000);
  std::lock_guard lock{client_sink.mutex};
  std::size_t largest = 0;
  for(auto size : client_sink.datagrams)
    largest = std::max(largest, size);
  INFO("datagrams " << client_sink.datagrams.size() << ", messages " << client_sink.messages.size()
                    << ", largest " << largest);
  REQUIRE(all);
  REQUIRE(client_sink.messages.size() == std::size_t(n));
  REQUIRE(largest <= 1400);
  REQUIRE(client_sink.datagrams.size() < std::size_t(n) / 20);
}

#include <csignal>
#include <cstring>
#include <cstdio>

namespace
{
std::atomic_bool g_load_server_stop{false};
void load_server_on_term(int)
{
  g_load_server_stop = true;
}

int64_t now_ns()
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

// One value as the load server received it.
struct load_arrival
{
  uint32_t leaf;
  float tick;
  int64_t ns;
};
// One datagram as the load server received it.
struct load_datagram
{
  uint32_t size;
  int64_t ns;
};
}

// The load server, run in its own process by the load bench (hidden from the
// normal runs). 3000 leaves; it announces OSC_LOAD_OSC in HOST_INFO and reads
// that port itself; on SIGTERM it writes what it received to OSSIA_LOAD_OUT.
TEST_CASE("zz_load_server", "[.]")
{
  const char* osc_s = std::getenv("OSSIA_LOAD_OSC");
  const char* ws_s = std::getenv("OSSIA_LOAD_WS");
  const char* out_s = std::getenv("OSSIA_LOAD_OUT");
  REQUIRE(osc_s);
  REQUIRE(ws_s);
  REQUIRE(out_s);
  std::signal(SIGTERM, load_server_on_term);
  const uint16_t osc = std::atoi(osc_s);

  auto ctx = ossia::net::create_network_context();
  auto mp = std::make_unique<ossia::net::multiplex_protocol>();
  auto* multiplex = mp.get();
  ossia::net::generic_device dev{std::move(mp), "load_server"};
  for(int i = 0; i < 3000; i++)
  {
    auto& n = ossia::net::create_node(dev.get_root_node(), "/load/p" + std::to_string(i));
    ossia::try_setup_parameter("float", n)->set_domain(ossia::make_domain(0.f, 1000000.f));
  }
  multiplex->expose_to(std::make_unique<ossia::oscquery_asio::oscquery_server_protocol_base>(
      ctx,
      std::vector<ossia::net::osc_server_configuration>{
          ossia::net::udp_server_configuration{{{"0.0.0.0"}, osc}}},
      std::atoi(ws_s), false));
  std::thread net([&] { ctx->run(); });

  std::vector<load_arrival> arrivals;
  arrivals.reserve(6'000'000);
  std::vector<load_datagram> datagrams;
  datagrams.reserve(400'000);

  boost::asio::io_context uctx;
  boost::asio::ip::udp::socket socket{uctx};
  socket.open(boost::asio::ip::udp::v4());
  socket.set_option(boost::asio::socket_base::receive_buffer_size(8 * 1024 * 1024));
  socket.bind({boost::asio::ip::make_address("0.0.0.0"), osc});
  socket.non_blocking(true);

  std::function<void(const char*, std::size_t, int64_t)> decode;
  decode = [&](const char* data, std::size_t size, int64_t ns) {
    oscpack::ReceivedPacket p{data, (oscpack::osc_bundle_element_size_t)size};
    if(p.IsBundle())
    {
      oscpack::ReceivedBundle b{p};
      for(auto it = b.ElementsBegin(); it != b.ElementsEnd(); ++it)
        decode(it->Contents(), it->Size(), ns);
      return;
    }
    oscpack::ReceivedMessage m{p};
    const char* addr = m.AddressPattern();
    const char* digits = std::strrchr(addr, 'p');
    if(!digits || m.ArgumentCount() < 1 || !m.ArgumentsBegin()->IsFloat())
      return;
    arrivals.push_back({uint32_t(std::atoi(digits + 1)), m.ArgumentsBegin()->AsFloat(), ns});
  };

  std::vector<char> buf(65536);
  // 5 minutes at most on its own
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(5);
  while(!g_load_server_stop && std::chrono::steady_clock::now() < deadline)
  {
    boost::system::error_code ec;
    boost::asio::ip::udp::endpoint from;
    auto n = socket.receive_from(boost::asio::buffer(buf), from, 0, ec);
    if(ec)
    {
      std::this_thread::sleep_for(std::chrono::microseconds(50));
      continue;
    }
    const auto ns = now_ns();
    datagrams.push_back({uint32_t(n), ns});
    try
    {
      decode(buf.data(), n, ns);
    }
    catch(...)
    {
    }
  }

  if(FILE* f = std::fopen(out_s, "wb"))
  {
    const uint64_t na = arrivals.size(), nd = datagrams.size();
    std::fwrite(&na, sizeof(na), 1, f);
    std::fwrite(arrivals.data(), sizeof(load_arrival), na, f);
    std::fwrite(&nd, sizeof(nd), 1, f);
    std::fwrite(datagrams.data(), sizeof(load_datagram), nd, f);
    std::fclose(f);
  }
  ossia::net::stop_network_context(*ctx);
  net.join();
}

// The load bench, run by hand, one mode per run (each under 120 s):
//   OSSIA_LOAD_MODE=bundle1|bundle0|changes1pct|remote1|remote0 [OSSIA_LOAD_SECONDS=30]
//   ossia_V8ConnectorTest "[load]"
// 3000 leaves of a server in another process, bound through a client by URL,
// all set at each tick, 60 ticks per second. With changes1pct, all are set at
// each tick but 1 % change, with the repetition filter on. With remote1 and
// remote0, the 3000 leaves are pushed as ossia.remote pushes them, through
// the bundles of the client's device (@bundle 1) or one by one (@bundle 0).
TEST_CASE("v8connector_load", "[.][load]")
{
  const std::string mode = std::getenv("OSSIA_LOAD_MODE") ? std::getenv("OSSIA_LOAD_MODE") : "bundle1";
  const int seconds = std::getenv("OSSIA_LOAD_SECONDS") ? std::atoi(std::getenv("OSSIA_LOAD_SECONDS")) : 30;
  const int n = 3000, rate = 60, ticks = seconds * rate;
  const bool remote = mode.rfind("remote", 0) == 0;
  const bool bundle = mode != "bundle0" && mode != "remote0";
  const bool one_percent = mode == "changes1pct";

  const uint16_t osc = free_port(false), ws = free_port(true);
  const std::string out = "/tmp/ossia_v8connector_load_" + std::to_string(getpid()) + ".bin";
  std::remove(out.c_str());
  setenv("OSSIA_LOAD_OSC", std::to_string(osc).c_str(), 1);
  setenv("OSSIA_LOAD_WS", std::to_string(ws).c_str(), 1);
  setenv("OSSIA_LOAD_OUT", out.c_str(), 1);
  const auto exe = self_path();
  const char* argv[] = {exe.c_str(), "zz_load_server", nullptr};
  pid_t pid{};
  REQUIRE(posix_spawn(&pid, exe.c_str(), nullptr, nullptr, const_cast<char**>(argv), environ) == 0);
  struct reaper
  {
    pid_t pid;
    ~reaper()
    {
      if(pid)
      {
        kill(pid, SIGKILL);
        waitpid(pid, nullptr, 0);
      }
    }
  } reap{pid};

  const std::string url = "ws://127.0.0.1:" + std::to_string(ws);
  bool up = false;
  for(int i = 0; i < 100 && !up; i++)
  {
    up = ossia::max_binding::oscquery_server_reachable(url, 100);
    if(!up)
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  INFO("step: load server up");
  REQUIRE(up);

  auto client = std::make_shared<ossia::net::generic_device>(
      std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(url, free_port(false)),
      "load_server");
  INFO("step: client reads the tree");
  REQUIRE(client->get_protocol().update(client->get_root_node()));
  ossia::net::generic_device max_dev{std::make_unique<ossia::net::multiplex_protocol>(), "max"};
  branch b;
  b.set_resolution_context({&max_dev.get_root_node()}, {&max_dev, client.get()});
  // the ossia.remote objects, and the bundles of their ossia.client
  ossia::max_binding::device_bundle_queue queue{*client};
  queue.set_mode(bundle ? 1 : 0);
  std::vector<ossia::net::parameter_base*> remotes;
  std::vector<std::string> addresses;
  for(int i = 0; i < n; i++)
  {
    addresses.push_back("load_server:/load/p" + std::to_string(i));
    remotes.push_back(
        ossia::net::find_node(client->get_root_node(), "/load/p" + std::to_string(i))->get_parameter());
    if(!remote)
      REQUIRE(b.bind(addresses.back()).empty());
    if(one_percent)
      ossia::net::find_node(client->get_root_node(), "/load/p" + std::to_string(i))
          ->get_parameter()
          ->set_repetition_filter(ossia::repetition_filter::ON);
  }
  INFO("step: 3000 binds resolved");
  REQUIRE((remote || b.bound_count(addresses[n - 1]) == 1));
  b.drain();
  b.set_bundle(bundle);

  // Tick k sends the value k + 1 (exact in a float) to every leaf, or, with
  // changes1pct, to the 1 % of the leaves whose turn it is.
  std::vector<int64_t> tick_start(ticks), tick_ns(ticks);
  std::vector<float> current(n, 0.f);
  std::size_t expected = 0;
  const auto period = std::chrono::nanoseconds(1'000'000'000 / rate);
  auto next = std::chrono::steady_clock::now();
  for(int k = 0; k < ticks; k++)
  {
    std::this_thread::sleep_until(next);
    next += period;
    const auto t0 = now_ns();
    tick_start[k] = t0;
    for(int i = 0; i < n; i++)
    {
      if(!one_percent || i % 100 == k % 100)
        current[i] = float(k + 1);
      if(!remote)
        b.set(addresses[i], {current[i]});
      // what object_base::push_parameter_value does
      else if(!ossia::max_binding::device_bundle_queue::push_if_active(*remotes[i], current[i]))
        remotes[i]->push_value(current[i]);
    }
    expected += one_percent ? std::size_t(n / 100) : std::size_t(n);
    if(!remote)
      b.flush();
    else
      queue.flush();
    tick_ns[k] = now_ns() - t0;
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  kill(pid, SIGTERM);
  int status = 0;
  waitpid(pid, &status, 0);
  reap.pid = 0;
  client.reset();

  // What the server received
  std::vector<load_arrival> arrivals;
  std::vector<load_datagram> datagrams;
  {
    FILE* f = std::fopen(out.c_str(), "rb");
    INFO("step: load server results");
    REQUIRE(f);
    uint64_t na = 0, nd = 0;
    std::fread(&na, sizeof(na), 1, f);
    arrivals.resize(na);
    std::fread(arrivals.data(), sizeof(load_arrival), na, f);
    std::fread(&nd, sizeof(nd), 1, f);
    datagrams.resize(nd);
    std::fread(datagrams.data(), sizeof(load_datagram), nd, f);
    std::fclose(f);
    std::remove(out.c_str());
  }

  // Latency of each value: from the start of the tick that set it to its
  // arrival. Only the values of the measured ticks count (k + 1 in 1..ticks).
  std::vector<double> latency_ms;
  latency_ms.reserve(arrivals.size());
  std::vector<uint8_t> seen;
  if(!one_percent)
    seen.assign(std::size_t(n) * ticks, 0);
  std::size_t received = 0;
  for(const auto& a : arrivals)
  {
    const int k = int(a.tick) - 1;
    if(k < 0 || k >= ticks || a.leaf >= uint32_t(n))
      continue;
    if(one_percent && int(a.leaf % 100) != k % 100)
      continue;
    if(!one_percent)
    {
      auto& s = seen[std::size_t(k) * n + a.leaf];
      if(s)
        continue;
      s = 1;
    }
    received++;
    latency_ms.push_back((a.ns - tick_start[k]) / 1e6);
  }
  auto pct = [](std::vector<double> v, double p) {
    if(v.empty())
      return 0.;
    std::sort(v.begin(), v.end());
    return v[std::min(v.size() - 1, std::size_t(p * (v.size() - 1)))];
  };
  std::vector<double> tick_ms(ticks);
  for(int k = 0; k < ticks; k++)
    tick_ms[k] = tick_ns[k] / 1e6;
  uint64_t bytes = 0;
  std::size_t max_size = 0;
  for(const auto& d : datagrams)
  {
    bytes += d.size;
    max_size = std::max<std::size_t>(max_size, d.size);
  }
  const double lost = expected ? 100. * double(expected - std::min(expected, received)) / expected : 0.;

  std::printf(
      "LOAD mode=%s ticks=%d values/tick=%.0f datagrams/tick=%.1f bytes/tick=%.0f max_datagram=%zu "
      "latency_p50=%.2fms latency_p99=%.2fms lost=%.3f%% tick_p50=%.2fms tick_p99=%.2fms tick_max=%.2fms\n",
      mode.c_str(), ticks, double(expected) / ticks, double(datagrams.size()) / ticks,
      double(bytes) / ticks, max_size, pct(latency_ms, 0.5), pct(latency_ms, 0.99), lost,
      pct(tick_ms, 0.5), pct(tick_ms, 0.99), pct(tick_ms, 1.0));
  std::fflush(stdout);
  REQUIRE(received > 0);
}

namespace
{
// A server as ossia.device runs it in Max: no thread of its own, its network
// context polled by the main thread between other work.
struct main_thread_server
{
  ossia::net::network_context_ptr ctx = ossia::net::create_network_context();
  std::unique_ptr<ossia::net::generic_device> dev;
  uint16_t ws = free_port(true);
  explicit main_thread_server(int leaves)
  {
    auto mp = std::make_unique<ossia::net::multiplex_protocol>();
    auto* multiplex = mp.get();
    dev = std::make_unique<ossia::net::generic_device>(std::move(mp), "load_demo");
    for(int i = 0; i < leaves; i++)
      ossia::try_setup_parameter(
          "float", ossia::net::create_node(dev->get_root_node(), "/load/p" + std::to_string(i)));
    multiplex->expose_to(std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(
        ctx, free_port(false), ws, false));
  }
  void poll()
  {
    ctx->context.poll();
    ctx->context.restart();
  }
  std::string url() const { return "ws://127.0.0.1:" + std::to_string(ws); }
};
}

// An ossia.client connected by URL to an ossia.device of the same Max: the
// main thread polls the server and must not wait for the client. The attempt
// runs on its own thread; the main thread only checks whether it is done.
TEST_CASE("v8connector_client_connects_to_same_process_server", "v8connector_client_connects_to_same_process_server")
{
  main_thread_server server{3000};
  for(int i = 0; i < 10; i++)
  {
    server.poll();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  auto t0 = std::chrono::steady_clock::now();
  ossia::max_binding::oscquery_connect_attempt attempt{server.url(), "load_demo"};
  const auto start_ms
      = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();

  // the main thread's loop: poll the server, check the attempt
  long long longest = 0;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while(!attempt.done() && std::chrono::steady_clock::now() < deadline)
  {
    const auto a = std::chrono::steady_clock::now();
    server.poll();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    longest = std::max<long long>(
        longest, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - a).count());
  }
  INFO("step: attempt done within 10 s");
  REQUIRE(attempt.done());
  auto dev = attempt.take();
  INFO("error: " << attempt.error());
  REQUIRE(dev != nullptr);
  auto* load = ossia::net::find_node(dev->get_root_node(), "/load");
  REQUIRE(load != nullptr);
  REQUIRE(load->children().size() == 3000);
  // the main thread was never held
  REQUIRE(start_ms < 50);
  REQUIRE(longest < 50);
}

// expose oscquery sent to the global [ossia] object: the object now creates
// and polls a network context as ossia.device does; before, the server was
// built on a null context and Max crashed in websocketpp's init_asio.
TEST_CASE("ossia_object_expose_oscquery", "ossia_object_expose_oscquery")
{
  // the global device of Max, as ossia_max creates it
  ossia::net::generic_device global{std::make_unique<ossia::net::multiplex_protocol>(), "max"};
  ossia::try_setup_parameter("float", ossia::net::create_node(global.get_root_node(), "/foo/bar"));

  // ossia_object::create, then device::expose
  auto ctx = ossia::max_binding::make_device_network_context();
  const uint16_t ws = free_port(true);
  auto& multiplex = static_cast<ossia::net::multiplex_protocol&>(global.get_protocol());
  multiplex.expose_to(
      std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, free_port(false), ws, false));

  // the server answers: a client by URL reads the tree while device::asio_timer polls
  ossia::max_binding::oscquery_connect_attempt attempt{"ws://127.0.0.1:" + std::to_string(ws), "max"};
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while(!attempt.done() && std::chrono::steady_clock::now() < deadline)
  {
    ossia::max_binding::poll_device_network_context(*ctx);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  INFO("step: client attempt done within 10 s");
  REQUIRE(attempt.done());
  auto dev = attempt.take();
  INFO("error: " << attempt.error());
  REQUIRE(dev != nullptr);
  REQUIRE(ossia::net::find_node(dev->get_root_node(), "/foo/bar") != nullptr);
  dev.reset();
  multiplex.clear();
}

// The bundles of ossia.device and ossia.client: what the objects of a device
// push during a tick leaves at its end, in OSC bundles, nothing merged.
TEST_CASE("device_bundles", "device_bundles")
{
  const int n = 3000;
  sink_server server{n};
  auto leaf = [&](ossia::net::device_base& d, int i) {
    return ossia::net::find_node(d.get_root_node(), "/load/p" + std::to_string(i))->get_parameter();
  };
  // what object_base::push_parameter_value now does
  auto push = [](ossia::max_binding::device_bundle_queue&, ossia::net::parameter_base& p, ossia::value v) {
    if(!ossia::max_binding::device_bundle_queue::push_if_active(p, v))
      p.push_value(v);
  };

  SECTION("ossia.parameter: 3000 values of a tick reach a client of the device in datagrams of at most 1400 bytes")
  {
    udp_sink client_sink{free_port(false)};
    raw_client client{server.url()};
    REQUIRE(client.ws.connected());
    client.stream_to(client_sink.socket.local_endpoint().port());
    client.listen_load(n);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    REQUIRE(ossia::max_binding::bundles_by_default(server.dev->get_protocol()));

    ossia::max_binding::device_bundle_queue q{*server.dev};
    for(int i = 0; i < n; i++)
      push(q, *leaf(*server.dev, i), float(i));
    REQUIRE(q.flush() == std::size_t(n));
    const bool all = client_sink.wait_messages(n, 3000);
    std::lock_guard lock{client_sink.mutex};
    std::size_t largest = 0;
    for(auto size : client_sink.datagrams)
      largest = std::max(largest, size);
    INFO("datagrams " << client_sink.datagrams.size() << ", largest " << largest);
    REQUIRE(all);
    REQUIRE(client_sink.messages.size() == std::size_t(n));
    REQUIRE(largest <= 1400);
    REQUIRE(client_sink.datagrams.size() < std::size_t(n) / 20);
  }

  auto client = std::make_shared<ossia::net::generic_device>(
      std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(server.url(), free_port(false)),
      "td_bench");
  REQUIRE(client->get_protocol().update(client->get_root_node()));
  server.sink.clear();

  SECTION("ossia.remote: 3000 values of a tick leave through the client's mirror in bundles")
  {
    REQUIRE(ossia::max_binding::bundles_by_default(client->get_protocol()));
    ossia::max_binding::device_bundle_queue q{*client};
    for(int i = 0; i < n; i++)
      push(q, *leaf(*client, i), float(i));
    REQUIRE(q.flush() == std::size_t(n));
    REQUIRE(server.sink.wait_messages(n, 3000));
    std::lock_guard lock{server.sink.mutex};
    for(auto size : server.sink.datagrams)
      REQUIRE(size <= 1400);
    REQUIRE(server.sink.datagrams.size() < std::size_t(n) / 20);
  }

  SECTION("two values on the same address in a tick both leave, in order, impulses included")
  {
    ossia::max_binding::device_bundle_queue q{*client};
    auto* p0 = leaf(*client, 0);
    auto* p1 = leaf(*client, 1);
    auto* bang = ossia::net::find_node(client->get_root_node(), "/bang")->get_parameter();
    push(q, *p0, 1.f);
    push(q, *bang, ossia::impulse{});
    push(q, *p1, 3.f);
    push(q, *p0, 2.f);
    push(q, *bang, ossia::impulse{});
    REQUIRE(q.flush() == 5);
    REQUIRE(server.sink.wait_messages(5, 3000));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::lock_guard lock{server.sink.mutex};
    REQUIRE(server.sink.datagrams.size() == 1);
    REQUIRE(server.sink.messages.size() == 5);
    const std::vector<std::pair<std::string, float>> expected{
        {"/load/p0", 1.f}, {"/bang", 0.f}, {"/load/p1", 3.f}, {"/load/p0", 2.f}, {"/bang", 0.f}};
    REQUIRE(server.sink.messages == expected);
  }

  SECTION("a parameter removed before the flush is not sent")
  {
    ossia::max_binding::device_bundle_queue q{*client};
    push(q, *leaf(*client, 0), 1.f);
    push(q, *leaf(*client, 1), 2.f);
    client->get_root_node().find_child("load")->remove_child("p0");
    REQUIRE(q.flush() == 1);
    REQUIRE(server.sink.wait_messages(1, 3000));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::lock_guard lock{server.sink.mutex};
    REQUIRE(server.sink.messages.size() == 1);
    REQUIRE(server.sink.messages[0].first == "/load/p1");
  }

  SECTION("@bundle 0, as 2.1.0+prism.4: one datagram per value")
  {
    ossia::max_binding::device_bundle_queue q{*client};
    q.set_mode(0);
    for(int i = 0; i < n; i++)
      push(q, *leaf(*client, i), float(i));
    REQUIRE(q.size() == 0);
    REQUIRE(server.sink.wait_messages(n, 5000));
    std::lock_guard lock{server.sink.mutex};
    REQUIRE(server.sink.datagrams.size() == std::size_t(n));
  }

  client.reset();
}

TEST_CASE("device_bundles_default", "device_bundles_default")
{
  // a device without OSCQuery server, and a plain OSC client: no bundles by default
  ossia::net::generic_device local{std::make_unique<ossia::net::multiplex_protocol>(), "local"};
  REQUIRE(!ossia::max_binding::bundles_by_default(local.get_protocol()));

  auto ctx = ossia::net::create_network_context();
  ossia::net::osc_protocol_configuration conf;
  ossia::net::udp_configuration udp;
  udp.local = ossia::net::inbound_socket_configuration{"127.0.0.1", free_port(false)};
  udp.remote = ossia::net::outbound_socket_configuration{"127.0.0.1", free_port(false)};
  conf.transport = udp;
  ossia::net::generic_device osc{ossia::net::make_osc_protocol(ctx, conf), "osc"};
  REQUIRE(!ossia::max_binding::bundles_by_default(osc.get_protocol()));
}

// Bundles of values: each element carries its own value, so a parameter may
// come several times. The client's mirror and the OSCQuery server send them;
// the server splits them under 1400 bytes.
TEST_CASE("oscquery_value_bundles", "oscquery_value_bundles")
{
  const int n = 3000;
  sink_server server{n};

  SECTION("client: the values of the elements, in order")
  {
    auto client = std::make_shared<ossia::net::generic_device>(
        std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(server.url(), free_port(false)),
        "td_bench");
    REQUIRE(client->get_protocol().update(client->get_root_node()));
    auto* p0 = ossia::net::find_node(client->get_root_node(), "/load/p0")->get_parameter();
    std::vector<ossia::bundle_element> elements{{p0, 1.f}, {p0, 2.f}, {p0, 3.f}};
    server.sink.clear();
    REQUIRE(client->get_protocol().push_bundle(std::span<ossia::bundle_element>(elements)));
    REQUIRE(server.sink.wait_messages(3, 3000));
    std::lock_guard lock{server.sink.mutex};
    const std::vector<std::pair<std::string, float>> expected{
        {"/load/p0", 1.f}, {"/load/p0", 2.f}, {"/load/p0", 3.f}};
    REQUIRE(server.sink.messages == expected);
    REQUIRE(server.sink.datagrams.size() == 1);
  }

  SECTION("server: 3000 values in datagrams of at most 1400 bytes")
  {
    udp_sink client_sink{free_port(false)};
    raw_client client{server.url()};
    REQUIRE(client.ws.connected());
    client.stream_to(client_sink.socket.local_endpoint().port());
    client.listen_load(n);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::vector<ossia::bundle_element> elements;
    for(int i = 0; i < n; i++)
      elements.push_back(
          {ossia::net::find_node(server.dev->get_root_node(), "/load/p" + std::to_string(i))->get_parameter(),
           float(i)});
    ossia::oscquery_asio::oscquery_server_protocol_base* proto{};
    static_cast<ossia::net::multiplex_protocol&>(server.dev->get_protocol())
        .for_each_protocol([&](ossia::net::protocol_base& p) {
      proto = dynamic_cast<ossia::oscquery_asio::oscquery_server_protocol_base*>(&p);
        });
    REQUIRE(proto);
    REQUIRE(proto->push_bundle(std::span<ossia::bundle_element>(elements)));
    const bool all = client_sink.wait_messages(n, 3000);
    std::lock_guard lock{client_sink.mutex};
    std::size_t largest = 0;
    for(auto size : client_sink.datagrams)
      largest = std::max(largest, size);
    INFO("datagrams " << client_sink.datagrams.size() << ", largest " << largest);
    REQUIRE(all);
    REQUIRE(largest <= 1400);
    REQUIRE(client_sink.messages[1234] == std::pair<std::string, float>{"/load/p1234", 1234.f});
  }
}

// ossia.client connected by URL: its poll runs on the scheduler thread
// (oscquery_link::tick applies the mirror's commands), while a connect or a
// disconnect on the main thread destroys the device. unwatch() must not
// return while a tick runs on the old mirror.
TEST_CASE("oscquery_link_unwatch_waits_for_tick", "oscquery_link_unwatch_waits_for_tick")
{
  sink_server server{50};
  // the server's tree keeps changing: the mirror always has commands to apply
  std::atomic_bool churn{true};
  std::thread changes([&] {
    int k = 0;
    while(churn)
    {
      auto& root = server.dev->get_root_node();
      const std::string name = "c" + std::to_string(k++ % 20);
      if(auto* n = root.find_child(name))
        root.remove_child(*n);
      else
        ossia::try_setup_parameter("float", ossia::net::create_node(root, "/" + name));
      std::this_thread::sleep_for(std::chrono::microseconds(300));
    }
  });

  bool ok = true;
  for(int round = 0; round < 20 && ok; round++)
  {
    auto client = std::make_shared<ossia::net::generic_device>(
        std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(server.url(), free_port(false)),
        "td_bench");
    ok = client->get_protocol().update(client->get_root_node());
    if(!ok)
      break;
    ossia::max_binding::oscquery_link link{ossia::max_binding::oscquery_link::host{
        [](int) {}, [] {}, [] {}, [] { return false; }}};
    link.watch(*client, server.url(), 1);

    // the scheduler's clock
    std::atomic_bool polling{true};
    std::thread scheduler([&] {
      while(polling)
        link.tick();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(20 + round));

    // the main thread: connect or disconnect
    link.unwatch();
    client.reset();

    polling = false;
    scheduler.join();
  }
  // Deterministic: the tick is held inside (its schedule callback runs under
  // the link's lock); unwatch returns only once that tick has ended.
  if(ok)
  {
    auto client = std::make_shared<ossia::net::generic_device>(
        std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(server.url(), free_port(false)),
        "td_bench");
    ok = client->get_protocol().update(client->get_root_node());
    std::atomic_bool in_tick{false}, tick_done{false};
    ossia::max_binding::oscquery_link link{ossia::max_binding::oscquery_link::host{
        [&](int) {
      if(tick_done)
        return;
      in_tick = true;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      tick_done = true;
        },
        [] {}, [] {}, [] { return false; }}};
    link.watch(*client, server.url(), 1);
    // watch scheduled once; the next schedule comes from the tick
    tick_done = false;
    in_tick = false;
    std::thread scheduler([&] { link.tick(); });
    for(int i = 0; i < 200 && !in_tick; i++)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const bool entered = in_tick;
    link.unwatch();
    const bool waited = tick_done;
    client.reset();
    scheduler.join();
    INFO("step: a tick held inside, then unwatch");
    REQUIRE(entered);
    REQUIRE(waited);
  }
  churn = false;
  changes.join();
  REQUIRE(ok);
}

#include <spdlog/sinks/callback_sink.h>

// A client connected before the server declares its leaves, with 3000 binds
// waiting (the load example in Max): each leaf arrives alone (PATH_ADDED,
// then ATTRIBUTES_CHANGED and an HTTP request for its value). The poll must
// not stall: before, every creation resolved every waiting binding again,
// 15 s in one run_commands, while the server of the same Max, polled by
// that thread, let the HTTP requests time out.
TEST_CASE("v8connector_binds_follow_a_burst_of_leaves", "v8connector_binds_follow_a_burst_of_leaves")
{
  std::atomic_int http_errors{0};
  auto sink = std::make_shared<spdlog::sinks::callback_sink_mt>([&](const spdlog::details::log_msg& m) {
    if(std::string_view(m.payload.data(), m.payload.size()).find("HTTP Error") != std::string_view::npos)
      http_errors++;
  });
  ossia::logger().sinks().push_back(sink);

  const int n = 3000;
  main_thread_server server{0};
  for(int i = 0; i < 10; i++)
  {
    server.poll();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  ossia::max_binding::oscquery_connect_attempt attempt{server.url(), "load_demo"};
  for(int i = 0; i < 10000 && !attempt.done(); i++)
  {
    server.poll();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  auto client = attempt.take();
  REQUIRE(client);
  auto* mirror = ossia::max_binding::polled_oscquery_mirror(*client);

  branch b;
  b.set_resolution_context({&client->get_root_node()}, {client.get()});
  for(int i = 0; i < n; i++)
    REQUIRE(b.bind("load/p" + std::to_string(i)).empty());

  // the emitter declares its leaves after the connection
  for(int i = 0; i < n; i++)
    ossia::try_setup_parameter(
        "float", ossia::net::create_node(server.dev->get_root_node(), "/load/p" + std::to_string(i)));

  // the main thread polls the server, the client's clock its mirror
  long long longest = 0;
  const auto t0 = std::chrono::steady_clock::now();
  const auto deadline = t0 + std::chrono::seconds(30);
  while(b.bound_count("load/p" + std::to_string(n - 1)) == 0 && std::chrono::steady_clock::now() < deadline)
  {
    server.poll();
    const auto a = std::chrono::steady_clock::now();
    mirror->run_commands();
    longest = std::max<long long>(
        longest, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - a).count());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  const auto total
      = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
  std::size_t bound = 0;
  for(int i = 0; i < n; i++)
    bound += b.bound_count("load/p" + std::to_string(i));
  client.reset();
  auto& sinks = ossia::logger().sinks();
  sinks.erase(std::remove(sinks.begin(), sinks.end(), sink), sinks.end());

  INFO("bound " << bound << " in " << total << " ms, longest run_commands " << longest << " ms, HTTP errors " << http_errors);
  REQUIRE(bound == std::size_t(n));
  REQUIRE(total < 10000);
  REQUIRE(longest < 1000);
  REQUIRE(http_errors == 0);
}

// [ossia] exposes the global device, which outlives it: deleting the object
// stops the servers it exposed and frees their ports, so that another
// [ossia] can expose on the same ports in the same Max.
TEST_CASE("ossia_object_delete_frees_its_ports", "ossia_object_delete_frees_its_ports")
{
  ossia::net::generic_device global{std::make_unique<ossia::net::multiplex_protocol>(), "max"};
  ossia::try_setup_parameter("float", ossia::net::create_node(global.get_root_node(), "/foo/bar"));
  auto& multiplex = static_cast<ossia::net::multiplex_protocol&>(global.get_protocol());
  const uint16_t osc = free_port(false), ws = free_port(true);
  const std::string url = "ws://127.0.0.1:" + std::to_string(ws);

  // what [ossia] does: its context, device::expose and its record
  auto expose = [&](ossia::net::network_context_ptr ctx, std::vector<const ossia::net::protocol_base*>& exposed) {
    const auto before = ossia::max_binding::exposed_protocols(global);
    multiplex.expose_to(std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, osc, ws, false));
    for(auto* p : ossia::max_binding::exposed_protocols(global))
      if(std::find(before.begin(), before.end(), p) == before.end())
        exposed.push_back(p);
  };
  auto answers = [&](ossia::net::network_context_ptr ctx) {
    ossia::max_binding::oscquery_connect_attempt attempt{url, "max"};
    for(int i = 0; i < 10000 && !attempt.done(); i++)
    {
      ossia::max_binding::poll_device_network_context(*ctx);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    auto dev = attempt.done() ? attempt.take() : nullptr;
    return dev && ossia::net::find_node(dev->get_root_node(), "/foo/bar") != nullptr;
  };

  // a first [ossia] exposes
  auto ctx1 = ossia::max_binding::make_device_network_context();
  std::vector<const ossia::net::protocol_base*> exposed1;
  expose(ctx1, exposed1);
  REQUIRE(exposed1.size() == 1);
  INFO("step: first server answers");
  REQUIRE(answers(ctx1));

  // it is deleted (ossia_object::destroy)
  ossia::max_binding::stop_exposing(global, exposed1);
  ctx1.reset();
  REQUIRE(ossia::max_binding::exposed_protocols(global).empty());

  // another [ossia] exposes on the same ports
  auto ctx2 = ossia::max_binding::make_device_network_context();
  std::vector<const ossia::net::protocol_base*> exposed2;
  bool exposed_again = true;
  try
  {
    expose(ctx2, exposed2);
  }
  catch(const std::exception& e)
  {
    exposed_again = false;
    UNSCOPED_INFO("second expose: " << e.what());
  }
  INFO("step: same ports exposed again");
  REQUIRE(exposed_again);
  REQUIRE(answers(ctx2));
  ossia::max_binding::stop_exposing(global, exposed2);
}

// D1. Stopping an OSCQuery server that a device exposes (ossia.device's
// "stop", multiplex_protocol::stop_expose_to) destroys the server at once,
// while its UDP socket has posted its closing to the network context. The
// next poll of the context runs that closing: it must find nothing to touch.
TEST_CASE("oscquery_server_stop_then_poll", "oscquery_server_stop_then_poll")
{
  auto ctx = std::make_shared<ossia::net::network_context>();
  ossia::net::generic_device dev{std::make_unique<ossia::net::multiplex_protocol>(), "a10_stop"};
  ossia::try_setup_parameter("float", ossia::net::create_node(dev.get_root_node(), "/foo/bar"));
  auto& multiplex = static_cast<ossia::net::multiplex_protocol&>(dev.get_protocol());
  auto server = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(
      ctx, free_port(false), free_port(true), false);
  auto* exposed = server.get();
  multiplex.expose_to(std::move(server));
  ctx->context.poll();
  ctx->context.restart();

  multiplex.stop_expose_to(*exposed);
  INFO("step: the poll that follows the stop");
  REQUIRE_NOTHROW(ctx->context.poll());
  ctx->context.restart();
  REQUIRE_NOTHROW(ctx->context.poll());
}

#include <ifaddrs.h>
#include <net/if.h>

namespace
{
// A passive mDNS listener for the tests: joins 224.0.0.251:5353 on every
// IPv4 interface (SO_REUSEPORT, as mDNSResponder does) and records, with
// their time, the PTR records that name a given service instance: TTL 0 is a
// goodbye.
struct mdns_watch
{
  struct record
  {
    std::chrono::steady_clock::time_point t;
    uint32_t ttl;
  };

  explicit mdns_watch(std::string instance)
      : instance{std::move(instance)}
  {
    fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    int one = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5353);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ok = ::bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0;
    ifaddrs* ifs = nullptr;
    if(ok && ::getifaddrs(&ifs) == 0)
    {
      for(auto* i = ifs; i; i = i->ifa_next)
      {
        if(!i->ifa_addr || i->ifa_addr->sa_family != AF_INET || (i->ifa_flags & IFF_LOOPBACK))
          continue;
        ip_mreq m{};
        m.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
        m.imr_interface = ((sockaddr_in*)i->ifa_addr)->sin_addr;
        ::setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m, sizeof(m));
      }
      ::freeifaddrs(ifs);
    }
    timeval tv{0, 100000};
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    thread = std::thread([this] { run(); });
  }
  ~mdns_watch()
  {
    done = true;
    thread.join();
    ::close(fd);
  }

  // First record after `since` with ttl 0 (goodbye) or not (announcement).
  std::optional<std::chrono::steady_clock::time_point>
  first(bool goodbye, std::chrono::steady_clock::time_point since)
  {
    std::lock_guard lock{mutex};
    for(auto& r : records)
      if(r.t >= since && (r.ttl == 0) == goodbye)
        return r.t;
    return std::nullopt;
  }

  std::string instance;
  int fd{-1};
  bool ok{};
  std::atomic_bool done{false};
  std::mutex mutex;
  std::vector<record> records;
  std::thread thread;

private:
  static bool name_at(const unsigned char* d, std::size_t n, std::size_t& off, std::string& out)
  {
    std::size_t pos = off;
    bool jumped = false;
    for(int guard = 0; guard < 64 && pos < n; guard++)
    {
      const unsigned len = d[pos];
      if((len & 0xC0) == 0xC0)
      {
        if(pos + 1 >= n)
          return false;
        if(!jumped)
          off = pos + 2;
        pos = ((len & 0x3F) << 8) | d[pos + 1];
        jumped = true;
        continue;
      }
      if(len == 0)
      {
        if(!jumped)
          off = pos + 1;
        return true;
      }
      if(pos + 1 + len > n)
        return false;
      out.append((const char*)d + pos + 1, len);
      out += '.';
      pos += 1 + len;
    }
    return false;
  }

  void parse(const unsigned char* d, std::size_t n)
  {
    if(n < 12 || !(d[2] & 0x80)) // responses only
      return;
    const unsigned qd = (d[4] << 8) | d[5];
    const unsigned rr = ((d[6] << 8) | d[7]) + ((d[8] << 8) | d[9]) + ((d[10] << 8) | d[11]);
    std::size_t off = 12;
    for(unsigned i = 0; i < qd; i++)
    {
      std::string q;
      if(!name_at(d, n, off, q) || off + 4 > n)
        return;
      off += 4;
    }
    const auto now = std::chrono::steady_clock::now();
    for(unsigned i = 0; i < rr; i++)
    {
      std::string name;
      if(!name_at(d, n, off, name) || off + 10 > n)
        return;
      const unsigned type = (d[off] << 8) | d[off + 1];
      const uint32_t ttl = (uint32_t(d[off + 4]) << 24) | (d[off + 5] << 16) | (d[off + 6] << 8) | d[off + 7];
      const unsigned len = (d[off + 8] << 8) | d[off + 9];
      off += 10;
      if(off + len > n)
        return;
      if(type == 12)
      {
        std::size_t p = off;
        std::string target;
        if(name_at(d, n, p, target) && target.rfind(instance + ".", 0) == 0)
        {
          std::lock_guard lock{mutex};
          records.push_back({now, ttl});
        }
      }
      off += len;
    }
  }

  void run()
  {
    unsigned char buf[9000];
    while(!done)
    {
      const auto n = ::recv(fd, buf, sizeof(buf), 0);
      if(n > 0)
        parse(buf, static_cast<std::size_t>(n));
    }
  }
};
}

// The server withdraws its mDNS announcement when it stops, before it closes
// its WebSocket connections, not only when it is destroyed: stop() alone,
// the server still alive, sends the goodbye (PTR with TTL 0). No client is
// connected: a connection that closes after stop() alone, the server still
// alive, reaches on_connectionClosed, which reads the WebSocket server that
// stop() has reset (a crash of its own, not this test's subject). A few
// seconds of listening on 5353.
TEST_CASE("oscquery_server_withdraws_on_stop", "oscquery_server_withdraws_on_stop")
{
  const std::string name = "a10_order_" + std::to_string(getpid());
  mdns_watch watch{name};
  REQUIRE(watch.ok);
  auto ctx = ossia::net::create_network_context();
  ossia::net::generic_device dev{std::make_unique<ossia::net::multiplex_protocol>(), name};
  ossia::try_setup_parameter("float", ossia::net::create_node(dev.get_root_node(), "/foo/bar"));
  auto& multiplex = static_cast<ossia::net::multiplex_protocol&>(dev.get_protocol());
  auto server = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(
      ctx, free_port(false), free_port(true), false);
  auto* exposed = server.get();
  const auto announced_from = std::chrono::steady_clock::now();
  multiplex.expose_to(std::move(server));
  std::thread net([&] { ctx->run(); });

  bool announced = false;
  for(int i = 0; i < 100 && !announced; i++)
  {
    announced = watch.first(false, announced_from).has_value();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  INFO("step: the announcement is seen within 5 s");
  REQUIRE(announced);

  const auto t0 = std::chrono::steady_clock::now();
  exposed->stop();
  std::optional<std::chrono::steady_clock::time_point> goodbye;
  for(int i = 0; i < 40 && !goodbye; i++)
  {
    goodbye = watch.first(true, t0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  const auto ms = [&](auto t) { return std::chrono::duration<double, std::milli>(t - t0).count(); };
  WARN("goodbye " << (goodbye ? std::to_string(ms(*goodbye)) + " ms" : std::string("not seen")) << " after stop()");
  multiplex.stop_expose_to(*exposed);
  ossia::net::stop_network_context(*ctx);
  net.join();

  INFO("step: stop() alone sends the goodbye within 2 s");
  REQUIRE(goodbye);
}

namespace
{
// An mDNS announcement of _oscjson._tcp held by a dns-sd process, apart from
// any OSCQuery server: it stays while the server behind its port dies.
struct dns_sd_announcement
{
  std::string name;
  uint16_t port{};
  pid_t pid{};

  dns_sd_announcement(std::string name, uint16_t port)
      : name{std::move(name)}
      , port{port}
  {
  }
  ~dns_sd_announcement() { withdraw(); }
  bool announce()
  {
    const std::string p = std::to_string(port);
    const char* argv[] = {"/usr/bin/dns-sd", "-R", name.c_str(), "_oscjson._tcp", "local", p.c_str(), nullptr};
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_addopen(&fa, 1, "/dev/null", O_WRONLY, 0);
    const bool ok = posix_spawn(&pid, argv[0], &fa, nullptr, const_cast<char**>(argv), environ) == 0;
    posix_spawn_file_actions_destroy(&fa);
    return ok;
  }
  void withdraw()
  {
    if(!pid)
      return;
    kill(pid, SIGTERM);
    int status = 0;
    waitpid(pid, &status, 0);
    pid = 0;
  }
};

// The directory polled every 100 ms, as the browse clock of ossia-max does,
// until `done` or `timeout_ms`; what it reported meanwhile is gathered.
struct directory_poller
{
  ossia::max_binding::oscquery_directory dir;
  std::vector<std::string> removed, lost;

  template <typename F>
  bool until(F done, int timeout_ms)
  {
    for(int t = 0; t <= timeout_ms; t += 100)
    {
      auto ch = dir.poll();
      removed.insert(removed.end(), ch.removed.begin(), ch.removed.end());
      lost.insert(lost.end(), ch.lost.begin(), ch.lost.end());
      if(done())
        return true;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
  }
  bool has(const std::vector<std::string>& v, const std::string& name) const
  {
    return std::find(v.begin(), v.end(), name) != v.end();
  }
};
}

// B7, the cause. A mirror built on the thread that polls the network context
// of the server it connects to never connects: the WebSocket handshake waits
// for that thread, busy waiting for the handshake. This is what the zeroconf
// listener of ossia-max did with a device of the same Max: it built the
// mirror on the Max clock that also polls the device's server.
TEST_CASE("oscquery_mirror_on_the_polling_thread_fails", "oscquery_mirror_on_the_polling_thread_fails")
{
  auto ctx = ossia::max_binding::make_device_network_context();
  ossia::net::generic_device dev{std::make_unique<ossia::net::multiplex_protocol>(), "a10_same_thread"};
  ossia::try_setup_parameter("float", ossia::net::create_node(dev.get_root_node(), "/foo/bar"));
  auto& multiplex = static_cast<ossia::net::multiplex_protocol&>(dev.get_protocol());
  const uint16_t ws = free_port(true);
  multiplex.expose_to(std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, free_port(false), ws, false));
  ossia::max_binding::poll_device_network_context(*ctx);
  bool built = false;
  try
  {
    // this thread polls ctx, and does not while it builds
    auto mirror = std::make_unique<ossia::net::generic_device>(
        std::make_unique<ossia::oscquery::oscquery_mirror_protocol>("ws://127.0.0.1:" + std::to_string(ws)), "m");
    built = mirror->get_protocol().update(mirror->get_root_node());
  }
  catch(...)
  {
  }
  REQUIRE(!built);
}

// B7, fixed. The directory finds the device of its own process by name: its
// mirror is built on a thread of its own while another thread polls the
// server, as Max's scheduler does every 10 ms.
TEST_CASE("oscquery_directory_finds_its_own_device", "oscquery_directory_finds_its_own_device")
{
  const std::string name = "a10_own_" + std::to_string(getpid());
  auto ctx = ossia::max_binding::make_device_network_context();
  ossia::net::generic_device dev{std::make_unique<ossia::net::multiplex_protocol>(), name};
  ossia::try_setup_parameter("float", ossia::net::create_node(dev.get_root_node(), "/foo/bar"));
  auto& multiplex = static_cast<ossia::net::multiplex_protocol&>(dev.get_protocol());
  auto server = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, free_port(false), free_port(true), false);
  auto* exposed = server.get();
  multiplex.expose_to(std::move(server));
  std::atomic_bool stop{false};
  std::thread scheduler([&] {
    while(!stop)
    {
      ossia::max_binding::poll_device_network_context(*ctx);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  directory_poller p;
  const bool found = p.until([&] {
    auto d = p.dir.find(name);
    return d && ossia::net::find_node(d->get_root_node(), "/foo/bar");
  }, 15000);
  multiplex.stop_expose_to(*exposed);
  stop = true;
  scheduler.join();
  INFO("step: the device of this process found by name, its tree read, within 15 s");
  REQUIRE(found);
}

// An announcement withdrawn and made again at the same address while the
// mirror of its device is being built, as when a server restarts: one build.
// The first went on, connected, and was dropped once the second had connected:
// two connections for one, seen several times per run of the bench.
TEST_CASE("oscquery_directory_keeps_its_build", "oscquery_directory_keeps_its_build")
{
  const std::string name = "a12_keep_" + std::to_string(getpid());
  auto ctx = ossia::max_binding::make_device_network_context();
  ossia::net::generic_device dev{std::make_unique<ossia::net::multiplex_protocol>(), name};
  ossia::try_setup_parameter("float", ossia::net::create_node(dev.get_root_node(), "/foo/bar"));
  auto& multiplex = static_cast<ossia::net::multiplex_protocol&>(dev.get_protocol());
  const uint16_t ws = free_port(true);
  // On 127.0.0.1: not announced, the test gives the announcements
  auto server = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(
      ctx, free_port(false), ws, false, "127.0.0.1");
  auto* exposed = server.get();
  multiplex.expose_to(std::move(server));
  std::atomic_bool stop{false};
  std::thread scheduler([&] {
    while(!stop)
    {
      ossia::max_binding::poll_device_network_context(*ctx);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });
  const std::string own = "max_client_" + std::to_string(getpid());
  std::set<int> ports;
  auto seen = [&] {
    rapidjson::Document doc;
    const auto json = exposed->query_connections();
    doc.Parse(json.c_str());
    for(auto& c : doc["CLIENTS"].GetArray())
      if(c.HasMember("NAME") && own == c["NAME"].GetString())
        ports.insert(c["PORT"].GetInt());
  };

  directory_poller p;
  p.dir.inject(true, name, "127.0.0.1", std::to_string(ws));
  p.dir.poll(); // the build starts
  p.dir.inject(false, name);
  p.dir.inject(true, name, "127.0.0.1", std::to_string(ws));
  REQUIRE(p.until([&] {
    seen();
    return p.dir.find(name) != nullptr;
  }, 5000));
  p.until([&] {
    seen();
    return false;
  }, 1000);
  multiplex.stop_expose_to(*exposed);
  stop = true;
  scheduler.join();
  INFO("client ports of the directory: " << ports.size());
  REQUIRE(ports.size() == 1);
}

// The directory of the package opens one connection per announced device.
// That client gives the server its own name, max_client_<process>, not the
// name of the device it mirrors: ?PRISM_CONNECTIONS of unity_demo showed a
// client unity_demo, an edge from a device to itself in prism_map. Only that
// name is checked: a Max running on the machine sees the device too, and its
// directory connects under the name of its own package.
TEST_CASE("oscquery_directory_names_its_client", "oscquery_directory_names_its_client")
{
  const std::string name = "a11_named_" + std::to_string(getpid());
  const std::string client = "max_client_" + std::to_string(getpid());
  auto ctx = ossia::max_binding::make_device_network_context();
  ossia::net::generic_device dev{std::make_unique<ossia::net::multiplex_protocol>(), name};
  ossia::try_setup_parameter("float", ossia::net::create_node(dev.get_root_node(), "/foo/bar"));
  auto& multiplex = static_cast<ossia::net::multiplex_protocol&>(dev.get_protocol());
  auto server = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, free_port(false), free_port(true), false);
  auto* exposed = server.get();
  multiplex.expose_to(std::move(server));
  std::atomic_bool stop{false};
  std::thread scheduler([&] {
    while(!stop)
    {
      ossia::max_binding::poll_device_network_context(*ctx);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  directory_poller p;
  const bool found = p.until([&] { return p.dir.find(name) != nullptr; }, 15000);
  std::string connections;
  const bool named = found && p.until([&] {
    connections = exposed->query_connections();
    return connections.find("\"NAME\":\"" + client + "\"") != std::string::npos;
  }, 3000);
  multiplex.stop_expose_to(*exposed);
  stop = true;
  scheduler.join();
  std::printf("oscquery_directory_names_its_client: %s\n", connections.c_str());
  INFO("step: the device of this process found by name, within 15 s");
  REQUIRE(found);
  INFO("step: a client named " << client << " within 3 s: " << connections);
  CHECK(named);
}

// The WebSocket of a device found by name goes down while the device stays
// announced (its process killed, the announcement held by dns-sd): the
// directory reports it lost within 2 s and drops it, then builds it again
// once the server answers on the same port.
TEST_CASE("oscquery_directory_follows_the_websocket", "oscquery_directory_follows_the_websocket")
{
  const std::string name = "a10_ws_" + std::to_string(getpid());
  server_process server{free_port(false), free_port(true)};
  server.name = "a10_srv_" + std::to_string(getpid());
  REQUIRE(server.start());
  dns_sd_announcement announcement{name, server.ws};
  REQUIRE(announcement.announce());

  directory_poller p;
  INFO("step: the device is built");
  REQUIRE(p.until([&] { return p.dir.find(name) != nullptr; }, 15000));

  server.stop();
  const auto t0 = std::chrono::steady_clock::now();
  const bool lost = p.until([&] { return p.has(p.lost, name); }, 2000);
  const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  WARN("WebSocket down reported " << ms << " ms after the kill");
  INFO("step: the loss is reported within 2 s, the device dropped");
  REQUIRE(lost);
  REQUIRE(p.dir.find(name) == nullptr);
  REQUIRE(!p.has(p.removed, name));

  REQUIRE(server.start());
  INFO("step: the device is built again once the server answers");
  REQUIRE(p.until([&] { return p.dir.find(name) != nullptr; }, 15000));
}

// A device announced before its server answers: the first builds fail, the
// directory tries again every second, and builds it once the server is up.
// Then the announcement goes, and the device with it.
TEST_CASE("oscquery_directory_retries_a_failed_build", "oscquery_directory_retries_a_failed_build")
{
  const std::string name = "a10_retry_" + std::to_string(getpid());
  server_process server{free_port(false), free_port(true)};
  server.name = "a10_srv2_" + std::to_string(getpid());
  dns_sd_announcement announcement{name, server.ws};
  REQUIRE(announcement.announce());

  directory_poller p;
  p.until([] { return false; }, 4000);
  INFO("step: nothing built while the server is down");
  REQUIRE(p.dir.find(name) == nullptr);

  REQUIRE(server.start());
  INFO("step: built once the server answers");
  REQUIRE(p.until([&] { return p.dir.find(name) != nullptr; }, 15000));

  // Servus reports the instance once per interface it is seen on: the
  // first removal drops the device, a late addition from another interface
  // may bring it back until that interface reports its removal too.
  announcement.withdraw();
  INFO("step: the device goes with its announcement");
  REQUIRE(p.until([&] { return p.has(p.removed, name) && p.dir.find(name) == nullptr; }, 10000));
}

// The name a server is announced under, as the directory sees it: the one it
// was given, or, when another announcement already has that name, the one the
// system chose (mDNSResponder renames, "name (2)"). Servus does not report it.
TEST_CASE("oscquery_directory_announced_name", "oscquery_directory_announced_name")
{
  const std::string name = "a10_dup_" + std::to_string(getpid());
  // another announcement takes the name first, on another port
  dns_sd_announcement first{name, free_port(true)};
  REQUIRE(first.announce());
  directory_poller p;
  INFO("step: the first announcement is seen");
  REQUIRE(p.until([&] { return p.dir.announced_name(first.port).has_value(); }, 10000));

  auto ctx = ossia::max_binding::make_device_network_context();
  ossia::net::generic_device dev{std::make_unique<ossia::net::multiplex_protocol>(), name};
  auto& multiplex = static_cast<ossia::net::multiplex_protocol&>(dev.get_protocol());
  const uint16_t ws = free_port(true);
  auto server = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, free_port(false), ws, false);
  auto* exposed = server.get();
  multiplex.expose_to(std::move(server));

  std::optional<std::string> real;
  p.until([&] { return (real = p.dir.announced_name(ws)).has_value(); }, 10000);
  multiplex.stop_expose_to(*exposed);
  INFO("step: the server's announcement is seen, under another name than the one taken");
  REQUIRE(real);
  WARN("asked " << name << ", announced " << *real);
  REQUIRE(*real != name);
  REQUIRE(real->rfind(name, 0) == 0);
}

// D1 as Max runs it. The device's network context is polled every 10 ms on
// another thread (Max's scheduler, device::asio_timer), a client is connected
// (in Max, the zeroconf listener's mirror of the device itself), and "stop"
// (multiplex_protocol::stop_expose_to) destroys the server on the main
// thread while that thread keeps polling. The closing handshake of the
// client's WebSocket then completes in a later poll, whose handlers must find
// nothing to touch. Before the fix: an exception out of poll (std::mutex::lock
// on the destroyed server's mutex, in Max), or AddressSanitizer.
TEST_CASE("oscquery_server_stopped_while_polled_with_a_client", "oscquery_server_stopped_while_polled_with_a_client")
{
  auto ctx = ossia::max_binding::make_device_network_context();
  ossia::net::generic_device dev{std::make_unique<ossia::net::multiplex_protocol>(), "a10_polled"};
  ossia::try_setup_parameter("float", ossia::net::create_node(dev.get_root_node(), "/foo/bar"));
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
        ossia::max_binding::poll_device_network_context(*ctx);
      }
      catch(const std::exception& e)
      {
        std::lock_guard lock{error_mutex};
        if(error.empty())
          error = e.what();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });

  ossia::max_binding::oscquery_connect_attempt attempt{"ws://127.0.0.1:" + std::to_string(ws), "a10_mirror"};
  for(int i = 0; i < 500 && !attempt.done(); i++)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  auto client = attempt.take();
  INFO("step: a client is connected");
  REQUIRE(client);

  multiplex.stop_expose_to(*exposed);
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  stop = true;
  scheduler.join();
  client.reset();

  INFO("step: no exception out of the polls after the stop");
  std::lock_guard lock{error_mutex};
  INFO("exception: " << error);
  REQUIRE(error.empty());
}

// A probe, hidden from the normal runs: runs the directory for
// OSSIA_PROBE_SECONDS, an OSCQuery server process on OSSIA_PROBE_PORT behind
// whatever announces OSSIA_PROBE_NAME (started apart), and prints, with
// their time, what the directory reports and whether it finds the name.
TEST_CASE("zz_directory_probe", "[.]")
{
  const char* name_s = std::getenv("OSSIA_PROBE_NAME");
  const char* port_s = std::getenv("OSSIA_PROBE_PORT");
  const char* secs_s = std::getenv("OSSIA_PROBE_SECONDS");
  REQUIRE(name_s);
  REQUIRE(port_s);
  const std::string name = name_s;
  const int seconds = secs_s ? std::atoi(secs_s) : 60;
  server_process server{free_port(false), static_cast<uint16_t>(std::atoi(port_s))};
  server.name = "a10_probe_srv_" + std::to_string(getpid());
  REQUIRE(server.start());
  const auto t0 = std::chrono::steady_clock::now();
  const auto at = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); };
  ossia::max_binding::oscquery_directory dir;
  bool had = false;
  for(int i = 0; i < seconds * 10; i++)
  {
    auto ch = dir.poll();
    for(auto& n : ch.removed)
      std::printf("%8.2f removed %s\n", at(), n.c_str());
    for(auto& n : ch.lost)
      std::printf("%8.2f lost %s\n", at(), n.c_str());
    const bool has = dir.find(name) != nullptr;
    if(has != had)
      std::printf("%8.2f find(%s) %s\n", at(), name.c_str(), has ? "found" : "nullptr");
    had = has;
    std::fflush(stdout);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

// demo.maxpat connected by address while prism_demo is announced: "bind
// /image/scale 2". The client first had the zeroconf listener's mirror of
// the device (connect by name, connect 1 at once), shared and kept alive by
// the listener, then its own device (connect oscquery <url>, the demo's
// fallback after 5 s). Its connector's resolution context
// goes from the first to the second; the first is not destroyed, so nothing
// tells the connector to forget it (the client clears its device before it
// announces the removal). Expected: one target, the second device's, and a
// value set leaves through it only.
TEST_CASE("v8connector_context_change_leaves_a_shared_device", "v8connector_context_change_leaves_a_shared_device")
{
  ossia::net::generic_device shared{std::make_unique<ossia::net::multiplex_protocol>(), "prism_demo"};
  ossia::net::generic_device own{std::make_unique<ossia::net::multiplex_protocol>(), "prism_demo"};
  ossia::net::generic_device max_dev{std::make_unique<ossia::net::multiplex_protocol>(), "max"};
  auto leaf = [](ossia::net::generic_device& d) {
    auto* p = ossia::try_setup_parameter("float", ossia::net::create_node(d.get_root_node(), "/image/scale"));
    p->set_domain(ossia::make_domain(0.f, 1.f));
    p->push_value(0.f);
    return p;
  };
  auto* in_shared = leaf(shared);
  auto* in_own = leaf(own);

  branch b;
  b.attach(ossia::net::create_node(max_dev.get_root_node(), "/demo_ui"));
  b.set_resolution_context({&shared.get_root_node()}, {&shared, &max_dev});
  REQUIRE(b.bind("/image/scale").empty());
  REQUIRE(b.bound_count("/image/scale") == 1);

  // the client leaves the shared mirror for its own device; the shared one lives on
  b.set_resolution_context({&own.get_root_node()}, {&own, &max_dev});
  const auto count = b.bound_count("/image/scale");
  REQUIRE(b.set("/image/scale", {0.75f}).empty());
  INFO("step: one target after the change, on the client's own device");
  REQUIRE(count == 1);
  REQUIRE(ossia::convert<float>(in_shared->value()) == 0.f);
  REQUIRE(ossia::convert<float>(in_own->value()) == 0.75f);

  // a value of the shared device no longer reaches the branch
  b.drain();
  in_shared->push_value(0.25f);
  bool from_shared = false;
  for(auto& in : b.drain())
    from_shared = from_shared || in.address == "/image/scale";
  REQUIRE(!from_shared);
}

#include <servus/servus.h>

// The Minuit listener of the package builds a device for each instance it sees
// under _minuit._tcp. That device must not announce itself: its announcement,
// "<instance> Minuit server", is one more instance for the listener, whose
// device announces itself in turn, without end (the names grow to the limit
// of mDNS, then the system numbers them: thousands of services on the
// network). Announcing also held the caller, the scheduler of Max.
TEST_CASE("minuit_listener_device_does_not_announce", "minuit_listener_device_does_not_announce")
{
  const std::string instance = "prism11_m_" + std::to_string(getpid());
  const auto start = std::chrono::steady_clock::now();
  auto dev = ossia::max_binding::make_minuit_listener_device(
      instance, "127.0.0.1", free_port(false), free_port(false));
  const auto build_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - start)
                            .count();

  servus::Servus browser{"_minuit._tcp"};
  browser.beginBrowsing(servus::Interface::IF_ALL);
  std::string seen;
  for(int i = 0; i < 300 && seen.empty(); i++)
  {
    browser.browse(10);
    for(const auto& s : browser.getInstances())
      if(s.rfind(instance, 0) == 0)
        seen = s;
  }
  browser.endBrowsing();
  std::printf("minuit_listener_device_does_not_announce: built in %lld ms, seen \"%s\"\n",
              (long long)build_ms, seen.c_str());

  INFO("step: no service of the device on the network within 3 s");
  CHECK(seen.empty());
  INFO("step: built in " << build_ms << " ms");
  CHECK(build_ms < 300);
}


// A node the server adds while the client is connected: what follows its
// parameter on the client (an ossia.remote, a callback set when the parameter
// appears) gets its value once, then once for each change. The server sends
// two ATTRIBUTES_CHANGED with a TYPE for one new parameter (the type, then the
// parameter), and the client read the value again for each: the value came
// twice. Set on the server without a push, so that only the reads bring it;
// a value equal to the default of the type comes once too.
TEST_CASE("oscquery_mirror_added_node_value_once", "oscquery_mirror_added_node_value_once")
{
  const float initial = GENERATE(0.5f, 0.f);
  auto ctx = ossia::net::create_network_context();
  auto server_multiplex = std::make_unique<ossia::net::multiplex_protocol>();
  auto* multiplex = server_multiplex.get();
  ossia::net::generic_device server{std::move(server_multiplex), "a11_added"};
  ossia::try_setup_parameter("float", ossia::net::create_node(server.get_root_node(), "/bench/first"))->push_value(0.f);
  const uint16_t ws = free_port(true);
  multiplex->expose_to(std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(
      ctx, free_port(false), ws, false));
  std::thread net([&] { ctx->run(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  auto client = std::make_shared<ossia::net::generic_device>(
      std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
          "ws://127.0.0.1:" + std::to_string(ws), free_port(false)),
      "a11_added");
  REQUIRE(client->get_protocol().update(client->get_root_node()));
  auto* mirror = ossia::max_binding::polled_oscquery_mirror(*client);
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
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  };
  auto* late = ossia::try_setup_parameter(
      "float", ossia::net::create_node(server.get_root_node(), "/bench/late"));
  late->set_value(initial);
  poll(1000);
  std::vector<ossia::value> first;
  {
    std::lock_guard l{m};
    first = received;
    received.clear();
  }
  late->push_value(0.75f);
  poll(1000);
  std::vector<ossia::value> then;
  {
    std::lock_guard l{m};
    then = received;
  }
  client->on_parameter_created.disconnect<&follower::on_created>(f);
  client.reset();
  ossia::net::stop_network_context(*ctx);
  net.join();

  std::printf("oscquery_mirror_added_node_value_once: initial %g, %zu value(s) when added, %zu after one change\n",
              initial, first.size(), then.size());
  INFO("step: the added node's value " << initial << ", once");
  CHECK(first.size() == 1);
  CHECK((first.empty() || first[0] == ossia::value(initial)));
  INFO("step: one change, one value");
  CHECK(then.size() == 1);
}

// The server of tests/Max/check_remote.py, the same as above, in a process of
// its own: device OSSIA_BENCH_NAME (announced over mDNS), /bench/first, on
// OSSIA_BENCH_OSC and OSSIA_BENCH_WS. Reads one command a line on its standard
// input and answers "done <command>": "add <v>" creates /bench/late and sets v
// without a push, "push <v>" pushes v to it, "quit" stops.
#include <iostream>
TEST_CASE("zz_added_node_server", "[.]")
{
  const char* osc_s = std::getenv("OSSIA_BENCH_OSC");
  const char* ws_s = std::getenv("OSSIA_BENCH_WS");
  const char* name_s = std::getenv("OSSIA_BENCH_NAME");
  REQUIRE(osc_s);
  REQUIRE(ws_s);
  REQUIRE(name_s);
  auto ctx = ossia::net::create_network_context();
  auto mp = std::make_unique<ossia::net::multiplex_protocol>();
  auto* multiplex = mp.get();
  ossia::net::generic_device dev{std::move(mp), name_s};
  ossia::try_setup_parameter("float", ossia::net::create_node(dev.get_root_node(), "/bench/first"))->push_value(0.f);
  auto server = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(
      ctx, std::atoi(osc_s), std::atoi(ws_s), false);
  // OSSIA_BENCH_LOG: what the server receives and sends, when set
  if(const char* log_s = std::getenv("OSSIA_BENCH_LOG"); log_s && *log_s)
  {
    ossia::net::network_logger l;
    l.inbound_logger = spdlog::basic_logger_mt("added_in", log_s);
    l.inbound_logger->flush_on(spdlog::level::info);
    l.outbound_logger = l.inbound_logger;
    server->set_logger(l);
  }
  multiplex->expose_to(std::move(server));
  std::thread net([&] { ctx->run(); });

  ossia::net::parameter_base* late{};
  std::string command;
  float v{};
  while(std::cin >> command && command != "quit")
  {
    if(command == "add" && std::cin >> v && !late)
    {
      late = ossia::try_setup_parameter("float", ossia::net::create_node(dev.get_root_node(), "/bench/late"));
      late->set_value(v);
    }
    else if(command == "push" && std::cin >> v && late)
      late->push_value(v);
    else
      command = "refused";
    std::printf("done %s\n", command.c_str());
    std::fflush(stdout);
  }
  ossia::net::stop_network_context(*ctx);
  net.join();
}
