#include "ZeroconfOscqueryListener.hpp"

#include "device.hpp"
#include "ossia-max.hpp"

#include <ossia/network/local/local.hpp>
#include <ossia/protocols/oscquery/oscquery_server_asio.hpp>

namespace ossia
{
namespace max_binding
{
std::unique_ptr<oscquery_directory> ZeroconfOscqueryListener::s_directory;
std::map<const void*, std::string> ZeroconfOscqueryListener::s_announced;

ZeroconfOscqueryListener::ZeroconfOscqueryListener()
{
  s_directory = std::make_unique<oscquery_directory>();
}

ZeroconfOscqueryListener::~ZeroconfOscqueryListener()
{
  s_directory.reset();
}

std::shared_ptr<ossia::net::generic_device>
ZeroconfOscqueryListener::find_device(const std::string& instance)
{
  return s_directory ? s_directory->find(instance) : nullptr;
}

std::vector<std::shared_ptr<ossia::net::generic_device>>
ZeroconfOscqueryListener::get_devices()
{
  return s_directory ? s_directory->devices() : std::vector<std::shared_ptr<ossia::net::generic_device>>{};
}

void ZeroconfOscqueryListener::browse()
{
  if(!s_directory)
    return;
  const auto changes = s_directory->poll();
  for(const auto& name : changes.removed)
    release_clients(name, false);
  for(const auto& name : changes.lost)
    release_clients(name, true);
  report_announced_names();
}

void ZeroconfOscqueryListener::report_announced_names()
{
  struct report
  {
    device* dev;
    std::string asked;
    std::string name;
  };
  std::vector<report> reports;
  std::map<const void*, std::string> still;

  critical_enter(0);
  for(auto dev : ossia_max::instance().devices.reference())
  {
    if(!dev->m_device)
      continue;
    auto* multiplex = dynamic_cast<ossia::net::multiplex_protocol*>(&dev->m_device->get_protocol());
    if(!multiplex)
      continue;
    const auto asked = dev->m_device->get_name();
    multiplex->for_each_protocol([&](ossia::net::protocol_base& p) {
      auto* server = dynamic_cast<ossia::oscquery_asio::oscquery_server_protocol*>(&p);
      if(!server)
        return;
      if(auto it = s_announced.find(server); it != s_announced.end())
      {
        still.insert(*it);
        return;
      }
      if(auto name = s_directory->announced_name(server->get_ws_port()))
      {
        still.emplace(server, *name);
        reports.push_back({dev, asked, *name});
      }
    });
  }
  s_announced.swap(still);

  // Out of the multiplex's lock: a patch may answer by stopping the device.
  for(const auto& r : reports)
  {
    if(r.name != r.asked)
      object_warn(
          (t_object*)r.dev, "announced as \"%s\": the name \"%s\" is taken on the network",
          r.name.c_str(), r.asked.c_str());
    t_atom a;
    A_SETSYM(&a, gensym(r.name.c_str()));
    outlet_anything(r.dev->m_dumpout, gensym("announced"), 1, &a);
  }
  critical_exit(0);
}

void ZeroconfOscqueryListener::release_clients(const std::string& name, bool lost)
{
  for(auto client : ossia_max::instance().clients.reference())
  {
    if(client->is_zeroconf() && client->m_device && client->m_device->get_name() == name)
    {
      ossia::max_binding::client::client::disconnect(client);
      if(lost)
      {
        t_atom a[2];
        A_SETFLOAT(a, 0);
        A_SETSYM(a + 1, gensym(name.c_str()));
        outlet_anything(client->m_dumpout, gensym("connect"), 2, a);
      }
      if(client->m_reconnect)
        clock_delay(client->m_clock, 1000); // hardcoded reconnection delay
    }
  }
}

}
} // namespace ossia
