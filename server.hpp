#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include "osport.hpp"
#include "socket.hpp"
#include "client.hpp"
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>

class Options;

class Server
{
public:
  using handle_type = OsPort::handle_type;

  Server(OsPort& os, const Options& opt);

  void run(const Socket::shared_ptr& socket);

  std::unique_lock<std::mutex> get_session_handle(int session, handle_type& handle);

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

  std::vector<handle_type> handles;
  std::mutex handles_mutex;
};

#endif  /* _SERVER_HPP_ */
