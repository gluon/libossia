// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// A LISTEN or IGNORE command whose DATA is not a string, sent by any peer
// over the WebSocket. Each TEST_CASE ends the test process when the defect is
// there: run them one at a time, by tag. ReproCommon.hpp, next to this file,
// has the client and the servers. Build with OSSIA_DNSSD=OFF to keep the asio
// server off mDNS.
#include "ReproCommon.hpp"

#include "include_catch.hpp"

using namespace repro;

namespace
{
template <typename Server>
void send_and_ask(Server& s, uint16_t ws, const char* text, const char* tag)
{
  ws_client c{ws};
  REQUIRE(c.connected());
  c.send_text(text);
  std::this_thread::sleep_for(500ms);
  const auto r = http_get(ws, "/f?VALUE");
  std::printf("%s: after %s, GET /f?VALUE -> %d %s\n", tag, text, r.status, r.body.c_str());
  CHECK(r.status == 200);
}
}

TEST_CASE("C API server: LISTEN with a number", "[listen-number-api]")
{
  api_server s;
  add_float(s.root(), "/f", 0.5f);
  send_and_ask(s, s.ws, R"({"COMMAND":"LISTEN","DATA":12})", "listen-number-api");
}

TEST_CASE("C API server: IGNORE with a number", "[ignore-number-api]")
{
  api_server s;
  add_float(s.root(), "/f", 0.5f);
  send_and_ask(s, s.ws, R"({"COMMAND":"IGNORE","DATA":12})", "ignore-number-api");
}

TEST_CASE("asio server (ossia.device): LISTEN with a number", "[listen-number-asio]")
{
  asio_server s;
  add_float(s.root(), "/f", 0.5f);
  s.start();
  send_and_ask(s, s.ws, R"({"COMMAND":"LISTEN","DATA":12})", "listen-number-asio");
}

TEST_CASE("asio server (ossia.device): IGNORE with a number", "[ignore-number-asio]")
{
  asio_server s;
  add_float(s.root(), "/f", 0.5f);
  s.start();
  send_and_ask(s, s.ws, R"({"COMMAND":"IGNORE","DATA":12})", "ignore-number-asio");
}

// The same with a string, for comparison: the server answers.
TEST_CASE("C API server: LISTEN with a string", "[listen-string-api]")
{
  api_server s;
  add_float(s.root(), "/f", 0.5f);
  send_and_ask(s, s.ws, R"({"COMMAND":"LISTEN","DATA":"/f"})", "listen-string-api");
}
