// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

#pragma once

// A synthetic OSCQuery tree the size of a Live set of 64 tracks of 8 devices,
// and a test server on the loopback that serves it, taking a host's time per
// answer. Shared by the large tree bench (LargeTreeBench.cpp) and the cases
// [path-changed-burst] and [large-tree] (ReproLargeTreeTest.cpp).


#include <ossia/network/base/node_functions.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/oscquery/oscquery_mirror.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>


namespace large_tree
{
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using clock_type = std::chrono::steady_clock;
using namespace std::chrono_literals;

inline double seconds_since(clock_type::time_point t0, clock_type::time_point t = clock_type::now())
{
  return std::chrono::duration<double>(t - t0).count();
}

// ---------------------------------------------------------------------------
// The tree: /tracks/<t>/devices/<d>/parameters/<p>/<leaf>, with leaves on
// tracks and devices too. The default shape is that of LiveOSCQuery's load
// set (docs/phase3.md: 25,534 containers, 340,414 leaves, 47.4 MB): 4 tracks
// for the base set, 64 added by the build, 8 devices of 48 parameters.
struct leaf
{
  const char* name;
  const char* type;
  const char* value; // JSON; "%" is replaced by the index of the owner
  const char* range; // JSON or nullptr
};

inline const leaf parameter_leaves[] = {
    {"value", "f", "0.5", "[{\"MIN\":0.0,\"MAX\":1.0}]"},
    {"min", "f", "0.0", nullptr},
    {"max", "f", "1.0", nullptr},
    {"default_value", "f", "0.5", "[{\"MIN\":0.0,\"MAX\":1.0}]"},
    {"is_enabled", "T", "true", nullptr},
    {"is_quantized", "F", "false", nullptr},
    {"automation_state", "i", "0", "[{\"MIN\":0,\"MAX\":2}]"},
    {"state", "i", "0", "[{\"MIN\":0,\"MAX\":2}]"},
    {"name", "s", "\"Parameter %\"", nullptr},
    {"original_name", "s", "\"Parameter %\"", nullptr},
    {"display_value", "s", "\"0.50\"", nullptr},
    {"value_items", "s", "\"\"", nullptr},
    {"short_value_items", "s", "\"\"", nullptr},
};
inline const leaf device_leaves[] = {
    {"name", "s", "\"Device %\"", nullptr},
    {"class_name", "s", "\"MidiPitcher\"", nullptr},
    {"is_active", "T", "true", nullptr},
    {"type", "i", "1", "[{\"MIN\":0,\"MAX\":4}]"},
};
inline const leaf track_leaves[] = {
    {"name", "s", "\"Track %\"", nullptr},
    {"color", "i", "16149507", nullptr},
    {"mute", "F", "false", nullptr},
    {"solo", "F", "false", nullptr},
    {"arm", "F", "false", nullptr},
    {"volume", "f", "0.85", "[{\"MIN\":0.0,\"MAX\":1.0}]"},
    {"panning", "f", "0.0", "[{\"MIN\":-1.0,\"MAX\":1.0}]"},
    {"send_a", "f", "0.0", "[{\"MIN\":0.0,\"MAX\":1.0}]"},
    {"send_b", "f", "0.0", "[{\"MIN\":0.0,\"MAX\":1.0}]"},
    {"is_foldable", "F", "false", nullptr},
    {"fold_state", "i", "0", "[{\"MIN\":0,\"MAX\":1}]"},
    {"is_visible", "T", "true", nullptr},
    {"has_midi_input", "T", "true", nullptr},
    {"has_audio_output", "T", "true", nullptr},
    {"output_meter_level", "f", "0.0", "[{\"MIN\":0.0,\"MAX\":1.0}]"},
    {"playing_slot_index", "i", "-1", nullptr},
    {"fired_slot_index", "i", "-1", nullptr},
    {"input_routing", "s", "\"All Ins\"", nullptr},
    {"output_routing", "s", "\"Master\"", nullptr},
    {"current_monitoring_state", "i", "1", "[{\"MIN\":0,\"MAX\":2}]"},
};

struct tree_shape
{
  int base_tracks = 4;
  int tracks = 64; // added by the build
  int devices = 8;
  int parameters = 48;

  long leaves_per_track() const
  {
    return std::size(track_leaves)
           + devices * (long(std::size(device_leaves)) + parameters * long(std::size(parameter_leaves)));
  }
  long containers_per_track() const { return 2 + devices * (2 + parameters); }
};

class synthetic_tree
{
public:
  explicit synthetic_tree(tree_shape s)
      : shape{s}
      , present{s.base_tracks + s.tracks}
  {
  }

  tree_shape shape;
  std::atomic<int> present; // tracks there now

  long leaves() const { return present * shape.leaves_per_track(); }
  long containers() const { return 2 + present * shape.containers_per_track(); }

  // The JSON of a path, or false when there is nothing there.
  bool json(const std::string& path, std::string& out) const
  {
    out.clear();
    const int n = present;
    if(path == "/")
    {
      out.reserve(std::size_t(n) * 720000);
      out += "{\"FULL_PATH\":\"/\",\"ACCESS\":0,\"CONTENTS\":{\"tracks\":";
      tracks(out, n);
      out += "}}";
      return true;
    }
    if(path == "/tracks")
    {
      out.reserve(std::size_t(n) * 720000);
      tracks(out, n);
      return true;
    }
    if(path.rfind("/tracks/", 0) == 0)
    {
      const std::string rest = path.substr(8);
      if(rest.empty() || rest.find('/') != std::string::npos)
        return false;
      const int t = std::atoi(rest.c_str());
      if(std::to_string(t) != rest || t < 0 || t >= n)
        return false;
      track(out, t);
      return true;
    }
    return false;
  }

private:
  static void leaves_of(std::string& out, const std::string& prefix, const leaf* begin,
                        const leaf* end, int index, bool& first)
  {
    const std::string idx = std::to_string(index);
    for(auto l = begin; l != end; ++l)
    {
      if(!first)
        out += ',';
      first = false;
      out += '"';
      out += l->name;
      out += "\":{\"FULL_PATH\":\"";
      out += prefix;
      out += l->name;
      out += "\",\"TYPE\":\"";
      out += l->type;
      out += "\",\"VALUE\":";
      for(const char* c = l->value; *c; ++c)
      {
        if(*c == '%')
          out += idx;
        else
          out += *c;
      }
      out += ",\"ACCESS\":3";
      if(l->type[0] != 's')
        out += ",\"CLIPMODE\":\"none\"";
      if(l->range)
      {
        out += ",\"RANGE\":";
        out += l->range;
      }
      out += '}';
    }
  }

  void tracks(std::string& out, int n) const
  {
    out += "{\"FULL_PATH\":\"/tracks\",\"ACCESS\":0,\"CONTENTS\":{";
    for(int t = 0; t < n; t++)
    {
      if(t)
        out += ',';
      out += '"';
      out += std::to_string(t);
      out += "\":";
      track(out, t);
    }
    out += "}}";
  }

  void track(std::string& out, int t) const
  {
    const std::string tp = "/tracks/" + std::to_string(t);
    out += "{\"FULL_PATH\":\"" + tp + "\",\"ACCESS\":0,\"CONTENTS\":{";
    bool first = true;
    leaves_of(out, tp + "/", std::begin(track_leaves), std::end(track_leaves), t, first);
    out += ",\"devices\":{\"FULL_PATH\":\"" + tp + "/devices\",\"ACCESS\":0,\"CONTENTS\":{";
    for(int d = 0; d < shape.devices; d++)
    {
      const std::string dp = tp + "/devices/" + std::to_string(d);
      if(d)
        out += ',';
      out += "\"" + std::to_string(d) + "\":{\"FULL_PATH\":\"" + dp
             + "\",\"ACCESS\":0,\"CONTENTS\":{";
      bool f = true;
      leaves_of(out, dp + "/", std::begin(device_leaves), std::end(device_leaves), d, f);
      out += ",\"parameters\":{\"FULL_PATH\":\"" + dp + "/parameters\",\"ACCESS\":0,\"CONTENTS\":{";
      for(int p = 0; p < shape.parameters; p++)
      {
        const std::string pp = dp + "/parameters/" + std::to_string(p);
        if(p)
          out += ',';
        out += "\"" + std::to_string(p) + "\":{\"FULL_PATH\":\"" + pp
               + "\",\"ACCESS\":0,\"CONTENTS\":{";
        bool g = true;
        leaves_of(out, pp + "/", std::begin(parameter_leaves), std::end(parameter_leaves), p, g);
        out += "}}";
      }
      out += "}}}}";
    }
    out += "}}}}";
  }
};

// ---------------------------------------------------------------------------
// The server. A network thread for the sockets, and one host thread that
// answers the readings one after the other, each taking host_ms_per_mb for
// each MB it answers: Live reads its values on its own thread, one query at
// a time (LiveOSCQuery, docs/phase3.md: a full read of 47 MB takes 2.8 to
// 3.1 s, about 64 ms per MB). 0: answered as fast as the tree is written.
// The cases of the judge also give each reading of the tree a fixed time
// (host_ms_per_tree_reading), make GET / fail (root_status) or never answer
// it (hold_root). A ?VALUE under a track is answered {"VALUE":0.5}, or with
// distinct_values the value of /tracks/t/devices/d/parameters/p/value is
// 1 + 10000 t + 100 d + p; each ?VALUE takes value_ms on the host thread.
struct reading
{
  std::string target;
  clock_type::time_point asked, served, done;
  std::size_t bytes{};
  bool sent{};
  // A reading of the tree: no query
  bool tree() const { return target.find('?') == std::string::npos; }
};

struct ws_record
{
  clock_type::time_point opened, closed;
  bool is_closed{};
};

class bench_server
{
public:
  struct tcp_token
  {
    bench_server& s;
    explicit tcp_token(bench_server& srv)
        : s{srv}
    {
      const int n = ++s.tcp_open;
      ++s.tcp_total;
      int m = s.tcp_open_max;
      while(n > m && !s.tcp_open_max.compare_exchange_weak(m, n))
        ;
    }
    ~tcp_token() { --s.tcp_open; }
  };

  class ws_session;
  class http_session;

  bench_server(synthetic_tree& t, double ms_per_mb)
      : tree{t}
      , host_ms_per_mb{ms_per_mb}
      , acceptor{ioc}
      , osc_port{free_udp_port()}
  {
    tcp::endpoint ep{asio::ip::make_address("127.0.0.1"), 0};
    acceptor.open(ep.protocol());
    acceptor.set_option(asio::socket_base::reuse_address(true));
    acceptor.bind(ep);
    acceptor.listen(asio::socket_base::max_listen_connections);
    port = acceptor.local_endpoint().port();
    start = clock_type::now();
    accept();
    net = std::thread([this] { ioc.run(); });
    host = std::thread([this] { host_loop(); });
  }

  ~bench_server() { stop(); }

  void stop();

  std::string url() const { return "ws://127.0.0.1:" + std::to_string(port); }

  void path_changed(const std::string& path);
  void path_removed(const std::string& path);

  // Set before the client connects
  double host_ms_per_tree_reading = 0;
  unsigned root_status = 200;
  std::atomic_bool hold_root{false};
  std::atomic_bool distinct_values{false};
  std::atomic<double> value_ms{0};

  // Statistics
  std::atomic<int> tcp_open{0}, tcp_open_max{0}, tcp_total{0};
  // TCP connections that carry HTTP (not upgraded to a WebSocket)
  std::atomic<int> http_open{0}, http_open_max{0};
  std::atomic<int> readings_open{0}, readings_open_max{0};
  std::atomic<int> ws_open{0};
  std::mutex stats_mutex;
  std::vector<reading> readings;
  std::vector<ws_record> ws_records;
  clock_type::time_point start;

  // The readings of a target, and how many of the readings that match were
  // under way at once, at most
  // The readings that match, asked at t0 or after
  int count_if(const std::function<bool(const reading&)>& match,
               clock_type::time_point t0 = {})
  {
    std::lock_guard lock{stats_mutex};
    return int(std::count_if(readings.begin(), readings.end(),
                             [&](const reading& r) { return r.asked >= t0 && match(r); }));
  }
  int count(const std::string& target)
  {
    std::lock_guard lock{stats_mutex};
    return int(std::count_if(readings.begin(), readings.end(),
                             [&](const reading& r) { return r.target == target; }));
  }
  int overlap(const std::function<bool(const reading&)>& match)
  {
    std::vector<std::pair<clock_type::time_point, int>> ev;
    {
      std::lock_guard lock{stats_mutex};
      for(auto& r : readings)
        if(match(r))
        {
          ev.emplace_back(r.asked, 1);
          ev.emplace_back(r.done, -1);
        }
    }
    std::sort(ev.begin(), ev.end(), [](auto& a, auto& b) {
      return a.first < b.first || (a.first == b.first && a.second < b.second);
    });
    int n = 0, m = 0;
    for(auto& e : ev)
      m = std::max(m, n += e.second);
    return m;
  }

  synthetic_tree& tree;
  const double host_ms_per_mb;
  uint16_t port{};

private:
  friend class http_session;
  friend class ws_session;

public:
  static uint16_t free_udp_port()
  {
    asio::io_context c;
    asio::ip::udp::socket s{c, asio::ip::udp::endpoint{asio::ip::udp::v4(), 0}};
    return s.local_endpoint().port();
  }

private:
  void accept();
  void host_loop();
  void enqueue(std::function<void()> job)
  {
    {
      std::lock_guard lock{host_mutex};
      host_jobs.push_back(std::move(job));
    }
    host_cv.notify_one();
  }
  void reading_started()
  {
    const int n = ++readings_open;
    int m = readings_open_max;
    while(n > m && !readings_open_max.compare_exchange_weak(m, n))
      ;
  }
  void reading_ended(reading r)
  {
    --readings_open;
    std::lock_guard lock{stats_mutex};
    readings.push_back(std::move(r));
  }
  std::size_t ws_opened(const std::shared_ptr<ws_session>& s)
  {
    ++ws_open;
    std::lock_guard lock{ws_mutex};
    sessions.push_back(s);
    std::lock_guard l2{stats_mutex};
    ws_records.push_back({clock_type::now(), {}, false});
    return ws_records.size() - 1;
  }
  void ws_closed(std::size_t idx)
  {
    --ws_open;
    std::lock_guard lock{stats_mutex};
    ws_records[idx].closed = clock_type::now();
    ws_records[idx].is_closed = true;
  }

  asio::io_context ioc;
  tcp::acceptor acceptor;
  uint16_t osc_port{};
  std::thread net, host;
  std::mutex host_mutex;
  std::condition_variable host_cv;
  std::deque<std::function<void()>> host_jobs;
  bool stopping{};
  std::mutex ws_mutex;
  std::vector<std::weak_ptr<ws_session>> sessions;
  std::mutex held_mutex;
  std::vector<std::shared_ptr<void>> held;
};

class bench_server::ws_session : public std::enable_shared_from_this<ws_session>
{
public:
  ws_session(bench_server& s, beast::tcp_stream&& stream, std::shared_ptr<tcp_token> tok)
      : srv{s}
      , ws{std::move(stream)}
      , token{std::move(tok)}
  {
  }
  void accept(http::request<http::string_body> req)
  {
    beast::get_lowest_layer(ws).expires_never();
    ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws.async_accept(req, [self = shared_from_this()](beast::error_code ec) {
      if(ec)
        return;
      self->idx = self->srv.ws_opened(self);
      self->open = true;
      self->read();
    });
  }
  void send(std::string text)
  {
    asio::post(ws.get_executor(), [self = shared_from_this(), t = std::move(text)]() mutable {
      if(!self->open)
        return;
      self->queue.push_back(std::move(t));
      if(self->queue.size() == 1)
        self->write_next();
    });
  }
  void close()
  {
    asio::post(ws.get_executor(), [self = shared_from_this()] {
      beast::error_code ec;
      beast::get_lowest_layer(self->ws).socket().close(ec);
    });
  }

private:
  void read()
  {
    ws.async_read(buffer, [self = shared_from_this()](beast::error_code ec, std::size_t) {
      if(ec)
      {
        self->finish();
        return;
      }
      self->buffer.consume(self->buffer.size());
      self->read();
    });
  }
  void write_next()
  {
    ws.text(true);
    ws.async_write(asio::buffer(queue.front()),
                   [self = shared_from_this()](beast::error_code ec, std::size_t) {
      self->queue.pop_front();
      if(ec)
      {
        self->finish();
        return;
      }
      if(!self->queue.empty())
        self->write_next();
    });
  }
  void finish()
  {
    if(!open)
      return;
    open = false;
    srv.ws_closed(idx);
    beast::error_code e;
    beast::get_lowest_layer(ws).socket().close(e);
    token.reset();
  }

  bench_server& srv;
  websocket::stream<beast::tcp_stream> ws;
  std::shared_ptr<tcp_token> token;
  beast::flat_buffer buffer;
  std::deque<std::string> queue;
  std::size_t idx{};
  bool open{};
};

class bench_server::http_session : public std::enable_shared_from_this<http_session>
{
public:
  http_session(bench_server& s, tcp::socket&& sock)
      : srv{s}
      , stream{std::move(sock)}
      , token{std::make_shared<tcp_token>(s)}
  {
    const int n = ++srv.http_open;
    int m = srv.http_open_max;
    while(n > m && !srv.http_open_max.compare_exchange_weak(m, n))
      ;
  }
  ~http_session()
  {
    if(http)
      --srv.http_open;
  }
  void start()
  {
    http::async_read(stream, buffer, req, [self = shared_from_this()](beast::error_code ec, std::size_t) {
      self->on_read(ec);
    });
  }

private:
  void on_read(beast::error_code ec)
  {
    if(ec)
      return; // a probe of the port, or a client gone
    if(websocket::is_upgrade(req))
    {
      http = false;
      --srv.http_open;
      std::make_shared<ws_session>(srv, std::move(stream), std::move(token))->accept(std::move(req));
      return;
    }
    rd.target = std::string(req.target());
    rd.asked = clock_type::now();
    srv.reading_started();
    srv.enqueue([self = shared_from_this()] { self->answer(); });
  }

  // On the host thread
  void answer()
  {
    const auto t0 = clock_type::now();
    std::string body;
    unsigned status = 200;
    const auto q = rd.target.find('?');
    const std::string path = rd.target.substr(0, q);
    const std::string query = q == std::string::npos ? std::string{} : rd.target.substr(q + 1);
    const bool tree_reading = q == std::string::npos;
    if(tree_reading && path == "/" && srv.hold_root)
    {
      // Never answered: the connection stays until the client leaves
      std::lock_guard lock{srv.held_mutex};
      srv.held.push_back(shared_from_this());
      return;
    }
    if(query.rfind("HOST_INFO", 0) == 0)
    {
      body = "{\"NAME\":\"large_tree\",\"OSC_PORT\":" + std::to_string(srv.osc_port)
             + ",\"OSC_TRANSPORT\":\"UDP\",\"EXTENSIONS\":{\"ACCESS\":true,\"VALUE\":true,"
               "\"RANGE\":true,\"TYPE\":true,\"LISTEN\":true,\"PATH_CHANGED\":true}}";
    }
    else if(query == "VALUE")
    {
      std::string track;
      if(path.rfind("/tracks/", 0) == 0
         && srv.tree.json(path.substr(0, path.find('/', 8)), track))
      {
        int t = -1, d = -1, p = -1;
        char tail[16] = {};
        if(srv.distinct_values
           && std::sscanf(path.c_str(), "/tracks/%d/devices/%d/parameters/%d/%15s", &t, &d, &p, tail) == 4
           && std::string(tail) == "value")
          body = "{\"VALUE\":" + std::to_string(1 + 10000 * t + 100 * d + p) + ".0}";
        else
          body = "{\"VALUE\":0.5}";
      }
      else
        status = 404;
      if(srv.value_ms > 0)
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(srv.value_ms.load()));
    }
    else if(tree_reading && path == "/" && srv.root_status != 200)
      status = srv.root_status;
    else if(!srv.tree.json(path, body))
    {
      status = 404;
      body.clear();
    }
    if(tree_reading && srv.host_ms_per_tree_reading > 0)
    {
      const auto want = std::chrono::duration<double, std::milli>(srv.host_ms_per_tree_reading);
      const auto spent = clock_type::now() - t0;
      if(spent < want)
        std::this_thread::sleep_for(want - spent);
    }
    // The host's own time for this answer
    if(srv.host_ms_per_mb > 0)
    {
      const auto want = std::chrono::duration<double, std::milli>(
          srv.host_ms_per_mb * double(body.size()) / 1e6);
      const auto spent = clock_type::now() - t0;
      if(spent < want)
        std::this_thread::sleep_for(want - spent);
    }
    rd.served = clock_type::now();
    rd.bytes = body.size();
    asio::post(stream.get_executor(), [self = shared_from_this(), status, b = std::move(body)]() mutable {
      self->write(status, std::move(b));
    });
  }

  void write(unsigned status, std::string body)
  {
    res = std::make_shared<http::response<http::string_body>>();
    res->version(11);
    res->result(status);
    res->set(http::field::content_type, "application/json");
    res->keep_alive(false);
    res->body() = std::move(body);
    res->prepare_payload();
    http::async_write(stream, *res, [self = shared_from_this()](beast::error_code ec, std::size_t) {
      self->rd.done = clock_type::now();
      self->rd.sent = !ec;
      beast::error_code e;
      self->stream.socket().shutdown(tcp::socket::shutdown_send, e);
      self->stream.socket().close(e);
      self->srv.reading_ended(std::move(self->rd));
      self->token.reset();
    });
  }

  bench_server& srv;
  beast::tcp_stream stream;
  std::shared_ptr<tcp_token> token;
  beast::flat_buffer buffer;
  http::request<http::string_body> req;
  std::shared_ptr<http::response<http::string_body>> res;
  reading rd;
  bool http{true};
};

inline void bench_server::accept()
{
  acceptor.async_accept(asio::make_strand(ioc), [this](beast::error_code ec, tcp::socket sock) {
    if(ec)
      return;
    std::make_shared<http_session>(*this, std::move(sock))->start();
    accept();
  });
}

inline void bench_server::stop()
{
  {
    std::lock_guard lock{host_mutex};
    if(stopping)
      return;
    stopping = true;
  }
  host_cv.notify_all();
  if(host.joinable())
    host.join();
  asio::post(ioc, [this] {
    boost::system::error_code ec;
    acceptor.close(ec);
    std::vector<std::shared_ptr<ws_session>> all;
    {
      std::lock_guard lock{ws_mutex};
      for(auto& w : sessions)
        if(auto s = w.lock())
          all.push_back(s);
    }
    for(auto& s : all)
      s->close();
    std::lock_guard l2{held_mutex};
    held.clear();
  });
  ioc.stop();
  if(net.joinable())
    net.join();
}

inline void bench_server::host_loop()
{
  for(;;)
  {
    std::function<void()> job;
    {
      std::unique_lock lock{host_mutex};
      host_cv.wait(lock, [this] { return stopping || !host_jobs.empty(); });
      if(stopping)
        return;
      job = std::move(host_jobs.front());
      host_jobs.pop_front();
    }
    job();
  }
}

inline void bench_server::path_removed(const std::string& path)
{
  const std::string msg = "{\"COMMAND\":\"PATH_REMOVED\",\"DATA\":\"" + path + "\"}";
  std::vector<std::shared_ptr<ws_session>> all;
  {
    std::lock_guard lock{ws_mutex};
    for(auto& w : sessions)
      if(auto s = w.lock())
        all.push_back(s);
  }
  for(auto& s : all)
    s->send(msg);
}

inline void bench_server::path_changed(const std::string& path)
{
  const std::string msg = "{\"COMMAND\":\"PATH_CHANGED\",\"DATA\":\"" + path + "\"}";
  std::vector<std::shared_ptr<ws_session>> all;
  {
    std::lock_guard lock{ws_mutex};
    for(auto& w : sessions)
      if(auto s = w.lock())
        all.push_back(s);
  }
  for(auto& s : all)
    s->send(msg);
}

// ---------------------------------------------------------------------------
// The client's established TCP connections to the server, counted by lsof
// on this process: the connections whose remote end is the server's port.
inline int lsof_client_connections(uint16_t port)
{
  const std::string cmd = "lsof -nP -a -p " + std::to_string(getpid()) + " -iTCP:"
                          + std::to_string(port) + " -sTCP:ESTABLISHED 2>/dev/null";
  FILE* f = popen(cmd.c_str(), "r");
  if(!f)
    return -1;
  const std::string remote = "->127.0.0.1:" + std::to_string(port);
  int n = 0;
  char line[1024];
  while(std::fgets(line, sizeof(line), f))
    if(std::strstr(line, remote.c_str()))
      n++;
  pclose(f);
  return n;
}

struct sampler
{
  bench_server& srv;
  std::atomic_bool running{true};
  std::thread thread;
  int lsof_max{0};
  int lsof_samples{0};
  std::vector<std::pair<double, int>> lsof_series;    // time, client connections
  std::vector<std::pair<double, int>> server_series;  // time, server connections

  explicit sampler(bench_server& s)
      : srv{s}
  {
    thread = std::thread([this] {
      while(running)
      {
        const double t = seconds_since(srv.start);
        const int n = lsof_client_connections(srv.port);
        if(n >= 0)
        {
          lsof_samples++;
          lsof_max = std::max(lsof_max, n);
          lsof_series.emplace_back(t, n);
        }
        server_series.emplace_back(t, srv.tcp_open.load());
        for(int i = 0; i < 25 && running; i++)
          std::this_thread::sleep_for(20ms);
      }
    });
  }
  void stop()
  {
    running = false;
    if(thread.joinable())
      thread.join();
  }
  ~sampler() { stop(); }
};

// ---------------------------------------------------------------------------
// Reporting
inline long count_leaves(const ossia::net::node_base& n)
{
  long c = 0;
  for(auto& child : n.children())
  {
    if(child->get_parameter())
      c++;
    c += count_leaves(*child);
  }
  return c;
}
}
