// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

#pragma once
// Helpers shared by the Repro*Test.cpp files.
//
// - free_port, http_get: a free loopback port, a blocking HTTP GET.
// - ws_client: a minimal WebSocket client on POSIX sockets, independent of
//   the WebSocket library the servers use.
// - osc_float, osc_messages: OSC messages sent and received in binary frames.
// - udp_sink: what an OSCQuery peer receives over UDP, datagram by datagram.
// - api_server: ossia::oscquery::oscquery_server_protocol, the server of the
//   C API (ossia_protocol_oscquery_server_create), zeroconf disabled.
// - asio_server: ossia::oscquery_asio::oscquery_server_protocol under a
//   multiplex, as ossia-max exposes an ossia.device, on a network context run
//   by a thread. It listens on the loopback only, which also keeps it off
//   mDNS: a server bound to one address is not announced.
//
// macOS and Linux. Loopback only.
#include <ossia/detail/config.hpp>

#include <ossia/network/base/node_functions.hpp>
#include <ossia/network/common/complex_type.hpp>
#include <ossia/network/context.hpp>
#include <ossia/network/context_functions.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/local/local.hpp>
#include <ossia/network/oscquery/oscquery_server.hpp>
#include <ossia/protocols/oscquery/oscquery_server_asio.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>

#include <oscpack/osc/OscReceivedElements.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace repro
{
using namespace std::chrono_literals;

inline uint16_t free_port(bool tcp)
{
  int fd = ::socket(AF_INET, tcp ? SOCK_STREAM : SOCK_DGRAM, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  ::bind(fd, (sockaddr*)&addr, sizeof(addr));
  socklen_t len = sizeof(addr);
  ::getsockname(fd, (sockaddr*)&addr, &len);
  ::close(fd);
  return ntohs(addr.sin_port);
}

inline int connect_loopback(uint16_t port, int timeout_ms)
{
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  timeval tv{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
  ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if(::connect(fd, (sockaddr*)&a, sizeof(a)) != 0)
  {
    ::close(fd);
    return -1;
  }
  return fd;
}

inline bool send_all(int fd, const std::string& s)
{
  std::size_t done = 0;
  while(done < s.size())
  {
    auto n = ::send(fd, s.data() + done, s.size() - done, 0);
    if(n <= 0)
      return false;
    done += std::size_t(n);
  }
  return true;
}

struct http_reply
{
  int status{-1}; // -1: no connection, -2: no answer in time
  std::string body;
};

// GET target, "Connection: close". Reads until the server closes or the body
// announced by Content-Length is complete.
inline http_reply http_get(uint16_t port, const std::string& target, int timeout_ms = 3000)
{
  http_reply r;
  int fd = connect_loopback(port, timeout_ms);
  if(fd < 0)
    return r;
  send_all(fd, "GET " + target + " HTTP/1.1\r\nHost: 127.0.0.1:" + std::to_string(port)
                   + "\r\nConnection: close\r\n\r\n");
  std::string raw;
  char buf[8192];
  std::size_t expected = std::string::npos;
  for(;;)
  {
    auto n = ::recv(fd, buf, sizeof(buf), 0);
    if(n <= 0)
      break;
    raw.append(buf, std::size_t(n));
    auto head_end = raw.find("\r\n\r\n");
    if(head_end != std::string::npos && expected == std::string::npos)
    {
      std::string head = raw.substr(0, head_end);
      for(auto& c : head)
        c = (char)std::tolower(c);
      auto cl = head.find("content-length:");
      if(cl != std::string::npos)
        expected = head_end + 4 + std::stoul(head.substr(cl + 15));
    }
    if(expected != std::string::npos && raw.size() >= expected)
      break;
  }
  ::close(fd);
  if(raw.size() < 12)
  {
    r.status = -2;
    return r;
  }
  r.status = std::atoi(raw.c_str() + 9);
  auto head_end = raw.find("\r\n\r\n");
  if(head_end != std::string::npos)
    r.body = raw.substr(head_end + 4);
  return r;
}

// One OSC message with one float.
inline std::string osc_float(const std::string& address, float v)
{
  auto pad = [](std::string s) {
    s.push_back('\0');
    while(s.size() % 4)
      s.push_back('\0');
    return s;
  };
  std::string m = pad(address) + pad(",f");
  uint32_t bits;
  std::memcpy(&bits, &v, 4);
  bits = htonl(bits);
  m.append((const char*)&bits, 4);
  return m;
}

struct osc_message
{
  std::string address;
  float value{}; // first argument when it is a float
};

inline void decode_osc(const char* data, std::size_t size, std::vector<osc_message>& out)
{
  try
  {
    oscpack::ReceivedPacket p{data, (oscpack::osc_bundle_element_size_t)size};
    if(p.IsBundle())
    {
      oscpack::ReceivedBundle b{p};
      for(auto it = b.ElementsBegin(); it != b.ElementsEnd(); ++it)
        decode_osc(it->Contents(), it->Size(), out);
    }
    else
    {
      oscpack::ReceivedMessage m{p};
      osc_message o{m.AddressPattern(), 0.f};
      if(m.ArgumentCount() > 0 && m.ArgumentsBegin()->IsFloat())
        o.value = m.ArgumentsBegin()->AsFloat();
      out.push_back(o);
    }
  }
  catch(...)
  {
  }
}

// A WebSocket client: handshake, masked frames out, frames in on a thread.
class ws_client
{
public:
  struct frame
  {
    bool text{};
    std::string data;
  };

  explicit ws_client(uint16_t port)
  {
    m_fd = connect_loopback(port, 3000);
    if(m_fd < 0)
      return;
    send_all(
        m_fd, "GET / HTTP/1.1\r\nHost: 127.0.0.1:" + std::to_string(port)
                  + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                    "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                    "Sec-WebSocket-Version: 13\r\n\r\n");
    std::string raw;
    char buf[4096];
    while(raw.find("\r\n\r\n") == std::string::npos)
    {
      auto n = ::recv(m_fd, buf, sizeof(buf), 0);
      if(n <= 0)
        return;
      raw.append(buf, std::size_t(n));
    }
    if(raw.compare(0, 12, "HTTP/1.1 101") != 0)
      return;
    m_buffer = raw.substr(raw.find("\r\n\r\n") + 4);
    timeval tv{0, 50000};
    ::setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    m_connected = true;
    m_thread = std::thread([this] { run(); });
  }

  ~ws_client()
  {
    m_stop = true;
    if(m_thread.joinable())
      m_thread.join();
    if(m_fd >= 0)
      ::close(m_fd);
  }

  bool connected() const { return m_connected && !m_closed; }
  bool closed() const { return m_closed; }

  void send_text(const std::string& s) { send_frame(0x1, s); }
  void send_binary(const std::string& s) { send_frame(0x2, s); }
  void command(const char* cmd, const std::string& path)
  {
    send_text(std::string(R"({"COMMAND":")") + cmd + R"(","DATA":")" + path + "\"}");
  }

  std::vector<frame> take()
  {
    std::lock_guard l{m_mutex};
    auto f = std::move(m_frames);
    m_frames.clear();
    return f;
  }

  // The OSC messages of the binary frames, by address.
  static std::vector<osc_message> messages(const std::vector<frame>& frames, const std::string& address = {})
  {
    std::vector<osc_message> all, out;
    for(auto& f : frames)
      if(!f.text)
        decode_osc(f.data.data(), f.data.size(), all);
    for(auto& m : all)
      if(address.empty() || m.address == address)
        out.push_back(m);
    return out;
  }

  static int count_text(const std::vector<frame>& frames, const std::string& needle)
  {
    int n = 0;
    for(auto& f : frames)
      if(f.text && f.data.find(needle) != std::string::npos)
        n++;
    return n;
  }

private:
  void send_frame(int opcode, const std::string& payload)
  {
    std::string f;
    f.push_back(char(0x80 | opcode));
    const std::size_t n = payload.size();
    if(n < 126)
      f.push_back(char(0x80 | n));
    else if(n < 65536)
    {
      f.push_back(char(0x80 | 126));
      f.push_back(char(n >> 8));
      f.push_back(char(n & 0xff));
    }
    else
    {
      f.push_back(char(0x80 | 127));
      for(int i = 7; i >= 0; i--)
        f.push_back(char((uint64_t(n) >> (8 * i)) & 0xff));
    }
    const unsigned char mask[4] = {0x12, 0x34, 0x56, 0x78};
    f.append((const char*)mask, 4);
    for(std::size_t i = 0; i < n; i++)
      f.push_back(char(payload[i] ^ mask[i % 4]));
    std::lock_guard l{m_send};
    if(m_fd >= 0)
      send_all(m_fd, f);
  }

  void run()
  {
    char buf[65536];
    while(!m_stop && !m_closed)
    {
      auto n = ::recv(m_fd, buf, sizeof(buf), 0);
      if(n == 0)
      {
        m_closed = true;
        break;
      }
      if(n < 0)
        continue;
      m_buffer.append(buf, std::size_t(n));
      parse();
    }
  }

  void parse()
  {
    for(;;)
    {
      if(m_buffer.size() < 2)
        return;
      const auto b0 = (unsigned char)m_buffer[0], b1 = (unsigned char)m_buffer[1];
      std::size_t len = b1 & 0x7f, head = 2;
      if(len == 126)
      {
        if(m_buffer.size() < 4)
          return;
        len = ((unsigned char)m_buffer[2] << 8) | (unsigned char)m_buffer[3];
        head = 4;
      }
      else if(len == 127)
      {
        if(m_buffer.size() < 10)
          return;
        len = 0;
        for(int i = 0; i < 8; i++)
          len = (len << 8) | (unsigned char)m_buffer[2 + i];
        head = 10;
      }
      const bool masked = b1 & 0x80;
      if(masked)
        head += 4;
      if(m_buffer.size() < head + len)
        return;
      std::string payload = m_buffer.substr(head, len);
      if(masked)
        for(std::size_t i = 0; i < len; i++)
          payload[i] ^= m_buffer[head - 4 + i % 4];
      m_buffer.erase(0, head + len);
      const int op = b0 & 0x0f;
      const bool fin = b0 & 0x80;
      if(op == 0x8)
      {
        m_closed = true;
        return;
      }
      if(op == 0x9)
      {
        send_frame(0xA, payload);
        continue;
      }
      if(op == 0x1 || op == 0x2)
      {
        m_pending = {op == 0x1, std::move(payload)};
        m_fragmented = !fin;
      }
      else if(op == 0x0 && m_fragmented)
      {
        m_pending.data += payload;
        m_fragmented = !fin;
      }
      else
        continue;
      if(!m_fragmented)
      {
        std::lock_guard l{m_mutex};
        m_frames.push_back(std::move(m_pending));
        m_pending = {};
      }
    }
  }

  int m_fd{-1};
  std::atomic_bool m_connected{false}, m_closed{false}, m_stop{false};
  std::string m_buffer;
  frame m_pending;
  bool m_fragmented{};
  std::mutex m_mutex, m_send;
  std::vector<frame> m_frames;
  std::thread m_thread;
};

// Every datagram received on a UDP port, and the OSC messages they hold.
struct udp_sink
{
  boost::asio::io_context ctx;
  boost::asio::ip::udp::socket socket{ctx};
  std::thread thread;
  std::mutex mutex;
  std::vector<std::size_t> datagrams;
  std::vector<osc_message> messages;
  std::atomic_bool running{true};

  explicit udp_sink(uint16_t port)
  {
    socket.open(boost::asio::ip::udp::v4());
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
          std::this_thread::sleep_for(200us);
          continue;
        }
        std::lock_guard lock{mutex};
        datagrams.push_back(n);
        decode_osc(buf.data(), n, messages);
      }
    });
  }
  ~udp_sink()
  {
    running = false;
    thread.join();
  }
  uint16_t port() const { return socket.local_endpoint().port(); }
  void clear()
  {
    std::lock_guard lock{mutex};
    datagrams.clear();
    messages.clear();
  }
  bool wait_messages(std::size_t n, int ms)
  {
    for(int i = 0; i < ms / 5; i++)
    {
      {
        std::lock_guard lock{mutex};
        if(messages.size() >= n)
          return true;
      }
      std::this_thread::sleep_for(5ms);
    }
    std::lock_guard lock{mutex};
    return messages.size() >= n;
  }
};

// The server of the C API, on its own thread. HTTP and WebSocket on `ws`.
struct api_server
{
  uint16_t osc = free_port(false);
  uint16_t ws = free_port(true);
  std::unique_ptr<ossia::net::generic_device> dev;

  explicit api_server(const std::string& name = "repro")
  {
    auto p = std::make_unique<ossia::oscquery::oscquery_server_protocol>(osc, ws);
    p->disable_zeroconf();
    dev = std::make_unique<ossia::net::generic_device>(std::move(p), name);
    std::this_thread::sleep_for(200ms);
  }
  ossia::net::node_base& root() { return dev->get_root_node(); }
};

// The server of an ossia.device in Max, as ossia-max builds it.
struct asio_server
{
  ossia::net::network_context_ptr ctx = ossia::net::create_network_context();
  std::unique_ptr<ossia::net::generic_device> dev;
  ossia::net::multiplex_protocol* multiplex{};
  ossia::oscquery_asio::oscquery_server_protocol* server{};
  uint16_t osc = free_port(false);
  uint16_t ws = free_port(true);
  std::thread net;

  explicit asio_server(const std::string& name = "repro")
  {
    auto mp = std::make_unique<ossia::net::multiplex_protocol>();
    multiplex = mp.get();
    dev = std::make_unique<ossia::net::generic_device>(std::move(mp), name);
  }
  // Exposes the server and runs the context; call after building the tree.
  void start()
  {
    auto s = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(
        ctx, osc, ws, false, "127.0.0.1");
    server = s.get();
    multiplex->expose_to(std::move(s));
    net = std::thread([this] { ctx->run(); });
    std::this_thread::sleep_for(200ms);
  }
  ~asio_server()
  {
    ossia::net::stop_network_context(*ctx);
    if(net.joinable())
      net.join();
  }
  ossia::net::node_base& root() { return dev->get_root_node(); }
};

inline ossia::net::parameter_base*
add_float(ossia::net::node_base& root, const std::string& path, float v)
{
  auto* p = ossia::try_setup_parameter("float", ossia::net::create_node(root, path));
  p->set_value(v);
  return p;
}
}
