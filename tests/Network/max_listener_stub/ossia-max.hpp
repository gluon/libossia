// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

#pragma once
// Stands in for src/ossia-max/src/ossia-max.hpp, which needs the Max SDK:
// what ZeroconfMinuitListener.cpp uses of the package, and nothing else.
// make_minuit_listener_device, which builds the device of an announcement,
// comes from client_mirror.hpp, as in the package: the test links the real
// client_mirror.cpp, which does not need the Max SDK.
#include "client_mirror.hpp"

#include <ossia/detail/algorithms.hpp>
#include <ossia/detail/safe_vec.hpp>
#include <ossia/network/generic/generic_device.hpp>

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

inline void clock_delay(void*, long) { }

namespace ossia::max_binding
{
class client
{
public:
  bool is_zeroconf() const { return m_zeroconf; }
  static void disconnect(client* c)
  {
    c->m_disconnected = true;
    if(c->m_on_disconnect)
      c->m_on_disconnect();
  }

  void* m_clock{};
  std::shared_ptr<ossia::net::generic_device> m_device{};
  bool m_zeroconf{};
  bool m_disconnected{};
  std::function<void()> m_on_disconnect{};
};

class ossia_max
{
public:
  static ossia_max& instance()
  {
    static ossia_max self;
    return self;
  }

  ossia::safe_vector<client*> clients;
};
}
