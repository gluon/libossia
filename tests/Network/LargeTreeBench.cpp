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

namespace
{
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using clock_type = std::chrono::steady_clock;
using namespace std::chrono_literals;

double seconds_since(clock_type::time_point t0, clock_type::time_point t = clock_type::now())
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

const leaf parameter_leaves[] = {
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
const leaf device_leaves[] = {
    {"name", "s", "\"Device %\"", nullptr},
    {"class_name", "s", "\"MidiPitcher\"", nullptr},
    {"is_active", "T", "true", nullptr},
    {"type", "i", "1", "[{\"MIN\":0,\"MAX\":4}]"},
};
const leaf track_leaves[] = {
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
struct reading
{
  std::string target;
  clock_type::time_point asked, served, done;
  std::size_t bytes{};
  bool sent{};
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

  // Statistics
  std::atomic<int> tcp_open{0}, tcp_open_max{0}, tcp_total{0};
  std::atomic<int> readings_open{0}, readings_open_max{0};
  std::atomic<int> ws_open{0};
  std::mutex stats_mutex;
  std::vector<reading> readings;
  std::vector<ws_record> ws_records;
  clock_type::time_point start;

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
    if(query.rfind("HOST_INFO", 0) == 0)
    {
      body = "{\"NAME\":\"large_tree\",\"OSC_PORT\":" + std::to_string(srv.osc_port)
             + ",\"OSC_TRANSPORT\":\"UDP\",\"EXTENSIONS\":{\"ACCESS\":true,\"VALUE\":true,"
               "\"RANGE\":true,\"TYPE\":true,\"LISTEN\":true,\"PATH_CHANGED\":true}}";
    }
    else if(!srv.tree.json(path, body))
    {
      status = 404;
      body.clear();
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
};

void bench_server::accept()
{
  acceptor.async_accept(asio::make_strand(ioc), [this](beast::error_code ec, tcp::socket sock) {
    if(ec)
      return;
    std::make_shared<http_session>(*this, std::move(sock))->start();
    accept();
  });
}

void bench_server::stop()
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
  });
  ioc.stop();
  if(net.joinable())
    net.join();
}

void bench_server::host_loop()
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

void bench_server::path_changed(const std::string& path)
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
int lsof_client_connections(uint16_t port)
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
double percentile(std::vector<double> v, double p)
{
  if(v.empty())
    return 0;
  std::sort(v.begin(), v.end());
  const auto i = std::min(v.size() - 1, std::size_t(p * double(v.size() - 1) + 0.5));
  return v[i];
}

long count_leaves(const ossia::net::node_base& n)
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
  // Until every reading asked is answered, two minutes at most
  const auto w0 = clock_type::now();
  for(;;)
  {
    std::this_thread::sleep_for(200ms);
    std::size_t n;
    {
      std::lock_guard lock{srv.stats_mutex};
      n = srv.readings.size();
    }
    if((srv.readings_open == 0 && int(n) >= 1 + sent) || seconds_since(w0) > 120)
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
