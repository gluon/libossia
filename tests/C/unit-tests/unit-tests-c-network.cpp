// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
// The C API of a client and of a server that sends bundles: an OSCQuery
// server on a network context, mirrors that follow it, bundles, echo.
#include <ossia/detail/config.hpp>

#include <ossia/network/oscquery/detail/json_writer.hpp>
#include <ossia/network/sockets/websocket_client.hpp>

#include <ossia-c/ossia-c.h>

#include "include_catch.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>

#include <oscpack/osc/OscReceivedElements.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace
{
// A port the system chooses, out of the ports other programs use
uint16_t free_port(bool tcp)
{
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

  explicit server(const char* name, uint16_t osc_port = 0)
      : osc{osc_port ? osc_port : free_port(false)}
  {
    auto multiplex = ossia_protocol_multiplex_create();
    dev = ossia_device_create(multiplex, name);
    auto proto = ossia_protocol_oscquery_server_asio_create(ctx, osc, ws);
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
