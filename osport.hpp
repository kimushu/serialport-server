#ifndef _OSPORT_HPP_
#define _OSPORT_HPP_

#include <string>
#include "socket.hpp"
#include <memory>

class OsPort
{
protected:
  OsPort() = default;

public:
  typedef std::shared_ptr<OsPort> shared_ptr;

  static shared_ptr create();

  virtual ~OsPort() = default;

  virtual int getpid() = 0;
  virtual Socket::shared_ptr create_socket_tcp() = 0;
};

#endif /* _OSPORT_HPP_ */
