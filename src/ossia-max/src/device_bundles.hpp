// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

#pragma once

// Max-independent part of the bundles of ossia.device and ossia.client: what
// the objects of a device push during a tick of Max's scheduler leaves at its
// end, in OSC bundles.

#include <ossia/network/base/bundle.hpp>
#include <ossia/network/base/device.hpp>
#include <ossia/network/base/protocol.hpp>

#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <vector>

namespace ossia
{
namespace max_binding
{

// Whether a device sends bundles when its @bundle is left to its default:
// yes when it speaks OSCQuery (a client's mirror, or a device exposed through
// an OSCQuery server), no for plain OSC, Minuit and the others.
bool bundles_by_default(ossia::net::protocol_base& protocol);

// The values the objects of a device pushed since the last flush, in order.
// Nothing is merged: a parameter pushed twice leaves twice, impulses
// included. The values were already set on the parameters (their local
// callbacks ran); the queue only sends them.
class device_bundle_queue
{
public:
  // schedule: called when the first value of a tick is queued; the host then
  // flushes at the end of the tick.
  explicit device_bundle_queue(
      ossia::net::device_base& device, std::function<void()> schedule = {});
  ~device_bundle_queue();
  device_bundle_queue(const device_bundle_queue&) = delete;
  device_bundle_queue& operator=(const device_bundle_queue&) = delete;

  // What object_base::push_parameter_value does: when the parameter's
  // device has a queue and its bundles are on, sets the value (the local
  // callbacks run) and queues it, then returns true; otherwise returns false
  // and the caller pushes the value at once, as before. The queue cannot be
  // destroyed meanwhile.
  static bool push_if_active(ossia::net::parameter_base& param, const ossia::value& value);

  // @bundle: -1 (default) bundles when the device speaks OSCQuery, 0 never,
  // 1 always.
  void set_mode(long mode) { m_mode = mode; }
  long mode() const { return m_mode; }
  bool active();

  // Any thread. Returns true when the queue was empty.
  bool push(ossia::net::parameter_base& param, ossia::value value);

  // Sends what was queued, through the device's protocol: a client's mirror
  // gets one bundle and splits it under 1400 bytes over UDP; a device
  // exposed through a multiplex gives the bundle to each OSCQuery server,
  // which splits it, and the values one by one to the other protocols; a
  // plain OSC protocol gets bundles of at most 1400 bytes; any other protocol
  // gets the values one by one. Returns the number of values sent.
  std::size_t flush();

  std::size_t size();

private:
  void on_parameter_removing(const ossia::net::parameter_base& p);

  ossia::net::device_base& m_device;
  std::function<void()> m_schedule;
  std::atomic_long m_mode{-1};
  std::mutex m_mutex;
  std::vector<ossia::bundle_element> m_queue;
};

}
}
