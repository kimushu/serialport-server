#include "socket.hpp"
#include "osport.hpp"
#include "winsock2.h"
#include <stdexcept>
#include <string>
#include <cassert>

class Win32Socket : public Socket
{
public:
  static void startup()
  {
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(1, 1), &wsa_data) != 0) {
      throw std::runtime_error("cannot initialize WinSock: " + error_string());
    }
  }

  static void cleanup()
  {
    WSACleanup();
  }

  Win32Socket(int af, int type, int protocol)
  {
    socket = ::socket(af, type, protocol);
    if (socket < 0) {
      throw std::runtime_error("cannot create socket: " + error_string());
    }
  }

  Win32Socket(SOCKET socket) : socket(socket)
  {
    assert(socket >= 0);
  }

  virtual ~Win32Socket()
  {
    close();
  }

  virtual void bind(const std::string& address, int port)
  {
    assert(socket >= 0);

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr(address.c_str());
    saddr.sin_port = htons(port);

    if (::bind(socket, (const struct sockaddr *)&saddr, sizeof(saddr)) != 0) {
      throw std::runtime_error("cannot bind address: " + error_string());
    }
  }

  virtual void get_address(std::string& address, int& port)
  {
    assert(socket >= 0);

    struct sockaddr_in saddr;
    int saddrlen = sizeof(saddr);
    if (getsockname(socket, (struct sockaddr *)&saddr, &saddrlen) != 0) {
      throw std::runtime_error("cannot get socket address: " + error_string());
    }
    address = inet_ntoa(saddr.sin_addr);
    port = ntohs(saddr.sin_port);
  }

  virtual void listen()
  {
    assert(socket >= 0);

    if (::listen(socket, 5) != 0) {
      throw std::runtime_error("cannot listen socket: " + error_string());
    }
  }

  virtual Socket::shared_ptr accept()
  {
    assert(socket >= 0);
    
    SOCKET client = ::accept(socket, nullptr, nullptr);
    if (client < 0) {
      throw std::runtime_error("cannot accept socket: " + error_string());
    }
    return Socket::shared_ptr(new Win32Socket(client));
  }

  virtual void close()
  {
    if (socket >= 0) {
      closesocket(socket);
      socket = -1;
    }
  }

private:
  static std::string error_string()
  {
    return std::to_string(WSAGetLastError());
  }

  SOCKET socket;
};

class Win32OsPort : public OsPort
{
public:
  Win32OsPort()
  {
    Win32Socket::startup();
  }

  virtual ~Win32OsPort()
  {
    Win32Socket::cleanup();
  }

  virtual int getpid()
  {
    return GetCurrentProcessId();
  }

  virtual Socket::shared_ptr create_socket_tcp()
  {
    return Socket::shared_ptr(new Win32Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
  }
};

OsPort::shared_ptr OsPort::create()
{
  return shared_ptr(new Win32OsPort());
}