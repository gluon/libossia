#pragma once
#include <ossia-max/src/node_base.hpp>

namespace ossia
{
namespace max_binding
{
class device_bundle_queue;

class device_base : public node_base
{
public:
  // @bundle: what the objects of this device push during a tick of the
  // scheduler leaves at its end, in OSC bundles. -1 (default): on when the
  // device speaks OSCQuery; 0 never; 1 always. The queue exists while the
  // device does; flushed by m_bundle_clock.
  long m_bundle{-1};
  device_bundle_queue* m_bundles{};
  void* m_bundle_clock{};
  void start_bundles();
  void stop_bundles();
  static void flush_bundles(device_base* x);
  static void bundle_class_setup(t_class* c);

  void on_parameter_created_callback(const ossia::net::parameter_base& param);
  void on_parameter_deleted_callback(const ossia::net::parameter_base& param);
  static void on_attribute_modified_callback(
      ossia::net::node_base& node, const std::string& attribute);
  void on_unhandled_message_callback(const std::string, const ossia::value& val);

  void
  on_node_renamed_callback(ossia::net::node_base& node, const std::string& old_name);
  void on_node_created_callback(ossia::net::node_base& node);
  void on_node_removing_callback(ossia::net::node_base& node);

  void connect_slots();
  void disconnect_slots();

  static Nano::Signal<void(device_base*)> on_device_created;  // The device being created
  static Nano::Signal<void(device_base*)> on_device_removing; // The device being removed
};

} // namespace max
} // namespace ossia
