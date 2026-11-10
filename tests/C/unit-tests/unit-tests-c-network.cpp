// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
// The C API of a client and of a server that sends bundles: an OSCQuery
// server on a network context, mirrors that follow it, bundles, echo.
#include <ossia/detail/config.hpp>

#include <ossia/network/context.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/oscquery/detail/json_writer.hpp>
#include <ossia/network/sockets/websocket_client.hpp>
#include <ossia/protocols/oscquery/oscquery_mirror_asio.hpp>
#include <ossia/protocols/oscquery/oscquery_server_asio.hpp>
#include <ossia/network/base/node_functions.hpp>

#include <ossia-c/ossia-c.h>

#include "include_catch.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>

#include <oscpack/osc/OscReceivedElements.h>
#include <rapidjson/document.h>

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
#include <mutex>
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
void send_osc_float(const std::string& ip, uint16_t port, const std::string& address, float f)
{
  std::string m = address;
  m.append(4 - m.size() % 4, '\0');
  m += ",f";
  m.append(2, '\0');
  uint32_t bits;
  std::memcpy(&bits, &f, 4);
  bits = htonl(bits);
  m.append(reinterpret_cast<const char*>(&bits), 4);
  int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  ::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
  ::sendto(fd, m.data(), m.size(), 0, (sockaddr*)&addr, sizeof(addr));
  ::close(fd);
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
  uint16_t ws = free_port(true);
  ossia_network_context_t ctx = ossia_network_context_create();
  ossia_device_t dev{};
  std::atomic_bool polling{true};
  std::thread poller;

  explicit server(const char* name, uint16_t osc_port = 0, const char* bind = nullptr)
      : osc{osc_port ? osc_port : free_port(false)}
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
  ossia::net::websocket_client ws{[](auto&&...) {}};
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
  void stream_to(uint16_t udp_port)
  {
    auto msg = ossia::oscquery::json_writer::start_osc_streaming(udp_port, 0);
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
