// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
#include "device_protocols.hpp"

#include <ossia/network/minuit/minuit.hpp>
#include <ossia/network/osc/osc.hpp>
#include <ossia/network/oscquery/oscquery_client.hpp>
#include <ossia/network/oscquery/oscquery_server.hpp>
#include <ossia/protocols/oscquery/oscquery_client_asio.hpp>
#include <ossia/protocols/oscquery/oscquery_server_asio.hpp>

namespace ossia
{
namespace max_binding
{
exposed_protocol describe_protocol(const ossia::net::protocol_base& p)
{
  if(auto osc = dynamic_cast<const ossia::net::osc_protocol*>(&p))
    return {"osc", osc->get_ip(), osc->get_remote_port(), osc->get_local_port()};
  // The server ossia.device exposes, for expose oscquery and oscquery_tcp
  if(auto oscq = dynamic_cast<const ossia::oscquery_asio::oscquery_server_protocol*>(&p))
    return {"oscquery", {}, oscq->get_osc_port(), oscq->get_ws_port()};
  if(auto oscq = dynamic_cast<const ossia::oscquery::oscquery_server_protocol*>(&p))
    return {"oscquery", {}, oscq->get_osc_port(), oscq->get_ws_port()};
  if(auto minuit = dynamic_cast<const ossia::net::minuit_protocol*>(&p))
    return {"minuit", minuit->get_ip(), minuit->get_remote_port(), minuit->get_local_port()};
  return {};
}

std::vector<oscquery_client_entry>
oscquery_clients(const ossia::net::multiplex_protocol& multiplex)
{
  std::vector<oscquery_client_entry> res;
  for(auto& p : multiplex.get_protocols())
  {
    if(auto oscq = dynamic_cast<ossia::oscquery_asio::oscquery_server_protocol_base*>(p.get()))
    {
      oscq->for_each_client([&](const auto& c) {
        res.push_back({c.client_ip, c.osc_port ? c.osc_port : -1});
      });
    }
    else if(auto oscq = dynamic_cast<const ossia::oscquery::oscquery_server_protocol*>(p.get()))
    {
      oscq->for_each_client([&](const auto& c) {
        res.push_back({c.client_ip, c.sender ? int(c.sender->port()) : -1});
      });
    }
  }
  return res;
}
}
}
