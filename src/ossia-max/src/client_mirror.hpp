// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

#pragma once

#include <ossia/network/base/device.hpp>
#include <ossia/network/oscquery/oscquery_mirror.hpp>

namespace ossia
{
namespace max_binding
{

// The OSCQuery mirror of a client's device, whether the client connected by
// URL or by name, set up as ossia-max sets up a connection by URL: a path the
// server removes is removed, not kept as a zombie. The tree changes the server
// sends (PATH_ADDED, PATH_REMOVED, ATTRIBUTES_CHANGED) wait in the mirror's
// command queue until run_commands is called, which the client does when it
// polls. Returns nullptr for a device of another protocol.
inline ossia::oscquery::oscquery_mirror_protocol*
polled_oscquery_mirror(ossia::net::device_base& dev)
{
  auto* mirror = dynamic_cast<ossia::oscquery::oscquery_mirror_protocol*>(&dev.get_protocol());
  if(mirror)
    mirror->set_zombie_on_remove(false);
  return mirror;
}

}
}
