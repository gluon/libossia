// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
// The C API of a client and of a server that sends bundles: an OSCQuery
// server on a network context, mirrors that follow it, bundles, echo.
#include <ossia/detail/config.hpp>

#include <ossia/network/context.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/oscquery/detail/json_writer.hpp>
#include <ossia/network/oscquery/oscquery_mirror.hpp>
#include <ossia/detail/logger.hpp>
#include <ossia/network/http/http_client.hpp>
#include <ossia/network/sockets/websocket_client_beast.hpp>
#include <ossia/network/sockets/websocket_server_beast.hpp>
#include <ossia/protocols/oscquery/oscquery_mirror_asio.hpp>
#include <ossia/protocols/oscquery/oscquery_server_asio.hpp>
#include <ossia/network/base/node_functions.hpp>

#include <ossia-c/ossia-c.h>

#include "include_catch.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>

#include <oscpack/osc/OscReceivedElements.h>
#include <rapidjson/document.h>
#include <spdlog/sinks/ostream_sink.h>
#include <boost/beast/websocket/detail/hybi13.hpp>

#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <functional>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
// With OSSIA_TEST_PORTS=<first>-<last>, the next free port of that range, in
// turn, out of the ports the system gives to outgoing connections; else a
// port the system chooses.
uint16_t free_port_in_range(bool tcp)
{
  const char* range = std::getenv("OSSIA_TEST_PORTS");
  if(!range)
    return 0;
  int first = 0, last = 0;
  if(std::sscanf(range, "%d-%d", &first, &last) != 2 || first <= 0 || last < first || last > 65535)
    return 0;
  static int next = 0;
  for(int i = 0; i <= last - first; i++)
  {
    const int port = first + (next++ % (last - first + 1));
    int fd = ::socket(AF_INET, tcp ? SOCK_STREAM : SOCK_DGRAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    const bool ok = ::bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0;
    ::close(fd);
    if(ok)
      return static_cast<uint16_t>(port);
  }
  return 0;
}

uint16_t free_port(bool tcp)
{
  if(const auto port = free_port_in_range(tcp))
    return port;
  int fd = ::socket(AF_INET, tcp ? SOCK_STREAM : SOCK_DGRAM, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;
  ::bind(fd, (sockaddr*)&addr, sizeof(addr));
  socklen_t len = sizeof(addr);
  ::getsockname(fd, (sockaddr*)&addr, &len);
  ::close(fd);
  return ntohs(addr.sin_port);
}

bool wait_for(const std::function<bool()>& done, int ms)
{
  for(int i = 0; i < ms / 5; i++)
  {
    if(done())
      return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return done();
}

float value_of(ossia_parameter_t p)
{
  auto v = ossia_parameter_get_value(p);
  float f = ossia_value_to_float(v);
  ossia_value_free(v);
  return f;
}

ossia_parameter_t float_parameter(ossia_device_t dev, const char* path)
{
  return ossia_node_create_parameter(
      ossia_node_create(ossia_device_get_root_node(dev), path), FLOAT_T);
}

ossia_parameter_t find_parameter(ossia_device_t dev, const char* path)
{
  auto node = ossia_node_find(ossia_device_get_root_node(dev), path);
  return node ? ossia_node_get_parameter(node) : nullptr;
}

// An IPv4 address of this machine on another interface than the loopback,
// empty if it has none
std::string other_ipv4()
{
  std::string found;
  if(std::getenv("OSSIA_TEST_NO_OTHER_INTERFACE"))
    return found;
  ifaddrs* addrs{};
  if(::getifaddrs(&addrs) != 0)
    return found;
  for(auto a = addrs; a && found.empty(); a = a->ifa_next)
  {
    if(!a->ifa_addr || a->ifa_addr->sa_family != AF_INET || !(a->ifa_flags & IFF_UP))
      continue;
    auto in = reinterpret_cast<sockaddr_in*>(a->ifa_addr)->sin_addr;
    if((ntohl(in.s_addr) >> 24) == 127)
      continue;
    char buf[INET_ADDRSTRLEN];
    ::inet_ntop(AF_INET, &in, buf, sizeof(buf));
    found = buf;
  }
  ::freeifaddrs(addrs);
  return found;
}

// How long the warm-up waits for its datagram
constexpr int warm_up_ms = 10000;

// The warm-up of a test that receives datagrams from another address than the
// loopback: a throwaway datagram from `other` to a socket of this process,
// sent again every 200 ms, without libossia, until one arrives or timeout_ms
// passes. The application firewall of macOS, when it is on, holds the first
// flow from another address that a binary it does not know yet receives,
// while it decides on that binary: 1.5 to 3.5 s seen, once 48 s. Right after
// a build, the datagram of the test then came after its 2 s window. Once the
// warm-up has arrived, the binary is known and the test's own datagram is
// not held. Returns whether the warm-up arrived.
bool warm_up_from(const std::string& other, int timeout_ms)
{
  int in = ::socket(AF_INET, SOCK_DGRAM, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  socklen_t len = sizeof(addr);
  if(::bind(in, (sockaddr*)&addr, sizeof(addr)) != 0
     || ::getsockname(in, (sockaddr*)&addr, &len) != 0)
  {
    ::close(in);
    return false;
  }
  ::fcntl(in, F_SETFL, O_NONBLOCK);
  sockaddr_in to{}, from{};
  to.sin_family = from.sin_family = AF_INET;
  to.sin_port = addr.sin_port;
  ::inet_pton(AF_INET, other.c_str(), &to.sin_addr);
  ::inet_pton(AF_INET, other.c_str(), &from.sin_addr);
  bool received = false;
  const auto start = std::chrono::steady_clock::now();
  while(!received
        && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(timeout_ms))
  {
    int out = ::socket(AF_INET, SOCK_DGRAM, 0);
    if(::bind(out, (sockaddr*)&from, sizeof(from)) == 0)
      ::sendto(out, "warm-up", 7, 0, (sockaddr*)&to, sizeof(to));
    ::close(out);
    for(int i = 0; i < 20 && !received; i++)
    {
      char buf[64];
      received = ::recv(in, buf, sizeof(buf), 0) > 0;
      if(!received)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
  ::close(in);
  return received;
}

// The interface that holds this IPv4 address now, with its state, for the
// message of a failed test
std::string interface_of(const std::string& ip)
{
  std::string found = "no interface";
  ifaddrs* addrs{};
  if(::getifaddrs(&addrs) != 0)
    return found;
  for(auto a = addrs; a; a = a->ifa_next)
  {
    if(!a->ifa_addr || a->ifa_addr->sa_family != AF_INET)
      continue;
    char buf[INET_ADDRSTRLEN];
    ::inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(a->ifa_addr)->sin_addr, buf, sizeof(buf));
    if(ip == buf)
    {
      found = std::string(a->ifa_name) + ((a->ifa_flags & IFF_UP) ? " up" : " down")
              + ((a->ifa_flags & IFF_RUNNING) ? " running" : " not running");
      break;
    }
  }
  ::freeifaddrs(addrs);
  return found;
}

// Whether a TCP connection to ip:port is accepted within 500 ms
bool tcp_connects(const std::string& ip, uint16_t port)
{
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  ::fcntl(fd, F_SETFL, O_NONBLOCK);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  ::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
  bool ok = ::connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0;
  if(!ok && errno == EINPROGRESS)
  {
    fd_set w;
    FD_ZERO(&w);
    FD_SET(fd, &w);
    timeval tv{0, 500000};
    if(::select(fd + 1, nullptr, &w, nullptr, &tv) == 1)
    {
      int err = 0;
      socklen_t len = sizeof(err);
      ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
      ok = err == 0;
    }
  }
  ::close(fd);
  return ok;
}

// Sends "<address> <f>" in one datagram to ip:port
std::string osc_float_message(const std::string& address, float f)
{
  std::string m = address;
  m.append(4 - m.size() % 4, '\0');
  m += ",f";
  m.append(2, '\0');
  uint32_t bits;
  std::memcpy(&bits, &f, 4);
  bits = htonl(bits);
  m.append(reinterpret_cast<const char*>(&bits), 4);
  return m;
}

// 0 when the datagram left, the errno of sendto otherwise
int send_osc_float(
    const std::string& ip, uint16_t port, const std::string& address, float f,
    const std::string& from_ip = {}, uint16_t from_port = 0)
{
  const auto m = osc_float_message(address, f);
  int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  ::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
  if(from_port != 0)
  {
    sockaddr_in from{};
    from.sin_family = AF_INET;
    from.sin_port = htons(from_port);
    ::inet_pton(AF_INET, from_ip.c_str(), &from.sin_addr);
    REQUIRE(::bind(fd, (sockaddr*)&from, sizeof(from)) == 0);
  }
  const auto n = ::sendto(fd, m.data(), m.size(), 0, (sockaddr*)&addr, sizeof(addr));
  const int error = n < 0 ? errno : 0;
  ::close(fd);
  return error;
}

// The body of the reply to GET <resource> on 127.0.0.1:port, empty on failure
std::string http_get(uint16_t port, const std::string& resource)
{
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if(::connect(fd, (sockaddr*)&addr, sizeof(addr)) != 0)
  {
    ::close(fd);
    return {};
  }
  timeval tv{2, 0};
  ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  const std::string req = "GET " + resource + " HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
  ::send(fd, req.data(), req.size(), 0);
  std::string reply;
  char buf[4096];
  for(ssize_t n; (n = ::recv(fd, buf, sizeof(buf), 0)) > 0;)
    reply.append(buf, n);
  ::close(fd);
  auto body = reply.find("\r\n\r\n");
  return body == std::string::npos ? std::string{} : reply.substr(body + 4);
}

// Whether a device of that name is announced over mDNS, browsing for ms
bool announced(const std::string& name, int ms)
{
  char** names{};
  char** hosts{};
  int* ports{};
  size_t size{};
  ossia_oscquery_list_devices(ms, &names, &hosts, &ports, &size);
  bool found = false;
  for(size_t i = 0; i < size; i++)
    found = found || name == names[i];
  ossia_oscquery_devices_free(names, hosts, ports, size);
  return found;
}

// An OSCQuery server of the C API, under a multiplex protocol as a host
// exposes its device, on a network context that a thread of the test polls.
struct server
{
  uint16_t osc;
  uint16_t ws;
  ossia_network_context_t ctx = ossia_network_context_create();
  ossia_device_t dev{};
  std::atomic_bool polling{true};
  std::thread poller;

  explicit server(
      const char* name, uint16_t osc_port = 0, const char* bind = nullptr,
      uint16_t ws_port = 0)
      : osc{osc_port ? osc_port : free_port(false)}
      , ws{ws_port ? ws_port : free_port(true)}
  {
    auto multiplex = ossia_protocol_multiplex_create();
    dev = ossia_device_create(multiplex, name);
    auto proto = bind ? ossia_protocol_oscquery_server_asio_create_bound(ctx, osc, ws, bind)
                      : ossia_protocol_oscquery_server_asio_create(ctx, osc, ws);
    REQUIRE(proto);
    ossia_protocol_multiplex_expose_to(multiplex, proto);
    poller = std::thread([this] {
      while(polling)
      {
        ossia_network_context_poll(ctx);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });
  }
  ~server()
  {
    stop();
    polling = false;
    poller.join();
    ossia_network_context_free(ctx);
  }
  // Frees the device, its server, and their ports
  void stop()
  {
    if(dev)
      ossia_device_free(dev);
    dev = nullptr;
  }
  std::string url() const { return "ws://127.0.0.1:" + std::to_string(ws); }
};

// A mirror of the C API, and the values it receives from the network: the
// callback also runs on the thread that pushes, for the mirror's own value.
struct mirror
{
  ossia_device_t dev{};
  std::thread::id pushing = std::this_thread::get_id();
  std::mutex mutex;
  std::vector<float> received;

  mirror(const std::string& url, const char* name)
  {
    dev = ossia_device_create(ossia_protocol_oscquery_mirror_create(url.c_str()), name);
    REQUIRE(dev);
    REQUIRE(ossia_device_update_namespace(dev) == 1);
  }
  ~mirror()
  {
    if(dev)
      ossia_device_free(dev);
  }
  ossia_parameter_t listen(const char* path)
  {
    auto p = find_parameter(dev, path);
    REQUIRE(p);
    ossia_parameter_add_callback(
        p,
        [](void* ctx, ossia_value_t v) {
      auto& self = *static_cast<mirror*>(ctx);
      if(std::this_thread::get_id() == self.pushing)
        return;
      std::lock_guard lock{self.mutex};
      self.received.push_back(ossia_value_to_float(v));
        },
        this);
    return p;
  }
  std::vector<float> values()
  {
    std::lock_guard lock{mutex};
    return received;
  }
  bool has(float f)
  {
    std::lock_guard lock{mutex};
    return std::find(received.begin(), received.end(), f) != received.end();
  }
};

// The datagrams a UDP port receives, and the messages in them.
struct udp_sink
{
  boost::asio::io_context ctx;
  boost::asio::ip::udp::socket socket{ctx};
  std::thread thread;
  std::mutex mutex;
  std::vector<std::size_t> datagrams;
  std::vector<std::pair<std::string, float>> messages;
  std::atomic_bool running{true};

  explicit udp_sink(uint16_t port)
  {
    socket.open(boost::asio::ip::udp::v4());
    // On 127.0.0.1, a port a server may have opened on 0.0.0.0: the
    // datagrams sent to 127.0.0.1 then reach the sink
    socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    socket.bind({boost::asio::ip::make_address("127.0.0.1"), port});
    socket.non_blocking(true);
    thread = std::thread([this] {
      std::vector<char> buf(65536);
      while(running)
      {
        boost::system::error_code ec;
        boost::asio::ip::udp::endpoint from;
        auto n = socket.receive_from(boost::asio::buffer(buf), from, 0, ec);
        if(ec)
        {
          std::this_thread::sleep_for(std::chrono::microseconds(200));
          continue;
        }
        std::lock_guard lock{mutex};
        datagrams.push_back(n);
        decode(buf.data(), n);
      }
    });
  }
  ~udp_sink()
  {
    running = false;
    thread.join();
  }
  uint16_t port() const { return socket.local_endpoint().port(); }
  void decode(const char* data, std::size_t size)
  {
    try
    {
      oscpack::ReceivedPacket p{data, (oscpack::osc_bundle_element_size_t)size};
      if(p.IsBundle())
      {
        oscpack::ReceivedBundle b{p};
        for(auto it = b.ElementsBegin(); it != b.ElementsEnd(); ++it)
          decode(it->Contents(), it->Size());
      }
      else
      {
        oscpack::ReceivedMessage m{p};
        float v = 0.f;
        if(m.ArgumentCount() > 0 && m.ArgumentsBegin()->IsFloat())
          v = m.ArgumentsBegin()->AsFloat();
        messages.emplace_back(m.AddressPattern(), v);
      }
    }
    catch(...)
    {
    }
  }
  void clear()
  {
    std::lock_guard lock{mutex};
    datagrams.clear();
    messages.clear();
  }
  bool wait_messages(std::size_t n, int ms)
  {
    return wait_for(
        [&] {
      std::lock_guard lock{mutex};
      return messages.size() >= n;
        },
        ms);
  }
};

// A client of an OSCQuery server reduced to its WebSocket, which asks the
// server to send the values to a UDP port of the test.
struct raw_client
{
  // Owns its context: stop() ends connect_and_run()
  ossia::net::websocket_client_beast ws{[](auto&&...) {}};
  std::thread thread;

  explicit raw_client(const std::string& url)
  {
    thread = std::thread([this, url] {
      try
      {
        ws.connect_and_run(url);
      }
      catch(...)
      {
      }
    });
    wait_for([&] { return ws.connected(); }, 2000);
  }
  ~raw_client()
  {
    ws.stop();
    thread.join();
  }
  void stream_to(uint16_t udp_port, uint16_t sender_port = 0)
  {
    auto msg = ossia::oscquery::json_writer::start_osc_streaming(udp_port, sender_port);
    ws.send_message(std::string(msg.GetString(), msg.GetSize()));
  }
};
}

TEST_CASE("C API: an OSCQuery server on a network context", "[network_c]")
{
  server s{"c_asio_server"};
  auto level = float_parameter(s.dev, "/level");
  mirror m{s.url(), "c_asio_mirror"};
  auto remote = m.listen("/level");

  // Client to server
  ossia_parameter_push_f(remote, 0.25f);
  REQUIRE(wait_for([&] { return value_of(level) == 0.25f; }, 2000));

  // Server to client
  ossia_parameter_push_f(level, 0.75f);
  REQUIRE(wait_for([&] { return m.has(0.75f); }, 2000));
}

TEST_CASE("C API: ossia_device_connected follows the WebSocket", "[network_c]")
{
  server s{"c_connected_server"};
  float_parameter(s.dev, "/level");
  mirror m{s.url(), "c_connected_mirror"};
  REQUIRE(ossia_device_connected(m.dev) == 1);

  s.stop();
  REQUIRE(wait_for([&] { return ossia_device_connected(m.dev) == 0; }, 3000));
}

TEST_CASE("C API: ossia_device_run_commands applies the nodes added later", "[network_c]")
{
  server s{"c_commands_server"};
  float_parameter(s.dev, "/level");
  mirror m{s.url(), "c_commands_mirror"};

  float_parameter(s.dev, "/later");
  // The node waits in the mirror's queue
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  REQUIRE(find_parameter(m.dev, "/later") == nullptr);

  REQUIRE(wait_for(
      [&] {
    ossia_device_run_commands(m.dev);
    return find_parameter(m.dev, "/later") != nullptr;
      },
      2000));
}

// A parameter the server adds while a client follows it reaches the client
// with its value, not 0: ossia.client showed 0 until the next change.
TEST_CASE("C API: a parameter added later comes with its value", "[network_c]")
{
  server s{"c_added_value_server", 0, "127.0.0.1"};
  float_parameter(s.dev, "/level");
  mirror m{s.url(), "c_added_value_mirror"};

  auto later = float_parameter(s.dev, "/later");
  ossia_parameter_push_f(later, 0.7f);
  ossia_parameter_t remote{};
  REQUIRE(wait_for(
      [&] {
    ossia_device_run_commands(m.dev);
    return (remote = find_parameter(m.dev, "/later")) != nullptr;
      },
      2000));
  const bool got = wait_for(
      [&] {
    ossia_device_run_commands(m.dev);
    return value_of(remote) == 0.7f;
      },
      2000);
  INFO("value in the client: " << value_of(remote));
  REQUIRE(got);
}

// Ossia.Remote binds as it does in Unity: it reads the value of the new
// parameter, then the Client adds its callback. The read asks the server, the
// parameter being followed by nothing yet; its answer comes after the reading
// of the new parameter and must not give the same value a second time.
TEST_CASE("C API: an added parameter read then followed gets its value once", "[network_c]")
{
  server s{"c_read_then_follow_server", 0, "127.0.0.1"};
  float_parameter(s.dev, "/level");
  mirror m{s.url(), "c_read_then_follow_mirror"};
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  auto later = float_parameter(s.dev, "/later");
  auto v = ossia_value_create_float(0.7f);
  ossia_parameter_set_value(later, v);
  ossia_value_free(v);
  ossia_parameter_t remote{};
  REQUIRE(wait_for(
      [&] {
    ossia_device_run_commands(m.dev);
    return (remote = find_parameter(m.dev, "/later")) != nullptr;
      },
      2000));
  value_of(remote);
  m.listen("/later");
  for(int t = 0; t < 1000; t += 5)
  {
    ossia_device_run_commands(m.dev);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  const auto got = m.values();
  INFO("received: " << Catch::Detail::stringify(got));
  REQUIRE(std::count(got.begin(), got.end(), 0.7f) == 1);
}

TEST_CASE("C API: ossia_device_push_bundle from a server", "[network_c]")
{
  const int n = 40;
  server s{"c_bundle_server"};
  std::vector<ossia_parameter_t> params;
  for(int i = 0; i < n; i++)
    params.push_back(float_parameter(s.dev, ("/p" + std::to_string(i)).c_str()));

  udp_sink sink{free_port(false)};
  raw_client client{s.url()};
  REQUIRE(client.ws.connected());
  client.stream_to(sink.port());
  for(int i = 0; i < n; i++)
    client.ws.send_message(ossia::oscquery::json_writer::listen("/p" + std::to_string(i)));
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  std::vector<ossia_value_t> values;
  for(int i = 0; i < n; i++)
    values.push_back(ossia_value_create_float(float(i)));
  sink.clear();
  ossia_device_push_bundle(s.dev, params.data(), values.data(), n);
  for(auto v : values)
    ossia_value_free(v);

  // Set here, as a push does
  REQUIRE(value_of(params[12]) == 12.f);

  const bool all = sink.wait_messages(n, 3000);
  std::lock_guard lock{sink.mutex};
  INFO("datagrams " << sink.datagrams.size() << ", messages " << sink.messages.size());
  REQUIRE(all);
  REQUIRE(sink.messages.size() == std::size_t(n));
  REQUIRE(sink.datagrams.size() == 1);
  REQUIRE(sink.messages[12].first == "/p12");
  REQUIRE(sink.messages[12].second == 12.f);
}

TEST_CASE("C API: ossia_device_push_bundle from a mirror", "[network_c]")
{
  const int n = 40;
  server s{"c_mirror_bundle_server"};
  // What the mirror sends to the server's OSC port, read by the test
  udp_sink sink{s.osc};
  for(int i = 0; i < n; i++)
    float_parameter(s.dev, ("/p" + std::to_string(i)).c_str());
  mirror m{s.url(), "c_mirror_bundle_mirror"};

  std::vector<ossia_parameter_t> params;
  std::vector<ossia_value_t> values;
  for(int i = 0; i < n; i++)
  {
    params.push_back(find_parameter(m.dev, ("/p" + std::to_string(i)).c_str()));
    REQUIRE(params.back());
    values.push_back(ossia_value_create_float(float(100 + i)));
  }
  sink.clear();
  ossia_device_push_bundle(m.dev, params.data(), values.data(), n);
  for(auto v : values)
    ossia_value_free(v);

  REQUIRE(value_of(params[7]) == 107.f);

  const bool all = sink.wait_messages(n, 3000);
  std::lock_guard lock{sink.mutex};
  INFO("datagrams " << sink.datagrams.size() << ", messages " << sink.messages.size());
  REQUIRE(all);
  REQUIRE(sink.messages.size() == std::size_t(n));
  REQUIRE(sink.datagrams.size() == 1);
  REQUIRE(sink.messages[7].second == 107.f);
}

TEST_CASE("C API: a parameter of another device is not sent", "[network_c]")
{
  server s{"c_bundle_other_server"};
  auto mine = float_parameter(s.dev, "/mine");
  auto other_dev = ossia_device_create(ossia_protocol_multiplex_create(), "c_bundle_other");
  auto other = float_parameter(other_dev, "/other");

  ossia_parameter_t params[] = {mine, other};
  ossia_value_t values[] = {ossia_value_create_float(1.f), ossia_value_create_float(2.f)};
  ossia_device_push_bundle(s.dev, params, values, 2);
  ossia_value_free(values[0]);
  ossia_value_free(values[1]);

  REQUIRE(value_of(mine) == 1.f);
  REQUIRE(value_of(other) == 0.f);
  ossia_device_free(other_dev);
}

// Two clients of one server see each other's values.
TEST_CASE("C API: ossia_device_set_echo", "[network_c]")
{
  server s{"c_echo_server"};
  float_parameter(s.dev, "/level");
  mirror a{s.url(), "c_echo_a"};
  mirror b{s.url(), "c_echo_b"};
  auto pa = a.listen("/level");
  b.listen("/level");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  ossia_device_set_echo(s.dev, 1);
  ossia_parameter_push_f(pa, 0.5f);
  REQUIRE(wait_for([&] { return b.has(0.5f); }, 2000));

  // Off: the server forwards nothing
  ossia_device_set_echo(s.dev, 0);
  ossia_parameter_push_f(pa, 0.6f);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  REQUIRE(!b.has(0.6f));
}

// The client that sent a value does not receive it back: the server knows it
// by the UDP port its values come from.
TEST_CASE("C API: ossia_device_set_echo spares the sender", "[network_c]")
{
  server s{"c_echo_sender_server"};
  float_parameter(s.dev, "/level");
  mirror a{s.url(), "c_echo_sender_a"};
  mirror b{s.url(), "c_echo_sender_b"};
  auto pa = a.listen("/level");
  b.listen("/level");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  ossia_device_set_echo(s.dev, 1);
  ossia_parameter_push_f(pa, 0.5f);
  REQUIRE(wait_for([&] { return b.has(0.5f); }, 2000));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  INFO("a received " << a.values().size() << " values");
  REQUIRE(a.values().empty());
}

// The same over the WebSocket: a mirror sends the values of a critical
// parameter there, and the server knows the client by its connection.
TEST_CASE("C API: ossia_device_set_echo spares the sender over the WebSocket", "[network_c]")
{
  server s{"c_echo_ws_server"};
  ossia_parameter_set_critical(float_parameter(s.dev, "/level"), 1);
  mirror a{s.url(), "c_echo_ws_a"};
  mirror b{s.url(), "c_echo_ws_b"};
  auto pa = a.listen("/level");
  b.listen("/level");
  REQUIRE(ossia_parameter_get_critical(pa) == 1);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  ossia_device_set_echo(s.dev, 1);
  ossia_parameter_push_f(pa, 0.5f);
  REQUIRE(wait_for([&] { return b.has(0.5f); }, 2000));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  INFO("a received " << a.values().size() << " values");
  REQUIRE(a.values().empty());
}

// The asio mirror of the core writes over UDP too, and declares in
// START_OSC_STREAMING the port its values leave from: the server, which
// forwards a client's values to the others, does not send them back to it.
TEST_CASE("C API: ossia_device_set_echo spares an asio mirror", "[network_c]")
{
  server s{"c_echo_asio_server"};
  float_parameter(s.dev, "/level");
  ossia_device_set_echo(s.dev, 1);
  mirror b{s.url(), "c_echo_asio_b"};
  b.listen("/level");

  auto ctx = std::make_shared<ossia::net::network_context>();
  auto proto = std::make_unique<ossia::oscquery_asio::oscquery_mirror_asio_protocol>(
      ctx, s.url());
  auto& a_proto = *proto;
  ossia::net::generic_device a{std::move(proto), "c_echo_asio_a"};
  REQUIRE(a_proto.update(a.get_root_node()));

  // Polls the mirror's context until the end of the test, failed or not
  struct poll_thread
  {
    ossia::net::network_context& ctx;
    std::atomic_bool polling{true};
    std::thread thread{[this] {
      while(polling)
      {
        ctx.context.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }};
    ~poll_thread()
    {
      polling = false;
      thread.join();
    }
  } poller{*ctx};
  REQUIRE(wait_for([&] { return a_proto.ws_connected() && a_proto.osc_connected(); }, 2000));

  auto node = ossia::net::find_node(a.get_root_node(), "/level");
  REQUIRE(node);
  auto param = node->get_parameter();
  REQUIRE(param);
  std::mutex mutex;
  std::vector<float> received;
  param->add_callback([&](const ossia::value& v) {
    // The callback also runs for the mirror's own push, on this thread
    if(std::this_thread::get_id() != poller.thread.get_id())
      return;
    std::lock_guard lock{mutex};
    received.push_back(ossia::convert<float>(v));
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  param->push_value(0.5f);
  REQUIRE(wait_for([&] { return b.has(0.5f); }, 2000));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  {
    std::lock_guard lock{mutex};
    INFO("the asio mirror received " << received.size() << " values");
    REQUIRE(received.empty());
  }
}

// A server on 127.0.0.1 only: the clients of the machine reach it, the
// other interfaces refuse its WebSocket and do not deliver OSC to it, and it
// is not announced over mDNS.
TEST_CASE("C API: a server on 127.0.0.1 only", "[network_c]")
{
  const auto other = other_ipv4();
  if(other.empty())
  {
    WARN("no interface other than the loopback: not tested");
    return;
  }
  server announced_server{"c_unbound_server"};
  server s{"c_bound_server", 0, "127.0.0.1"};
  auto level = float_parameter(s.dev, "/level");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  mirror a{s.url(), "c_bound_a"};
  REQUIRE(tcp_connects("127.0.0.1", s.ws));
  INFO("other interface: " << other);
  REQUIRE(!tcp_connects(other, s.ws));

  send_osc_float(other, s.osc, "/level", 0.25f);
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  REQUIRE(value_of(level) != 0.25f);
  send_osc_float("127.0.0.1", s.osc, "/level", 0.75f);
  REQUIRE(wait_for([&] { return value_of(level) == 0.75f; }, 2000));

  REQUIRE(announced("c_unbound_server", 2500));
  REQUIRE(!announced("c_bound_server", 2500));
}

// ?PRISM_CONNECTIONS, declared in the EXTENSIONS of HOST_INFO: the clients
// of the server, with the name a libossia mirror gives it, the addresses each
// one listens to and the packets it sent; and the UDP senders that are none
// of its clients.
TEST_CASE("C API: ?PRISM_CONNECTIONS lists the clients and the senders", "[network_c]")
{
  // On 127.0.0.1, not announced: no client of the machine that follows the
  // devices announced comes in the list
  server s{"c_conn_server", 0, "127.0.0.1"};
  float_parameter(s.dev, "/level");
  float_parameter(s.dev, "/other");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  const auto host_info = http_get(s.ws, "/?HOST_INFO");
  INFO(host_info);
  REQUIRE(host_info.find("\"PRISM_CONNECTIONS\":true") != std::string::npos);

  mirror a{s.url(), "c_conn_a"};
  auto pa = a.listen("/level");
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  ossia_parameter_push_f(pa, 0.5f);

  // A sender that is not a client, from a port of its own
  int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  sockaddr_in local{};
  local.sin_family = AF_INET;
  local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  REQUIRE(::bind(fd, (sockaddr*)&local, sizeof(local)) == 0);
  socklen_t len = sizeof(local);
  ::getsockname(fd, (sockaddr*)&local, &len);
  const int sender_port = ntohs(local.sin_port);
  {
    std::string m = "/other";
    m.append(4 - m.size() % 4, '\0');
    m += ",f";
    m.append(2, '\0');
    float f = 0.25f;
    uint32_t bits;
    std::memcpy(&bits, &f, 4);
    bits = htonl(bits);
    m.append(reinterpret_cast<const char*>(&bits), 4);
    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_port = htons(s.osc);
    to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    for(int i = 0; i < 2; i++)
      ::sendto(fd, m.data(), m.size(), 0, (sockaddr*)&to, sizeof(to));
  }
  ::close(fd);
  REQUIRE(wait_for([&] { return value_of(find_parameter(s.dev, "/other")) == 0.25f; }, 2000));
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  const auto json = http_get(s.ws, "/?PRISM_CONNECTIONS");
  INFO(json);
  rapidjson::Document doc;
  doc.Parse(json.c_str());
  REQUIRE(!doc.HasParseError());
  REQUIRE(doc.IsObject());
  REQUIRE(doc.HasMember("CLIENTS"));
  REQUIRE(doc.HasMember("SENDERS"));

  // A missing member reads as "" or 0
  auto str = [](const rapidjson::Value& v, const char* k) {
    return v.HasMember(k) && v[k].IsString() ? std::string(v[k].GetString()) : std::string{};
  };
  auto num = [](const rapidjson::Value& v, const char* k) {
    return v.HasMember(k) && v[k].IsInt64() ? v[k].GetInt64() : int64_t{0};
  };
  const auto& clients = doc["CLIENTS"];
  REQUIRE(clients.IsArray());
  REQUIRE(clients.Size() == 1);
  const auto& c = clients[0];
  CHECK(str(c, "IP") == "127.0.0.1");
  CHECK(num(c, "PORT") > 0);
  CHECK(str(c, "NAME") == "c_conn_a");
  CHECK(num(c, "OSC_PORT") > 0);
  CHECK(num(c, "SENDER_PORT") > 0);
  REQUIRE(c.HasMember("LISTEN"));
  REQUIRE(c["LISTEN"].IsArray());
  REQUIRE(c["LISTEN"].Size() == 1);
  CHECK(std::string(c["LISTEN"][0].GetString()) == "/level");
  CHECK(num(c, "PACKETS") >= 1);

  const auto& senders = doc["SENDERS"];
  REQUIRE(senders.IsArray());
  REQUIRE(senders.Size() == 1);
  CHECK(str(senders[0], "IP") == "127.0.0.1");
  CHECK(num(senders[0], "PORT") == sender_port);
  CHECK(num(senders[0], "PACKETS") == 2);
  CHECK(senders[0].HasMember("LAST_MS"));
}

// A source that changes its port at each datagram: the list keeps the last 256
// to send, and a source that sends again is one of the last.
TEST_CASE("C API: ?PRISM_CONNECTIONS keeps the last 256 senders", "[network_c]")
{
  server s{"c_senders_server", 0, "127.0.0.1"};
  auto other = float_parameter(s.dev, "/other");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  float last = 0.f;
  auto send = [&](int fd) {
    last += 1.f;
    std::string m = "/other";
    m.append(4 - m.size() % 4, '\0');
    m += ",f";
    m.append(2, '\0');
    uint32_t bits;
    std::memcpy(&bits, &last, 4);
    bits = htonl(bits);
    m.append(reinterpret_cast<const char*>(&bits), 4);
    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_port = htons(s.osc);
    to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ::sendto(fd, m.data(), m.size(), 0, (sockaddr*)&to, sizeof(to));
  };
  auto received = [&] { return wait_for([&] { return value_of(other) == last; }, 2000); };
  auto open_sender = [](int& port) {
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ::bind(fd, (sockaddr*)&local, sizeof(local));
    socklen_t len = sizeof(local);
    ::getsockname(fd, (sockaddr*)&local, &len);
    port = ntohs(local.sin_port);
    return fd;
  };
  // n more sources that the server has not seen, from a port each
  std::vector<int> ports;
  auto others = [&](int n) {
    for(int i = 0; i < n;)
    {
      int port;
      int fd = open_sender(port);
      if(std::find(ports.begin(), ports.end(), port) == ports.end())
      {
        ports.push_back(port);
        send(fd);
        REQUIRE(received());
        i++;
      }
      ::close(fd);
    }
  };

  int kept_port, gone_port;
  int kept = open_sender(kept_port), gone = open_sender(gone_port);
  ports.push_back(kept_port);
  ports.push_back(gone_port);
  send(kept);
  REQUIRE(received());
  send(gone);
  REQUIRE(received());

  // 256 sources: nothing goes yet. kept sends again, so gone is now the oldest.
  others(254);
  send(kept);
  REQUIRE(received());
  others(1);
  const int newest_port = ports.back();
  ::close(kept);
  ::close(gone);

  const auto json = http_get(s.ws, "/?PRISM_CONNECTIONS");
  rapidjson::Document doc;
  doc.Parse(json.c_str());
  REQUIRE(!doc.HasParseError());
  const auto& senders = doc["SENDERS"];
  REQUIRE(senders.IsArray());
  CHECK(senders.Size() == 256);
  auto packets_of = [&](int port) {
    for(const auto& v : senders.GetArray())
      if(v["PORT"].GetInt() == port)
        return v["PACKETS"].GetInt64();
    return int64_t{0};
  };
  CHECK(packets_of(kept_port) == 2);
  CHECK(packets_of(gone_port) == 0);
  CHECK(packets_of(newest_port) == 1);
}

// The server sets a value and a client writes another one to the same
// address in the same instant: each value crosses the other. The server
// applies the client's value last, the client applies the server's after
// its own; without an answer from the server, each keeps the other's value.
TEST_CASE("C API: crossed writes converge", "[network_c]")
{
  server s{"c_cross_server"};
  auto level = float_parameter(s.dev, "/level");
  mirror m{s.url(), "c_cross_mirror"};
  auto remote = m.listen("/level");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  int divergences = 0;
  std::chrono::steady_clock::duration latest{};
  for(int i = 0; i < 20; i++)
  {
    std::atomic_bool go{false};
    std::thread client([&] {
      while(!go)
      {
      }
      ossia_parameter_push_f(remote, 200.f + i);
    });
    go = true;
    const auto sent = std::chrono::steady_clock::now();
    ossia_parameter_push_f(level, 100.f + i);
    client.join();
    // How long after it sent its value the server holds the client's
    wait_for([&] { return value_of(level) == 200.f + i; }, 300);
    latest = std::max(latest, std::chrono::steady_clock::now() - sent);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    const float at_server = value_of(level), at_client = value_of(remote);
    if(at_server != at_client)
    {
      divergences++;
      UNSCOPED_INFO("trial " << i << ": server " << at_server << ", client " << at_client);
    }
  }
  const double ms = std::chrono::duration<double, std::milli>(latest).count();
  WARN("crossed writes: " << divergences << " divergences in 20; the client's value reached the server "
       << ms << " ms after it sent its own, at most");
  REQUIRE(divergences == 0);
}

// A server that does not answer crossings, as servers other than ours: the
// client applied the server's value after it sent its own, the server
// applied the client's last. The client reads the server's value again, a
// moment after a value that crossed its own, and both end on it.
TEST_CASE("C API: a mirror reads the value again after a crossing", "[network_c]")
{
  auto ctx = std::make_shared<ossia::net::network_context>();
  const uint16_t osc = free_port(false), ws = free_port(true);
  auto proto = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(ctx, osc, ws);
  proto->set_crossing_window(std::chrono::milliseconds{0});
  ossia::net::generic_device dev{std::move(proto), "c_reread_server"};
  auto& level = *ossia::net::create_node(dev.get_root_node(), "/level")
                     .create_parameter(ossia::val_type::FLOAT);
  struct poll_thread
  {
    ossia::net::network_context& ctx;
    std::atomic_bool polling{true};
    std::thread thread{[this] {
      while(polling)
      {
        ctx.context.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }};
    ~poll_thread()
    {
      polling = false;
      thread.join();
    }
  } poller{*ctx};

  mirror m{"ws://127.0.0.1:" + std::to_string(ws), "c_reread_mirror"};
  auto remote = m.listen("/level");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  int divergences = 0;
  for(int i = 0; i < 20; i++)
  {
    std::atomic_bool go{false};
    std::thread client([&] {
      while(!go)
      {
      }
      ossia_parameter_push_f(remote, 200.f + i);
    });
    go = true;
    level.push_value(100.f + i);
    client.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    const float at_server = ossia::convert<float>(level.value()), at_client = value_of(remote);
    if(at_server != at_client)
    {
      divergences++;
      UNSCOPED_INFO("trial " << i << ": server " << at_server << ", client " << at_client);
    }
  }
  WARN("crossed writes with a server that does not answer them: " << divergences
       << " divergences in 20");
  REQUIRE(divergences == 0);
}

// A bool is served as a JSON boolean, as the OSCQuery proposal asks: a
// client that reads VALUE, or ?VALUE after a crossing, gets its state.
TEST_CASE("C API: a bool is served as true or false", "[network_c]")
{
  server s{"c_bool_server", 0, "127.0.0.1"};
  auto flag = ossia_node_create_parameter(
      ossia_node_create(ossia_device_get_root_node(s.dev), "/flag"), BOOL_T);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  ossia_parameter_push_b(flag, 1);
  auto value = http_get(s.ws, "/flag?VALUE");
  INFO(value);
  REQUIRE(value.find("\"VALUE\":true") != std::string::npos);
  auto node = http_get(s.ws, "/flag");
  INFO(node);
  REQUIRE((node.find("\"VALUE\":true") != std::string::npos
           || node.find("\"VALUE\":[true]") != std::string::npos));

  ossia_parameter_push_b(flag, 0);
  value = http_get(s.ws, "/flag?VALUE");
  INFO(value);
  REQUIRE(value.find("\"VALUE\":false") != std::string::npos);

  // A mirror reads it
  mirror m{s.url(), "c_bool_mirror"};
  auto remote = find_parameter(m.dev, "/flag");
  REQUIRE(remote);
  ossia_parameter_push_b(flag, 1);
  auto bool_of = [](ossia_parameter_t p) {
    auto v = ossia_parameter_get_value(p);
    const bool b = ossia_value_to_bool(v);
    ossia_value_free(v);
    return b;
  };
  REQUIRE(wait_for([&] { return bool_of(remote); }, 2000));
}

// The client reads the value again after a crossing: when the server already
// sent it the value it holds, the client does not get it a second time.
TEST_CASE("C API: a value read again that did not change is not a new value", "[network_c]")
{
  server s{"c_same_server"};
  auto level = float_parameter(s.dev, "/level");
  mirror m{s.url(), "c_same_mirror"};
  auto remote = m.listen("/level");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  ossia_parameter_push_f(remote, 1.f);
  REQUIRE(wait_for([&] { return value_of(level) == 1.f; }, 2000));
  ossia_parameter_push_f(level, 2.f);
  REQUIRE(wait_for([&] { return m.has(2.f); }, 2000));
  // Past the time the value is read again
  std::this_thread::sleep_for(std::chrono::milliseconds(400));
  auto got = m.values();
  INFO("the client received " << got.size() << " values");
  REQUIRE(got == std::vector<float>{2.f});
  REQUIRE(value_of(remote) == 2.f);
}

// Out of a crossing, nothing comes back: writes long after the server's last
// value are not answered.
TEST_CASE("C API: a write that crossed nothing is not sent back", "[network_c]")
{
  server s{"c_nocross_server"};
  auto level = float_parameter(s.dev, "/level");
  mirror m{s.url(), "c_nocross_mirror"};
  auto remote = m.listen("/level");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  ossia_parameter_push_f(level, 1.f);
  REQUIRE(wait_for([&] { return m.has(1.f); }, 2000));
  // Past the window
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  for(int i = 0; i < 20; i++)
  {
    ossia_parameter_push_f(remote, 300.f + i);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  REQUIRE(wait_for([&] { return value_of(level) == 319.f; }, 2000));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  auto got = m.values();
  INFO("the client received " << got.size() << " values");
  REQUIRE(got == std::vector<float>{1.f});
}

// The answer to a crossing is the client's own value: it opens no window.
// A client that keeps writing after a crossing gets answers only within the
// window of the server's value.
TEST_CASE("C API: the answer to a crossing opens no window", "[network_c]")
{
  server s{"c_answer_server"};
  auto level = float_parameter(s.dev, "/level");
  mirror m{s.url(), "c_answer_mirror"};
  auto remote = m.listen("/level");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  const auto sent = std::chrono::steady_clock::now();
  ossia_parameter_push_f(level, 1.f);
  std::vector<std::chrono::steady_clock::duration> at;
  for(int i = 0; i < 30; i++)
  {
    at.push_back(std::chrono::steady_clock::now() - sent);
    ossia_parameter_push_f(remote, 400.f + i);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  REQUIRE(wait_for([&] { return value_of(level) == 429.f; }, 2000));
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  int answered = 0, late = 0;
  for(float v : m.values())
  {
    if(v < 400.f)
      continue;
    answered++;
    // Written past the window and a margin for the trip
    if(at[int(v - 400.f)] > std::chrono::milliseconds(150))
      late++;
  }
  INFO(answered << " writes answered, " << late << " of them written past the window");
  REQUIRE(late == 0);
  REQUIRE(value_of(remote) == 429.f);
}

// The same while the server forwards each client's values to the others: the
// value the server forwards for the client must not hide the one it sent.
TEST_CASE("C API: crossed writes converge while the server forwards", "[network_c]")
{
  server s{"c_cross_echo_server"};
  auto level = float_parameter(s.dev, "/level");
  ossia_device_set_echo(s.dev, 1);
  mirror m{s.url(), "c_cross_echo_a"};
  mirror other{s.url(), "c_cross_echo_b"};
  auto remote = m.listen("/level");
  auto seen = other.listen("/level");
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  int divergences = 0;
  for(int i = 0; i < 20; i++)
  {
    std::atomic_bool go{false};
    std::thread client([&] {
      while(!go)
      {
      }
      ossia_parameter_push_f(remote, 200.f + i);
    });
    go = true;
    ossia_parameter_push_f(level, 100.f + i);
    client.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    const float at_server = value_of(level), at_client = value_of(remote), at_other = value_of(seen);
    if(at_server != at_client || at_server != at_other)
    {
      divergences++;
      UNSCOPED_INFO("trial " << i << ": server " << at_server << ", client " << at_client << ", other " << at_other);
    }
  }
  INFO("divergences " << divergences << " in 20");
  REQUIRE(divergences == 0);
}

namespace
{
// The status code of the reply to GET <resource> on 127.0.0.1:port, 0 on failure
int http_status(uint16_t port, const std::string& resource)
{
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if(::connect(fd, (sockaddr*)&addr, sizeof(addr)) != 0)
  {
    ::close(fd);
    return 0;
  }
  timeval tv{2, 0};
  ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  const std::string req
      = "GET " + resource + " HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
  ::send(fd, req.data(), req.size(), 0);
  char buf[64]{};
  const auto n = ::recv(fd, buf, sizeof(buf) - 1, 0);
  ::close(fd);
  int code = 0;
  return n > 0 && std::sscanf(buf, "HTTP/1.1 %d", &code) == 1 ? code : 0;
}
}

// A peer of the network must not be able to take the server down: the DATA of
// LISTEN and IGNORE is a string in the OSCQuery proposal.
TEST_CASE("C API: LISTEN and IGNORE whose DATA is not a string are ignored", "[network_c]")
{
  server s{"c_listen_data_server"};
  auto level = float_parameter(s.dev, "/level");
  ossia_parameter_push_f(level, 0.5f);
  raw_client c{s.url()};
  REQUIRE(c.ws.connected());

  for(const char* command : {"LISTEN", "IGNORE"})
    for(const char* data : {"12", "null", "true", "[1]", "{\"a\":1}"})
      c.ws.send_message(
          std::string{"{\"COMMAND\":\""} + command + "\",\"DATA\":" + data + "}");
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  REQUIRE(http_get(s.ws, "/level?VALUE") == "{\"VALUE\":0.5}");
}

// The proposal: 404 for a path that does not exist, 400 for a bad request.
TEST_CASE("C API: a missing node is 404 and a bad request 400", "[network_c]")
{
  server s{"c_http_status_server"};
  float_parameter(s.dev, "/level");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  REQUIRE(http_status(s.ws, "/level?VALUE") == 200);
  REQUIRE(http_status(s.ws, "/nothing") == 404);
  REQUIRE(http_status(s.ws, "/nothing?TYPE") == 404);
  // LISTEN over HTTP has no WebSocket client to apply to
  REQUIRE(http_status(s.ws, "/level?LISTEN=true") == 400);
}

namespace
{
// Whether a strict parser reads this text: JSON has no NaN or Infinity
bool strict_json(const std::string& text)
{
  rapidjson::Document doc;
  doc.Parse<rapidjson::kParseNoFlags>(text.c_str(), text.size());
  return !text.empty() && !doc.HasParseError();
}
}

// A value JSON cannot write is null: one of them made the whole tree unreadable
TEST_CASE("C API: a value that is not finite is served as null", "[network_c]")
{
  server s{"c_nan_server"};
  auto nan = float_parameter(s.dev, "/nan");
  auto inf = float_parameter(s.dev, "/inf");
  auto vec = ossia_node_create_parameter(
      ossia_node_create(ossia_device_get_root_node(s.dev), "/vec"), VEC3F_T);
  ossia_parameter_push_f(nan, std::nanf(""));
  ossia_parameter_push_f(inf, HUGE_VALF);
  ossia_parameter_push_3f(vec, 1.f, std::nanf(""), 3.f);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  REQUIRE(http_get(s.ws, "/nan?VALUE") == "{\"VALUE\":null}");
  REQUIRE(http_get(s.ws, "/inf?VALUE") == "{\"VALUE\":null}");
  REQUIRE(http_get(s.ws, "/vec?VALUE") == "{\"VALUE\":[1.0,null,3.0]}");
  REQUIRE(strict_json(http_get(s.ws, "/")));
}

TEST_CASE("C API: a range or an attribute that is not finite keeps the JSON valid", "[network_c]")
{
  server s{"c_nan_attr_server"};
  auto level = float_parameter(s.dev, "/level");
  auto min = ossia_value_create_float(std::nanf(""));
  auto max = ossia_value_create_float(1.f);
  ossia_parameter_set_domain(level, ossia_domain_make_min_max(min, max));
  ossia_value_free(min);
  ossia_value_free(max);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  REQUIRE(strict_json(http_get(s.ws, "/level")));

  auto node = ossia_node_find(ossia_device_get_root_node(s.dev), "/level");
  ossia_node_set_priority(node, std::nanf(""));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  REQUIRE(strict_json(http_get(s.ws, "/level")));

  ossia_node_unset_priority(node);
  ossia_node_set_value_step_size(node, std::nan(""));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  REQUIRE(strict_json(http_get(s.ws, "/level")));
}

// CHAR_T of the C API is converted to val_type::MAP. A map has no JSON writer:
// its key was left without a value, and the tree could not be read.
TEST_CASE("C API: a parameter of type map keeps the JSON valid", "[network_c]")
{
  server s{"c_map_server"};
  float_parameter(s.dev, "/level");
  ossia_node_create_parameter(ossia_node_create(ossia_device_get_root_node(s.dev), "/map"), CHAR_T);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  REQUIRE(http_get(s.ws, "/map?VALUE") == "{\"VALUE\":null}");
  REQUIRE(strict_json(http_get(s.ws, "/map")));
  REQUIRE(strict_json(http_get(s.ws, "/")));
}

namespace
{
// An entry of the EXTENSIONS of HOST_INFO: 1 true, 0 false, -1 absent
int extension(uint16_t port, const char* key)
{
  rapidjson::Document doc;
  doc.Parse(http_get(port, "/?HOST_INFO").c_str());
  if(doc.HasParseError() || !doc.HasMember("EXTENSIONS") || !doc["EXTENSIONS"].HasMember(key))
    return -1;
  return doc["EXTENSIONS"][key].GetBool() ? 1 : 0;
}
}

TEST_CASE("C API: HOST_INFO says whether the server echoes", "[network_c]")
{
  server s{"c_echo_declared_server"};
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  REQUIRE(extension(s.ws, "ECHO") == 0);
  ossia_device_set_echo(s.dev, 1);
  REQUIRE(extension(s.ws, "ECHO") == 1);
  ossia_device_set_echo(s.dev, 0);
  REQUIRE(extension(s.ws, "ECHO") == 0);
}

// An optional attribute that is not listed is not supported, for a client
TEST_CASE("C API: EXTENDED_TYPE is declared in HOST_INFO", "[network_c]")
{
  server s{"c_ext_type_declared_server"};
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  REQUIRE(extension(s.ws, "EXTENDED_TYPE") == 1);
}

namespace
{
// A datagram from another address than a client's, from the source port that
// client declared, is not that client's: it is forwarded to it like any other.
void same_port_on_another_address(
    ossia_device_t dev, uint16_t osc, const std::string& url, const std::string& other)
{
  auto level = float_parameter(dev, "/level");
  ossia_device_set_echo(dev, 1);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  udp_sink sink{free_port(false)};
  raw_client client{url};
  REQUIRE(client.ws.connected());
  const uint16_t sender = free_port(false);
  client.stream_to(sink.port(), sender);
  client.ws.send_message(ossia::oscquery::json_writer::listen("/level"));
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  const int sent = send_osc_float(other, osc, "/level", 0.25f, other, sender);
  const bool applied = wait_for([&] { return value_of(level) == 0.25f; }, 2000);
  if(!applied)
  {
    // What the server saw of the datagram: a sender that is none of its
    // clients is listed in ?PEERS
    const auto ws = static_cast<uint16_t>(std::stoi(url.substr(url.rfind(':') + 1)));
    UNSCOPED_INFO(
        "sendto: " << (sent == 0 ? "sent" : std::strerror(sent)) << "; the other address is on "
                   << interface_of(other) << "; ?PEERS: " << http_get(ws, "/?PEERS"));
  }
  REQUIRE(applied);
  REQUIRE(sink.wait_messages(1, 2000));
}
}

TEST_CASE("C API: asio server knows a sender by its address and port", "[.][sender_address]")
{
  const auto other = other_ipv4();
  if(other.empty())
    SKIP("no interface other than the loopback");
  if(!warm_up_from(other, warm_up_ms))
    SKIP("the warm-up datagram from " << interface_of(other) << " did not arrive in "
                                      << warm_up_ms / 1000
                                      << " s: the macOS firewall held the flow");
  server s{"c_sender_address_asio"};
  same_port_on_another_address(s.dev, s.osc, s.url(), other);
}

TEST_CASE("C API: WebSocket++ server knows a sender by its address and port", "[.][sender_address]")
{
  const auto other = other_ipv4();
  if(other.empty())
    SKIP("no interface other than the loopback");
  if(!warm_up_from(other, warm_up_ms))
    SKIP("the warm-up datagram from " << interface_of(other) << " did not arrive in "
                                      << warm_up_ms / 1000
                                      << " s: the macOS firewall held the flow");
  const uint16_t osc = free_port(false), ws = free_port(true);
  auto dev = ossia_device_create(ossia_protocol_oscquery_server_create(osc, ws), "c_sender_address_wspp");
  REQUIRE(dev);
  same_port_on_another_address(dev, osc, "ws://127.0.0.1:" + std::to_string(ws), other);
  ossia_device_free(dev);
}

namespace
{
// A WebSocket client that keeps the text messages of the server
struct text_client
{
  std::mutex mutex;
  std::vector<std::string> texts;
  // Owns its context: stop() ends connect_and_run()
  ossia::net::websocket_client_beast ws{[this](const ossia::net::ws_connection_handle&, ossia::net::ws_opcode op, std::string& text) {
    if(op == ossia::net::ws_opcode::text)
    {
      std::lock_guard lock{mutex};
      texts.push_back(text);
    }
  }};
  std::thread thread;

  explicit text_client(const std::string& url)
  {
    thread = std::thread([this, url] {
      try
      {
        ws.connect_and_run(url);
      }
      catch(...)
      {
      }
    });
    wait_for([&] { return ws.connected(); }, 2000);
  }
  ~text_client()
  {
    ws.stop();
    thread.join();
  }
  int count(const std::string& what)
  {
    std::lock_guard lock{mutex};
    return std::count_if(texts.begin(), texts.end(), [&](const auto& t) {
      return t.find(what) != std::string::npos;
    });
  }
};

void settle()
{
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void tree_changes_of_a_parameter(ossia_device_t dev, const std::string& url)
{
  settle();
  text_client c{url};
  REQUIRE(c.ws.connected());
  settle();
  auto root = ossia_device_get_root_node(dev);

  // Added: its type once
  auto node = ossia_node_create(root, "/added");
  ossia_node_create_parameter(node, FLOAT_T);
  settle();
  REQUIRE(c.count("\"PATH_ADDED\"") == 1);
  REQUIRE(c.count("\"ATTRIBUTES_CHANGED\"") == 1);

  // The parameter alone removed, the node stays: clients are told
  ossia_node_remove_parameter(node);
  settle();
  REQUIRE(c.count("\"ATTRIBUTES_CHANGED\"") == 2);
  REQUIRE(c.count("\"PATH_REMOVED\"") == 0);

  // The node removed: PATH_REMOVED and nothing after it
  ossia_node_create_parameter(node, FLOAT_T);
  settle();
  REQUIRE(c.count("\"ATTRIBUTES_CHANGED\"") == 3);
  ossia_node_remove_child(root, node);
  settle();
  REQUIRE(c.count("\"PATH_REMOVED\"") == 1);
  REQUIRE(c.count("\"ATTRIBUTES_CHANGED\"") == 3);

  // Nor for the parameters below a node removed
  auto parent = ossia_node_create(root, "/parent");
  ossia_node_create_parameter(ossia_node_create(root, "/parent/child"), FLOAT_T);
  settle();
  const auto before = c.count("\"ATTRIBUTES_CHANGED\"");
  ossia_node_remove_child(root, parent);
  settle();
  REQUIRE(c.count("\"ATTRIBUTES_CHANGED\"") == before);
}
}

TEST_CASE("C API: asio server sends each tree change of a parameter once", "[network_c]")
{
  server s{"c_tree_changes_asio"};
  tree_changes_of_a_parameter(s.dev, s.url());
}

TEST_CASE("C API: WebSocket++ server sends each tree change of a parameter once", "[network_c]")
{
  const uint16_t osc = free_port(false), ws = free_port(true);
  auto dev = ossia_device_create(ossia_protocol_oscquery_server_create(osc, ws), "c_tree_changes_wspp");
  REQUIRE(dev);
  tree_changes_of_a_parameter(dev, "ws://127.0.0.1:" + std::to_string(ws));
  ossia_device_free(dev);
}

namespace
{
// One member of the JSON at that resource, as text
std::string member_of(uint16_t port, const std::string& resource, const char* key)
{
  rapidjson::Document doc;
  doc.Parse(http_get(port, resource).c_str());
  if(doc.HasParseError() || !doc.IsObject() || !doc.HasMember(key))
    return "absent";
  rapidjson::StringBuffer buf;
  rapidjson::Writer<rapidjson::StringBuffer> wr(buf);
  doc[key].Accept(wr);
  return buf.GetString();
}
}

// What a node does not have is an empty object, as for the TYPE of a container
// in the proposal; what a query says agrees with the tree.
TEST_CASE("C API: attribute queries agree with the tree", "[network_c]")
{
  server s{"c_attr_query_server"};
  auto root = ossia_device_get_root_node(s.dev);
  float_parameter(s.dev, "/ctn/sub");
  float_parameter(s.dev, "/f");
  ossia_node_create_parameter(ossia_node_create(root, "/i"), INT_T);
  auto v3 = ossia_node_create_parameter(ossia_node_create(root, "/v3"), VEC3F_T);
  ossia_parameter_set_unit(v3, "position.cart3D");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for(const char* query : {"/ctn?TYPE", "/ctn?VALUE", "/ctn?RANGE", "/?VALUE", "/f?UNIT",
                           "/f?EXTENDED_TYPE", "/i?RANGE", "/v3?UNIT"})
  {
    INFO(query);
    REQUIRE(http_status(s.ws, query) == 200);
    REQUIRE(http_get(s.ws, query) == "{}");
  }

  for(const char* key : {"FULL_PATH", "TYPE", "VALUE", "ACCESS", "CLIPMODE", "EXTENDED_TYPE"})
  {
    INFO(key);
    REQUIRE(member_of(s.ws, std::string{"/v3?"} + key, key) == member_of(s.ws, "/v3", key));
  }
  REQUIRE(member_of(s.ws, "/v3?EXTENDED_TYPE", "EXTENDED_TYPE").find("position.cartesian.x") != std::string::npos);
  REQUIRE(member_of(s.ws, "/ctn?CONTENTS", "CONTENTS") == member_of(s.ws, "/ctn", "CONTENTS"));
  REQUIRE(http_get(s.ws, "/f?TYPE&ACCESS") == "{\"TYPE\":\"f\",\"ACCESS\":3}");
}

TEST_CASE("C API: an unknown attribute is 204 without a body", "[network_c]")
{
  server s{"c_attr_unknown_server"};
  float_parameter(s.dev, "/ctn/sub");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for(const char* query : {"/ctn/sub?BOGUS", "/ctn?BOGUS", "/?BOGUS", "/ctn/sub?BOGUS=1"})
  {
    INFO(query);
    REQUIRE(http_status(s.ws, query) == 204);
    REQUIRE(http_get(s.ws, query).empty());
  }
  REQUIRE(http_status(s.ws, "/nothing?BOGUS") == 404);
  // A known attribute with a value is still that attribute
  REQUIRE(http_status(s.ws, "/ctn/sub?VALUE=1") == 200);
  REQUIRE(member_of(s.ws, "/ctn/sub?VALUE=1", "VALUE") == member_of(s.ws, "/ctn/sub", "VALUE"));
}

// The proposal: the fragment of the URL is stripped and disregarded
TEST_CASE("C API: the fragment of a URL is disregarded", "[network_c]")
{
  server s{"c_fragment_server"};
  auto level = float_parameter(s.dev, "/level");
  ossia_parameter_push_f(level, 0.5f);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  REQUIRE(http_status(s.ws, "/level#frag") == 200);
  REQUIRE(http_get(s.ws, "/level#frag") == http_get(s.ws, "/level"));
  REQUIRE(http_get(s.ws, "/level?VALUE#frag") == "{\"VALUE\":0.5}");
  REQUIRE(http_status(s.ws, "/nothing#frag") == 404);
}

namespace
{
// What a client writes to a read-only parameter is ignored, by each way a
// value can come in. /rw takes the same way each time, to prove the way works.
void access_is_applied(ossia_device_t dev, uint16_t osc, uint16_t ws, const std::string& url)
{
  auto ro = float_parameter(dev, "/ro");
  auto rw = float_parameter(dev, "/rw");
  auto so = float_parameter(dev, "/so");
  ossia_parameter_set_access_mode(ro, GET);
  ossia_parameter_set_access_mode(so, SET);
  ossia_parameter_push_f(ro, 0.5f);
  ossia_parameter_push_f(so, 0.25f);
  settle();
  raw_client c{url};
  REQUIRE(c.ws.connected());
  settle();

  int n = 1;
  auto written = [&](const std::function<void(const char*, float)>& write) {
    const float value = 10.f * n++;
    write("/ro", value + 1.f);
    write("/rw", value);
    REQUIRE(wait_for([&] { return value_of(rw) == value; }, 2000));
    settle();
    REQUIRE(value_of(ro) == 0.5f);
  };
  written([&](const char* a, float f) { send_osc_float("127.0.0.1", osc, a, f); });
  written([&](const char* a, float f) { c.ws.send_binary_message(osc_float_message(a, f)); });
  written([&](const char* a, float f) {
    c.ws.send_message(std::string{"{\"" } + a + "\":" + std::to_string(f) + "}");
  });

  // By a pattern: /rw is reached, /ro is not
  send_osc_float("127.0.0.1", osc, "/r?", 77.f);
  REQUIRE(wait_for([&] { return value_of(rw) == 77.f; }, 2000));
  settle();
  REQUIRE(value_of(ro) == 0.5f);

  // The same once the parameters are observed, which is another way in the server
  auto noop = [](void*, ossia_value_t) {};
  ossia_parameter_add_callback(ro, noop, nullptr);
  ossia_parameter_add_callback(rw, noop, nullptr);
  settle();
  written([&](const char* a, float f) { send_osc_float("127.0.0.1", osc, a, f); });
  send_osc_float("127.0.0.1", osc, "/r?", 78.f);
  REQUIRE(wait_for([&] { return value_of(rw) == 78.f; }, 2000));
  settle();
  REQUIRE(value_of(ro) == 0.5f);

  // A write-only parameter: its value cannot be queried, and it can be written
  send_osc_float("127.0.0.1", osc, "/so", 3.f);
  REQUIRE(wait_for([&] { return value_of(so) == 3.f; }, 2000));
  REQUIRE(http_status(ws, "/so?VALUE") == 204);
  REQUIRE(http_get(ws, "/so?VALUE").empty());
  REQUIRE(member_of(ws, "/so?VALUE&TYPE", "VALUE") == "absent");
  REQUIRE(member_of(ws, "/so?VALUE&TYPE", "TYPE") == "\"f\"");
  REQUIRE(http_status(ws, "/ro?VALUE") == 200);
}
}

TEST_CASE("C API: asio server applies ACCESS", "[network_c]")
{
  server s{"c_access_asio", 0, "127.0.0.1"};
  access_is_applied(s.dev, s.osc, s.ws, s.url());
}

TEST_CASE("C API: WebSocket++ server applies ACCESS", "[network_c]")
{
  const uint16_t osc = free_port(false), ws = free_port(true);
  auto dev = ossia_device_create(ossia_protocol_oscquery_server_create(osc, ws), "c_access_wspp");
  REQUIRE(dev);
  access_is_applied(dev, osc, ws, "ws://127.0.0.1:" + std::to_string(ws));
  ossia_device_free(dev);
}

namespace
{
// The addresses a sink received, each once, in order of arrival
std::vector<std::string> addresses_of(udp_sink& sink)
{
  std::lock_guard lock{sink.mutex};
  std::vector<std::string> out;
  for(auto& [address, value] : sink.messages)
    if(std::find(out.begin(), out.end(), address) == out.end())
      out.push_back(address);
  std::sort(out.begin(), out.end());
  return out;
}

void clear(udp_sink& sink)
{
  std::lock_guard lock{sink.mutex};
  sink.messages.clear();
}

using addresses = std::vector<std::string>;

// A client whose values come over UDP, to a sink of its own
struct udp_follower
{
  udp_sink sink{free_port(false)};
  uint16_t sender = free_port(false);
  raw_client client;

  explicit udp_follower(const std::string& url)
      : client{url}
  {
    REQUIRE(client.ws.connected());
    client.stream_to(sink.port(), sender);
  }
  void listen(const char* path)
  {
    client.ws.send_message(ossia::oscquery::json_writer::listen(path));
  }
  void ignore(const char* path)
  {
    client.ws.send_message(ossia::oscquery::json_writer::ignore(path));
  }
};

// A client whose values come over the WebSocket
struct ws_follower
{
  udp_sink sink{free_port(false)};
  // Owns its context: stop() ends connect_and_run()
  ossia::net::websocket_client_beast ws{[this](const ossia::net::ws_connection_handle&, ossia::net::ws_opcode op, std::string& data) {
    if(op == ossia::net::ws_opcode::binary)
    {
      std::lock_guard lock{sink.mutex};
      sink.decode(data.data(), data.size());
    }
  }};
  std::thread thread;

  explicit ws_follower(const std::string& url)
  {
    thread = std::thread([this, url] {
      try
      {
        ws.connect_and_run(url);
      }
      catch(...)
      {
      }
    });
    wait_for([&] { return ws.connected(); }, 2000);
  }
  ~ws_follower()
  {
    ws.stop();
    thread.join();
  }
  void listen(const char* path)
  {
    ws.send_message(ossia::oscquery::json_writer::listen(path));
  }
};

struct listen_device
{
  ossia_parameter_t a, b, sub;
  explicit listen_device(ossia_device_t dev)
  {
    a = float_parameter(dev, "/a");
    b = float_parameter(dev, "/b");
    sub = float_parameter(dev, "/ctn/sub");
  }
  void push_all(float f)
  {
    ossia_parameter_push_f(a, f);
    ossia_parameter_push_f(b, f);
    ossia_parameter_push_f(sub, f);
  }
};

void listen_filters(ossia_device_t dev, uint16_t osc, const std::string& url)
{
  listen_device d{dev};
  ossia_device_set_echo(dev, 1);
  settle();
  udp_follower never{url}, partial{url}, ignoring{url}, container{url}, writer{url};
  udp_follower unknown{url}, by_query{url};
  unknown.listen("/nope");
  by_query.client.ws.send_message(std::string{"/a?LISTEN=TRUE"});
  partial.listen("/a");
  ignoring.listen("/a");
  ignoring.listen("/b");
  ignoring.ignore("/b");
  container.listen("/ctn");
  writer.listen("/b");
  settle();

  auto check = [&](const addresses& for_writer) {
    settle();
    REQUIRE(addresses_of(never.sink).empty());
    REQUIRE(addresses_of(partial.sink) == addresses{"/a"});
    REQUIRE(addresses_of(ignoring.sink) == addresses{"/a"});
    REQUIRE(addresses_of(container.sink).empty());
    REQUIRE(addresses_of(unknown.sink).empty());
    REQUIRE(addresses_of(by_query.sink) == addresses{"/a"});
    REQUIRE(addresses_of(writer.sink) == for_writer);
    for(auto s : {&never.sink, &partial.sink, &ignoring.sink, &container.sink, &unknown.sink,
                  &by_query.sink, &writer.sink})
      clear(*s);
  };

  // A value of the server
  d.push_all(0.5f);
  check({"/b"});

  // A value of another client, forwarded by the echo
  for(const char* path : {"/a", "/b", "/ctn/sub"})
    send_osc_float("127.0.0.1", osc, path, 0.75f);
  check({"/b"});

  // Over the WebSocket, for a critical parameter
  ossia_parameter_set_critical(d.a, 1);
  ossia_parameter_set_critical(d.b, 1);
  ws_follower ws_partial{url}, ws_never{url};
  ws_partial.listen("/a");
  settle();
  ossia_parameter_push_f(d.a, 0.1f);
  ossia_parameter_push_f(d.b, 0.1f);
  settle();
  REQUIRE(addresses_of(ws_partial.sink) == addresses{"/a"});
  REQUIRE(addresses_of(ws_never.sink).empty());
  clear(ws_partial.sink);
  clear(ws_never.sink);

  // A critical value of another client, forwarded by the echo
  for(const char* path : {"/a", "/b"})
    send_osc_float("127.0.0.1", osc, path, 0.2f);
  settle();
  REQUIRE(addresses_of(ws_partial.sink) == addresses{"/a"});
  REQUIRE(addresses_of(ws_never.sink).empty());
}
}

TEST_CASE("C API: asio server filters by LISTEN", "[network_c]")
{
  server s{"c_listen_asio", 0, "127.0.0.1"};
  listen_filters(s.dev, s.osc, s.url());
}

TEST_CASE("C API: WebSocket++ server filters by LISTEN", "[network_c]")
{
  const uint16_t osc = free_port(false), ws = free_port(true);
  auto dev = ossia_device_create(ossia_protocol_oscquery_server_create(osc, ws), "c_listen_wspp");
  REQUIRE(dev);
  listen_filters(dev, osc, "ws://127.0.0.1:" + std::to_string(ws));
  ossia_device_free(dev);
}

// A bundle holds several addresses: each client gets those it listens to
TEST_CASE("C API: a bundle follows what each client listens to", "[network_c]")
{
  server s{"c_listen_bundle", 0, "127.0.0.1"};
  listen_device d{s.dev};
  settle();
  udp_follower never{s.url()}, partial{s.url()}, none{s.url()};
  partial.listen("/a");
  none.listen("/ctn");
  settle();

  ossia_parameter_t params[]{d.a, d.b, d.sub};
  ossia_value_t values[]{ossia_value_create_float(1.f), ossia_value_create_float(2.f),
                         ossia_value_create_float(3.f)};
  ossia_device_push_bundle(s.dev, params, values, 3);
  for(auto v : values)
    ossia_value_free(v);
  settle();

  REQUIRE(addresses_of(never.sink).empty());
  REQUIRE(addresses_of(partial.sink) == addresses{"/a"});
  REQUIRE(addresses_of(none.sink).empty());
}

// A client that does not listen to an address was sent nothing there: its
// write cannot have crossed anything, and it gets no answer
TEST_CASE("C API: no answer to a crossing on an address not listened to", "[network_c]")
{
  server s{"c_listen_crossing", 0, "127.0.0.1"};
  listen_device d{s.dev};
  settle();
  udp_follower writer{s.url()};
  writer.listen("/b");
  settle();

  ossia_parameter_push_f(d.a, 0.5f);
  send_osc_float("127.0.0.1", s.osc, "/a", 0.75f, "127.0.0.1", writer.sender);
  REQUIRE(wait_for([&] { return value_of(d.a) == 0.75f; }, 2000));
  settle();
  REQUIRE(addresses_of(writer.sink).empty());
}

namespace
{
struct http_ignored
{
  static constexpr int reserve_expect = 65535;
  template <typename T, typename S>
  void operator()(T&, const S&)
  {
  }
};
struct http_error_ignored
{
  template <typename T>
  void operator()(T&)
  {
  }
};
std::string http_client_log(
    uint16_t port, const std::string& path, const std::string& host = "127.0.0.1")
{
  std::ostringstream log;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(log);
  auto& sinks = ossia::logger().sinks();
  sinks.push_back(sink);
  boost::asio::io_context ctx;
  using request = ossia::net::http_get_request<http_ignored, http_error_ignored>;
  std::make_shared<request>(http_ignored{}, http_error_ignored{}, ctx, host, path)
      ->resolve(host, std::to_string(port));
  ctx.run_for(std::chrono::seconds(2));
  sinks.pop_back();
  return log.str();
}
}

// The value of a write-only parameter is 204 since ACCESS is applied: a client
// that asks for it is told so, it is not an error of the network
TEST_CASE("C API: the HTTP client does not log a 204 as an error", "[network_c]")
{
  server s{"c_http_204", 0, "127.0.0.1"};
  ossia_parameter_set_access_mode(float_parameter(s.dev, "/so"), SET);
  settle();
  REQUIRE(http_client_log(s.ws, "/so?VALUE").find("HTTP Error") == std::string::npos);
  REQUIRE(http_client_log(s.ws, "/nothing").find("status code 404") != std::string::npos);
}

// "HTTP Error: Host not found (authoritative)" in the console of Max said
// neither which device nor which request: the line names both
TEST_CASE("C API: an HTTP error names the host and the request", "[network_c]")
{
  const auto unknown = http_client_log(1234, "/f?VALUE", "prism-host.invalid");
  INFO(unknown);
  REQUIRE(unknown.find("HTTP Error") != std::string::npos);
  REQUIRE(unknown.find("prism-host.invalid:1234/f?VALUE") != std::string::npos);

  const auto closed_port = free_port(true);
  const auto refused = http_client_log(closed_port, "/f");
  INFO(refused);
  REQUIRE(refused.find("127.0.0.1:" + std::to_string(closed_port) + "/f") != std::string::npos);

  server s{"c_http_named", 0, "127.0.0.1"};
  settle();
  const auto missing = http_client_log(s.ws, "/nothing");
  INFO(missing);
  REQUIRE(missing.find("127.0.0.1:" + std::to_string(s.ws) + "/nothing: status code 404") != std::string::npos);
}

namespace
{
rapidjson::Document json_at(uint16_t port, const std::string& resource)
{
  rapidjson::Document doc;
  doc.Parse(http_get(port, resource).c_str());
  return doc;
}

bool has_key(const rapidjson::Value& v, const char* key)
{
  return v.IsObject() && v.HasMember(key);
}
}

TEST_CASE("C API: PEERS and CLIENT_NAME are declared", "[network_c]")
{
  server s{"c_peers_declared", 0, "127.0.0.1"};
  settle();
  REQUIRE(extension(s.ws, "PEERS") == 1);
  REQUIRE(extension(s.ws, "CLIENT_NAME") == 1);
  REQUIRE(extension(s.ws, "PRISM_CONNECTIONS") == 1);
}

TEST_CASE("C API: ?PEERS lists the clients and the senders", "[network_c]")
{
  server s{"c_peers_server", 0, "127.0.0.1"};
  float_parameter(s.dev, "/level");
  settle();
  raw_client client{s.url()};
  REQUIRE(client.ws.connected());
  settle();

  // Two UDP sources, the first one sent first
  const uint16_t first = free_port(false), second = free_port(false);
  send_osc_float("127.0.0.1", s.osc, "/level", 0.25f, "127.0.0.1", first);
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  send_osc_float("127.0.0.1", s.osc, "/level", 0.5f, "127.0.0.1", second);
  settle();

  auto doc = json_at(s.ws, "/?PEERS");
  REQUIRE(!doc.HasParseError());
  REQUIRE(doc["CLIENTS"].Size() == 1);
  const auto& c = doc["CLIENTS"][0];
  REQUIRE(std::string{c["IP"].GetString()} == "127.0.0.1");
  REQUIRE(c["PORT"].GetInt() > 0);
  REQUIRE(c["LISTEN"].IsArray());
  REQUIRE(c["PACKETS"].IsInt64());
  // A field the server cannot fill is omitted
  REQUIRE(!has_key(c, "NAME"));
  REQUIRE(!has_key(c, "OSC_PORT"));
  REQUIRE(!has_key(c, "SENDER_PORT"));

  // Most recent first
  REQUIRE(doc["SENDERS"].Size() == 2);
  REQUIRE(doc["SENDERS"][0]["PORT"].GetInt() == second);
  REQUIRE(doc["SENDERS"][1]["PORT"].GetInt() == first);
  REQUIRE(doc["SENDERS"][0]["LAST_MS"].IsInt64());

  // Whatever the order of the table the server keeps them in
  std::vector<uint16_t> sources;
  for(int i = 0; i < 12; i++)
  {
    sources.push_back(free_port(false));
    send_osc_float("127.0.0.1", s.osc, "/level", 0.25f, "127.0.0.1", sources.back());
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
  }
  settle();
  auto ordered = json_at(s.ws, "/?PEERS");
  REQUIRE(ordered["SENDERS"].Size() == 14);
  for(int i = 0; i < 12; i++)
    REQUIRE(ordered["SENDERS"][i]["PORT"].GetInt() == sources[11 - i]);

  // A source that a client has since declared is that client
  udp_sink sink{free_port(false)};
  client.stream_to(sink.port(), first);
  settle();
  auto after = json_at(s.ws, "/?PEERS");
  REQUIRE(after["SENDERS"].Size() == 13);
  for(auto& sender : after["SENDERS"].GetArray())
    REQUIRE(sender["PORT"].GetInt() != first);
  REQUIRE(after["CLIENTS"][0]["OSC_PORT"].GetInt() == sink.port());
  REQUIRE(after["CLIENTS"][0]["SENDER_PORT"].GetInt() == first);

  // ?PRISM_CONNECTIONS does not change
  auto old = json_at(s.ws, "/level?PRISM_CONNECTIONS");
  REQUIRE(!old.HasParseError());
  REQUIRE(old["SENDERS"].Size() == 14);

  // At the root only: elsewhere it is an unknown attribute
  REQUIRE(http_status(s.ws, "/level?PEERS") == 204);
  REQUIRE(http_status(s.ws, "/nothing?PEERS") == 404);
}

TEST_CASE("C API: CLIENT_NAME names a client", "[network_c]")
{
  server s{"c_client_name_server", 0, "127.0.0.1"};
  settle();
  raw_client client{s.url()};
  REQUIRE(client.ws.connected());
  auto name = [&] {
    settle();
    auto doc = json_at(s.ws, "/?PEERS");
    const auto& c = doc["CLIENTS"][0];
    return has_key(c, "NAME") ? std::string{c["NAME"].GetString()} : std::string{"absent"};
  };
  REQUIRE(name() == "absent");
  client.ws.send_message(std::string{"{\"COMMAND\":\"CLIENT_NAME\",\"DATA\":\"one\"}"});
  REQUIRE(name() == "one");
  client.ws.send_message(std::string{"{\"COMMAND\":\"CLIENT_NAME\",\"DATA\":\"two\"}"});
  REQUIRE(name() == "two");
  client.ws.send_message(std::string{"{\"COMMAND\":\"CLIENT_NAME\",\"DATA\":12}"});
  REQUIRE(name() == "two");
  // PRISM_NAME still does, during the transition
  client.ws.send_message(std::string{"{\"COMMAND\":\"PRISM_NAME\",\"DATA\":\"old\"}"});
  REQUIRE(name() == "old");
}

namespace
{
// A WebSocket client that opens its connection and resets it at once: with
// read_answer, once the server has answered the handshake
void open_and_reset(uint16_t port, bool read_answer)
{
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  REQUIRE(fd >= 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  REQUIRE(::connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
  const std::string handshake
      = "GET / HTTP/1.1\r\nHost: 127.0.0.1:" + std::to_string(port)
        + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
          "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
  REQUIRE(::send(fd, handshake.data(), handshake.size(), 0) == (ssize_t)handshake.size());
  if(read_answer)
  {
    std::string answer;
    char buf[512];
    while(answer.find("\r\n\r\n") == std::string::npos)
    {
      auto n = ::recv(fd, buf, sizeof(buf), 0);
      if(n <= 0)
        break;
      answer.append(buf, n);
    }
    REQUIRE(answer.find(" 101 ") != std::string::npos);
  }
  linger l{1, 0};
  ::setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
  ::close(fd);
}
}

// A client that leaves as soon as it is connected (a mirror stopped as it
// connects, a port scan) put "on_connectionOpen: remote_endpoint: Invalid
// argument" in red in the console of Max
TEST_CASE("C API: asio server: a client that leaves as it connects is no error", "[network_c]")
{
  std::ostringstream log;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(log);
  ossia::logger().sinks().push_back(sink);
  {
    server s{"c_leaves_at_once", 0, "127.0.0.1"};
    settle();
    for(int i = 0; i < 200; i++)
      open_and_reset(s.ws, i % 2 == 0);
    settle();
    auto doc = json_at(s.ws, "/?PEERS");
    REQUIRE(doc["CLIENTS"].Size() == 0);

    // The clients that stay are known by their address and port
    raw_client client{s.url()};
    REQUIRE(client.ws.connected());
    settle();
    auto after = json_at(s.ws, "/?PEERS");
    REQUIRE(after["CLIENTS"].Size() == 1);
    REQUIRE(std::string{after["CLIENTS"][0]["IP"].GetString()} == "127.0.0.1");
    REQUIRE(after["CLIENTS"][0]["PORT"].GetInt() > 0);
  }
  ossia::logger().sinks().pop_back();
  INFO(log.str());
  REQUIRE(log.str().find("on_connectionOpen") == std::string::npos);
  REQUIRE(log.str().find("[error]") == std::string::npos);
}

TEST_CASE("C API: WebSocket++ server: a client that leaves as it connects is no error", "[network_c]")
{
  std::ostringstream log;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(log);
  ossia::logger().sinks().push_back(sink);
  {
    const uint16_t osc = free_port(false), ws = free_port(true);
    auto dev = ossia_device_create(ossia_protocol_oscquery_server_create(osc, ws), "c_leaves_wspp");
    REQUIRE(dev);
    settle();
    for(int i = 0; i < 200; i++)
      open_and_reset(ws, i % 2 == 0);
    settle();
    {
      raw_client client{"ws://127.0.0.1:" + std::to_string(ws)};
      REQUIRE(client.ws.connected());
    }
    ossia_device_free(dev);
  }
  ossia::logger().sinks().pop_back();
  INFO(log.str());
  REQUIRE(log.str().find("on_connectionOpen") == std::string::npos);
}

namespace
{
// A WebSocket server of the test: it accepts one client, then sends it a
// close frame and keeps the connection, as a server that is slow to end it
struct closing_server
{
  int listener = ::socket(AF_INET, SOCK_STREAM, 0);
  int client = -1;
  uint16_t port{};
  std::atomic_bool done{false};
  std::thread thread;

  closing_server()
  {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    REQUIRE(::bind(listener, (sockaddr*)&addr, sizeof(addr)) == 0);
    socklen_t len = sizeof(addr);
    ::getsockname(listener, (sockaddr*)&addr, &len);
    port = ntohs(addr.sin_port);
    REQUIRE(::listen(listener, 4) == 0);
    thread = std::thread([this] {
      client = ::accept(listener, nullptr, nullptr);
      if(client < 0)
        return;
      std::string request;
      char buf[1024];
      while(request.find("\r\n\r\n") == std::string::npos)
      {
        auto n = ::recv(client, buf, sizeof(buf), 0);
        if(n <= 0)
          return;
        request.append(buf, n);
      }
      const std::string field = "Sec-WebSocket-Key: ";
      auto k = request.find(field) + field.size();
      auto key = request.substr(k, request.find("\r\n", k) - k);
      boost::beast::websocket::detail::sec_ws_accept_type accept_key;
      boost::beast::websocket::detail::make_sec_ws_accept(accept_key, key);
      const std::string accept{accept_key.data(), accept_key.size()};
      const std::string answer = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n"
                                 "Connection: Upgrade\r\nSec-WebSocket-Accept: "
                                 + accept + "\r\n\r\n";
      ::send(client, answer.data(), answer.size(), 0);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      // Close, 1001: going away
      const unsigned char close_frame[] = {0x88, 0x02, 0x03, 0xE9};
      ::send(client, close_frame, sizeof(close_frame), 0);
      while(!done)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    });
  }
  ~closing_server()
  {
    done = true;
    ::shutdown(listener, SHUT_RDWR);
    ::close(listener);
    thread.join();
    if(client >= 0)
      ::close(client);
  }
  std::string url() const { return "ws://127.0.0.1:" + std::to_string(port); }
};
}

// A mirror stopped while its server closes the connection (a device of
// TouchDesigner or of Max stopped, then its client in Max) put "Error when
// stopping WS server" in red in the console of Max
TEST_CASE("C API: a mirror stopped as its server closes is no error", "[network_c]")
{
  std::ostringstream log;
  auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(log);
  ossia::logger().sinks().push_back(sink);
  std::chrono::milliseconds stopping{};
  {
    closing_server s;
    auto dev = ossia_device_create(ossia_protocol_oscquery_mirror_create(s.url().c_str()), "c_closing_mirror");
    REQUIRE(dev);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    const auto t0 = std::chrono::steady_clock::now();
    ossia_device_free(dev);
    stopping = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);
  }
  ossia::logger().sinks().pop_back();
  INFO(log.str());
  REQUIRE(log.str().find("Error when stopping WS server") == std::string::npos);
  // Nor does it wait for the server to end the connection
  INFO(stopping.count() << " ms");
  REQUIRE(stopping < std::chrono::milliseconds(1000));
}

namespace
{
// A server of the test: it declares these extensions, has an empty tree and
// keeps the commands it receives
struct fake_device
{
  uint16_t port = free_port(true);
  std::string host_info;
  std::mutex mutex;
  std::vector<std::string> commands;
  std::atomic<float> value{1.f};
  std::atomic_bool late_child{false};
  std::atomic_int value_delay_ms{0};
  std::atomic_int lost_answers{0};
  std::atomic_int value_requests{0};
  std::map<std::string, std::string> extra; // path -> JSON of the node
  ossia::net::ws_connection_handle client;
  // The beast server runs on a context of its own, which the thread runs
  ossia::net::network_context_ptr ctx = std::make_shared<ossia::net::network_context>();
  ossia::net::websocket_server_beast ws{ctx};
  std::thread thread;

  explicit fake_device(const std::string& extensions)
  {
    host_info = "{\"NAME\":\"fake\",\"OSC_PORT\":" + std::to_string(free_port(false))
                + ",\"OSC_TRANSPORT\":\"UDP\",\"EXTENSIONS\":{" + extensions + "}}";
    ws.set_open_handler([this](auto hdl) {
      std::lock_guard lock{mutex};
      client = hdl;
    });
    ws.set_close_handler([](auto) {});
    ws.set_message_handler([this](auto, auto, const std::string& text) {
      ossia::net::server_reply reply;
      reply.type = ossia::net::server_reply::data_type::json;
      if(text.find("HOST_INFO") != std::string::npos)
        reply.data = host_info;
      else if(text.find("?VALUE") != std::string::npos)
      {
        value_requests++;
        if(value_delay_ms > 0)
          std::this_thread::sleep_for(std::chrono::milliseconds(value_delay_ms.load()));
        if(lost_answers > 0)
        {
          lost_answers--;
          return reply;
        }
        reply.data = "{\"VALUE\":" + std::to_string(value.load()) + "}";
      }
      else if(auto node = extra_node(text); !node.empty())
        reply.data = node;
      else if(text.size() > 1 && text[0] == '/' && text.find('?') == std::string::npos
              && text != "/f" && text != "/so")
        throw ossia::node_not_found_error{text};
      else if(!text.empty() && text[0] == '/')
        reply.data = "{\"FULL_PATH\":\"/\",\"ACCESS\":0,\"CONTENTS\":{"
                     "\"f\":{\"FULL_PATH\":\"/f\",\"TYPE\":\"f\",\"ACCESS\":3,\"VALUE\":1.0},"
                     "\"so\":{\"FULL_PATH\":\"/so\",\"TYPE\":\"f\",\"ACCESS\":2,\"VALUE\":1.0}"
                     + std::string{late_child ? ",\"late\":{\"FULL_PATH\":\"/late\",\"TYPE\":\"f\",\"ACCESS\":3,\"VALUE\":1.0}" : ""}
                     + "}}";
      else
      {
        std::lock_guard lock{mutex};
        commands.push_back(text);
      }
      return reply;
    });
    ws.listen(port);
    thread = std::thread([this] { ws.run(); });
  }
  ~fake_device()
  {
    ws.stop();
    ctx->context.stop();
    thread.join();
  }
  std::string url() const { return "ws://127.0.0.1:" + std::to_string(port); }
  std::string extra_node(const std::string& path)
  {
    std::lock_guard lock{mutex};
    auto it = extra.find(path);
    return it == extra.end() ? std::string{} : it->second;
  }
  void set_node(const std::string& path, const std::string& json)
  {
    std::lock_guard lock{mutex};
    extra[path] = json;
  }
  void remove_node(const std::string& path)
  {
    std::lock_guard lock{mutex};
    extra.erase(path);
  }
  void send(const std::string& text) { ws.send_message(client, text); }
  void path_changed(const std::string& path)
  {
    send("{\"COMMAND\":\"PATH_CHANGED\",\"DATA\":\"" + path + "\"}");
  }
  int count(const std::string& what)
  {
    std::lock_guard lock{mutex};
    return std::count_if(commands.begin(), commands.end(), [&](const auto& t) {
      return t.find(what) != std::string::npos;
    });
  }
};
}

TEST_CASE("C API: a mirror gives its name with what the server declares", "[network_c]")
{
  struct expected
  {
    const char* extensions;
    int client_name, prism_name;
  };
  for(auto e : {expected{"\"CLIENT_NAME\":true,\"PRISM_CONNECTIONS\":true", 1, 0},
                expected{"\"CLIENT_NAME\":true", 1, 0},
                expected{"\"PRISM_CONNECTIONS\":true", 0, 1}, expected{"", 0, 0}})
  {
    INFO(e.extensions);
    fake_device d{e.extensions};
    settle();
    mirror m{d.url(), "c_named_mirror"};
    settle();
    REQUIRE(d.count("\"CLIENT_NAME\"") == e.client_name);
    REQUIRE(d.count("\"PRISM_NAME\"") == e.prism_name);
  }
}

// A parameter no callback follows gets no value from the server, which sends
// only what is listened to: reading it asks the server, without waiting, and
// at most every refresh_interval
TEST_CASE("C API: reading a mirrored parameter nothing follows asks the server", "[network_c]")
{
  fake_device d{"\"LISTEN\":true"};
  settle();
  mirror m{d.url(), "c_read_mirror"};
  auto f = find_parameter(m.dev, "/f");
  REQUIRE(f);
  settle();

  d.value = 2.f;
  REQUIRE(wait_for([&] { return value_of(f) == 2.f; }, 2000));
  REQUIRE(d.value_requests >= 1);

  // Read every millisecond, it asks every refresh_interval
  using clock = std::chrono::steady_clock;
  {
    const int before = d.value_requests;
    const auto end = clock::now() + std::chrono::milliseconds(350);
    while(clock::now() < end)
    {
      value_of(f);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const int n = d.value_requests - before;
    INFO(n << " requests in 350 ms");
    REQUIRE(n >= 2);
    REQUIRE(n <= 4);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  const int asked = d.value_requests;

  // A read does not wait for the server, and reads in a row ask once
  d.value_delay_ms = 500;
  d.value = 3.f;
  double worst = 0;
  for(int i = 0; i < 100; i++)
  {
    const auto t0 = clock::now();
    REQUIRE(value_of(f) == 2.f);
    worst = std::max(worst, std::chrono::duration<double, std::milli>(clock::now() - t0).count());
  }
  INFO("longest read " << worst << " ms");
  REQUIRE(worst < 50);
  for(int i = 0; i < 2; i++)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    REQUIRE(value_of(f) == 2.f);
  }
  REQUIRE(wait_for([&] { return value_of(f) == 3.f; }, 2000));
  d.value_delay_ms = 0;
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  // The request, and one that a read after the answer may have sent
  REQUIRE(d.value_requests <= asked + 2);

  // Once followed, the values come by themselves: no request
  auto noop = [](void*, ossia_value_t) {};
  auto cb = ossia_parameter_add_callback(f, noop, nullptr);
  const int followed = d.value_requests;
  d.value = 4.f;
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  REQUIRE(value_of(f) == 3.f);
  REQUIRE(d.value_requests == followed);
  ossia_parameter_remove_callback(f, cb);

  // What this client has just written is the latest it knows
  ossia_parameter_push_f(f, 9.f);
  REQUIRE(value_of(f) == 9.f);
  REQUIRE(d.value_requests == followed);

  // The value of a write-only parameter cannot be read
  auto so = find_parameter(m.dev, "/so");
  REQUIRE(so);
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  value_of(so);
  settle();
  REQUIRE(d.value_requests == followed);

  // An answer that never comes holds the address one second at most
  std::this_thread::sleep_for(std::chrono::milliseconds(600));
  d.lost_answers = 1;
  d.value = 5.f;
  value_of(f);
  REQUIRE(wait_for([&] { return d.value_requests == followed + 1; }, 1000));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  value_of(f);
  REQUIRE(d.value_requests == followed + 1);
  REQUIRE(wait_for([&] { return value_of(f) == 5.f; }, 3000));
}

// A Remote of Unity reads the value as it binds, then follows it, and its
// script writes the field before the answer to that read comes: the answer,
// older than the write, must not bring the old value back
// (ClientTests.RemoteRaisesReceivedForTheValuesOfTheDevice got it twice)
TEST_CASE("C API: an answer older than a write of this client does not undo it", "[network_c]")
{
  fake_device d{"\"LISTEN\":true"};
  settle();
  mirror m{d.url(), "c_stale_read"};
  auto f = find_parameter(m.dev, "/f");
  REQUIRE(f);
  settle();
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  d.value = 0.3f;
  d.value_delay_ms = 300;
  const int before = d.value_requests;
  value_of(f);
  m.listen("/f");
  REQUIRE(wait_for([&] { return d.value_requests == before + 1; }, 1000));
  ossia_parameter_push_f(f, 0.5f);
  std::this_thread::sleep_for(std::chrono::milliseconds(700));
  INFO("received " << m.values().size());
  REQUIRE(m.values().empty());
  REQUIRE(value_of(f) == 0.5f);
}

// The mirror does not reopen its WebSocket: after a reconnection the host makes
// a new one and binds again, and that one must listen to what is followed
TEST_CASE("C API: a mirror listens again after a reconnection", "[network_c]")
{
  const uint16_t osc = free_port(false), ws_port = free_port(true);
  auto listened = [&] {
    settle();
    auto doc = json_at(ws_port, "/?PEERS");
    std::vector<std::string> out;
    for(auto& c : doc["CLIENTS"].GetArray())
      for(auto& l : c["LISTEN"].GetArray())
        out.emplace_back(l.GetString());
    return out;
  };
  auto noop = [](void*, ossia_value_t) {};
  auto make_server = [&](const char* name, std::unique_ptr<server>& s) {
    s = std::make_unique<server>(name, osc, "127.0.0.1", ws_port);
    float_parameter(s->dev, "/a");
    float_parameter(s->dev, "/b");
    settle();
  };

  std::unique_ptr<server> s;
  make_server("c_relisten_1", s);
  {
    mirror m{s->url(), "c_relisten_m"};
    ossia_parameter_add_callback(find_parameter(m.dev, "/a"), noop, nullptr);
    REQUIRE(listened() == std::vector<std::string>{"/a"});

    s.reset();
    REQUIRE(wait_for([&] { return ossia_device_connected(m.dev) == 0; }, 3000));
  }

  make_server("c_relisten_2", s);
  REQUIRE(listened().empty());
  mirror again{s->url(), "c_relisten_m"};
  ossia_parameter_add_callback(find_parameter(again.dev, "/a"), noop, nullptr);
  REQUIRE(listened() == std::vector<std::string>{"/a"});
}

// The proposal: PATH_CHANGED says a method changed, and the client asks for
// its structure
TEST_CASE("C API: a mirror reads again the address of a PATH_CHANGED", "[network_c]")
{
  fake_device d{"\"LISTEN\":true"};
  settle();
  mirror m{d.url(), "c_path_changed"};
  auto root = ossia_device_get_root_node(m.dev);
  auto apply_until = [&](const std::function<bool()>& done) {
    return wait_for([&] {
      ossia_device_run_commands(m.dev);
      return done();
    }, 3000);
  };
  auto node_json = [](const char* value) {
    return std::string{"{\"FULL_PATH\":\"/new\",\"TYPE\":\"f\",\"ACCESS\":3,\"VALUE\":"} + value + "}";
  };

  // A method that appears
  d.set_node("/new", node_json("1.5"));
  d.path_changed("/new");
  REQUIRE(apply_until([&] { return find_parameter(m.dev, "/new") != nullptr; }));
  auto noop = [](void*, ossia_value_t) {};
  auto p = find_parameter(m.dev, "/new");
  ossia_parameter_add_callback(p, noop, nullptr); // read the local value
  REQUIRE(value_of(p) == 1.5f);

  // A method that changes
  d.set_node("/new", node_json("2.5"));
  d.path_changed("/new");
  REQUIRE(apply_until([&] { return value_of(p) == 2.5f; }));

  // A method that goes: the server answers 404
  d.remove_node("/new");
  d.path_changed("/new");
  auto zombie = [&] {
    auto node = static_cast<ossia::net::node_base*>(ossia_node_find(root, "/new"));
    return !node || bool(ossia::net::get_zombie(node->get_extended_attributes()));
  };
  REQUIRE(apply_until(zombie));

  // The whole tree
  d.late_child = true;
  d.path_changed("/");
  REQUIRE(apply_until([&] { return find_parameter(m.dev, "/late") != nullptr; }));
  REQUIRE(find_parameter(m.dev, "/f"));

  // What is not a path is ignored
  d.send("{\"COMMAND\":\"PATH_CHANGED\",\"DATA\":12}");
  settle();
  ossia_device_run_commands(m.dev);
  REQUIRE(find_parameter(m.dev, "/f"));
}

// Two ATTRIBUTES_CHANGED with a TYPE for one new parameter, the second one
// applied after the answer to the reading the first one asked for, which is
// what a loaded host does: what follows the parameter gets its value once.
TEST_CASE("a value read for a new parameter comes once however late the second change", "[network_c]")
{
  fake_device d{"\"LISTEN\":true"};
  settle();
  auto client = std::make_shared<ossia::net::generic_device>(
      std::make_unique<ossia::oscquery::oscquery_mirror_protocol>(d.url()), "c_once");
  REQUIRE(client->get_protocol().update(client->get_root_node()));
  auto& proto = static_cast<ossia::oscquery::oscquery_mirror_protocol&>(client->get_protocol());

  std::mutex mutex;
  std::vector<float> received;
  struct follower
  {
    std::mutex* m;
    std::vector<float>* r;
    void on_created(const ossia::net::parameter_base& p)
    {
      const_cast<ossia::net::parameter_base&>(p).add_callback([m = m, r = r](const ossia::value& v) {
        std::lock_guard l{*m};
        r->push_back(ossia::convert<float>(v));
      });
    }
  } f{&mutex, &received};
  client->on_parameter_created.connect<&follower::on_created>(f);

  auto poll = [&](int ms) {
    for(int t = 0; t < ms; t += 5)
    {
      proto.run_commands();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  };
  auto count = [&] {
    std::lock_guard l{mutex};
    return received.size();
  };
  const std::string changed
      = "{\"COMMAND\":\"ATTRIBUTES_CHANGED\",\"DATA\":{\"FULL_PATH\":\"/n\",\"TYPE\":\"f\"}}";

  d.send("{\"COMMAND\":\"PATH_ADDED\",\"DATA\":\"/n\"}");
  d.send(changed);
  poll(300);
  REQUIRE(d.value_requests == 1);
  REQUIRE(count() == 1);

  // Answered, then the second change: nothing more
  d.send(changed);
  poll(300);
  REQUIRE(count() == 1);

  // Later: read again, and the same value is not a new one
  poll(1200);
  d.send(changed);
  poll(300);
  REQUIRE(d.value_requests == 2);
  REQUIRE(count() == 1);

  // A value that did change comes
  poll(1200);
  d.value = 8.f;
  d.send(changed);
  poll(300);
  REQUIRE(count() == 2);

  client->on_parameter_created.disconnect<&follower::on_created>(f);
}

// The cost of reading, every frame, parameters of a mirror that nothing
// follows: an Update loop of Unity, a metro on namespace in Max. Not a test of
// behaviour: it prints what a read costs the calling thread and the network.
TEST_CASE("bench: reading a mirrored parameter nothing follows, every frame", "[.][read_bench]")
{
  using clock = std::chrono::steady_clock;
  auto ms = [](auto d) { return std::chrono::duration<double, std::milli>(d).count(); };

  // 50 parameters read once each in one frame, as namespace on a client
  {
    server s{"c_read_bench_many", 0, "127.0.0.1"};
    for(int i = 0; i < 50; i++)
      float_parameter(s.dev, ("/p" + std::to_string(i)).c_str());
    settle();
    mirror m{s.url(), "c_read_bench_many_mirror"};
    std::vector<ossia_parameter_t> params;
    for(int i = 0; i < 50; i++)
      params.push_back(find_parameter(m.dev, ("/p" + std::to_string(i)).c_str()));
    for(int pass = 0; pass < 3; pass++)
    {
      const auto t0 = clock::now();
      for(auto p : params)
        value_of(p);
      std::printf("read bench, 50 parameters in one frame, pass %d: %.2f ms\n", pass + 1,
                  ms(clock::now() - t0));
      std::this_thread::sleep_for(std::chrono::milliseconds(17));
    }
  }

  // One parameter at 60 frames per second for 2 s: requests sent, time blocked
  {
    fake_device d{"\"LISTEN\":true"};
    settle();
    mirror m{d.url(), "c_read_bench_one"};
    auto f = find_parameter(m.dev, "/f");
    REQUIRE(f);
    double worst = 0, total = 0;
    const int frames = 120;
    const int before = d.value_requests;
    const auto start = clock::now();
    for(int i = 0; i < frames; i++)
    {
      const auto t0 = clock::now();
      value_of(f);
      const auto spent = ms(clock::now() - t0);
      worst = std::max(worst, spent);
      total += spent;
      std::this_thread::sleep_until(start + std::chrono::microseconds(16667 * (i + 1)));
    }
    std::printf(
        "read bench, 1 parameter at 60 fps for 2 s: %d requests (%.1f per second), "
        "%.3f ms per read on average, %.3f ms at worst\n",
        d.value_requests - before, (d.value_requests - before) / 2.0, total / frames, worst);
  }
}
