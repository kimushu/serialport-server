#include "options.hpp"
#include <memory>
#include "osport.hpp"
#include "socket.hpp"
#include "server.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

int main(int argc, char *argv[])
{
  try {
    // Create OS port
    auto os = OsPort::create();

    Options opt;
    if (!opt.parse(*os.get(), argc, argv)) {
      return EXIT_FAILURE;
    }

    // Create a new socket
    auto server_socket = os->create_socket_tcp();

    // Bind address
    server_socket->bind(opt.get_address(), opt.get_port());

    // Print "pid:address:port" to id file
    {
      std::ofstream file;
      const char* filename = opt.get_idfile();
      if (filename) {
        file.open(filename, std::ios::trunc);
      }
      std::string address;
      int port = 0;
      server_socket->get_address(address, port);
      (filename ? file : std::cout) <<
        os->getpid() << ":" << address << ":" << port << std::endl;
    }

    // Listen
    server_socket->listen();

    // Main loop
    Server(*os, opt).run(server_socket);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
