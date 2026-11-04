#include <ossia/network/sockets/websocket_server_interface.hpp>

#include <stdexcept>

namespace ossia::net
{
websocket_server_interface::~websocket_server_interface() = default;

void websocket_server_interface::listen(const std::string& address, uint16_t)
{
  throw std::runtime_error("this WebSocket server cannot listen on " + address + " only");
}
}
