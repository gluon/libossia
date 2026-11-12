// SPDX-FileCopyrightText: 2026 CNRS
// SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>

// mDNS announcements of an OSCQuery server and of a Minuit device. These two
// tests need OSSIA_DNSSD=ON and announce on the local network for a few
// seconds; they listen on UDP 5353 next to the system's responder.
#include "ReproCommon.hpp"

#include <ossia/network/minuit/minuit.hpp>

#include "include_catch.hpp"

#include <functional>

#include <servus/servus.h>

#include <ifaddrs.h>
#include <net/if.h>

#include <optional>

using namespace repro;

namespace
{
// A passive mDNS listener: joins 224.0.0.251:5353 on every IPv4 interface and
// records, with their time, the PTR records that name a service instance.
// TTL 0 is a goodbye.
struct mdns_watch
{
  struct record
  {
    std::chrono::steady_clock::time_point t;
    uint32_t ttl;
  };

  explicit mdns_watch(std::string instance)
      : instance{std::move(instance)}
  {
    fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    int one = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5353);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ok = ::bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0;
    ifaddrs* ifs = nullptr;
    if(ok && ::getifaddrs(&ifs) == 0)
    {
      for(auto* i = ifs; i; i = i->ifa_next)
      {
        if(!i->ifa_addr || i->ifa_addr->sa_family != AF_INET || (i->ifa_flags & IFF_LOOPBACK))
          continue;
        ip_mreq m{};
        m.imr_multiaddr.s_addr = inet_addr("224.0.0.251");
        m.imr_interface = ((sockaddr_in*)i->ifa_addr)->sin_addr;
        ::setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m, sizeof(m));
      }
      ::freeifaddrs(ifs);
    }
    timeval tv{0, 100000};
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    thread = std::thread([this] { run(); });
  }
  ~mdns_watch()
  {
    done = true;
    thread.join();
    ::close(fd);
  }

  std::optional<std::chrono::steady_clock::time_point>
  first(bool goodbye, std::chrono::steady_clock::time_point since)
  {
    std::lock_guard lock{mutex};
    for(auto& r : records)
      if(r.t >= since && (r.ttl == 0) == goodbye)
        return r.t;
    return std::nullopt;
  }

  std::string instance;
  int fd{-1};
  bool ok{};
  std::atomic_bool done{false};
  std::mutex mutex;
  std::vector<record> records;
  std::thread thread;

private:
  static bool name_at(const unsigned char* d, std::size_t n, std::size_t& off, std::string& out)
  {
    std::size_t pos = off;
    bool jumped = false;
    for(int guard = 0; guard < 64 && pos < n; guard++)
    {
      const unsigned len = d[pos];
      if((len & 0xC0) == 0xC0)
      {
        if(pos + 1 >= n)
          return false;
        if(!jumped)
          off = pos + 2;
        pos = ((len & 0x3F) << 8) | d[pos + 1];
        jumped = true;
        continue;
      }
      if(len == 0)
      {
        if(!jumped)
          off = pos + 1;
        return true;
      }
      if(pos + 1 + len > n)
        return false;
      out.append((const char*)d + pos + 1, len);
      out += '.';
      pos += 1 + len;
    }
    return false;
  }

  void parse(const unsigned char* d, std::size_t n)
  {
    if(n < 12 || !(d[2] & 0x80)) // responses only
      return;
    const unsigned qd = (d[4] << 8) | d[5];
    const unsigned rr = ((d[6] << 8) | d[7]) + ((d[8] << 8) | d[9]) + ((d[10] << 8) | d[11]);
    std::size_t off = 12;
    for(unsigned i = 0; i < qd; i++)
    {
      std::string q;
      if(!name_at(d, n, off, q) || off + 4 > n)
        return;
      off += 4;
    }
    const auto now = std::chrono::steady_clock::now();
    for(unsigned i = 0; i < rr; i++)
    {
      std::string name;
      if(!name_at(d, n, off, name) || off + 10 > n)
        return;
      const unsigned type = (d[off] << 8) | d[off + 1];
      const uint32_t ttl
          = (uint32_t(d[off + 4]) << 24) | (d[off + 5] << 16) | (d[off + 6] << 8) | d[off + 7];
      const unsigned len = (d[off + 8] << 8) | d[off + 9];
      off += 10;
      if(off + len > n)
        return;
      if(type == 12)
      {
        std::size_t p = off;
        std::string target;
        if(name_at(d, n, p, target) && target.rfind(instance + ".", 0) == 0)
        {
          std::lock_guard lock{mutex};
          records.push_back({now, ttl});
        }
      }
      off += len;
    }
  }

  void run()
  {
    unsigned char buf[9000];
    while(!done)
    {
      const auto n = ::recv(fd, buf, sizeof(buf), 0);
      if(n > 0)
        parse(buf, static_cast<std::size_t>(n));
    }
  }
};
}

// stop() alone, the server still alive, withdraws the announcement (goodbye,
// TTL 0) within 2 s, as ossia-max's "stop" keeps the device.
TEST_CASE("mDNS: a stopped server withdraws its announcement", "[mdns-withdraw]")
{
  const std::string name = "repro_withdraw_" + std::to_string(getpid());
  mdns_watch watch{name};
  REQUIRE(watch.ok);
  auto ctx = ossia::net::create_network_context();
  ossia::net::generic_device dev{std::make_unique<ossia::net::multiplex_protocol>(), name};
  add_float(dev.get_root_node(), "/foo/bar", 0.f);
  auto& multiplex = static_cast<ossia::net::multiplex_protocol&>(dev.get_protocol());
  auto server = std::make_unique<ossia::oscquery_asio::oscquery_server_protocol>(
      ctx, free_port(false), free_port(true), false);
  auto* exposed = server.get();
  const auto announced_from = std::chrono::steady_clock::now();
  multiplex.expose_to(std::move(server));
  std::thread net([&] { ctx->run(); });

  bool announced = false;
  for(int i = 0; i < 100 && !announced; i++)
  {
    announced = watch.first(false, announced_from).has_value();
    std::this_thread::sleep_for(50ms);
  }
  REQUIRE(announced);

  const auto t0 = std::chrono::steady_clock::now();
  exposed->stop();
  std::optional<std::chrono::steady_clock::time_point> goodbye;
  for(int i = 0; i < 40 && !goodbye; i++)
  {
    goodbye = watch.first(true, t0);
    std::this_thread::sleep_for(50ms);
  }
  std::printf("mdns-withdraw: goodbye %s after stop()\n",
              goodbye ? (std::to_string(int(std::chrono::duration<double, std::milli>(*goodbye - t0).count())) + " ms").c_str()
                      : "not seen within 2 s");
  multiplex.stop_expose_to(*exposed);
  ossia::net::stop_network_context(*ctx);
  net.join();
  CHECK(goodbye);
}

// Builds a Minuit device and looks for its announcement for 3 s. Returns the
// instance name seen on the network, empty when none was.
static std::string minuit_announcement(
    const std::string& instance,
    const std::function<std::unique_ptr<ossia::net::generic_device>()>& build, const char* how)
{
  const auto start = std::chrono::steady_clock::now();
  auto dev = build();
  const auto build_ms
      = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

  servus::Servus browser{"_minuit._tcp"};
  browser.beginBrowsing(servus::Interface::IF_ALL);
  std::string seen;
  for(int i = 0; i < 300 && seen.empty(); i++)
  {
    browser.browse(10);
    for(const auto& s : browser.getInstances())
      if(s.rfind(instance, 0) == 0)
        seen = s;
  }
  browser.endBrowsing();
  std::printf("minuit-announce: %s: built in %lld ms; seen on the network as \"%s\"\n", how,
              (long long)build_ms, seen.c_str());
  return seen;
}

// ossia-max's Minuit listener builds a device for each Minuit device it finds
// (ZeroconfMinuitListener.cpp). If that device announced itself, the listener
// would find it as a new device, build one more, and so on. The follower is
// built as the listener of this tree builds it, with the protocol's defaults.
// The first case checks that a device built with those defaults is seen, so
// that "not seen" in the second one means something.
TEST_CASE("mDNS: a Minuit device built to follow another one does not announce itself", "[minuit-announce]")
{
  const std::string instance = "repro_minuit_" + std::to_string(getpid());
  const std::string announced = instance + "_announced", follower = instance + "_follower";
  REQUIRE(!minuit_announcement(announced, [&] {
    return std::make_unique<ossia::net::generic_device>(
        std::make_unique<ossia::net::minuit_protocol>(
            announced, "127.0.0.1", free_port(false), free_port(false)),
        announced);
  }, "minuit_protocol with its defaults").empty());
  CHECK(minuit_announcement(follower, [&] {
    return std::make_unique<ossia::net::generic_device>(
        std::make_unique<ossia::net::minuit_protocol>(
            follower, "127.0.0.1", free_port(false), free_port(false)),
        follower);
  }, "as the listener builds it").empty());
}
