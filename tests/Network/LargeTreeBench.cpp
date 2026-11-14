// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// The client of ossia (oscquery_mirror_protocol, what ossia.client builds)
// against a large OSCQuery tree, without Live: a synthetic tree of the size
// of a Live set of 64 tracks of 8 devices, served by a test server that takes
// the time a host takes to read its values.
//
// Scenarios:
//   tree   the size of the tree: leaves, containers, bytes of GET /.
//   read   one mirror reads the whole tree, with no bound: how long it takes.
//   loop   the connection of ossia.client to that device, by name or by URL
//          (oscquery_connect_attempt, then a new attempt one second after a
//          failure, as oscquery_directory and ossia.client do), for a while.
//   burst  a mirror connected to the base tree; the server adds one track
//          at a time and sends PATH_CHANGED /tracks after each, as a host
//          building a Live set does.
//
// The server counts the TCP connections it holds, the readings under way,
// the WebSockets; lsof counts the client's established connections to it.
// Everything runs on the loopback: no mDNS, no other application.

#include "client_mirror.hpp"
#include "LargeTreeCommon.hpp"

#include <mach/mach.h>

#include <fstream>
#include <map>
#include <sstream>

namespace
{
using namespace large_tree;

// The resident memory of this process, MB
double resident_mb()
{
  mach_task_basic_info info{};
  mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
  if(task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) != KERN_SUCCESS)
    return 0;
  return double(info.resident_size) / 1e6;
}

double percentile(std::vector<double> v, double p)
{
  if(v.empty())
    return 0;
  std::sort(v.begin(), v.end());
  const auto i = std::min(v.size() - 1, std::size_t(p * double(v.size() - 1) + 0.5));
  return v[i];
}

struct json_out
{
  std::ostringstream s;
  bool first = true;
  json_out() { s << "{"; }
  template <typename T>
  void kv(const char* k, const T& v)
  {
    s << (first ? "" : ",") << "\n  \"" << k << "\": " << v;
    first = false;
  }
  void ks(const char* k, const std::string& v)
  {
    std::string e;
    for(char c : v)
    {
      if(c == '"' || c == '\\')
        e += '\\';
      if(c == '\n')
      {
        e += "\\n";
        continue;
      }
      e += c;
    }
    s << (first ? "" : ",") << "\n  \"" << k << "\": \"" << e << "\"";
    first = false;
  }
  std::string str() { return s.str() + "\n}\n"; }
};

struct options
{
  std::string scenario;
  tree_shape shape;
  double host_ms_per_mb = 64;
  double seconds = 60;
  int interval_ms = 900;
  int retry_ms = ossia::max_binding::oscquery_directory::retry_ms;
  std::string json;
};

void write_json(const options& o, json_out& j)
{
  const auto text = j.str();
  std::fputs(text.c_str(), stdout);
  if(!o.json.empty())
  {
    std::ofstream f{o.json};
    f << text;
  }
}

void common(json_out& j, const options& o, const synthetic_tree& t)
{
  j.ks("scenario", o.scenario);
  j.kv("base_tracks", o.shape.base_tracks);
  j.kv("added_tracks", o.shape.tracks);
  j.kv("devices", o.shape.devices);
  j.kv("parameters", o.shape.parameters);
  j.kv("host_ms_per_mb", o.host_ms_per_mb);
  (void)t;
}

void server_stats(json_out& j, bench_server& srv, sampler& smp, const std::string& only = {})
{
  std::lock_guard lock{srv.stats_mutex};
  std::vector<double> secs;
  std::size_t max_bytes = 0;
  int sent = 0, cut = 0, n = 0;
  std::map<std::string, int> by_target;
  for(auto& r : srv.readings)
  {
    by_target[r.target]++;
    if(!only.empty() && r.target != only)
      continue;
    n++;
    secs.push_back(std::chrono::duration<double>(r.done - r.asked).count());
    max_bytes = std::max(max_bytes, r.bytes);
    (r.sent ? sent : cut)++;
  }
  std::string targets;
  for(auto& [t, c] : by_target)
    targets += (targets.empty() ? "" : ", ") + t + " " + std::to_string(c);
  j.ks("readings_by_target", targets);
  j.kv("readings", n);
  j.kv("readings_sent", sent);
  j.kv("readings_not_received", cut);
  j.kv("reading_seconds_p50", percentile(secs, 0.5));
  j.kv("reading_seconds_p95", percentile(secs, 0.95));
  j.kv("reading_seconds_max", percentile(secs, 1.0));
  j.kv("reading_mb_max", double(max_bytes) / 1e6);
  j.kv("readings_under_way_max", srv.readings_open_max.load());
  j.kv("server_tcp_connections_max", srv.tcp_open_max.load());
  j.kv("server_tcp_connections_total", srv.tcp_total.load());
  j.kv("server_http_connections_max", srv.http_open_max.load());
  j.kv("client_established_max_lsof", smp.lsof_max);
  j.kv("lsof_samples", smp.lsof_samples);
  j.kv("websockets_opened", int(srv.ws_records.size()));
}

// ---------------------------------------------------------------------------
int scenario_tree(const options& o)
{
  synthetic_tree tree{o.shape};
  std::string body;
  const auto t0 = clock_type::now();
  tree.json("/", body);
  json_out j;
  common(j, o, tree);
  j.kv("tracks", tree.present.load());
  j.kv("leaves", tree.leaves());
  j.kv("containers", tree.containers());
  j.kv("bytes", body.size());
  j.kv("mb", double(body.size()) / 1e6);
  j.kv("write_seconds", seconds_since(t0));
  write_json(o, j);
  return 0;
}

// One mirror, one reading of the whole tree, waited for without bound: what
// update() would need instead of its 3 s.
int scenario_read(const options& o)
{
  synthetic_tree tree{o.shape};
  bench_server srv{tree, o.host_ms_per_mb};
  sampler smp{srv};
  json_out j;
  common(j, o, tree);
  j.kv("leaves_served", tree.leaves());

  const double rss0 = resident_mb();
  const auto t0 = clock_type::now();
  auto proto = std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
      srv.url(), bench_server::free_udp_port());
  auto* mirror = proto.get();
  auto dev = std::make_unique<ossia::net::generic_device>(std::move(proto), "large_tree");
  const double ws_s = seconds_since(t0);
  auto fut = mirror->update_async(dev->get_root_node());
  const bool ready = fut.wait_for(std::chrono::duration<double>(o.seconds)) == std::future_status::ready;
  const double read_s = seconds_since(t0);
  const bool within_3s = read_s - ws_s <= 3.0;
  const long got = count_leaves(dev->get_root_node());
  const double rss1 = resident_mb();
  const auto t1 = clock_type::now();
  dev.reset();
  const double destroy_s = seconds_since(t1);
  smp.stop();
  srv.stop();

  j.kv("websocket_seconds", ws_s);
  j.kv("tree_read", ready ? "true" : "false");
  j.kv("tree_read_seconds_after_websocket", read_s - ws_s);
  j.kv("within_update_bound_3s", within_3s ? "true" : "false");
  j.kv("leaves_in_mirror", got);
  j.kv("client_resident_mb_before", rss0);
  j.kv("client_resident_mb_with_tree", rss1);
  j.kv("mirror_destroy_seconds", destroy_s);
  server_stats(j, srv, smp);
  write_json(o, j);
  return 0;
}

// The connection of ossia.client by name (oscquery_directory) or by URL
// (client.cpp with @reconnect 1): an oscquery_connect_attempt, polled every
// 10 ms; a failure, then a new attempt retry_ms later; a success, then the
// mirror polled while its WebSocket is up.
int scenario_loop(const options& o)
{
  synthetic_tree tree{o.shape};
  bench_server srv{tree, o.host_ms_per_mb};
  sampler smp{srv};
  json_out j;
  common(j, o, tree);
  j.kv("leaves_served", tree.leaves());
  j.kv("retry_ms", o.retry_ms);

  struct attempt_record
  {
    double start, end;
    bool ok;
    std::string error;
    double connected_for{};
  };
  std::vector<attempt_record> attempts;
  const auto t0 = clock_type::now();
  double connected_total = 0, longest = 0;
  while(seconds_since(t0) < o.seconds)
  {
    attempt_record a{seconds_since(t0), 0, false, {}};
    auto attempt = std::make_unique<ossia::max_binding::oscquery_connect_attempt>(
        srv.url(), "large_tree", "bench_client");
    while(!attempt->done())
      std::this_thread::sleep_for(10ms);
    auto dev = attempt->take();
    a.end = seconds_since(t0);
    a.ok = bool(dev);
    if(!dev)
      a.error = attempt->error();
    attempt.reset();
    if(dev)
    {
      auto* mirror = ossia::max_binding::polled_oscquery_mirror(*dev);
      const auto c0 = clock_type::now();
      while(seconds_since(t0) < o.seconds && mirror->connected())
      {
        mirror->run_commands();
        std::this_thread::sleep_for(10ms);
      }
      a.connected_for = seconds_since(c0);
      connected_total += a.connected_for;
      longest = std::max(longest, a.connected_for);
      dev.reset();
    }
    attempts.push_back(a);
    std::printf("attempt %zu: %.2f s to %.2f s, %s%s%s\n", attempts.size(), a.start, a.end,
                a.ok ? "connected" : "failed", a.ok ? "" : ": ", a.error.c_str());
    std::fflush(stdout);
    if(seconds_since(t0) >= o.seconds)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(o.retry_ms));
  }
  smp.stop();
  srv.stop();

  int ok = 0;
  std::vector<double> durations, ws_lives, periods;
  for(std::size_t i = 0; i < attempts.size(); i++)
  {
    ok += attempts[i].ok;
    durations.push_back(attempts[i].end - attempts[i].start);
    if(i)
      periods.push_back(attempts[i].start - attempts[i - 1].start);
  }
  {
    std::lock_guard lock{srv.stats_mutex};
    for(auto& w : srv.ws_records)
      if(w.is_closed)
        ws_lives.push_back(std::chrono::duration<double>(w.closed - w.opened).count());
  }
  j.kv("attempts", int(attempts.size()));
  j.kv("attempts_connected", ok);
  j.ks("first_error", attempts.empty() ? std::string{} : attempts.front().error);
  j.kv("attempt_seconds_p50", percentile(durations, 0.5));
  j.kv("attempt_seconds_max", percentile(durations, 1.0));
  j.kv("period_seconds_p50", percentile(periods, 0.5));
  j.kv("websocket_life_seconds_p50", percentile(ws_lives, 0.5));
  j.kv("websocket_life_seconds_max", percentile(ws_lives, 1.0));
  j.kv("connected_seconds_total", connected_total);
  j.kv("connected_seconds_longest", longest);
  j.kv("run_seconds", seconds_since(t0));
  server_stats(j, srv, smp, "/");
  write_json(o, j);
  return 0;
}

// A mirror connected to the base tree; the server adds one track every
// interval_ms and sends PATH_CHANGED /tracks after each.
int scenario_burst(const options& o)
{
  synthetic_tree tree{o.shape};
  tree.present = o.shape.base_tracks;
  bench_server srv{tree, o.host_ms_per_mb};
  sampler smp{srv};
  json_out j;
  common(j, o, tree);
  j.kv("interval_ms", o.interval_ms);

  auto proto = std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(
      srv.url(), bench_server::free_udp_port());
  auto* mirror = proto.get();
  auto dev = std::make_shared<ossia::net::generic_device>(std::move(proto), "large_tree");
  ossia::max_binding::polled_oscquery_mirror(*dev);
  const bool updated = mirror->update(dev->get_root_node());
  j.kv("base_tree_read_within_3s", updated ? "true" : "false");
  j.kv("base_leaves_in_mirror", count_leaves(dev->get_root_node()));
  if(!updated)
  {
    write_json(o, j);
    return 1;
  }
  std::atomic_bool polling{true};
  std::thread poller([&] {
    while(polling)
    {
      mirror->run_commands();
      std::this_thread::sleep_for(10ms);
    }
  });
  std::this_thread::sleep_for(500ms); // the WebSocket is up

  const auto b0 = clock_type::now();
  int sent = 0;
  for(int i = 0; i < o.shape.tracks; i++)
  {
    tree.present++;
    srv.path_changed("/tracks");
    sent++;
    std::this_thread::sleep_for(std::chrono::milliseconds(o.interval_ms));
  }
  const double build_s = seconds_since(b0);
  // Until no reading is under way and none came for 2 s, two minutes at most
  const auto w0 = clock_type::now();
  std::size_t last = std::size_t(-1);
  auto quiet_since = clock_type::now();
  for(;;)
  {
    std::this_thread::sleep_for(200ms);
    std::size_t n;
    {
      std::lock_guard lock{srv.stats_mutex};
      n = srv.readings.size();
    }
    if(n != last || srv.readings_open != 0)
    {
      last = n;
      quiet_since = clock_type::now();
    }
    if(seconds_since(quiet_since) >= 2 || seconds_since(w0) > 120)
      break;
  }
  std::this_thread::sleep_for(1s); // the last tree applied by the poller
  const double drain_s = seconds_since(w0);
  polling = false;
  poller.join();
  long tracks_in_mirror = 0;
  if(auto n = ossia::net::find_node(dev->get_root_node(), "/tracks"))
    tracks_in_mirror = long(n->children().size());
  const long leaves = count_leaves(dev->get_root_node());
  const bool connected = mirror->connected();
  smp.stop();
  dev.reset();
  srv.stop();

  j.kv("path_changed_sent", sent);
  j.kv("build_seconds", build_s);
  j.kv("drain_seconds", drain_s);
  j.kv("tracks_served", tree.present.load());
  j.kv("tracks_in_mirror", tracks_in_mirror);
  j.kv("leaves_served", tree.leaves());
  j.kv("leaves_in_mirror", leaves);
  j.kv("websocket_still_up", connected ? "true" : "false");
  server_stats(j, srv, smp, "/tracks");
  write_json(o, j);
  return 0;
}

void usage()
{
  std::puts(
      "ossia_LargeTreeBench tree|read|loop|burst [options]\n"
      "  --tracks N           tracks added by the build (64)\n"
      "  --base-tracks N      tracks of the base set (4)\n"
      "  --devices N          devices per track (8)\n"
      "  --parameters N       parameters per device (48)\n"
      "  --host-ms-per-mb X   the host's time per MB answered, 0 for none (64)\n"
      "  --seconds S          loop: how long; read: the longest wait (60)\n"
      "  --interval-ms N      burst: between two tracks (900)\n"
      "  --retry-ms N         loop: between a failure and the next attempt (1000)\n"
      "  --json FILE          also write the result there");
}
}

int main(int argc, char** argv)
{
  if(argc < 2)
  {
    usage();
    return 2;
  }
  options o;
  o.scenario = argv[1];
  for(int i = 2; i + 1 < argc; i += 2)
  {
    const std::string k = argv[i];
    const char* v = argv[i + 1];
    if(k == "--tracks")
      o.shape.tracks = std::atoi(v);
    else if(k == "--base-tracks")
      o.shape.base_tracks = std::atoi(v);
    else if(k == "--devices")
      o.shape.devices = std::atoi(v);
    else if(k == "--parameters")
      o.shape.parameters = std::atoi(v);
    else if(k == "--host-ms-per-mb")
      o.host_ms_per_mb = std::atof(v);
    else if(k == "--seconds")
      o.seconds = std::atof(v);
    else if(k == "--interval-ms")
      o.interval_ms = std::atoi(v);
    else if(k == "--retry-ms")
      o.retry_ms = std::atoi(v);
    else if(k == "--json")
      o.json = v;
    else
    {
      usage();
      return 2;
    }
  }
  if(o.scenario == "tree")
    return scenario_tree(o);
  if(o.scenario == "read")
    return scenario_read(o);
  if(o.scenario == "loop")
    return scenario_loop(o);
  if(o.scenario == "burst")
    return scenario_burst(o);
  usage();
  return 2;
}
