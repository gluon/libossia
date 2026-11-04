// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "client.hpp"

#include "ZeroconfMinuitListener.hpp"
#include "ZeroconfOscqueryListener.hpp"
#include "client_mirror.hpp"
#include "device.hpp"
#include "model.hpp"
#include "ossia-max.hpp"
#include "parameter.hpp"
#include "remote.hpp"
#include "utils.hpp"
#include "view.hpp"

#include <ossia/network/minuit/minuit.hpp>
#include <ossia/network/osc/osc.hpp>
#include <ossia/network/oscquery/oscquery_mirror.hpp>
#include <ossia/network/zeroconf/zeroconf.hpp>

#include <boost/algorithm/string.hpp>

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>

using namespace ossia::max_binding;

#pragma mark -
#pragma mark ossia_client class methods

extern "C" void ossia_client_setup()
{
  auto& ossia_library = ossia_max::instance();

  // instantiate the ossia.client class
  t_class* c = class_new(
      "ossia.client", (method)client::create, (method)client::destroy,
      (short)sizeof(client), 0L, A_GIMME, 0);

  device_base::class_setup(c);
  device_base::bundle_class_setup(c);

  class_addmethod(c, (method)client::update, "update", A_NOTHING, 0);

  class_addmethod(c, (method)client::connect_mess_cb, "connect", A_GIMME, 0);

  class_addmethod(c, (method)client::disconnect, "disconnect", A_GIMME, 0);

  class_addmethod(c, (method)client::get_mess_cb, "get", A_SYM, 0);

  class_addmethod(c, (method)client::assist, "assist", A_NOTHING, 0);

  CLASS_ATTR_LONG(c, "feedback", 0, client, m_feedback);
  CLASS_ATTR_STYLE(c, "feedback", 0, "onoff");
  CLASS_ATTR_LABEL(c, "feedback", 0, "Control feebdack to host device");

  CLASS_ATTR_LONG(c, "reconnect", 0, client, m_reconnect);
  CLASS_ATTR_STYLE(c, "reconnect", 0, "onoff");
  CLASS_ATTR_LABEL(c, "reconnect", 0, "Reconnect to an OSCQuery server reached by URL or by name when it goes away");

  class_addmethod(c, (method)client::do_notify, "notify", A_CANT, 0);

  class_register(CLASS_BOX, c);
  ossia_library.ossia_client_class = c;
}

namespace ossia
{
namespace max_binding
{


t_max_err
client::do_notify(client* x, t_symbol* s, t_symbol* msg, void* a, void* data)
{
  t_symbol* attrname;

  if(!x->m_lock && msg == gensym("attr_modified"))
  {
    attrname = (t_symbol*)object_method((t_object*)data, gensym("getname"));

    if(attrname == gensym("feedback"))
      x->set_feedback(x->m_feedback);
    else if(attrname == gensym("reconnect"))
    {
      if(x->m_link)
        x->m_link->set_reconnect(x->m_reconnect > 0);
    }
    else
      return object_base::notify(x, s, msg, a, data);
  }
  return 0;
}

void client::set_feedback(bool b) {

  if(m_device)
  {
    m_device->get_protocol().set_feedback(b);
  }
}
void* client::create(t_symbol* name, long argc, t_atom* argv)
{
  auto& ossia_library = ossia_max::instance();
  auto x = make_ossia<client>();

  if(x)
  {
    critical_enter(0);
    auto& pat_desc = ossia_max::get_patcher_descriptor(x->m_patcher);
    if(!pat_desc.client && !pat_desc.device)
      pat_desc.client = x;
    else
    {
      error("You can put only one [ossia.device] or [ossia.client] per patcher");
      object_free(x);
      critical_exit(0);
      return nullptr;
    }

    // make outlets
    x->m_dumpout = outlet_new(x, NULL); // anything outlet to dump client state

    x->m_device = 0;

    x->m_otype = object_class::client;

    x->m_poll_clock = clock_new(x, (method)client::poll_message);
    x->m_clock = clock_new(x, (method)client::connect);
    x->m_attempt_clock = clock_new(x, (method)client::attempt_check);

    // The link to a server reached by URL. Everything it does runs on the
    // main thread, through the poll clock.
    x->m_link = std::make_unique<oscquery_link>(oscquery_link::host{
        [x](int ms) { clock_delay(x->m_poll_clock, ms); },
        [x] { clock_unset(x->m_poll_clock); },
        [x] {
      // The WebSocket went down: reported as a failed connection is.
      t_atom a[4];
      A_SETFLOAT(a, 0);
      A_SETSYM(a + 1, gensym("oscquery"));
      A_SETSYM(a + 2, x->m_name);
      A_SETSYM(a + 3, gensym(x->m_wsurl.c_str()));
      outlet_anything(x->m_dumpout, gensym("connect"), 4, a);
        },
        [x] {
      // The server answers again: connect as the first time did. The
      // connection ends in attempt_check; on failure, the link tries again
      // itself, connect must not.
      if(x->m_attempt)
        return false;
      client::connect(x);
      const bool ok = x->m_link->watching() && !x->m_link->lost();
      if(!ok && x->m_clock)
        clock_unset(x->m_clock);
      return ok;
        }});

    // parse arguments
    long attrstart = attr_args_offset(argc, argv);

    // check name argument
    x->m_name = _sym_max;
    if(attrstart && argv)
    {
      if(atom_gettype(argv) == A_SYM)
      {
        x->m_name = atom_getsym(argv);
        bool autoconnect = true;
        if(argc == 2 && atom_gettype(argv + 1) == A_FLOAT)
        {
          float f = atom_getfloat(argv + 1);
          autoconnect = f > 0.;
          if(autoconnect)
            connect_mess_cb(x, nullptr, 1, argv);
        }
        // [ossia.client <name>] names the client's device: nothing to connect
        // to, and a connect without argument printed the protocols and tried
        // again every second
        else if(attrstart > 1)
          connect_mess_cb(x, nullptr, attrstart - 1, argv);
      }
    }

    // process attr args, if any
    attr_args_process(x, argc - attrstart, argv + attrstart);
    x->m_link->set_reconnect(x->m_reconnect > 0);

    object_attach_byptr_register(x, x, CLASS_BOX);
    defer_low(x, (method)object_base::loadbang, nullptr, 0, nullptr);

    ossia_library.clients.push_back(x);
    critical_exit(0);
  }

  return (x);
}

void client::destroy(client* x)
{
  critical_enter(0);

  x->m_dead = true;
  x->m_node_selection.clear();
  x->m_matchers.clear();

  if(x->m_clock)
    clock_free((t_object*)x->m_clock);
  x->m_clock = nullptr;

  disconnect(x);
  // No tick after this: the link forgets its clock, then the clock goes.
  x->m_link.reset();
  if(x->m_attempt_clock)
  {
    clock_unset(x->m_attempt_clock);
    clock_free((t_object*)x->m_attempt_clock);
    x->m_attempt_clock = nullptr;
  }
  // Waits for the attempts still running: one attempt at most each.
  x->m_attempt.reset();
  x->m_abandoned.clear();
  x->stop_bundles();
  if(x->m_bundle_clock)
  {
    clock_free((t_object*)x->m_bundle_clock);
    x->m_bundle_clock = nullptr;
  }
  if(x->m_poll_clock)
  {
    clock_unset(x->m_poll_clock);
    clock_free((t_object*)x->m_poll_clock);
    x->m_poll_clock = nullptr;
  }
  if(x->m_dumpout)
    outlet_delete(x->m_dumpout);
  ossia_max::instance().clients.remove_all(x);

  x->~client();
  critical_exit(0);
}

#pragma mark -
#pragma mark t_client structure functions

void client::assist(client*, void*, long m, long a, char* s)
{
  if(m == ASSIST_INLET)
  {
    sprintf(s, "Client messages input");
  }
  else
  {
    switch(a)
    {
      case 0:
        sprintf(s, "Dumpout");
        break;
      default:;
    }
  }
}

void client::get_mess_cb(client* x, t_symbol* s)
{
  if(s == gensym("devices"))
    client::get_devices(x);
  else
    device_base::get_mess_cb(x, s);
}

void client::connect_mess_cb(client* x, t_symbol*, int argc, t_atom* argv)
{
  if(x->m_argv)
    delete x->m_argv;

  x->m_argc = argc;
  x->m_argv = new t_atom[argc];

  memcpy(x->m_argv, argv, argc * sizeof(t_atom));

  client::connect(x);
}

void client::connect(client* x)
{
  // The poll and the end of a connection run on the scheduler thread: they
  // never touch the device while connect replaces it.
  critical_enter(0);
  connect_impl(x);
  critical_exit(0);
}

void client::connect_impl(client* x)
{
  int argc = x->m_argc;
  t_atom* argv = x->m_argv;

  disconnect(x);

  ossia::net::minuit_connection_data minuit_settings;
  minuit_settings.name = x->m_name->s_name;
  minuit_settings.host = "127.0.0.1";
  minuit_settings.remote_port = 6666;
  minuit_settings.local_port = 9999;

  ossia::net::oscquery_connection_data oscq_settings;
  oscq_settings.name = x->m_name->s_name;
  oscq_settings.host = "127.0.0.1";
  oscq_settings.port = 5678;

  ossia::net::osc_connection_data osc_settings;
  osc_settings.name = x->m_name->s_name;
  osc_settings.host = "127.0.0.1";
  osc_settings.remote_port = 6666;
  osc_settings.local_port = 9999;

  t_atom connection_status[6];
  int count = 0;

  if(argc && argv->a_type == A_SYM)
  {
    std::string protocol_name = argv->a_w.w_sym->s_name;
    std::string name = protocol_name;
    boost::algorithm::to_lower(protocol_name);

    x->m_zeroconf = false;

    if(protocol_name == "minuit")
    {
      argc--;
      argv++;
      if(argc == 4 && argv[0].a_type == A_SYM && argv[1].a_type == A_SYM
         && (argv[2].a_type == A_LONG) && (argv[3].a_type == A_LONG))
      {
        minuit_settings.name = atom_getsym(argv++)->s_name;
        minuit_settings.host = atom_getsym(argv++)->s_name;
        minuit_settings.remote_port = atom_getlong(argv++);
        minuit_settings.local_port = atom_getlong(argv++);
      }

      A_SETSYM(connection_status + 1, gensym("minuit"));
      A_SETSYM(connection_status + 2, gensym(minuit_settings.name.c_str()));
      A_SETSYM(connection_status + 3, gensym(minuit_settings.host.c_str()));
      A_SETFLOAT(connection_status + 4, minuit_settings.remote_port);
      A_SETFLOAT(connection_status + 5, minuit_settings.local_port);

      try
      {
        x->m_device = std::make_shared<ossia::net::generic_device>(
            std::make_unique<ossia::net::minuit_protocol>(
                minuit_settings.name, minuit_settings.host, minuit_settings.remote_port,
                minuit_settings.local_port),
            x->m_name->s_name);
      }
      catch(const std::exception& e)
      {
        object_error((t_object*)x, "%s", e.what());
      }

      count = 6;
    }
    else if(protocol_name == "oscquery")
    {
      argc--;
      argv++;
      std::string wsurl
          = "ws://" + oscq_settings.host + ":" + std::to_string(oscq_settings.port);
      if(argc == 1 && argv[0].a_type == A_SYM)
      {
        wsurl = atom_getsym(argv)->s_name;
      }

      A_SETSYM(connection_status + 1, gensym("oscquery"));
      A_SETSYM(connection_status + 2, gensym(oscq_settings.name.c_str()));
      A_SETSYM(connection_status + 3, gensym(wsurl.c_str()));
      x->m_wsurl = wsurl;

      // Building the mirror on a server that is down blocks for 1.5 s: the
      // port is probed first. While the link is trying again after a loss,
      // a server still down is not an error.
      if(!oscquery_server_reachable(wsurl, 100))
      {
        if(!x->m_link->lost())
          object_error((t_object*)x, "Could not connect to %s", wsurl.c_str());
      }
      else
      {
        // The mirror waits for the WebSocket, then for the tree: on the main
        // thread these waits would freeze Max, and an ossia.device of the
        // same Max, whose server this thread polls, could not answer. The
        // attempt runs on its own thread; attempt_check ends the connection.
        x->m_attempt = std::make_unique<oscquery_connect_attempt>(wsurl, oscq_settings.name);
        x->m_attempt_status.assign(connection_status, connection_status + 4);
        clock_delay(x->m_attempt_clock, 10);
        return;
      }

      count = 4;
    }
    else if(protocol_name == "osc")
    {
      argc--;
      argv++;
      if(argc == 4 && argv[0].a_type == A_SYM && argv[1].a_type == A_SYM
         && (argv[2].a_type == A_FLOAT || argv[2].a_type == A_LONG)
         && (argv[3].a_type == A_FLOAT || argv[3].a_type == A_LONG))
      {
        osc_settings.name = atom_getsym(argv++)->s_name;
        osc_settings.host = atom_getsym(argv++)->s_name;
        osc_settings.remote_port = atom_getfloat(argv++);
        osc_settings.local_port = atom_getfloat(argv++);
      }

      A_SETSYM(connection_status + 1, gensym("osc"));
      A_SETSYM(connection_status + 2, gensym(osc_settings.name.c_str()));
      A_SETSYM(connection_status + 3, gensym(osc_settings.host.c_str()));
      A_SETFLOAT(connection_status + 4, osc_settings.remote_port);
      A_SETFLOAT(connection_status + 5, osc_settings.local_port);

      try
      {
        x->m_device = std::make_shared<ossia::net::generic_device>(
            std::make_unique<ossia::net::osc_protocol>(
                osc_settings.host, osc_settings.remote_port, osc_settings.local_port,
                osc_settings.name),
            x->m_name->s_name);
      }
      catch(const std::exception& e)
      {
        object_error((t_object*)x, "%s", e.what());
      }
      count = 6;
    }
    else
    {
      // Connect by device name : retrieve connection info
      x->m_device = ZeroconfOscqueryListener::find_device(name);
      if(!x->m_device)
      {
        x->m_device = ZeroconfMinuitListener::find_device(name);
      }
      else
      {
        x->m_zeroconf = true;
        // Poll the mirror as for a connection by URL, or the tree changes the
        // server sends are never applied.
        x->m_oscq_protocol = polled_oscquery_mirror(*x->m_device);
        if(x->m_oscq_protocol)
          clock_set(x->m_poll_clock, 1);
      }

      A_SETSYM(connection_status + 1, gensym(name.c_str()));
      count = 2;
    }
  }
  else
  {
    // Nothing to connect to: nothing to try again
    client::print_protocol_help();
    return;
  }

  if(x->m_device)
  {
    // By name, the listener's mirror holds the tree already: read again, it
    // removed and created every node, and the values in between came out
    // as osc messages
    finish_connect(x, count, connection_status, !x->m_zeroconf);
  }
  else
  {
    A_SETFLOAT(connection_status, 0);

    outlet_anything(x->m_dumpout, gensym("connect"), count, connection_status);

    // @reconnect 0: a first connection that fails is not tried again either
    if(x->m_reconnect > 0)
      clock_delay(x->m_clock, 1000); // hardcoded reconnection delay
  }
}


void client::finish_connect(client* x, int count, t_atom* status, bool read_tree)
{
  A_SETFLOAT(status, 1);

  outlet_anything(x->m_dumpout, gensym("connect"), count, status);

  x->connect_slots();
  x->start_bundles();
  if(read_tree)
    client::update(x);
  client::on_device_created(x);
  clock_unset(x->m_clock);

  register_children_in_patcher_recursively(x->m_patcher, x);
  output_all_values(x->m_patcher, true);
}

// Main thread: is the connection by URL done?
void client::attempt_check(client* x)
{
  critical_enter(0);
  attempt_check_impl(x);
  critical_exit(0);
}

void client::attempt_check_impl(client* x)
{
  std::erase_if(x->m_abandoned, [](const auto& a) { return a->done(); });
  if(!x->m_attempt || !x->m_attempt->done())
  {
    if(x->m_attempt || !x->m_abandoned.empty())
      clock_delay(x->m_attempt_clock, 10);
    return;
  }

  auto attempt = std::move(x->m_attempt);
  auto status = std::move(x->m_attempt_status);
  const bool from_link = x->m_link->lost();
  if(auto dev = attempt->take())
  {
    x->m_device = std::move(dev);
    x->m_oscq_protocol = polled_oscquery_mirror(*x->m_device);
    // Polls the mirror from now on, and reconnects when the server goes away.
    x->m_link->watch(*x->m_device, x->m_wsurl, static_cast<int>(x->m_rate));
    // The tree was read by the attempt.
    finish_connect(x, static_cast<int>(status.size()), status.data(), false);
  }
  else if(!from_link)
  {
    object_error((t_object*)x, "%s", attempt->error().c_str());
    A_SETFLOAT(status.data(), 0);
    outlet_anything(x->m_dumpout, gensym("connect"), static_cast<short>(status.size()), status.data());
    if(x->m_reconnect > 0)
      clock_delay(x->m_clock, 1000); // hardcoded reconnection delay
  }
  // After a loss, the link tries again by itself.
  if(!x->m_abandoned.empty())
    clock_delay(x->m_attempt_clock, 10);
}

void client::get_devices(client* x)
{
  auto minuit_devices = ZeroconfMinuitListener::get_devices();
  auto oscq_devices = ZeroconfOscqueryListener::get_devices();

  t_atom a;
  A_SETLONG(&a, minuit_devices.size() + oscq_devices.size());
  outlet_anything(x->m_dumpout, gensym("devices"), 1, &a);

  std::array<t_atom, 2> av;
  for(const auto& dev : minuit_devices)
  {
    A_SETSYM(av.data(), gensym("minuit"));
    A_SETSYM(av.data() + 1, gensym(dev->get_name().c_str()));
    outlet_anything(x->m_dumpout, gensym("device"), 2, av.data());
  }

  for(const auto& dev : oscq_devices)
  {
    A_SETSYM(av.data(), gensym("oscquery"));
    A_SETSYM(av.data() + 1, gensym(dev->get_name().c_str()));
    outlet_anything(x->m_dumpout, gensym("device"), 2, av.data());
  }
}

void client::unregister_children()
{
  std::vector<object_base*> children_view
      = find_children_to_register(&m_object, m_patcher, gensym("ossia.view"));

  for(auto child : children_view)
  {
    if(child->m_otype == object_class::view)
    {
      ossia::max_binding::view* view = (ossia::max_binding::view*)child;
      view->unregister();
    }
    else if(child->m_otype == object_class::remote)
    {
      ossia::max_binding::remote* remote = (ossia::max_binding::remote*)child;
      remote->unregister();
    }
  }
}

void client::update(client* x)
{

  if(x->m_device)
  {
    x->m_device->get_protocol().update(*x->m_device);
    output_all_values(x->m_patcher, true);
  }
}

void client::disconnect(client* x)
{
  critical_enter(0);
  disconnect_impl(x);
  critical_exit(0);
}

void client::disconnect_impl(client* x)
{
  // An explicit disconnect, or a new connection: no more attempts. Ignored
  // by the link while it is the one connecting.
  if(x->m_link)
    x->m_link->unwatch();
  // A connection by URL in progress is left to end on its own thread.
  if(x->m_attempt)
  {
    x->m_abandoned.push_back(std::move(x->m_attempt));
    x->m_attempt_status.clear();
    if(x->m_attempt_clock)
      clock_delay(x->m_attempt_clock, 10);
  }
  if(x->m_device)
  {
    x->m_node_selection.clear();
    x->m_matchers.clear();
    x->disconnect_slots();
    // FIXME it should not be necessary to unregister_children because they should listen
    // to on_node_removing signal
    x->unregister_children();
    x->stop_bundles();
    x->m_device = nullptr;
    x->m_oscq_protocol = nullptr;
    client::on_device_removing(x);

    outlet_anything(x->m_dumpout, gensym("disconnected"), 0, nullptr);
  }
  if(x->m_clock)
    clock_unset(x->m_clock); // avoid automatic reconnection
}

void client::poll_message(client* x)
{
  critical_enter(0);
  poll_message_impl(x);
  critical_exit(0);
}

void client::poll_message_impl(client* x)
{
  // Connection by URL: the link polls, and reconnects when the server goes away.
  if(x->m_link && x->m_link->watching())
  {
    x->m_link->tick();
    return;
  }
  // Connection by name: the device comes from the zeroconf listener, which
  // connects again when the announcement comes back.
  if(x->m_oscq_protocol)
  {
    x->m_oscq_protocol->run_commands();
    clock_delay(x->m_poll_clock, x->m_rate);
  }
}

} // max namespace
} // ossia namespace
