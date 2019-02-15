#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include <memory>

class Socket
{
protected:
  Socket() = default;

public:
  typedef std::shared_ptr<Socket> shared_ptr;

  virtual ~Socket() = default;

  virtual void bind(const std::string& address, int port) = 0;
  virtual void get_address(std::string& address, int& port) = 0;
  virtual void listen() = 0;
  virtual shared_ptr accept() = 0;
  virtual void close() = 0;

};

#endif /* _SOCKET_HPP_ */
