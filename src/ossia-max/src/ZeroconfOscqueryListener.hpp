#pragma once

#include "client_mirror.hpp"

#include <ossia/network/generic/generic_device.hpp>

#include <memory>
#include <string>
#include <vector>

namespace ossia
{
namespace max_binding
{
// The OSCQuery devices announced over mDNS, for ossia.client (connect <name>)
// and ossia.fuzzysearch. The mirrors live in an oscquery_directory, which
// browse() polls from a Max clock; this class tells the clients connected by
// name when their device is no longer announced, or when its WebSocket went
// down.
class ZeroconfOscqueryListener final
{
public:
  ZeroconfOscqueryListener();
  ~ZeroconfOscqueryListener();

  static std::shared_ptr<ossia::net::generic_device>
  find_device(const std::string& name);
  static std::vector<std::shared_ptr<ossia::net::generic_device>> get_devices();

  void browse();

private:
  // The clients connected by name to `name` let go of their device, and try
  // again in one second if their @reconnect is on. `lost`: the device is
  // still announced, its WebSocket went down; reported as a failed
  // connection is (connect 0 <name>).
  static void release_clients(const std::string& name, bool lost);

  static std::unique_ptr<oscquery_directory> s_directory;
};

} // namespace max_binding
} // namespace ossia
