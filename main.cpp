#include "options.hpp"
#include <memory>
#include "osport.hpp"
#include "socket.hpp"
#include <fstream>
#include <iostream>
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>

class MainLoop
{
public:
  MainLoop(const Options& opt) : opt(opt) {}

  void run_server(const Socket::shared_ptr& server)
  {
    for (;;) {
      auto lock = wait_empty_client();
      active_clients.emplace_front();
      auto iter = active_clients.begin();
      lock.unlock();

      if (opt.get_verbosity() >= 1) {
        std::cerr << "Info: accept" << std::endl;
      }
      auto socket = server->accept();
      cleanup_clients();
      iter->reset(new std::thread([this, iter, socket](){
        if (opt.get_verbosity() >= 1) {
          std::cerr << "Info: thread started: " << std::this_thread::get_id() << std::endl;
        }
        run_client(socket);
        socket->close();
        {
          std::lock_guard<std::mutex> lock(mutex);
          dead_clients.splice(dead_clients.begin(), std::move(active_clients), iter);
        }
        cond.notify_one();
        if (opt.get_verbosity() >= 1) {
          std::cerr << "Info: thread exiting: " << std::this_thread::get_id() << std::endl;
        }
      }));
    }
  }

private:
  std::unique_lock<std::mutex> wait_empty_client()
  {
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock, [this]{ return active_clients.size() < opt.get_max_clients(); });
    return lock;
  }

  void cleanup_clients()
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

  void run_client(Socket::shared_ptr socket)
  {
  }

private:
  const Options& opt;
  std::list< std::shared_ptr<std::thread> > active_clients;
  std::list< std::shared_ptr<std::thread> > dead_clients;
  std::mutex mutex;
  std::condition_variable cond;
};

int main(int argc, char *argv[])
{
  try {
    Options opt;
    if (!opt.parse(argc, argv)) {
      return EXIT_FAILURE;
    }

    auto os = OsPort::create();

    // Create a new socket
    auto server = os->create_socket_tcp();

    // Bind address
    server->bind(opt.get_address(), opt.get_port());

    // Print "pid:address:port" to id file
    {
      std::ofstream file;
      const char* filename = opt.get_idfile();
      if (filename) {
        file.open(filename, std::ios::trunc);
      }
      std::string address;
      int port = 0;
      server->get_address(address, port);
      (filename ? file : std::cout) <<
        os->getpid() << ":" << address << ":" << port << std::endl;
    }

    // Listen
    server->listen();

    // Main loop
    MainLoop(opt).run_server(server);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
