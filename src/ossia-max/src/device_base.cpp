#include "device_bundles.hpp"
#include <ossia/network/base/node_functions.hpp>
#include <ossia/network/base/parameter.hpp>

#include <ossia-max/src/device_base.hpp>
#include <ossia-max/src/ossia-max.hpp>

namespace ossia
{
namespace max_binding
{
Nano::Signal<void(device_base*)> device_base::on_device_created{};

namespace
{
t_max_err set_bundle_attr(device_base* x, void*, long argc, t_atom* argv)
{
  if(argc && argv)
    x->m_bundle = atom_getlong(argv);
  if(x->m_bundles)
    x->m_bundles->set_mode(x->m_bundle);
  return MAX_ERR_NONE;
}
}

void device_base::bundle_class_setup(t_class* c)
{
  CLASS_ATTR_LONG(c, "bundle", 0, device_base, m_bundle);
  CLASS_ATTR_ACCESSORS(c, "bundle", nullptr, set_bundle_attr);
  CLASS_ATTR_LABEL(
      c, "bundle", 0,
      "Send the values of a tick in OSC bundles (-1: when the device speaks OSCQuery)");
}

void device_base::start_bundles()
{
  stop_bundles();
  if(!m_device)
    return;
  if(!m_bundle_clock)
    m_bundle_clock = clock_new(this, (method)device_base::flush_bundles);
  auto clock = m_bundle_clock;
  m_bundles = new device_bundle_queue{*m_device, [clock] { clock_delay(clock, 0); }};
  m_bundles->set_mode(m_bundle);
}

void device_base::stop_bundles()
{
  if(m_bundle_clock)
    clock_unset(m_bundle_clock);
  critical_enter(0);
  delete m_bundles;
  m_bundles = nullptr;
  critical_exit(0);
}

// The end of the tick: what the objects pushed since the last flush leaves.
void device_base::flush_bundles(device_base* x)
{
  critical_enter(0);
  if(x->m_bundles)
    x->m_bundles->flush();
  critical_exit(0);
}
Nano::Signal<void(device_base*)> device_base::on_device_removing{};

void device_base::notify_class_setup(t_class* c)
{
  CLASS_ATTR_LONG(c, "notify", 0, device_base, m_notify);
  CLASS_ATTR_FILTER_CLIP(c, "notify", 0, 2);
  CLASS_ATTR_LABEL(
      c, "notify",
      0, "Tree changes out of the dumpout: 0 namespace_changed only, 1 also each change when few, 2 each change at once");
}

// From the thread that changes the tree: the main thread for a polled client,
// another one for some protocols. No symbol is made here unless @notify 2.
void device_base::note_change(
    change_batch::kind k, const ossia::net::node_base& node, const std::string& old_name)
{
  const std::string& path = node.osc_address();
  std::string old_path;
  if(k == change_batch::kind::node_renamed)
    old_path = path.substr(0, path.size() - node.get_name().size()) + old_name;
  const std::string& device = node.get_device().get_name();
  if(m_notify >= 2)
  {
    change_batch::change c;
    c.what = k;
    c.address = device + ":" + path;
    if(k == change_batch::kind::node_renamed)
      c.old_address = device + ":" + old_path;
    output_change(c);
  }
  if(m_changes.add(k, device, path, old_path) && m_changes_qelem)
    qelem_set(m_changes_qelem);
}

// One change, as ossia-max always gave it
void device_base::output_change(const change_batch::change& c)
{
  if(!m_dumpout)
    return;
  t_atom a[3];
  using k = change_batch::kind;
  switch(c.what)
  {
    case k::node_created:
    case k::node_removed:
      A_SETSYM(a, gensym(c.what == k::node_created ? "created" : "removed"));
      A_SETSYM(a + 1, gensym(c.address.c_str()));
      outlet_anything(m_dumpout, gensym("node"), 2, a);
      break;
    case k::parameter_created:
    case k::parameter_removed:
      A_SETSYM(a, gensym(c.what == k::parameter_created ? "created" : "removed"));
      A_SETSYM(a + 1, gensym(c.address.c_str()));
      outlet_anything(m_dumpout, gensym("parameter"), 2, a);
      break;
    case k::node_renamed:
      A_SETSYM(a, gensym("renamed"));
      A_SETSYM(a + 1, gensym(c.old_address.c_str()));
      A_SETSYM(a + 2, gensym(c.address.c_str()));
      outlet_anything(m_dumpout, gensym("node"), 3, a);
      break;
  }
}

// On the main thread: what changed since the last flush
void device_base::flush_changes(device_base* x)
{
  auto r = x->m_changes.take();
  if(r.empty() || !x->m_dumpout)
    return;
  if(x->m_notify == 1 && !r.overflow)
    for(const auto& c : r.changes)
      x->output_change(c);
  std::vector<t_atom> va(4 + r.paths.size());
  A_SETLONG(&va[0], r.nodes_created);
  A_SETLONG(&va[1], r.nodes_removed);
  A_SETLONG(&va[2], r.parameters_created);
  A_SETLONG(&va[3], r.parameters_removed);
  for(std::size_t i = 0; i < r.paths.size(); i++)
    A_SETSYM(&va[4 + i], gensym(r.paths[i].c_str()));
  outlet_anything(x->m_dumpout, gensym("namespace_changed"), short(va.size()), va.data());
}

void device_base::free_notify()
{
  if(m_changes_qelem)
  {
    qelem_unset(m_changes_qelem);
    qelem_free(m_changes_qelem);
    m_changes_qelem = nullptr;
  }
  m_changes.take();
}

void device_base::on_parameter_created_callback(const ossia::net::parameter_base& param)
{
  note_change(change_batch::kind::parameter_created, param.get_node());
}

void device_base::on_unhandled_message_callback(
    const std::string addr, const ossia::value& val)
{
  std::vector<t_atom> va;
  t_atom a;
  A_SETSYM(&a, gensym(addr.c_str()));
  va.push_back(std::move(a));
  value2atom vm{va};
  val.apply(vm);

  outlet_anything(m_dumpout, gensym("osc"), va.size(), va.data());
}

void device_base::on_parameter_deleted_callback(const ossia::net::parameter_base& param)
{
  note_change(change_batch::kind::parameter_removed, param.get_node());
}

void device_base::on_attribute_modified_callback(
    ossia::net::node_base& node, const std::string& attribute)
{
  auto& omax = ossia_max::instance();
  std::lock_guard _{omax.s_node_matchers_mut};

  auto candidates_it = omax.s_node_matchers_map.find(&node);

  // If it's an intermediary node it does not match any proper ossia object
  if(candidates_it == omax.s_node_matchers_map.end())
    return;

  auto& matchers = candidates_it->second;
  for(const auto& m : matchers)
  {
    auto obj = m->get_owner();
    if(obj)
    {
      switch(obj->m_otype)
      {
        case object_class::remote: {
          auto rmt = static_cast<ossia::max_binding::remote*>(obj);
          // only break for 'unit' attribute
          if(attribute == ossia::net::text_unit()
             || attribute == ossia::net::text_extended_type())
          {
            rmt->set_unit();
            break;
          }
        }
        case object_class::attribute:
        case object_class::param: {
          auto oc = static_cast<ossia::max_binding::parameter_base*>(obj);
          oc->update_attribute(oc, attribute, &node);
          break;
        }
        case object_class::model:
        case object_class::view:
        case object_class::device:
        case object_class::client: {
          auto oc = static_cast<ossia::max_binding::node_base*>(obj);
          oc->update_attribute(oc, attribute, &node);
          break;
        }
        break;
        default:;
      }
    }
  }
}

void device_base::on_node_renamed_callback(
    ossia::net::node_base& node, const std::string& old_name)
{
  note_change(change_batch::kind::node_renamed, node, old_name);
}

void device_base::on_node_created_callback(ossia::net::node_base& node)
{
  note_change(change_batch::kind::node_created, node);
}

void device_base::on_node_removing_callback(ossia::net::node_base& node)
{
  note_change(change_batch::kind::node_removed, node);
}

void device_base::connect_slots()
{
  if(m_device)
  {
    if(!m_changes_qelem)
      m_changes_qelem = qelem_new(this, (method)device_base::flush_changes);
    m_device->on_parameter_created.connect<&device_base::on_parameter_created_callback>(
        this);
    m_device->on_parameter_removing.connect<&device_base::on_parameter_deleted_callback>(
        this);
    m_device->on_unhandled_message.connect<&device_base::on_unhandled_message_callback>(
        this);
    m_device->on_attribute_modified
        .connect<&device_base::on_attribute_modified_callback>();
    // TODO add callback for node creation request,
    // node deletion request and node_rename request

    m_device->on_node_renamed.connect<&device_base::on_node_renamed_callback>(this);
    m_device->on_node_created.connect<&device_base::on_node_created_callback>(this);
    m_device->on_node_removing.connect<&device_base::on_node_removing_callback>(this);

    m_matchers.emplace_back(
        std::make_shared<matcher>(&m_device->get_root_node(), nullptr));
    int size = m_matchers.size();
    m_matchers.back()->m_index = size;
    // This is to handle [get address( message only
    // so is it really needed ?
    assert(!m_matchers.empty());
    m_node_selection.push_back(m_matchers[0].get());
  }
}

void device_base::disconnect_slots()
{
  if(m_device)
  {
    m_device->on_parameter_created
        .disconnect<&device_base::on_parameter_created_callback>(this);
    m_device->on_parameter_removing
        .disconnect<&device_base::on_parameter_deleted_callback>(this);
    m_device->on_unhandled_message
        .disconnect<&device_base::on_unhandled_message_callback>(this);

    m_device->on_attribute_modified
        .disconnect<&device_base::on_attribute_modified_callback>();

    m_device->on_node_renamed.disconnect<&device_base::on_node_renamed_callback>(this);
    m_device->on_node_created.disconnect<&device_base::on_node_created_callback>(this);
    m_device->on_node_removing.disconnect<&device_base::on_node_removing_callback>(this);
    // TODO add callback for command request
  }
}

} // namespace max
} // namespace ossia
