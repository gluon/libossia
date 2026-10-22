#pragma once

// Max-independent part of ossia.v8connector: a branch of an ossia device
// whose leaves (parameters) are declared at runtime by a script.
//
// The branch does not own its node. The host object creates the node in the
// device tree, attaches the branch to it and detaches it before the node
// disappears. The declarations are kept, so that attaching the branch to a new
// node recreates every leaf.
//
// A branch can also follow parameters that exist elsewhere (bind), the way
// ossia.remote does: nothing is created, the subscription follows the
// parameter when it disappears and comes back, and set pushes to it.

#include <ossia/network/base/device.hpp>
#include <ossia/network/base/node.hpp>
#include <ossia/network/base/parameter.hpp>
#include <ossia/network/value/value.hpp>

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ossia
{
namespace max_binding
{
namespace v8connector_detail
{

// A value as it arrives from the host: integer, float or symbol.
using scalar = std::variant<int, float, std::string>;
using scalars = std::vector<scalar>;

// One "@name values..." group of a declaration.
struct attribute
{
  std::string name;
  scalars values;
};

// Validated declaration of a leaf.
struct leaf_spec
{
  std::string address; // relative to the branch, "a" or "a/b", no leading slash
  std::string type;    // float int bool impulse string list vec2f vec3f vec4f
  std::vector<float> min, max;  // one value per component, numeric types only
  std::vector<std::string> enumeration; // range of a string parameter
  std::string unit;
  std::string description;
  std::vector<std::string> tags;
  std::optional<scalars> default_value;
  std::string clip{"free"};
  std::string mode{"bi"};
  std::optional<float> priority;
  std::optional<float> rate;
  bool critical{false};
  bool invisible{false};
  bool repetitions{true};
  bool enable{true};
  bool recall_safe{false};
};

// A value received for a leaf, to be output by the host.
struct inbound
{
  std::string address; // relative to the branch
  ossia::value value;
};

// Normalize a relative address: trims, removes leading and trailing slashes,
// refuses empty components. Returns an empty string on refusal.
std::string normalize_relative_address(std::string_view address);

// Build a leaf declaration from an address and its attributes, using the
// attribute names of ossia.parameter. Returns an empty string on success,
// otherwise the reason of the refusal. Numeric types require min and max
// (or range).
std::string make_leaf_spec(
    std::string_view address, const std::vector<attribute>& attrs, leaf_spec& out);

// Convert host scalars to a value of the parameter's type. Returns an empty
// string on success, otherwise the reason.
std::string
scalars_to_value(const ossia::net::parameter_base& param, const scalars& in, ossia::value& out);

class branch
{
public:
  branch();
  ~branch();
  branch(const branch&) = delete;
  branch& operator=(const branch&) = delete;

  // Attach to a node of the device tree and recreate every declared leaf.
  void attach(ossia::net::node_base& node);
  // Forget the node and the leaves it holds. The declarations are kept.
  void detach();
  bool attached() const { return m_node != nullptr; }
  ossia::net::node_base* node() const { return m_node; }

  // Declare a leaf. Refused if the address is already declared, or if the
  // branch is attached and the address already holds a parameter.
  std::string add(const leaf_spec& spec);
  // Remove a leaf. Returns false if the address is not declared.
  bool remove(std::string_view address);
  // Remove every leaf of this branch.
  void clear();
  // Push a value from the host, to a declared leaf or to a bound address. The
  // value is clipped to the parameter's range. The host's own callback is not
  // fired for this push.
  std::string set(std::string_view address, const scalars& values);

  // Follow a parameter that exists elsewhere. The address follows the rules of
  // ossia.remote: relative to the parents given by set_resolution_context,
  // absolute ("/a/b") in the parents' device, or global ("device:/a/b");
  // patterns are accepted. Refused if the address is empty, already bound, or
  // declared as a leaf of this branch. A bind to a parameter that does not
  // exist yet is kept and resolved when the parameter appears.
  std::string bind(std::string_view address);
  // Drop a subscription. Returns false if the address is not bound.
  bool unbind(std::string_view address);
  // Number of parameters currently followed by a bound address.
  std::size_t bound_count(std::string_view address) const;

  // Where relative and absolute bound addresses are resolved: the nodes an
  // ossia.remote placed next to the host would register under, and the devices
  // known to the host. Resolves the pending bindings, and listens to parameter
  // creation on these devices to resolve them later.
  void set_resolution_context(
      std::vector<ossia::net::node_base*> parents,
      std::vector<ossia::net::device_base*> devices);
  // Forget a device before it is destroyed.
  void drop_device(ossia::net::device_base* dev);
  // Called when the number of parameters followed by a bound address changes.
  void set_binding_changed_callback(
      std::function<void(const std::string& address, std::size_t count)> cb);

  std::vector<std::string> addresses() const;
  std::size_t size() const { return m_leaves.size(); }

  // Called from the thread that pushed a value, possibly not the host's main
  // thread. It must only schedule a drain on the main thread.
  void set_wake_callback(std::function<void()> cb);
  // Main thread only: take every value received since the last call.
  std::vector<inbound> drain();

  // Called when the node the branch is attached to is destroyed by its owner
  // (for instance when the enclosing model registers again). The branch is
  // detached when it fires; the host should attach it to a new node later.
  void set_node_lost_callback(std::function<void()> cb);

private:
  struct leaf
  {
    leaf_spec spec;
    ossia::net::node_base* node{};
    ossia::net::parameter_base* param{};
  };

  struct target
  {
    ossia::net::node_base* node{};
    ossia::net::parameter_base* param{};
    ossia::net::parameter_base::callback_index callback{};
    std::string out_address;
  };

  struct binding
  {
    std::string address;
    std::vector<target> targets;
  };

  leaf* find(std::string_view address);
  binding* find_binding(std::string_view address);
  std::string create(leaf& l);
  void destroy(leaf& l);
  void resolve(binding& b, bool emit_current);
  void resolve_bindings(bool emit_current);
  void release(binding& b);
  void on_value(const std::string& address, const ossia::value& v);
  void on_node_removing(const ossia::net::node_base& n);
  void on_parameter_created(const ossia::net::parameter_base& p);

  ossia::net::node_base* m_node{};
  std::vector<leaf> m_leaves;
  std::vector<binding> m_bindings;
  std::vector<ossia::net::node_base*> m_parents;
  std::vector<ossia::net::device_base*> m_devices;
  std::function<void(const std::string&, std::size_t)> m_binding_changed;

  std::mutex m_queue_mutex;
  std::vector<inbound> m_queue;
  std::function<void()> m_wake;
  std::function<void()> m_node_lost;
};

}
}
}
