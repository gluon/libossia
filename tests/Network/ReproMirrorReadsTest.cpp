// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// What the client of ossia (oscquery_mirror_protocol) reads and removes, on
// the large tree server of LargeTreeCommon.hpp: each value answer on the
// parameter it was asked for, the value reads of a removed node not sent,
// the errors of value reads written at most once a second, many parameters
// nothing follows read as one tree, removals applied in slices by a polled
// mirror; and what ossia-max makes of the tree changes of a device
// (change_batch, namespace_changed).

#include "change_batch.hpp"
#include "client_mirror.hpp"
#include "LargeTreeCommon.hpp"

#include "include_catch.hpp"

#include <ossia/detail/logger.hpp>

#include <spdlog/sinks/base_sink.h>

#include <functional>

using namespace large_tree;

// ThreadSanitizer stops every thread at times, tens of ms (a step that removes
// one node took 52 ms once in a run of all the cases, 10 ms alone): there the
// time of a call says nothing, and what a call removed is counted instead (a
// call of 10 ms removes at most 500 nodes at 20 us each; a track whole, 5,278)
#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define OSSIA_TEST_THREAD_SANITIZER 1
#endif
#endif

namespace
{
std::string value_address(int t, int d, int p)
{
  return "/tracks/" + std::to_string(t) + "/devices/" + std::to_string(d) + "/parameters/"
         + std::to_string(p) + "/value";
}

struct client
{
  std::unique_ptr<ossia::net::generic_device> dev;
  ossia::oscquery::oscquery_mirror_protocol* mirror{};
  bool polled{};

  client(bench_server& s, bool polled_as_max)
      : polled{polled_as_max}
  {
    auto p = std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
        s.url(), bench_server::free_udp_port());
    mirror = p.get();
    dev = std::make_unique<ossia::net::generic_device>(std::move(p), "large_tree");
    REQUIRE(mirror->update(dev->get_root_node()));
    if(polled)
      ossia::max_binding::polled_oscquery_mirror(*dev);
  }
  ossia::net::node_base& root() { return dev->get_root_node(); }
  ossia::net::parameter_base* param(const std::string& address)
  {
    auto n = ossia::net::find_node(root(), address);
    return n ? n->get_parameter() : nullptr;
  }
  long tracks()
  {
    auto n = ossia::net::find_node(root(), "/tracks");
    return n ? long(n->children().size()) : 0;
  }
  // Polls as ossia-max does (all of it at once) until done holds, ms at most
  bool poll_until(const std::function<bool()>& done, int ms)
  {
    const auto t0 = clock_type::now();
    while(seconds_since(t0) * 1000 < ms)
    {
      mirror->run_commands();
      if(done())
        return true;
      std::this_thread::sleep_for(5ms);
    }
    mirror->run_commands();
    return done();
  }
  void poll_for(int ms)
  {
    poll_until([] { return false; }, ms);
  }
};

bool is_value_read(const reading& r)
{
  return r.target.size() > 6 && r.target.compare(r.target.size() - 6, 6, "?VALUE") == 0;
}

// What the library writes, as Max's console gets it
struct captured_log final : spdlog::sinks::base_sink<std::mutex>
{
  std::vector<std::string> lines;
  std::vector<std::string> take()
  {
    std::lock_guard lock{mutex_};
    return lines;
  }

protected:
  void sink_it_(const spdlog::details::log_msg& msg) override
  {
    if(msg.level >= spdlog::level::warn)
      lines.emplace_back(msg.payload.data(), msg.payload.size());
  }
  void flush_() override { }
};

// The removal of a node costs what Max's console output of it costs
struct slow_listener
{
  long removed{};
  void on_removing(const ossia::net::node_base&)
  {
    removed++;
    const auto end = clock_type::now() + 20us;
    while(clock_type::now() < end)
      ;
  }
};
}

// Proposal, VALUE: a value read answers with the value only, without its
// address. The client must put each answer on the parameter it asked about,
// even when a read before it failed (404: the node is gone from the server,
// the client does not know it yet).
TEST_CASE("Mirror: each value answer goes to the parameter it was asked for", "[value-pairing]")
{
  const bool polled = GENERATE(false, true);
  tree_shape sh;
  sh.base_tracks = 4;
  sh.tracks = 0;
  sh.devices = 1;
  sh.parameters = 4;
  synthetic_tree tree{sh};
  bench_server s{tree, 0};
  s.distinct_values = true;
  client c{s, polled};
  // Past the values the tree reading brought
  std::this_thread::sleep_for(1100ms);

  tree.present = 3;
  const auto t0 = clock_type::now();
  c.mirror->refresh_unfollowed(value_address(3, 0, 0));
  for(int p = 0; p < 4; p++)
    c.mirror->refresh_unfollowed(value_address(0, 0, p));
  const bool answered = c.poll_until(
      [&] { return s.count_if(is_value_read, t0) == 5 && s.readings_open == 0; }, 5000);
  c.poll_for(200);
  CHECK(answered);
  for(int p = 0; p < 4; p++)
  {
    auto* param = c.param(value_address(0, 0, p));
    REQUIRE(param);
    INFO("polled " << polled << ", parameter " << p);
    CHECK(param->value() == ossia::value{float(1 + p)});
  }
}

// The value reads still to send for a node the server removed (PATH_REMOVED)
// are dropped once the client removes it: they would only come back 404, one
// line each in Max's console.
TEST_CASE("Mirror: the value reads of a removed node are not sent", "[value-reads-cancelled]")
{
  tree_shape sh;
  sh.base_tracks = 4;
  sh.tracks = 0;
  sh.devices = 2;
  sh.parameters = 48;
  synthetic_tree tree{sh};
  bench_server s{tree, 0};
  s.value_ms = 5;
  client c{s, true};
  std::this_thread::sleep_for(1100ms);

  std::vector<std::string> addresses;
  auto track = ossia::net::find_node(c.root(), "/tracks/3");
  REQUIRE(track);
  for(auto* n : ossia::net::list_all_children(track))
    if(n->get_parameter())
      addresses.push_back(n->osc_address());
  REQUIRE(addresses.size() > 1000);
  for(auto& a : addresses)
    c.mirror->refresh_unfollowed(a);
  std::this_thread::sleep_for(100ms);

  tree.present = 3;
  s.path_removed("/tracks/3");
  REQUIRE(c.poll_until([&] { return c.tracks() == 3; }, 5000));
  c.poll_for(1500);
  auto under_track_3 = [](const reading& r) {
    return is_value_read(r) && r.target.rfind("/tracks/3/", 0) == 0;
  };
  const int sent = s.count_if(under_track_3);
  const auto stats = c.mirror->get_value_read_stats();
  std::printf("value-reads-cancelled: %zu reads queued, %d reached the server; cancelled %zu\n",
              addresses.size(), sent, stats.cancelled);
  CHECK(sent < 100);
  CHECK(stats.cancelled + sent >= addresses.size() - 2);
}

// Max writes each error of the library in its console: a burst of failed
// value reads gives at most one line a second, which says how many failed.
TEST_CASE("Mirror: the errors of value reads are written at most once a second", "[value-read-errors]")
{
  auto sink = std::make_shared<captured_log>();
  auto& sinks = ossia::logger().sinks();
  sinks.push_back(sink);
  {
    tree_shape sh;
    sh.base_tracks = 1;
    sh.tracks = 0;
    sh.devices = 1;
    sh.parameters = 1;
    synthetic_tree tree{sh};
    bench_server s{tree, 0};
    client c{s, true};
    std::this_thread::sleep_for(1100ms);

    auto nothing = [](const reading& r) { return r.target.rfind("/nothing/", 0) == 0; };
    for(int i = 0; i < 100; i++)
      c.mirror->refresh_unfollowed("/nothing/" + std::to_string(i));
    const bool failed
        = c.poll_until([&] { return s.count_if(nothing) == 100 && s.readings_open == 0; }, 5000);
    c.poll_for(1500);
    CHECK(failed);
    int lines = 0;
    bool counted = false;
    for(auto& l : sink->take())
      if(l.find("/nothing/") != std::string::npos)
      {
        lines++;
        if(l.find("value reads") != std::string::npos)
          counted = true;
      }
    std::printf("value-read-errors: 100 reads failed, %d lines written\n", lines);
    CHECK(lines >= 1);
    CHECK(lines <= 2);
    CHECK(counted);
    CHECK(c.mirror->get_value_read_stats().failed == 100);
  }
  sinks.erase(std::remove(sinks.begin(), sinks.end(), sink), sinks.end());
}

// ossia.client's namespace gives the value of each parameter, and reads again
// those nothing follows: many of them, the tree under the root is read once
// instead (one request, not one a parameter, 77 ms each on Live); and a value
// that came with a reading of the tree less than a second ago is not read.
TEST_CASE("Mirror: many parameters nothing follows are read as one tree", "[bulk-refresh]")
{
  tree_shape sh;
  sh.base_tracks = 4;
  sh.tracks = 0;
  sh.devices = 1;
  sh.parameters = 48;
  synthetic_tree tree{sh};
  bench_server s{tree, 0};
  client c{s, true};
  std::this_thread::sleep_for(1100ms);

  std::vector<ossia::net::parameter_base*> all;
  for(auto* n : ossia::net::list_all_children(&c.root()))
    if(auto p = n->get_parameter())
      all.push_back(p);
  REQUIRE(all.size() > 2000);
  auto tree_reading = [](const reading& r) { return r.tree(); };

  SECTION("many")
  {
    const auto t0 = clock_type::now();
    ossia::oscquery::refresh_followed_values(all);
    c.poll_for(1500);
    const int values = s.count_if(is_value_read, t0), trees = s.count_if(tree_reading, t0);
    std::printf("bulk-refresh: %zu parameters, %d value reads, %d tree readings\n", all.size(),
                values, trees);
    CHECK(values == 0);
    CHECK(trees == 1);
  }
  SECTION("few")
  {
    const auto t0 = clock_type::now();
    std::vector<ossia::net::parameter_base*> few(all.begin(), all.begin() + 5);
    ossia::oscquery::refresh_followed_values(few);
    c.poll_for(1000);
    CHECK(s.count_if(is_value_read, t0) == 5);
    CHECK(s.count_if(tree_reading, t0) == 0);
  }
  SECTION("fresh")
  {
    auto fut = c.mirror->update_async(c.root());
    REQUIRE(c.poll_until(
        [&] { return fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready; },
        5000));
    const auto t0 = clock_type::now();
    c.mirror->refresh_unfollowed(value_address(0, 0, 0));
    c.poll_for(300);
    CHECK(s.count_if(is_value_read, t0) == 0);
    std::this_thread::sleep_for(1100ms);
    c.mirror->refresh_unfollowed(value_address(0, 0, 0));
    c.poll_for(300);
    CHECK(s.count_if(is_value_read, t0) == 1);
  }
}

// The polling thread is Max's main thread, and each node removed costs there
// what its output in Max costs: a removal (PATH_REMOVED, or the tree read
// again without some nodes) is applied in slices of the budget too, not one
// subtree at once (a track of the load set has 5,278 nodes). A path removed
// then added again ends added.
TEST_CASE("Mirror: a polled mirror removes in slices", "[sliced-removal]")
{
  tree_shape sh;
  sh.base_tracks = 8;
  sh.tracks = 0;
  synthetic_tree tree{sh};
  bench_server s{tree, 0};
  client c{s, true};
  slow_listener listener;
  c.dev->on_node_removing.connect<&slow_listener::on_removing>(&listener);

  double longest_ms = 0;
  long most_removed = 0;
  auto poll_until = [&](const std::function<bool()>& done) {
    const auto t0 = clock_type::now();
    while(seconds_since(t0) < 60)
    {
      const auto c0 = clock_type::now();
      const long r0 = listener.removed;
      const bool more = c.mirror->run_commands_for(std::chrono::milliseconds(10));
      longest_ms = std::max(
          longest_ms, std::chrono::duration<double, std::milli>(clock_type::now() - c0).count());
      most_removed = std::max(most_removed, listener.removed - r0);
      if(!more && done())
        return true;
      if(!more)
        std::this_thread::sleep_for(2ms);
    }
    return false;
  };

  SECTION("PATH_REMOVED")
  {
    tree.present = 4;
    for(int t = 7; t >= 4; t--)
      s.path_removed("/tracks/" + std::to_string(t));
    CHECK(poll_until([&] { return c.tracks() == 4; }));
  }
  SECTION("the tree read again")
  {
    tree.present = 4;
    auto fut = c.mirror->update_async(c.root());
    CHECK(poll_until(
        [&] { return fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }));
  }
  SECTION("removed then added again")
  {
    tree.present = 7;
    s.path_removed("/tracks/7");
    std::this_thread::sleep_for(50ms);
    tree.present = 8;
    s.path_changed("/tracks/7");
    std::this_thread::sleep_for(200ms);
    CHECK(poll_until([&] { return c.tracks() == 8 && s.readings_open == 0; }));
    c.poll_for(300);
  }
  const long leaves = count_leaves(c.root());
  std::printf("sliced-removal: longest call %.1f ms, at most %ld nodes removed in a call, leaves "
              "%ld of %ld\n",
              longest_ms, most_removed, leaves, tree.leaves());
  CHECK(most_removed < 2000);
#if !defined(OSSIA_TEST_THREAD_SANITIZER)
  CHECK(longest_ms < 40);
#endif
  CHECK(leaves == tree.leaves());
  c.dev->on_node_removing.disconnect<&slow_listener::on_removing>(&listener);
}

// The changes of the tree come from the WebSocket (PATH_REMOVED) and from the
// HTTP thread (the reading of a PATH_CHANGED): the polling thread applies them
// in the order they came, whichever thread brought them. A path removed then
// added again, both waiting for the poll, ends added.
TEST_CASE("Mirror: the tree changes are applied in the order they came", "[command-order]")
{
  tree_shape sh;
  sh.base_tracks = 8;
  sh.tracks = 0;
  sh.devices = 1;
  sh.parameters = 2;
  synthetic_tree tree{sh};
  bench_server s{tree, 0};
  client c{s, true};
  int wrong = 0;
  for(int round = 0; round < 20; round++)
  {
    tree.present = 7;
    s.path_removed("/tracks/7");
    std::this_thread::sleep_for(20ms);
    tree.present = 8;
    s.path_changed("/tracks/7");
    // Both wait in the queue: the removal, then the reading
    std::this_thread::sleep_for(150ms);
    c.poll_until([&] { return s.readings_open == 0; }, 5000);
    c.poll_for(100);
    if(c.tracks() != 8 || count_leaves(c.root()) != tree.leaves())
      wrong++;
  }
  std::printf("command-order: %d rounds of 20 ended without the track\n", wrong);
  CHECK(wrong == 0);
}

// ossia.client and ossia.device: one namespace_changed for the tree changes
// between two flushes, with the paths that cover them (at most 16, a common
// parent when more places changed), and each change as before while there
// are at most 32 of them.
TEST_CASE("ossia-max: the tree changes between two flushes", "[change-batch]")
{
  using cb = ossia::max_binding::change_batch;
  using k = cb::kind;

  SECTION("few")
  {
    cb b{32};
    CHECK(b.add(k::node_created, "dev", "/a"));
    CHECK_FALSE(b.add(k::parameter_created, "dev", "/a"));
    CHECK_FALSE(b.add(k::node_created, "dev", "/b/c"));
    auto r = b.take();
    CHECK_FALSE(r.empty());
    CHECK(r.nodes_created == 2);
    CHECK(r.parameters_created == 1);
    CHECK(r.nodes_removed == 0);
    CHECK(r.parameters_removed == 0);
    CHECK_FALSE(r.overflow);
    REQUIRE(r.changes.size() == 3);
    CHECK(r.changes[0].what == k::node_created);
    CHECK(r.changes[0].address == "dev:/a");
    CHECK(r.changes[1].what == k::parameter_created);
    CHECK(r.changes[2].address == "dev:/b/c");
    CHECK(r.paths == std::vector<std::string>{"/a", "/b/c"});
    CHECK(b.take().empty());
    CHECK(b.add(k::node_removed, "dev", "/a"));
  }
  SECTION("many")
  {
    cb b{32};
    for(int i = 0; i < 100; i++)
      b.add(k::node_created, "dev", "/tracks/5/x/" + std::to_string(i));
    b.add(k::node_created, "dev", "/tracks/5/x");
    auto r = b.take();
    CHECK(r.overflow);
    CHECK(r.changes.empty());
    CHECK(r.nodes_created == 101);
    CHECK(r.paths == std::vector<std::string>{"/tracks/5/x"});
  }
  SECTION("at most 16 paths, covering every change")
  {
    cb b{32};
    std::vector<std::string> changed;
    for(int t = 0; t < 40; t++)
    {
      changed.push_back("/tracks/" + std::to_string(t) + "/name");
      b.add(k::node_created, "dev", changed.back());
    }
    auto r = b.take();
    CHECK(r.paths.size() <= cb::max_paths);
    for(auto& p : changed)
    {
      bool covered = false;
      for(auto& root : r.paths)
        covered = covered || root == "/" || p == root || p.rfind(root + "/", 0) == 0;
      INFO(p);
      CHECK(covered);
    }
    CHECK(r.paths == std::vector<std::string>{"/tracks"});
  }
  SECTION("a subtree removed leaf first")
  {
    cb b{32};
    b.add(k::parameter_removed, "dev", "/t/1/a");
    b.add(k::node_removed, "dev", "/t/1/a");
    b.add(k::node_removed, "dev", "/t/1/b");
    b.add(k::node_removed, "dev", "/t/1");
    auto r = b.take();
    CHECK(r.nodes_removed == 3);
    CHECK(r.parameters_removed == 1);
    CHECK(r.paths == std::vector<std::string>{"/t/1"});
  }
  SECTION("renamed")
  {
    cb b{32};
    b.add(k::node_renamed, "dev", "/b", "/a");
    auto r = b.take();
    REQUIRE(r.changes.size() == 1);
    CHECK(r.changes[0].address == "dev:/b");
    CHECK(r.changes[0].old_address == "dev:/a");
    CHECK(r.paths == std::vector<std::string>{"/a", "/b"});
  }
}
