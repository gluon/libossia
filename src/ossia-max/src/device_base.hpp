#pragma once
#include <ossia-max/src/change_batch.hpp>
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

  // @notify: what the tree changes of the device give out of the dumpout,
  // flushed on the main thread. 0: one namespace_changed for the changes
  // since the last flush (counts, and the paths that cover them); 1
  // (default): the same, and before it each change as before (node created,
  // parameter removed...) when there were at most notify_keep of them; 2: each
  // change at once, as before, and namespace_changed.
  long m_notify{1};
  static constexpr std::size_t notify_keep = 32;
  change_batch m_changes{notify_keep};
  void* m_changes_qelem{};
  void note_change(
      change_batch::kind k, const ossia::net::node_base& node,
      const std::string& old_name = {});
  void output_change(const change_batch::change& c);
  static void flush_changes(device_base* x);
  static void notify_class_setup(t_class* c);
  void free_notify();

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
