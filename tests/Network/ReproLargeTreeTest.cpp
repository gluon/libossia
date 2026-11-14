// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// The client of ossia (oscquery_mirror_protocol, what ossia.client and the
// mDNS directory of ossia-max build) against a server that takes its time:
// a burst of PATH_CHANGED, and a tree read in more than 3 s. The server is the
// one of the large tree bench (LargeTreeCommon.hpp), on the loopback.

#include "client_mirror.hpp"
#include "LargeTreeCommon.hpp"

#include "include_catch.hpp"

#include <functional>

using namespace large_tree;

namespace
{
// A mirror polled as ossia-max polls it
struct polled_mirror
{
  std::unique_ptr<ossia::net::generic_device> dev;
  ossia::oscquery::oscquery_mirror_protocol* mirror{};
  std::atomic_bool polling{true};
  std::thread poller;
  bool updated{};

  explicit polled_mirror(const std::string& url)
  {
    auto p = std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
        url, bench_server::free_udp_port());
    mirror = p.get();
    dev = std::make_unique<ossia::net::generic_device>(std::move(p), "large_tree");
    // Read before anything polls it, as oscquery_connect_attempt does: once
    // polled, the tree read again waits for a poll
    updated = mirror->update(dev->get_root_node());
    ossia::max_binding::polled_oscquery_mirror(*dev);
    poller = std::thread([this] {
      while(polling)
      {
        mirror->run_commands();
        std::this_thread::sleep_for(5ms);
      }
    });
  }
  ~polled_mirror()
  {
    polling = false;
    poller.join();
    dev.reset();
  }
  long tracks()
  {
    auto n = ossia::net::find_node(dev->get_root_node(), "/tracks");
    return n ? long(n->children().size()) : 0;
  }
};

// Until the server has no reading under way and done() holds, ms at most
bool settle(bench_server& s, const std::function<bool()>& done, int ms)
{
  for(int t = 0; t < ms; t += 50)
  {
    std::this_thread::sleep_for(50ms);
    if(s.readings_open == 0 && done())
    {
      // nothing more comes
      std::this_thread::sleep_for(300ms);
      if(s.readings_open == 0 && done())
        return true;
    }
  }
  return false;
}

tree_shape small_shape()
{
  tree_shape sh;
  sh.base_tracks = 2;
  sh.tracks = 10;
  sh.devices = 1;
  sh.parameters = 2;
  return sh;
}
}

// Proposal, line 158: at PATH_CHANGED the client asks for the structure of
// that address. While a reading of a path is under way, a PATH_CHANGED of the
// same path gives one more reading, after it; a reading of an ancestor covers
// the path only if it was asked after the PATH_CHANGED, else one more reading
// of the ancestor is queued. At most two HTTP connections at a time, one for
// the tree, one for the values.
TEST_CASE("Mirror: a burst of PATH_CHANGED on a slow server", "[path-changed-burst]")
{
  synthetic_tree tree{small_shape()};
  tree.present = tree.shape.base_tracks;
  bench_server s{tree, 0};
  s.host_ms_per_tree_reading = 400;
  polled_mirror m{s.url()};
  REQUIRE(m.updated);
  REQUIRE(m.tracks() == 2);

  auto is_tracks = [](const reading& r) { return r.target == "/tracks"; };
  auto all_done = [] { return true; };

  SECTION("Ten PATH_CHANGED during one reading: one reading more")
  {
    for(int i = 0; i < 10; i++)
    {
      tree.present++;
      s.path_changed("/tracks");
      std::this_thread::sleep_for(10ms);
    }
    const bool settled = settle(s, [&] { return m.tracks() == 12; }, 15000);
    std::printf("path-changed-burst: 10 PATH_CHANGED, readings of /tracks %d, at once %d, "
                "tracks %ld, settled %d\n",
                s.count("/tracks"), s.overlap(is_tracks), m.tracks(), int(settled));
    CHECK(settled);
    CHECK(s.count("/tracks") == 2);
    CHECK(s.overlap(is_tracks) == 1);
    CHECK(m.tracks() == 12);
  }

  SECTION("An ancestor's reading asked before the change: the ancestor read again")
  {
    s.path_changed("/tracks");
    std::this_thread::sleep_for(100ms); // under way
    s.path_changed("/tracks/1");
    std::this_thread::sleep_for(20ms);
    s.path_changed("/tracks/0");
    const bool settled = settle(s, all_done, 15000);
    std::printf("path-changed-burst: ancestor under way: /tracks %d, /tracks/1 %d, /tracks/0 %d\n",
                s.count("/tracks"), s.count("/tracks/1"), s.count("/tracks/0"));
    CHECK(settled);
    CHECK(s.count("/tracks") == 2);
    CHECK(s.count("/tracks/1") == 0);
    CHECK(s.count("/tracks/0") == 0);
  }

  SECTION("An ancestor's reading asked after the change covers the path")
  {
    s.path_changed("/tracks/1");
    std::this_thread::sleep_for(100ms); // under way
    s.path_changed("/tracks/0");        // waits
    std::this_thread::sleep_for(20ms);
    s.path_changed("/tracks");          // waits, and covers /tracks/0
    std::this_thread::sleep_for(20ms);
    s.path_changed("/tracks/1");        // covered by /tracks, asked after
    const bool settled = settle(s, all_done, 15000);
    std::printf("path-changed-burst: ancestor waiting: /tracks/1 %d, /tracks/0 %d, /tracks %d\n",
                s.count("/tracks/1"), s.count("/tracks/0"), s.count("/tracks"));
    CHECK(settled);
    CHECK(s.count("/tracks/1") == 1);
    CHECK(s.count("/tracks/0") == 0);
    CHECK(s.count("/tracks") == 1);
  }

  SECTION("At most two HTTP connections, one for the tree, one for the values")
  {
    auto node = ossia::net::find_node(m.dev->get_root_node(), "/tracks/0/volume");
    REQUIRE(node);
    REQUIRE(node->get_parameter());
    for(int i = 0; i < 10; i++)
    {
      tree.present++;
      s.path_changed("/tracks");
      for(int k = 0; k < 5; k++)
        m.mirror->request(*node->get_parameter());
      std::this_thread::sleep_for(10ms);
    }
    auto values = [&] {
      std::lock_guard lock{s.stats_mutex};
      return int(std::count_if(s.readings.begin(), s.readings.end(), [](const reading& r) {
        return r.target.find("?VALUE") != std::string::npos;
      }));
    };
    const bool settled = settle(s, [&] { return values() == 50 && m.tracks() == 12; }, 20000);
    const int tree_at_once = s.overlap([](const reading& r) { return r.tree(); });
    const int values_at_once = s.overlap(
        [](const reading& r) { return r.target.find("?VALUE") != std::string::npos; });
    std::printf("path-changed-burst: HTTP connections at once %d, tree readings at once %d, "
                "value reads at once %d, value reads %d, tracks %ld\n",
                s.http_open_max.load(), tree_at_once, values_at_once, values(), m.tracks());
    CHECK(settled);
    CHECK(s.http_open_max <= 2);
    CHECK(tree_at_once == 1);
    CHECK(values_at_once == 1);
    CHECK(values() == 50);
    CHECK(m.tracks() == 12);
  }
}

// A polled mirror (ossia-max) is read and changed by the thread that polls it:
// every change of the tree a server sends is applied by run_commands(). The
// tree read again (update(), update_async(): the "update" message of
// ossia.client, a connection by URL) must be applied there too, not on the
// HTTP thread while the polling thread walks the tree. Run it under TSan and
// ASan.
TEST_CASE("Mirror: the tree read again while the polling thread walks it", "[namespace-reread]")
{
  tree_shape sh;
  sh.base_tracks = 16;
  sh.tracks = 0;
  synthetic_tree tree{sh};
  bench_server s{tree, 0};
  auto p = std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
      s.url(), bench_server::free_udp_port());
  auto* mirror = p.get();
  auto dev = std::make_unique<ossia::net::generic_device>(std::move(p), "large_tree");
  auto& root = dev->get_root_node();
  // Read once before anything polls it, as oscquery_connect_attempt does
  REQUIRE(mirror->update(root));
  ossia::max_binding::polled_oscquery_mirror(*dev);

  std::atomic_bool stop{false};
  std::atomic_long walks{0};
  std::size_t names = 0;
  std::thread owner([&] {
    while(!stop)
    {
      mirror->run_commands();
      for(auto* n : ossia::net::list_all_children(&root))
      {
        names += n->get_name().size();
        if(auto* param = n->get_parameter())
          (void)param->value();
      }
      walks++;
    }
  });

  int answered = 0;
  for(int i = 0; i < 5; i++)
  {
    auto fut = mirror->update_async(root);
    if(fut.wait_for(std::chrono::seconds(20)) == std::future_status::ready)
    {
      try
      {
        fut.get();
        answered++;
      }
      catch(...)
      {
      }
    }
  }
  stop = true;
  owner.join();
  const long leaves = count_leaves(root);
  std::printf("namespace-reread: %d of 5 readings applied, %ld walks of the tree, leaves %ld of %ld\n",
              answered, walks.load(), leaves, tree.leaves());
  CHECK(answered == 5);
  CHECK(leaves == tree.leaves());
}

// The polling thread is Max's main thread: a large reading is applied there
// in slices of the budget, not whole (a reading of the tracks of a Live set of
// 64 tracks takes 110 to 120 ms whole, the tree read again 300 ms).
TEST_CASE("Mirror: a polled mirror applies a large reading in slices", "[sliced-apply]")
{
  tree_shape sh;
  sh.base_tracks = 16;
  sh.tracks = 16;
  synthetic_tree tree{sh};
  tree.present = sh.base_tracks;
  bench_server s{tree, 0};
  auto p = std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
      s.url(), bench_server::free_udp_port());
  auto* mirror = p.get();
  auto dev = std::make_unique<ossia::net::generic_device>(std::move(p), "large_tree");
  auto& root = dev->get_root_node();
  REQUIRE(mirror->update(root));
  ossia::max_binding::polled_oscquery_mirror(*dev);
  auto tracks = [&] {
    auto n = ossia::net::find_node(root, "/tracks");
    return n ? long(n->children().size()) : 0;
  };

  double longest_ms = 0;
  int calls_left_work = 0;
  auto poll_until = [&](const std::function<bool()>& done) {
    const auto t0 = clock_type::now();
    while(seconds_since(t0) < 30)
    {
      const auto c0 = clock_type::now();
      const bool more = mirror->run_commands_for(std::chrono::milliseconds(10));
      longest_ms = std::max(
          longest_ms, std::chrono::duration<double, std::milli>(clock_type::now() - c0).count());
      if(more)
        calls_left_work++;
      else if(done())
        return true;
      else
        std::this_thread::sleep_for(2ms);
    }
    return false;
  };

  tree.present = sh.base_tracks + sh.tracks;
  s.path_changed("/tracks");
  const bool read = poll_until([&] { return tracks() == tree.present; });
  const int reading_calls = calls_left_work;
  auto fut = mirror->update_async(root);
  const bool reread = poll_until(
      [&] { return fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready; });
  const long leaves = count_leaves(root);
  std::printf("sliced-apply: tracks reading applied %d, in %d calls that left work; tree read again "
              "%d; longest call %.1f ms; leaves %ld of %ld\n",
              int(read), reading_calls, int(reread), longest_ms, leaves, tree.leaves());
  CHECK(read);
  CHECK(reread);
  CHECK(reading_calls >= 2);
  CHECK(longest_ms < 40);
  CHECK(leaves == tree.leaves());
}

// The connection of ossia-max (oscquery_connect_attempt, by name or by URL)
// to a server that takes more than 3 s to give its tree, as Live does with a
// set of 64 tracks: it connects. A tree that fails ends the attempt at once;
// a reading given up is closed.
TEST_CASE("Mirror: a tree read in more than 3 s", "[large-tree]")
{
  tree_shape sh;
  sh.base_tracks = 16;
  sh.tracks = 0;
  synthetic_tree tree{sh};
  bench_server s{tree, 0};

  SECTION("The attempt connects, then holds")
  {
    s.host_ms_per_tree_reading = 3500;
    const auto t0 = clock_type::now();
    ossia::max_binding::oscquery_connect_attempt a{s.url(), "large_tree", "judge"};
    while(!a.done() && seconds_since(t0) < 60)
      std::this_thread::sleep_for(10ms);
    auto dev = a.take();
    const double took = seconds_since(t0);
    long leaves = dev ? count_leaves(dev->get_root_node()) : 0;
    bool held = false;
    if(dev)
    {
      auto* mirror = ossia::max_binding::polled_oscquery_mirror(*dev);
      const auto c0 = clock_type::now();
      while(mirror->connected() && seconds_since(c0) < 2)
      {
        mirror->run_commands();
        std::this_thread::sleep_for(10ms);
      }
      held = mirror->connected();
    }
    const int client = lsof_client_connections(s.port);
    std::printf("large-tree: connected %d in %.2f s (%s), leaves %ld of %ld, held 2 s %d, "
                "websockets %zu, client connections %d\n",
                int(bool(dev)), took, a.error().c_str(), leaves, tree.leaves(), int(held),
                s.ws_records.size(), client);
    REQUIRE(dev);
    CHECK(leaves == tree.leaves());
    CHECK(held);
    CHECK(s.ws_records.size() == 1);
    CHECK(client == 1); // the WebSocket
  }

  SECTION("A tree that fails ends the attempt at once")
  {
    s.root_status = 500;
    const auto t0 = clock_type::now();
    ossia::max_binding::oscquery_connect_attempt a{s.url(), "large_tree", "judge"};
    while(!a.done() && seconds_since(t0) < 60)
      std::this_thread::sleep_for(10ms);
    const double took = seconds_since(t0);
    auto dev = a.take();
    std::printf("large-tree: GET / answered 500: connected %d, attempt %.2f s (%s)\n",
                int(bool(dev)), took, a.error().c_str());
    CHECK(!dev);
    CHECK(took < 1.5);
  }

  SECTION("A reading given up is closed")
  {
    s.hold_root = true;
    auto p = std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
        s.url(), bench_server::free_udp_port());
    auto* mirror = p.get();
    auto dev = std::make_unique<ossia::net::generic_device>(std::move(p), "large_tree");
    auto fut = mirror->update_async(dev->get_root_node());
    std::this_thread::sleep_for(500ms);
    const int during = lsof_client_connections(s.port);
    dev.reset();
    std::this_thread::sleep_for(300ms);
    const int after = lsof_client_connections(s.port);
    std::printf("large-tree: tree never answered: client connections %d, after the mirror %d\n",
                during, after);
    CHECK(during == 2); // the WebSocket, the reading
    CHECK(after == 0);
  }
}
