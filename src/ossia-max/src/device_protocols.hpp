// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
#pragma once
// Max-independent part of the getprotocols and getoscqclients messages of
// ossia.device: what they report of the protocols a device is exposed to.
#include <ossia/network/base/protocol.hpp>
#include <ossia/network/local/local.hpp>

#include <string>
#include <vector>

namespace ossia
{
namespace max_binding
{
// One protocol of the device, as getprotocols lists it
struct exposed_protocol
{
  // "osc", "oscquery" or "minuit"; empty for a protocol it does not list
  std::string kind;
  // osc and minuit: the remote address
  std::string ip;
  // osc and minuit: the remote port; oscquery: the OSC port
  int first_port{};
  // osc and minuit: the local port; oscquery: the WebSocket port
  int second_port{};
};

exposed_protocol describe_protocol(const ossia::net::protocol_base& p);

// One client of an OSCQuery server of the device, as getoscqclients lists it
struct oscquery_client_entry
{
  std::string ip;
  // The UDP port the server sends the client's values to, -1 when the client
  // receives them over the WebSocket
  int osc_port{-1};
};

std::vector<oscquery_client_entry>
oscquery_clients(const ossia::net::multiplex_protocol& multiplex);
}
}
