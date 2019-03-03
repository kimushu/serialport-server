#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include "socket.hpp"
#include "client.hpp"
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>

class OsPort;
class Options;

class Server
{
public:
  Server(OsPort& os, const Options& opt);

  void run(const Socket::shared_ptr& socket);

private:
  std::unique_lock<std::mutex> wait_empty_client();
  void cleanup_clients();

public:
  OsPort& os;
  const Options& opt;

private:
  std::list<std::shared_ptr<std::thread>> active_clients;
  std::list<std::shared_ptr<std::thread>> dead_clients;
  std::mutex mutex;
  std::condition_variable cond;
};

#endif  /* _SERVER_HPP_ */
