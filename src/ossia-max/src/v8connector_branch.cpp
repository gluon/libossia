#include "v8connector_branch.hpp"

#include <ossia/network/base/address_scope.hpp>
#include <ossia/network/base/node_attributes.hpp>
#include <ossia/network/base/node_functions.hpp>
#include <ossia/network/common/path.hpp>
#include <ossia/network/common/complex_type.hpp>
#include <ossia/network/common/value_bounding.hpp>
#include <ossia/network/dataspace/dataspace_visitors.hpp>
#include <ossia/network/domain/domain.hpp>
#include <ossia/network/value/impulse.hpp>
#include <ossia/network/value/vec.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

namespace ossia
{
namespace max_binding
{
namespace v8connector_detail
{

namespace
{

// Set on the pushing thread while a branch pushes a value itself, with the
// value the host asked for. libossia calls the value callbacks synchronously on
// that thread, so the branch can tell its own pushes apart from every other
// source without a race, and let through a value the parameter changed.
thread_local const branch* t_pushing = nullptr;
thread_local const scalars* t_pushing_request = nullptr;

struct pushing_scope
{
  pushing_scope(const branch* b, const scalars* request)
  {
    t_pushing = b;
    t_pushing_request = request;
  }
  ~pushing_scope()
  {
    t_pushing = nullptr;
    t_pushing_request = nullptr;
  }
};

bool is_number(const scalar& s)
{
  return !std::holds_alternative<std::string>(s);
}

float to_float(const scalar& s)
{
  switch(s.index())
  {
    case 0:
      return static_cast<float>(std::get<int>(s));
    case 1:
      return std::get<float>(s);
    default:
      return 0.f;
  }
}

std::string to_string(const scalar& s)
{
  switch(s.index())
  {
    case 0:
      return std::to_string(std::get<int>(s));
    case 1: {
      std::ostringstream o;
      o << std::get<float>(s);
      return o.str();
    }
    default:
      return std::get<std::string>(s);
  }
}

ossia::value to_value(const scalar& s)
{
  switch(s.index())
  {
    case 0:
      return std::get<int>(s);
    case 1:
      return std::get<float>(s);
    default:
      return std::get<std::string>(s);
  }
}

struct flatten
{
  scalars& out;
  void operator()(float f) const { out.push_back(f); }
  void operator()(int i) const { out.push_back(i); }
  void operator()(bool b) const { out.push_back(b ? 1 : 0); }
  void operator()(ossia::impulse) const { }
  void operator()(const std::string& s) const { out.push_back(s); }
  void operator()(const std::vector<ossia::value>& l) const
  {
    for(const auto& v : l)
      v.apply(*this);
  }
  void operator()(const ossia::value_map_type&) const { }
  template <std::size_t N>
  void operator()(const std::array<float, N>& a) const
  {
    for(float f : a)
      out.push_back(f);
  }
  void operator()() const { }
};

// True when a value holds exactly what the host sent: same count, numbers equal
// as numbers, symbols equal as text. An impulse carries nothing to compare.
bool scalars_match(const scalars& request, const ossia::value& v)
{
  if(v.get_type() == ossia::val_type::IMPULSE)
    return true;
  scalars got;
  if(v.valid())
    v.apply(flatten{got});
  if(got.size() != request.size())
    return false;
  for(std::size_t i = 0; i < got.size(); ++i)
  {
    const bool num_r = is_number(request[i]);
    const bool num_g = is_number(got[i]);
    if(num_r != num_g)
      return false;
    if(num_r)
    {
      if(to_float(request[i]) != to_float(got[i]))
        return false;
    }
    else if(std::get<std::string>(request[i]) != std::get<std::string>(got[i]))
      return false;
  }
  return true;
}

// Number of numeric components of a type, 0 for non numeric types.
int numeric_components(std::string_view type)
{
  if(type == "float" || type == "int")
    return 1;
  if(type == "vec2f")
    return 2;
  if(type == "vec3f")
    return 3;
  if(type == "vec4f")
    return 4;
  return 0;
}

bool known_type(std::string_view type)
{
  static constexpr std::string_view types[]
      = {"float", "int",   "bool",  "impulse", "string",
         "list",  "vec2f", "vec3f", "vec4f"};
  return std::find(std::begin(types), std::end(types), type) != std::end(types);
}

std::string one_symbol(const attribute& a, std::string& out)
{
  if(a.values.size() != 1 || !std::holds_alternative<std::string>(a.values[0]))
    return "@" + a.name + " expects one symbol";
  out = std::get<std::string>(a.values[0]);
  return {};
}

std::string numbers(const attribute& a, std::vector<float>& out)
{
  if(a.values.empty())
    return "@" + a.name + " expects at least one number";
  out.clear();
  for(const auto& v : a.values)
  {
    if(!is_number(v))
      return "@" + a.name + " expects numbers";
    out.push_back(to_float(v));
  }
  return {};
}

std::string one_number(const attribute& a, float& out)
{
  if(a.values.size() != 1 || !is_number(a.values[0]))
    return "@" + a.name + " expects one number";
  out = to_float(a.values[0]);
  return {};
}

std::string flag(const attribute& a, bool& out)
{
  if(a.values.size() != 1 || !is_number(a.values[0]))
    return "@" + a.name + " expects 0 or 1";
  out = to_float(a.values[0]) != 0.f;
  return {};
}

ossia::bounding_mode to_bounding(std::string_view clip)
{
  if(clip == "both")
    return ossia::bounding_mode::CLIP;
  if(clip == "low")
    return ossia::bounding_mode::CLAMP_LOW;
  if(clip == "high")
    return ossia::bounding_mode::CLAMP_HIGH;
  if(clip == "wrap")
    return ossia::bounding_mode::WRAP;
  if(clip == "fold")
    return ossia::bounding_mode::FOLD;
  return ossia::bounding_mode::FREE;
}

ossia::access_mode to_access(std::string_view mode)
{
  if(mode == "get" || mode == "r")
    return ossia::access_mode::GET;
  if(mode == "set" || mode == "w")
    return ossia::access_mode::SET;
  return ossia::access_mode::BI;
}

template <std::size_t N>
std::string to_vec(const scalars& in, ossia::value& out)
{
  if(in.size() != N)
    return "expects " + std::to_string(N) + " numbers, got " + std::to_string(in.size());
  std::array<float, N> a{};
  for(std::size_t i = 0; i < N; ++i)
  {
    if(!is_number(in[i]))
      return "expects numbers";
    a[i] = to_float(in[i]);
  }
  out = a;
  return {};
}

}

std::string normalize_relative_address(std::string_view address)
{
  while(!address.empty() && std::isspace(static_cast<unsigned char>(address.front())))
    address.remove_prefix(1);
  while(!address.empty() && std::isspace(static_cast<unsigned char>(address.back())))
    address.remove_suffix(1);
  while(!address.empty() && address.front() == '/')
    address.remove_prefix(1);
  while(!address.empty() && address.back() == '/')
    address.remove_suffix(1);
  if(address.empty())
    return {};
  if(address.find("//") != std::string_view::npos)
    return {};
  if(address.find(':') != std::string_view::npos)
    return {};
  for(unsigned char c : address)
    if(std::isspace(c))
      return {};
  return std::string(address);
}

std::string make_leaf_spec(
    std::string_view address, const std::vector<attribute>& attrs, leaf_spec& out)
{
  leaf_spec spec;
  spec.address = normalize_relative_address(address);
  if(spec.address.empty())
    return "invalid address '" + std::string(address) + "'";

  const std::string where = spec.address + ": ";
  std::vector<scalar> range;
  bool has_type = false;

  for(const auto& a : attrs)
  {
    std::string err;
    if(a.name == "type")
    {
      err = one_symbol(a, spec.type);
      if(err.empty())
      {
        if(spec.type == "symbol")
          spec.type = "string";
        if(!known_type(spec.type))
          err = "unknown type '" + spec.type + "'";
        has_type = true;
      }
    }
    else if(a.name == "min")
      err = numbers(a, spec.min);
    else if(a.name == "max")
      err = numbers(a, spec.max);
    else if(a.name == "range")
    {
      if(a.values.empty())
        err = "@range expects at least one value";
      range = a.values;
    }
    else if(a.name == "unit")
      err = one_symbol(a, spec.unit);
    else if(a.name == "default")
    {
      if(a.values.empty())
        err = "@default expects a value";
      spec.default_value = a.values;
    }
    else if(a.name == "clip")
    {
      err = one_symbol(a, spec.clip);
      if(err.empty())
      {
        static constexpr std::string_view clips[]
            = {"free", "both", "low", "high", "wrap", "fold"};
        if(std::find(std::begin(clips), std::end(clips), spec.clip) == std::end(clips))
          err = "unknown clip mode '" + spec.clip + "'";
      }
    }
    else if(a.name == "mode")
    {
      err = one_symbol(a, spec.mode);
      if(err.empty())
      {
        static constexpr std::string_view modes[] = {"bi", "get", "set", "rw", "r", "w"};
        if(std::find(std::begin(modes), std::end(modes), spec.mode) == std::end(modes))
          err = "unknown mode '" + spec.mode + "'";
      }
    }
    else if(a.name == "description")
      err = one_symbol(a, spec.description);
    else if(a.name == "tags")
    {
      spec.tags.clear();
      for(const auto& v : a.values)
        spec.tags.push_back(to_string(v));
    }
    else if(a.name == "priority")
    {
      float f{};
      err = one_number(a, f);
      spec.priority = f;
    }
    else if(a.name == "rate")
    {
      float f{};
      err = one_number(a, f);
      spec.rate = f;
    }
    else if(a.name == "critical")
      err = flag(a, spec.critical);
    else if(a.name == "invisible")
      err = flag(a, spec.invisible);
    else if(a.name == "repetitions")
      err = flag(a, spec.repetitions);
    else if(a.name == "enable")
      err = flag(a, spec.enable);
    else if(a.name == "recall_safe")
      err = flag(a, spec.recall_safe);
    else
      return where + "unknown attribute '@" + a.name + "'";

    if(!err.empty())
      return where + err;
  }

  if(!has_type)
    return where + "missing @type";

  const int n = numeric_components(spec.type);
  if(n > 0)
  {
    if(!range.empty())
    {
      if(!spec.min.empty() || !spec.max.empty())
        return where + "@range and @min/@max are exclusive";
      std::vector<float> r;
      for(const auto& v : range)
      {
        if(!is_number(v))
          return where + "@range expects numbers for a numeric type";
        r.push_back(to_float(v));
      }
      if(r.size() == 2)
      {
        spec.min = {r[0]};
        spec.max = {r[1]};
      }
      else if(static_cast<int>(r.size()) == 2 * n)
      {
        for(int i = 0; i < n; ++i)
        {
          spec.min.push_back(r[2 * i]);
          spec.max.push_back(r[2 * i + 1]);
        }
      }
      else
        return where + "@range expects 2 or " + std::to_string(2 * n) + " numbers";
    }
    if(spec.min.empty() || spec.max.empty())
      return where + "numeric type '" + spec.type
             + "' requires both @min and @max (or @range)";
    if(spec.min.size() == 1 && n > 1)
    {
      const float m = spec.min[0];
      spec.min.assign(n, m);
    }
    if(spec.max.size() == 1 && n > 1)
    {
      const float m = spec.max[0];
      spec.max.assign(n, m);
    }
    if(static_cast<int>(spec.min.size()) != n || static_cast<int>(spec.max.size()) != n)
      return where + "@min and @max expect 1 or " + std::to_string(n) + " numbers";
    for(int i = 0; i < n; ++i)
      if(spec.min[i] > spec.max[i])
        return where + "@min is greater than @max";
  }
  else if(spec.type == "string" && !range.empty())
  {
    for(const auto& v : range)
      spec.enumeration.push_back(to_string(v));
  }
  else if(!spec.min.empty() || !spec.max.empty() || !range.empty())
  {
    return where + "@min, @max and @range do not apply to type '" + spec.type + "'";
  }

  if(!spec.unit.empty())
  {
    if(!ossia::parse_pretty_unit(spec.unit))
      return where + "unknown unit '" + spec.unit + "'";
  }

  out = std::move(spec);
  return {};
}

std::string scalars_to_value(
    const ossia::net::parameter_base& param, const scalars& in, ossia::value& out)
{
  switch(param.get_value_type())
  {
    case ossia::val_type::IMPULSE:
      out = ossia::impulse{};
      return {};

    case ossia::val_type::FLOAT:
      if(in.size() != 1 || !is_number(in[0]))
        return "expects one number";
      out = to_float(in[0]);
      return {};

    case ossia::val_type::INT:
      if(in.size() != 1 || !is_number(in[0]))
        return "expects one number";
      out = static_cast<int>(std::lround(to_float(in[0])));
      return {};

    case ossia::val_type::BOOL:
      if(in.size() != 1 || !is_number(in[0]))
        return "expects 0 or 1";
      out = to_float(in[0]) != 0.f;
      return {};

    case ossia::val_type::STRING:
      if(in.size() != 1)
        return "expects one symbol";
      out = to_string(in[0]);
      return {};

    case ossia::val_type::LIST: {
      std::vector<ossia::value> v;
      v.reserve(in.size());
      for(const auto& s : in)
        v.push_back(to_value(s));
      out = std::move(v);
      return {};
    }

    case ossia::val_type::VEC2F:
      return to_vec<2>(in, out);
    case ossia::val_type::VEC3F:
      return to_vec<3>(in, out);
    case ossia::val_type::VEC4F:
      return to_vec<4>(in, out);

    default:
      return "unsupported parameter type";
  }
}

branch::branch() = default;

branch::~branch()
{
  detach();
  for(auto& b : m_bindings)
    release(b);
  for(auto* dev : m_devices)
    dev->on_parameter_created.disconnect<&branch::on_parameter_created>(this);
}

void branch::set_node_factory(std::function<ossia::net::node_base*()> factory)
{
  m_node_factory = std::move(factory);
}

void branch::reattach()
{
  ensure_node();
}

// A node is only asked for when there is a leaf to put under it: a branch that
// only binds creates nothing, like an ossia.remote.
void branch::ensure_node()
{
  if(m_node || m_leaves.empty() || !m_node_factory)
    return;
  if(auto* node = m_node_factory())
    attach(*node);
}

void branch::attach(ossia::net::node_base& node)
{
  detach();
  m_node = &node;
  m_node->about_to_be_deleted.connect<&branch::on_node_removing>(this);
  for(auto& l : m_leaves)
    create(l);
}

void branch::detach()
{
  if(!m_node)
    return;
  m_node->about_to_be_deleted.disconnect<&branch::on_node_removing>(this);
  for(auto& l : m_leaves)
  {
    if(l.node)
      l.node->about_to_be_deleted.disconnect<&branch::on_node_removing>(this);
    l.node = nullptr;
    l.param = nullptr;
  }
  m_node = nullptr;
}

branch::leaf* branch::find(std::string_view address)
{
  for(auto& l : m_leaves)
    if(l.spec.address == address)
      return &l;
  return nullptr;
}

std::string branch::add(const leaf_spec& spec)
{
  if(find(spec.address))
    return spec.address + ": already declared";
  if(find_binding(spec.address))
    return spec.address + ": already used by bind";
  if(m_node)
  {
    if(auto* existing = ossia::net::find_node(*m_node, spec.address))
      if(existing->get_parameter())
        return spec.address + ": a parameter already exists at this address";
  }
  m_leaves.push_back(leaf{spec, nullptr, nullptr});
  if(m_node)
  {
    if(auto err = create(m_leaves.back()); !err.empty())
    {
      m_leaves.pop_back();
      return err;
    }
  }
  else
  {
    // First leaf: the node is created now, and attach creates the leaf.
    ensure_node();
  }
  return {};
}

std::string branch::create(leaf& l)
{
  const auto& spec = l.spec;
  auto& node = ossia::net::find_or_create_node(*m_node, spec.address);
  if(node.get_parameter())
    return spec.address + ": a parameter already exists at this address";

  auto* param = ossia::try_setup_parameter(spec.type, node);
  if(!param)
    return spec.address + ": cannot create a parameter of type '" + spec.type + "'";

  const auto type = param->get_value_type();
  if(!spec.min.empty() && !spec.max.empty())
  {
    std::vector<ossia::value> mn, mx;
    for(std::size_t i = 0; i < spec.min.size(); ++i)
    {
      if(type == ossia::val_type::INT)
      {
        mn.emplace_back(static_cast<int>(std::lround(spec.min[i])));
        mx.emplace_back(static_cast<int>(std::lround(spec.max[i])));
      }
      else
      {
        mn.emplace_back(spec.min[i]);
        mx.emplace_back(spec.max[i]);
      }
    }
    param->set_domain(ossia::make_domain_from_minmax(mn, mx, type));
  }
  else if(!spec.enumeration.empty())
  {
    param->set_domain(ossia::make_domain(spec.enumeration));
  }

  param->set_bounding(to_bounding(spec.clip));
  param->set_access(to_access(spec.mode));
  if(!spec.unit.empty())
    param->set_unit(ossia::parse_pretty_unit(spec.unit));
  if(!spec.description.empty())
    ossia::net::set_description(node, spec.description.c_str());
  if(!spec.tags.empty())
    ossia::net::set_tags(node, spec.tags);
  if(spec.priority)
    ossia::net::set_priority(node, *spec.priority);
  if(spec.rate)
    ossia::net::set_refresh_rate(node, *spec.rate);
  param->set_critical(spec.critical);
  param->set_repetition_filter(
      spec.repetitions ? ossia::repetition_filter::OFF : ossia::repetition_filter::ON);
  if(!spec.enable)
    ossia::net::set_disabled(node, true);
  if(spec.invisible)
    ossia::net::set_hidden(node, true);
  if(spec.recall_safe)
    ossia::net::set_recall_safe(node, true);

  if(spec.default_value)
  {
    ossia::value v;
    if(auto err = scalars_to_value(*param, *spec.default_value, v); !err.empty())
    {
      node.remove_parameter();
      return spec.address + ": invalid @default: " + err;
    }
    ossia::net::set_default_value(node, v);
    // Same as ossia.parameter at registration: the value is stored and sent
    // to the network, the local callbacks are not fired. The host then outputs
    // it locally, as ossia-max outputs the defaults of its objects after they
    // registered.
    param->push_value_quiet(v);
    on_value(spec.address, v);
  }

  const std::string address = spec.address;
  param->add_callback([this, address](const ossia::value& v) { on_value(address, v); });
  node.about_to_be_deleted.connect<&branch::on_node_removing>(this);

  l.node = &node;
  l.param = param;
  return {};
}

void branch::destroy(leaf& l)
{
  if(!l.node || !m_node)
  {
    l.node = nullptr;
    l.param = nullptr;
    return;
  }
  auto* node = l.node;
  node->about_to_be_deleted.disconnect<&branch::on_node_removing>(this);
  l.node = nullptr;
  l.param = nullptr;

  // A leaf that holds other leaves stays as a container.
  if(!node->children().empty())
  {
    node->remove_parameter();
    return;
  }

  // Remove the leaf, then the intermediate nodes it leaves empty, up to the
  // branch node itself.
  auto* parent = node->get_parent();
  while(parent)
  {
    auto* grandparent = parent->get_parent();
    parent->remove_child(*node);
    if(parent == m_node || !parent->children().empty() || parent->get_parameter())
      break;
    node = parent;
    parent = grandparent;
  }
}

bool branch::remove(std::string_view address)
{
  const auto a = normalize_relative_address(address);
  auto* l = a.empty() ? nullptr : find(a);
  if(!l)
    return false;
  destroy(*l);
  m_leaves.erase(m_leaves.begin() + (l - m_leaves.data()));
  return true;
}

void branch::clear()
{
  for(auto& l : m_leaves)
    destroy(l);
  m_leaves.clear();
}

std::string branch::set(std::string_view address, const scalars& values)
{
  const auto a = normalize_relative_address(address);
  if(auto* l = a.empty() ? nullptr : find(a))
  {
    if(!l->param)
      return a + ": not attached to a device yet";

    ossia::value v;
    if(auto err = scalars_to_value(*l->param, values, v); !err.empty())
      return a + ": " + err;

    pushing_scope guard{this, &values};
    // libossia only applies the range to values written from the network.
    l->param->push_value(ossia::bound_value(*l->param, std::move(v)));
    return {};
  }

  if(auto* b = find_binding(address))
  {
    if(b->targets.empty())
      return b->address + ": bound parameter not found yet";
    pushing_scope guard{this, &values};
    for(auto& t : b->targets)
    {
      ossia::value v;
      if(auto err = scalars_to_value(*t.param, values, v); !err.empty())
        return b->address + ": " + err;
      t.param->push_value(ossia::bound_value(*t.param, std::move(v)));
    }
    return {};
  }

  return "unknown address '" + std::string(address) + "'";
}

std::vector<std::string> branch::addresses() const
{
  std::vector<std::string> out;
  out.reserve(m_leaves.size());
  for(const auto& l : m_leaves)
    out.push_back(l.spec.address);
  return out;
}

void branch::set_wake_callback(std::function<void()> cb)
{
  m_wake = std::move(cb);
}

std::vector<inbound> branch::drain()
{
  std::vector<queued> q;
  {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    q.swap(m_queue);
  }
  std::vector<inbound> out;
  out.reserve(q.size());
  for(auto& e : q)
  {
    if(e.read_from)
    {
      // Only read a parameter this branch still follows.
      bool live = false;
      for(const auto& b : m_bindings)
        for(const auto& t : b.targets)
          live = live || t.param == e.read_from;
      if(!live || !e.read_from->value().valid())
        continue;
      out.push_back({std::move(e.address), e.read_from->value()});
    }
    else
      out.push_back({std::move(e.address), std::move(e.value)});
  }
  return out;
}

void branch::queue_current_value(const std::string& address, ossia::net::parameter_base* param)
{
  {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_queue.push_back({address, {}, param});
  }
  if(m_wake)
    m_wake();
}

void branch::on_value(const std::string& address, const ossia::value& v)
{
  // The branch's own push comes back only if the parameter changed the value
  // (clip, type conversion): the host then learns the effective value.
  if(t_pushing == this && t_pushing_request && scalars_match(*t_pushing_request, v))
    return;
  {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_queue.push_back({address, v, nullptr});
  }
  if(m_wake)
    m_wake();
}

namespace
{
std::string trim_address(std::string_view address)
{
  while(!address.empty() && std::isspace(static_cast<unsigned char>(address.front())))
    address.remove_prefix(1);
  while(!address.empty() && std::isspace(static_cast<unsigned char>(address.back())))
    address.remove_suffix(1);
  return std::string(address);
}

// Address of a node relative to a root node, without leading slash.
std::string relative_to(const ossia::net::node_base& root, const ossia::net::node_base& node)
{
  const auto full = node.osc_address();
  auto base = root.osc_address();
  if(base == "/")
    return full.substr(1);
  if(full.rfind(base + "/", 0) == 0)
    return full.substr(base.size() + 1);
  return full;
}
}

branch::binding* branch::find_binding(std::string_view address)
{
  const auto a = trim_address(address);
  for(auto& b : m_bindings)
    if(b.address == a)
      return &b;
  return nullptr;
}

std::string branch::bind(std::string_view address)
{
  const auto a = trim_address(address);
  if(a.empty())
    return "bind expects an address";
  if(find_binding(a))
    return a + ": already bound";
  if(find(normalize_relative_address(a)))
    return a + ": already declared by add";
  m_bindings.push_back(binding{a, {}});
  resolve(m_bindings.back());
  if(m_binding_changed)
    m_binding_changed(a, m_bindings.back().targets.size());
  return {};
}

bool branch::unbind(std::string_view address)
{
  auto* b = find_binding(address);
  if(!b)
    return false;
  release(*b);
  m_bindings.erase(m_bindings.begin() + (b - m_bindings.data()));
  return true;
}

std::size_t branch::bound_count(std::string_view address) const
{
  const auto a = trim_address(address);
  for(const auto& b : m_bindings)
    if(b.address == a)
      return b.targets.size();
  return 0;
}

void branch::release(binding& b)
{
  for(auto& t : b.targets)
  {
    if(t.node)
      t.node->about_to_be_deleted.disconnect<&branch::on_node_removing>(this);
    if(t.param)
      t.param->remove_callback(t.callback);
  }
  b.targets.clear();
}

void branch::set_resolution_context(
    std::vector<ossia::net::node_base*> parents,
    std::vector<ossia::net::device_base*> devices)
{
  for(auto* dev : m_devices)
    dev->on_parameter_created.disconnect<&branch::on_parameter_created>(this);
  m_parents = std::move(parents);
  m_devices = std::move(devices);
  for(auto* dev : m_devices)
    dev->on_parameter_created.connect<&branch::on_parameter_created>(this);
  resolve_bindings();
}

void branch::drop_device(ossia::net::device_base* dev)
{
  auto it = std::find(m_devices.begin(), m_devices.end(), dev);
  if(it == m_devices.end())
    return;
  dev->on_parameter_created.disconnect<&branch::on_parameter_created>(this);
  m_devices.erase(it);
  std::erase_if(m_parents, [dev](ossia::net::node_base* n) { return &n->get_device() == dev; });
  for(auto& b : m_bindings)
  {
    const auto before = b.targets.size();
    for(auto& t : b.targets)
    {
      if(&t.node->get_device() == dev)
      {
        t.node->about_to_be_deleted.disconnect<&branch::on_node_removing>(this);
        t.param->remove_callback(t.callback);
        t.node = nullptr;
      }
    }
    std::erase_if(b.targets, [](const target& t) { return t.node == nullptr; });
    if(b.targets.size() != before && m_binding_changed)
      m_binding_changed(b.address, b.targets.size());
  }
}

void branch::set_binding_changed_callback(
    std::function<void(const std::string&, std::size_t)> cb)
{
  m_binding_changed = std::move(cb);
}

void branch::resolve_bindings()
{
  for(auto& b : m_bindings)
  {
    const auto before = b.targets.size();
    resolve(b);
    if(b.targets.size() != before && m_binding_changed)
      m_binding_changed(b.address, b.targets.size());
  }
}

void branch::resolve(binding& b)
{
  // Candidate (root, node) pairs for the address, with the rules of ossia.remote.
  std::vector<std::pair<ossia::net::node_base*, ossia::net::node_base*>> found;
  std::string_view address = b.address;
  const auto scope = ossia::net::get_address_scope(address);
  const bool pattern = ossia::traversal::is_pattern(address);

  auto collect = [&](ossia::net::node_base& root, std::string_view addr) {
    if(pattern)
    {
      for(auto* n : ossia::net::find_nodes(root, addr))
        found.emplace_back(&root, n);
    }
    else if(auto* n = ossia::net::find_node(root, addr))
      found.emplace_back(&root, n);
  };

  switch(scope)
  {
    case ossia::net::address_scope::relative: {
      if(address.rfind("//", 0) == 0)
        address.remove_prefix(2);
      for(auto* p : m_parents)
        collect(*p, address);
      break;
    }
    case ossia::net::address_scope::absolute: {
      if(!m_parents.empty())
        collect(m_parents.front()->get_device().get_root_node(), address);
      break;
    }
    case ossia::net::address_scope::global: {
      const auto pos = address.find(":/");
      const auto name = address.substr(0, pos);
      const auto osc = address.substr(pos + 1);
      for(auto* dev : m_devices)
        if(dev->get_name() == name)
          collect(dev->get_root_node(), osc);
      break;
    }
  }

  for(auto [root, node] : found)
  {
    auto* param = node->get_parameter();
    if(!param)
      continue;
    const bool already = std::any_of(
        b.targets.begin(), b.targets.end(), [node](const target& t) { return t.node == node; });
    if(already)
      continue;

    target t;
    t.node = node;
    t.param = param;
    if(!pattern)
      t.out_address = b.address;
    else if(scope == ossia::net::address_scope::global)
      t.out_address = std::string(b.address.substr(0, b.address.find(":/"))) + ":" + node->osc_address();
    else if(scope == ossia::net::address_scope::absolute)
      t.out_address = node->osc_address();
    else
      t.out_address = relative_to(*root, *node);

    const std::string out = t.out_address;
    t.callback = param->add_callback([this, out](const ossia::value& v) { on_value(out, v); });
    node->about_to_be_deleted.connect<&branch::on_node_removing>(this);
    b.targets.push_back(std::move(t));

    // The value the parameter holds when the subscription starts, read when
    // the host drains, as an ossia.remote outputs the value it finds.
    queue_current_value(out, param);
  }
}

void branch::on_parameter_created(const ossia::net::parameter_base&)
{
  bool pending = false;
  for(const auto& b : m_bindings)
    if(b.targets.empty() || ossia::traversal::is_pattern(b.address))
      pending = true;
  if(pending)
    resolve_bindings();
}

void branch::on_node_removing(const ossia::net::node_base& n)
{
  for(auto& b : m_bindings)
  {
    const auto before = b.targets.size();
    std::erase_if(b.targets, [&n](const target& t) { return t.node == &n; });
    if(b.targets.size() != before && m_binding_changed)
      m_binding_changed(b.address, b.targets.size());
  }

  // The node is being destroyed: forget it without touching its signals.
  if(&n == m_node)
  {
    for(auto& l : m_leaves)
    {
      l.node = nullptr;
      l.param = nullptr;
    }
    m_node = nullptr;
    if(m_node_lost)
      m_node_lost();
    return;
  }
  for(auto& l : m_leaves)
  {
    if(l.node == &n)
    {
      l.node = nullptr;
      l.param = nullptr;
    }
  }
}

void branch::set_node_lost_callback(std::function<void()> cb)
{
  m_node_lost = std::move(cb);
}

}
}
}
