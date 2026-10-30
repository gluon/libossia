// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

#pragma once

#include <ossia/network/context.hpp>

#include <memory>

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

}
}
