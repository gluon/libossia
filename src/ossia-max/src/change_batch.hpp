#pragma once
#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

namespace ossia
{
namespace max_binding
{
// What the tree changes of a device leave between two flushes: how many nodes
// and parameters were created and removed, the paths that cover the changes,
// and the changes themselves while there are few. Without Max: ossia.client
// and ossia.device give it each change, from any thread, and flush it on the
// main thread (namespace_changed, @notify).
class change_batch
{
public:
  enum class kind
  {
    node_created,
    node_removed,
    node_renamed,
    parameter_created,
    parameter_removed
  };
  struct change
  {
    kind what{};
    std::string address;     // the device's name, ':', the OSC address
    std::string old_address; // node_renamed only
  };
  struct result
  {
    long nodes_created{}, nodes_removed{}, parameters_created{}, parameters_removed{};
    // OSC addresses; each change is at one of them or under it
    std::vector<std::string> paths;
    // In their order, when there were at most `keep` of them
    std::vector<change> changes;
    bool overflow{};
    bool empty() const noexcept;
  };

  static constexpr std::size_t max_paths = 16;

  explicit change_batch(std::size_t keep = 32)
      : m_keep{keep}
  {
  }

  // A change at the OSC address path, of the device named device. True when
  // it is the first since the last take: the caller schedules a flush.
  bool add(kind k, const std::string& device, const std::string& path,
           const std::string& old_path = {});

  // What came since the last take, and a new batch
  result take();

private:
  std::mutex m_mutex;
  std::size_t m_keep{};
  std::size_t m_count{};
  result m_current;
};

}
}
