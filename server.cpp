#include "server.hpp"
#include "osport.hpp"
#include "options.hpp"

Server::Server(OsPort& os, const Options& opt)
: os(os), opt(opt)
{
}

void Server::run(const Socket::shared_ptr& socket)
{
  for (;;) {
    auto lock = wait_empty_client();
    active_clients.emplace_front();
    auto iter = active_clients.begin();
    lock.unlock();

    if (opt.get_verbosity() >= 1) {
      std::cerr << "Info: accept" << std::endl;
    }
    auto client_socket = socket->accept();
    cleanup_clients();
    iter->reset(new std::thread([this, iter, client_socket](){
      std::stringstream ss;
      ss << "(thread #" << std::this_thread::get_id() << ") ";
      const auto prefix = ss.str();

      if (opt.get_verbosity() >= 1) {
        std::cerr << "Info: " << prefix << "started." << std::endl;
      }

      try {
        Client(*this).run(client_socket);
      } catch (const std::exception& e) {
        std::cerr << "Error: " << prefix << e.what() << std::endl;
      }
      client_socket->close();

      {
        std::lock_guard<std::mutex> lock(mutex);
        dead_clients.splice(dead_clients.begin(), std::move(active_clients), iter);
      }
      cond.notify_one();
      if (opt.get_verbosity() >= 1) {
        std::cerr << "Info: " << prefix << "exiting." << std::endl;
      }
    }));
  }
}

std::unique_lock<std::mutex> Server::wait_empty_client()
{
  std::unique_lock<std::mutex> lock(mutex);
  cond.wait(lock, [this]{ return (int)active_clients.size() < opt.get_max_clients(); });
  return lock;
}

void Server::cleanup_clients()
{
  std::lock_guard<std::mutex> lock(mutex);
  while (!dead_clients.empty()) {
    auto id = dead_clients.front()->get_id();
    dead_clients.front()->join();
    if (opt.get_verbosity() >= 1) {
      std::cerr << "Info: thread joined: " << id << std::endl;
    }
    dead_clients.pop_front();
  }
}
