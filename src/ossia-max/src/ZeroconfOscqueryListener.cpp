#include "ZeroconfOscqueryListener.hpp"

#include "ossia-max.hpp"

namespace ossia
{
namespace max_binding
{
std::unique_ptr<oscquery_directory> ZeroconfOscqueryListener::s_directory;

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
