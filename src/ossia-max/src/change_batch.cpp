#include "change_batch.hpp"

#include <algorithm>
#include <string_view>

namespace ossia
{
namespace max_binding
{
namespace
{
// a is the path p or one of its ancestors
bool covers(std::string_view a, std::string_view p)
{
  if(a == "/" || a == p)
    return true;
  return p.size() > a.size() && p.substr(0, a.size()) == a && p[a.size()] == '/';
}

// The deepest path that covers a and b: a parent they share, "/" at least
std::string common_parent(std::string_view a, std::string_view b)
{
  std::size_t end = 0; // where the last segment they share ends
  for(std::size_t i = 1;; i++)
  {
    const bool a_end = i >= a.size() || a[i] == '/';
    const bool b_end = i >= b.size() || b[i] == '/';
    if(a_end && b_end)
    {
      end = i;
      if(i >= a.size() || i >= b.size())
        break;
      continue;
    }
    if(a_end != b_end || a[i] != b[i])
      break;
  }
  return end == 0 ? std::string{"/"} : std::string{a.substr(0, end)};
}

// Each changed path is at one of paths or under it; when more than max places
// changed, the two that share the deepest parent give way to that parent
void cover(std::vector<std::string>& paths, const std::string& p, std::size_t max)
{
  for(const auto& root : paths)
    if(covers(root, p))
      return;
  std::erase_if(paths, [&](const std::string& root) { return covers(p, root); });
  paths.push_back(p);
  while(paths.size() > max)
  {
    std::size_t best_i = 0, best_j = 1;
    std::string best;
    for(std::size_t i = 0; i < paths.size(); i++)
      for(std::size_t j = i + 1; j < paths.size(); j++)
      {
        auto c = common_parent(paths[i], paths[j]);
        if(best.empty() || c.size() > best.size())
        {
          best_i = i;
          best_j = j;
          best = std::move(c);
        }
      }
    paths.erase(paths.begin() + best_j);
    paths.erase(paths.begin() + best_i);
    std::erase_if(paths, [&](const std::string& root) { return covers(best, root); });
    paths.push_back(std::move(best));
  }
}
}

bool change_batch::result::empty() const noexcept
{
  return nodes_created == 0 && nodes_removed == 0 && parameters_created == 0
         && parameters_removed == 0 && paths.empty() && changes.empty();
}

bool change_batch::add(
    kind k, const std::string& device, const std::string& path, const std::string& old_path)
{
  std::lock_guard lock{m_mutex};
  const bool first = m_count == 0;
  m_count++;
  switch(k)
  {
    case kind::node_created:
      m_current.nodes_created++;
      break;
    case kind::node_removed:
      m_current.nodes_removed++;
      break;
    case kind::parameter_created:
      m_current.parameters_created++;
      break;
    case kind::parameter_removed:
      m_current.parameters_removed++;
      break;
    case kind::node_renamed:
      break;
  }
  if(k == kind::node_renamed && !old_path.empty())
    cover(m_current.paths, old_path, max_paths);
  cover(m_current.paths, path, max_paths);

  if(m_count <= m_keep)
  {
    change c;
    c.what = k;
    c.address = device + ":" + path;
    if(k == kind::node_renamed)
      c.old_address = device + ":" + old_path;
    m_current.changes.push_back(std::move(c));
  }
  else if(!m_current.overflow)
  {
    m_current.overflow = true;
    m_current.changes.clear();
    m_current.changes.shrink_to_fit();
  }
  return first;
}

change_batch::result change_batch::take()
{
  std::lock_guard lock{m_mutex};
  result r = std::move(m_current);
  m_current = result{};
  m_count = 0;
  return r;
}

}
}
