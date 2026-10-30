// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

#include "device_bundles.hpp"

#include <ossia/network/local/local.hpp>
#include <ossia/network/oscquery/detail/osc_writer.hpp>
#include <ossia/network/oscquery/oscquery_mirror.hpp>
#include <ossia/protocols/oscquery/oscquery_server_asio.hpp>

#include <algorithm>
#include <span>
#include <unordered_map>

namespace ossia
{
namespace max_binding
{

bool bundles_by_default(ossia::net::protocol_base& protocol)
{
  if(dynamic_cast<ossia::oscquery::oscquery_mirror_protocol*>(&protocol))
    return true;
  bool oscquery = false;
  if(auto* multiplex = dynamic_cast<ossia::net::multiplex_protocol*>(&protocol))
    multiplex->for_each_protocol([&](ossia::net::protocol_base& p) {
      if(dynamic_cast<ossia::oscquery_asio::oscquery_server_protocol_base*>(&p))
        oscquery = true;
    });
  return oscquery;
}

namespace
{
// The queue of each device that has one: the objects push from Max's main
// thread or its scheduler, while devices come and go on the main thread.
std::mutex g_registry_mutex;
std::unordered_map<const ossia::net::device_base*, device_bundle_queue*> g_registry;
}

device_bundle_queue::device_bundle_queue(
    ossia::net::device_base& device, std::function<void()> schedule)
    : m_device{device}
    , m_schedule{std::move(schedule)}
{
  m_device.on_parameter_removing.connect<&device_bundle_queue::on_parameter_removing>(this);
  std::lock_guard lock{g_registry_mutex};
  g_registry[&m_device] = this;
}

device_bundle_queue::~device_bundle_queue()
{
  {
    std::lock_guard lock{g_registry_mutex};
    if(auto it = g_registry.find(&m_device); it != g_registry.end() && it->second == this)
      g_registry.erase(it);
  }
  m_device.on_parameter_removing.disconnect<&device_bundle_queue::on_parameter_removing>(this);
}

bool device_bundle_queue::active()
{
  const long mode = m_mode;
  return mode < 0 ? bundles_by_default(m_device.get_protocol()) : mode > 0;
}

bool device_bundle_queue::push_if_active(
    ossia::net::parameter_base& param, const ossia::value& value)
{
  const auto* device = &param.get_node().get_device();
  {
    std::lock_guard lock{g_registry_mutex};
    auto it = g_registry.find(device);
    if(it == g_registry.end() || !it->second->active())
      return false;
  }
  // Out of the lock: the local callbacks may push again.
  auto res = param.set_value(value);
  if(!res.valid())
    return true;
  {
    std::lock_guard lock{g_registry_mutex};
    if(auto it = g_registry.find(device); it != g_registry.end())
    {
      it->second->push(param, std::move(res));
      return true;
    }
  }
  // The queue went away meanwhile: sent at once.
  param.get_protocol().push(param, res);
  return true;
}

bool device_bundle_queue::push(ossia::net::parameter_base& param, ossia::value value)
{
  bool first;
  {
    std::lock_guard lock{m_mutex};
    m_queue.push_back({&param, std::move(value)});
    first = m_queue.size() == 1;
  }
  if(first && m_schedule)
    m_schedule();
  return first;
}

std::size_t device_bundle_queue::size()
{
  std::lock_guard lock{m_mutex};
  return m_queue.size();
}

// A parameter removed before the flush is not sent.
void device_bundle_queue::on_parameter_removing(const ossia::net::parameter_base& p)
{
  std::lock_guard lock{m_mutex};
  std::erase_if(m_queue, [&p](const ossia::bundle_element& e) { return e.parameter == &p; });
}

std::size_t device_bundle_queue::flush()
{
  // Held while sending: a parameter cannot be removed meanwhile.
  std::lock_guard lock{m_mutex};
  if(m_queue.empty())
    return 0;
  std::vector<ossia::bundle_element> elements;
  elements.swap(m_queue);
  std::span<ossia::bundle_element> all{elements};
  auto& protocol = m_device.get_protocol();

  if(dynamic_cast<ossia::oscquery::oscquery_mirror_protocol*>(&protocol))
  {
    protocol.push_bundle(all);
  }
  else if(auto* multiplex = dynamic_cast<ossia::net::multiplex_protocol*>(&protocol))
  {
    multiplex->for_each_protocol([&](ossia::net::protocol_base& p) {
      if(dynamic_cast<ossia::oscquery_asio::oscquery_server_protocol_base*>(&p))
        p.push_bundle(all);
      else
        for(auto& e : elements)
          p.push(*e.parameter, e.values);
    });
  }
  else if(!dynamic_cast<ossia::net::can_learn<ossia::net::protocol_base>*>(&protocol))
  {
    // Not an OSC protocol: push_bundle would set the values again. One by one.
    for(auto& e : elements)
      protocol.push(*e.parameter, e.values);
  }
  else
  {
    // The OSC protocols send what they are given as one bundle: bundles of at
    // most 1400 bytes of messages.
    constexpr std::size_t header = 16, limit = 1400;
    std::size_t begin = 0, size = header;
    for(std::size_t i = 0; i < elements.size(); i++)
    {
      const std::size_t message
          = 4
            + ossia::oscquery::osc_writer::to_message(*elements[i].parameter, elements[i].values)
                  .size();
      if(i > begin && size + message > limit)
      {
        protocol.push_bundle(all.subspan(begin, i - begin));
        begin = i;
        size = header;
      }
      size += message;
    }
    protocol.push_bundle(all.subspan(begin));
  }
  return elements.size();
}

}
}
