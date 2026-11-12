// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// What the OSCQuery server answers over HTTP, against the OSCQuery proposal
// (github.com/Vidvox/OSCQueryProposal, README.md). Server: the one of the C API
// (ossia::oscquery::oscquery_server_protocol), zeroconf disabled, loopback.
// Each TEST_CASE checks what the proposal asks, and prints what it got.
#include "ReproCommon.hpp"

#include <ossia/detail/json.hpp>
#include <ossia/network/dataspace/dataspace_visitors.hpp>
#include <ossia/network/domain/domain.hpp>

#include "include_catch.hpp"

#include <limits>

using namespace repro;

namespace
{
bool strict_json(const std::string& text)
{
  // ossia/detail/config.hpp makes rapidjson accept NaN, comments and trailing
  // commas by default: parse as a strict JSON reader would.
  rapidjson::Document d;
  d.Parse<rapidjson::kParseNoFlags>(text.c_str());
  return !d.HasParseError();
}

void show(const char* test, const std::string& target, const http_reply& r)
{
  std::printf("%s: GET %s -> %d %s\n", test, target.c_str(), r.status, r.body.substr(0, 200).c_str());
}
}

// Proposal, line 72: 404 for a path that does not exist. An attribute the
// server does not know answers 204, as line 64 says of a query for an
// attribute the server does not support. Line 71 gives 400 for the same case,
// among its examples of errors; line 64 is the rule of the query part of a
// URL, and lines 97 and 98 also answer 204, so 204 it is.
TEST_CASE("HTTP: 404 for a missing path, 204 for an unknown attribute", "[http-status]")
{
  api_server s;
  add_float(s.root(), "/f", 0.5f);
  auto missing = http_get(s.ws, "/nothing");
  auto missing_attr = http_get(s.ws, "/nothing?TYPE");
  auto bad_attr = http_get(s.ws, "/f?BOGUS");
  show("http-status", "/nothing", missing);
  show("http-status", "/nothing?TYPE", missing_attr);
  show("http-status", "/f?BOGUS", bad_attr);
  CHECK(missing.status == 404);
  CHECK(missing_attr.status == 404);
  CHECK(bad_attr.status == 204);
  CHECK(bad_attr.body.empty());
}

// Proposal, line 65: the fragment of the URL is not part of the path.
TEST_CASE("HTTP: a URL fragment is ignored", "[http-fragment]")
{
  api_server s;
  add_float(s.root(), "/f", 0.5f);
  auto plain = http_get(s.ws, "/f?VALUE");
  auto frag = http_get(s.ws, "/f#frag");
  auto frag_attr = http_get(s.ws, "/f?VALUE#frag");
  show("http-fragment", "/f?VALUE", plain);
  show("http-fragment", "/f#frag", frag);
  show("http-fragment", "/f?VALUE#frag", frag_attr);
  CHECK(plain.status == 200);
  CHECK(frag.status == 200);
  CHECK(frag_attr.status == 200);
}

// Proposal, line 127: VALUE is JSON. A NaN must not make the tree unreadable.
TEST_CASE("JSON: a NaN value keeps the tree valid JSON", "[json-nan]")
{
  api_server s;
  add_float(s.root(), "/nan", std::numeric_limits<float>::quiet_NaN());
  auto value = http_get(s.ws, "/nan?VALUE");
  auto node = http_get(s.ws, "/nan");
  auto tree = http_get(s.ws, "/");
  show("json-nan", "/nan?VALUE", value);
  show("json-nan", "/nan", node);
  std::printf("json-nan: tree is strict JSON: %d\n", int(strict_json(tree.body)));
  CHECK(value.status == 200);
  CHECK(strict_json(value.body));
  CHECK(strict_json(tree.body));
}

// A parameter of type MAP (the C API makes one from CHAR_T) must not break
// the tree either.
TEST_CASE("JSON: a MAP parameter keeps the tree valid JSON", "[json-map]")
{
  api_server s;
  ossia::net::create_node(s.root(), "/map").create_parameter(ossia::val_type::MAP);
  auto value = http_get(s.ws, "/map?VALUE");
  auto node = http_get(s.ws, "/map");
  auto tree = http_get(s.ws, "/");
  show("json-map", "/map?VALUE", value);
  show("json-map", "/map", node);
  std::printf("json-map: tree is strict JSON: %d\n", int(strict_json(tree.body)));
  CHECK(strict_json(value.body));
  CHECK(strict_json(node.body));
  CHECK(strict_json(tree.body));
}

// Proposal, line 132: a boolean VALUE is a JSON boolean.
TEST_CASE("JSON: a boolean is served as true or false", "[json-bool]")
{
  api_server s;
  auto& t = ossia::net::create_node(s.root(), "/t");
  t.create_parameter(ossia::val_type::BOOL)->set_value(true);
  auto& f = ossia::net::create_node(s.root(), "/f");
  f.create_parameter(ossia::val_type::BOOL)->set_value(false);
  auto rt = http_get(s.ws, "/t?VALUE");
  auto rf = http_get(s.ws, "/f?VALUE");
  show("json-bool", "/t?VALUE", rt);
  show("json-bool", "/f?VALUE", rf);
  rapidjson::Document dt, df;
  dt.Parse(rt.body.c_str());
  df.Parse(rf.body.c_str());
  REQUIRE(!dt.HasParseError());
  REQUIRE(!df.HasParseError());
  CHECK((dt.HasMember("VALUE") && dt["VALUE"].IsTrue()));
  CHECK((df.HasMember("VALUE") && df["VALUE"].IsFalse()));
}

// HOST_INFO declares ECHO; the device's echo is off by default. What it
// declares must be what a second client sees when the first one writes.
TEST_CASE("HOST_INFO: ECHO says what the server does", "[hostinfo-echo]")
{
  api_server s;
  add_float(s.root(), "/f", 0.5f);
  auto info = http_get(s.ws, "/?HOST_INFO");
  rapidjson::Document d;
  d.Parse(info.body.c_str());
  REQUIRE(!d.HasParseError());
  const bool declared = d.HasMember("EXTENSIONS") && d["EXTENSIONS"].HasMember("ECHO")
                        && d["EXTENSIONS"]["ECHO"].IsTrue();

  ws_client writer{s.ws}, other{s.ws};
  REQUIRE(writer.connected());
  REQUIRE(other.connected());
  writer.command("LISTEN", "/f");
  other.command("LISTEN", "/f");
  std::this_thread::sleep_for(300ms);
  other.take();
  writer.send_binary(osc_float("/f", 3.f));
  std::this_thread::sleep_for(500ms);
  const auto applied = http_get(s.ws, "/f?VALUE");
  const auto seen = ws_client::messages(other.take(), "/f").size();
  std::printf("hostinfo-echo: device echo %d, ECHO declared %d; the write is applied: %s; the other client got %zu value(s)\n",
              int(s.dev->has_echo()), int(declared), applied.body.c_str(), seen);
  REQUIRE(applied.body.find('3') != std::string::npos);
  CHECK(declared == (seen > 0));
}

// Proposal, line 84: an attribute not listed in EXTENSIONS is not supported.
TEST_CASE("HOST_INFO: EXTENDED_TYPE is declared when it is served", "[hostinfo-extended-type]")
{
  api_server s;
  auto& n = ossia::net::create_node(s.root(), "/file");
  n.create_parameter(ossia::val_type::STRING)->set_value(std::string("/tmp/x"));
  ossia::net::set_extended_type(n, std::string("filepath"));
  auto node = http_get(s.ws, "/file");
  auto info = http_get(s.ws, "/?HOST_INFO");
  show("hostinfo-extended-type", "/file", node);
  rapidjson::Document d;
  d.Parse(info.body.c_str());
  REQUIRE(!d.HasParseError());
  const bool served = node.body.find("\"EXTENDED_TYPE\"") != std::string::npos;
  const bool declared = d.HasMember("EXTENSIONS") && d["EXTENSIONS"].HasMember("EXTENDED_TYPE");
  std::printf("hostinfo-extended-type: served %d, declared %d\n", int(served), int(declared));
  REQUIRE(served);
  CHECK(declared);
}

// A parameter with a unit gets one EXTENDED_TYPE key, not two.
TEST_CASE("JSON: one EXTENDED_TYPE key for a parameter with a unit", "[extended-type-twice]")
{
  api_server s;
  const char* units[] = {"color.rgba", "color.hsv", "position.cart3D"};
  const ossia::val_type types[] = {ossia::val_type::VEC4F, ossia::val_type::VEC3F, ossia::val_type::VEC3F};
  int worst = 0;
  for(int i = 0; i < 3; i++)
  {
    const std::string path = "/u" + std::to_string(i);
    auto* p = ossia::net::create_node(s.root(), path).create_parameter(types[i]);
    p->set_unit(ossia::parse_pretty_unit(units[i]));
    auto node = http_get(s.ws, path);
    int keys = 0;
    for(auto pos = node.body.find("\"EXTENDED_TYPE\""); pos != std::string::npos;
        pos = node.body.find("\"EXTENDED_TYPE\"", pos + 1))
      keys++;
    std::printf("extended-type-twice: %s: %d EXTENDED_TYPE key(s) in %s\n", units[i], keys, node.body.c_str());
    worst = std::max(worst, keys);
  }
  CHECK(worst == 1);
}

// Proposal, lines 102 and 338-346: /path?ATTRIBUTE answers what the tree says
// for that attribute; an attribute the node does not have has no value.
TEST_CASE("HTTP: an attribute query agrees with the tree", "[attribute-queries]")
{
  api_server s;
  auto* f = add_float(s.root(), "/f", 0.5f);
  f->set_domain(ossia::make_domain(0.f, 10.f));
  ossia::net::create_node(s.root(), "/i").create_parameter(ossia::val_type::INT)->set_value(3);
  ossia::net::create_node(s.root(), "/v3").create_parameter(ossia::val_type::VEC3F)->set_unit(
      ossia::parse_pretty_unit("position.cart3D"));
  add_float(s.root(), "/ctn/sub", 4.f);

  const std::pair<const char*, const char*> queries[] = {
      {"/ctn", "TYPE"},  {"/ctn", "VALUE"},          {"/f", "UNIT"},           {"/f", "EXTENDED_TYPE"},
      {"/i", "RANGE"},   {"/v3", "UNIT"},            {"/v3", "EXTENDED_TYPE"}, {"/f", "RANGE"},
      {"/f", "VALUE"}};
  int disagree = 0;
  for(auto [path, attr] : queries)
  {
    auto node = http_get(s.ws, path);
    auto q = http_get(s.ws, std::string(path) + "?" + attr);
    rapidjson::Document tree, answer;
    tree.Parse(node.body.c_str());
    REQUIRE(!tree.HasParseError());
    bool agrees;
    if(tree.HasMember(attr))
    {
      answer.Parse(q.body.c_str());
      agrees = q.status == 200 && !answer.HasParseError() && answer.IsObject() && answer.HasMember(attr)
               && answer[attr] == tree[attr];
    }
    else
    {
      // No value: 204, or an object without the attribute ({}).
      answer.Parse(q.body.c_str());
      agrees = q.status == 204
               || (q.status == 200 && !answer.HasParseError() && answer.IsObject() && !answer.HasMember(attr));
    }
    std::printf("attribute-queries: %s?%s -> %d %s ; in the tree: %s%s\n", path, attr, q.status, q.body.c_str(),
                tree.HasMember(attr) ? "yes" : "no", agrees ? "" : "  <- disagrees");
    if(!agrees)
      disagree++;
  }
  std::printf("attribute-queries: %d of 9 disagree\n", disagree);
  CHECK(disagree == 0);
}
