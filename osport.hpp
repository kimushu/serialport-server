#ifndef _OSPORT_HPP_
#define _OSPORT_HPP_

#include <string>
#include "socket.hpp"
#include <memory>

struct SerialPortInfo
{
  std::string path;
  std::string name;
  int vendor_id;
  int product_id;
  int order;

  SerialPortInfo(const std::string& path, const std::string& name)
  : path(path), name(name)
  {
  }
};

class OsPort
{
protected:
  OsPort() = default;

public:
  using shared_ptr = std::shared_ptr<OsPort>;

  static shared_ptr create();

  virtual ~OsPort() = default;

  virtual int getopt(int argc, char *argv[], const char *options, char*& optarg, int& optind) = 0;

  virtual int getpid() = 0;
  virtual Socket::shared_ptr create_socket_tcp() = 0;
  virtual void enumerate(std::vector<SerialPortInfo>& list) = 0;
};

#endif /* _OSPORT_HPP_ */
