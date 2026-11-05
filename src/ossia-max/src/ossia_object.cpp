#include "device_network.hpp"
#include "ossia_object.hpp"

#include "ossia-max.hpp"

using namespace ossia::max_binding;

extern "C" void ossia_ossia_setup()
{
  t_class* c = class_new(
      "ossia", (method)ossia_object::create, (method)ossia_object::destroy,
      (long)sizeof(ossia::max_binding::ossia_object), 0L, A_GIMME, 0);

  device::class_setup(c);

  CLASS_ATTR_SYM(c, "log_level", 0, ossia_object, m_log_level);
  std::stringstream lvl_list;
  for(auto lvl : SPDLOG_LEVEL_NAMES)
    lvl_list << lvl.data() << " ";
  CLASS_ATTR_ENUM(c, "log_level", 0, lvl_list.str().c_str());
  CLASS_ATTR_LABEL(c, "log_level", 0, "Log Level");
  CLASS_ATTR_DEFAULT(c, "log_level", 0, "error");

  class_register(CLASS_BOX, c);

  auto& ossia_library = ossia::max_binding::ossia_max::instance();
  ossia_library.ossia_ossia_class = c;
}

namespace ossia
{
namespace max_binding
{

void* ossia_object::create(t_symbol* name, long argc, t_atom* argv)
{
  auto& ossia_library = ossia::max_binding::ossia_max::instance();

  auto x = make_ossia<ossia_object>();

  x->m_dumpout = outlet_new(x, NULL); // anything outlet to dump device state
  x->m_device = ossia_library.get_default_device();
  x->m_otype = object_class::device;
  x->m_name = gensym(x->m_device->get_name().c_str());

  x->m_log_level = gensym("error");

  if(argc > 0 && argv[0].a_type == A_SYM)
  {
    x->m_name = argv[0].a_w.w_sym;
    x->m_device->set_name(x->m_name->s_name);
  }

  x->connect_slots();
  // @bundle, as for ossia.device: shown, and never applied before
  x->start_bundles();

  // expose builds its servers on this context, as for ossia.device; without
  // it, expose oscquery dereferenced a null context.
  x->network_context = make_device_network_context();
  x->network_poll_clock = clock_new(x, (method)device::asio_timer);
  clock_delay(x->network_poll_clock, 10);

  // inhibit loadbang for ossia object
  x->m_registered = true;

  return (x);
}

void ossia_object::destroy(ossia_object* x)
{
  ossia_max::instance().devices.remove_all(x);

  // The global device stays: the servers this object exposed are stopped and
  // their ports freed, while their network context still exists.
  stop_exposing(*x->m_device, x->m_exposed);
  x->m_exposed.clear();
  x->stop_bundles();
  if(x->m_bundle_clock)
  {
    clock_free((t_object*)x->m_bundle_clock);
    x->m_bundle_clock = nullptr;
  }

  if(x->network_poll_clock)
  {
    clock_unset(x->network_poll_clock);
    clock_free((t_object*)x->network_poll_clock);
    x->network_poll_clock = nullptr;
  }
  x->network_context.reset();

  x->disconnect_slots();

  outlet_delete(x->m_dumpout);
  x->~ossia_object();
}

t_max_err ossia_object::notify(
    ossia_object* x, t_symbol* s, t_symbol* msg, void* sender, void* data)
{
  t_symbol* attrname;

  if(!x->m_lock && msg == gensym("attr_modified"))
  {
    attrname = (t_symbol*)object_method((t_object*)data, gensym("getname"));

    if(attrname == gensym("log_level"))
    {
      ossia_max::instance().set_log_level(x->m_log_level);
    }
  }
  return 0;
}

} // namespace max
} // namespace ossia
