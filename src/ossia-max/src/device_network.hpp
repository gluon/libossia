// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

#pragma once

#include <ossia/network/base/device.hpp>
#include <ossia/network/context.hpp>
#include <ossia/network/local/local.hpp>

#include <algorithm>
#include <memory>
#include <vector>

namespace ossia
{
namespace max_binding
{

// The network context of an ossia-max device: the servers it exposes run on
// it, and Max's main thread polls it every 10 ms (device::asio_timer). Every
// object that exposes a device needs one, the global [ossia] object included.
inline ossia::net::network_context_ptr make_device_network_context()
{
  return std::make_shared<ossia::net::network_context>();
}

inline void poll_device_network_context(ossia::net::network_context& ctx)
{
  ctx.context.poll();
  ctx.context.restart();
}

// The protocols a device exposes now (expose), when it is multiplexed.
inline std::vector<const ossia::net::protocol_base*>
exposed_protocols(ossia::net::device_base& device)
{
  std::vector<const ossia::net::protocol_base*> out;
  if(auto* multiplex = dynamic_cast<ossia::net::multiplex_protocol*>(&device.get_protocol()))
    multiplex->for_each_protocol([&](ossia::net::protocol_base& p) { out.push_back(&p); });
  return out;
}

// Stops the given protocols that the device still exposes: their ports are
// free once it returns. The global device of Max outlives the [ossia] object
// that exposed it: the object stops what it exposed when it is deleted, while
// its network context still exists.
inline void stop_exposing(
    ossia::net::device_base& device, const std::vector<const ossia::net::protocol_base*>& protocols)
{
  auto* multiplex = dynamic_cast<ossia::net::multiplex_protocol*>(&device.get_protocol());
  if(!multiplex)
    return;
  const auto now = exposed_protocols(device);
  for(auto* p : protocols)
    if(std::find(now.begin(), now.end(), p) != now.end())
      multiplex->stop_expose_to(*p);
}

}
}
