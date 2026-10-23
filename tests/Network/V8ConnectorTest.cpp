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

#include <v8connector_branch.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <string>
#include <thread>

using namespace ossia::max_binding::v8connector_detail;

namespace
{

uint16_t free_port(bool tcp)
{
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
  REQUIRE(!b1.add(gain).empty()); // same address twice in one branch

  SECTION("the default of a leaf comes out of its own branch once it is created")
  {
    // as ossia-max outputs the defaults of its parameters after they registered
    auto in = b1.drain();
    REQUIRE(in.size() == 3);
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
    REQUIRE(gain1->value() == ossia::value(6.f));

    REQUIRE(!b1.set("nothere", {1}).empty());
    REQUIRE(!b1.set("pos", {1}).empty());
    REQUIRE(b1.set("pos", {0.5f, -0.5f}).empty());

    int wakes = 0;
    b1.set_wake_callback([&] { ++wakes; });
    gain1->push_value(0.5f);
    auto in = b1.drain();
    REQUIRE(in.size() == 1);
    REQUIRE(in[0].address == "gain");
    REQUIRE(in[0].value == ossia::value(0.5f));
    REQUIRE(wakes == 1);
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
    REQUIRE(!b.bind("synth/gain").empty()); // twice
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

    // the owner's type and range apply to the follower's pushes
    REQUIRE(b.set("synth/gain", {100}).empty());
    REQUIRE(ossia::net::find_node(root, "/synth/gain")->get_parameter()->value() == ossia::value(6.f));
    REQUIRE(!b.set("synth/gain", {S("loud")}).empty());

    // unbind: nothing flows any more, the parameter is untouched
    REQUIRE(b.unbind("synth/gain"));
    REQUIRE(!b.unbind("synth/gain"));
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
