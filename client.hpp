#ifndef _CLIENT_HPP_
#define _CLIENT_HPP_

#include "socket.hpp"
#include <iostream>
#include "json5pp/json5pp.hpp"

class Server;

class Client
{
public:
  using jvalue = json5pp::value;

  Client(Server& server);

  void run(const Socket::shared_ptr& socket);

private:
  void list(const jvalue& input, jvalue::object_type& output, int session);
  void open(const jvalue& input, jvalue::object_type& output, int session);
  void setup(const jvalue& input, jvalue::object_type& output, int session);
  void modem(const jvalue& input, jvalue::object_type& output, int session);
  void write(const jvalue& input, jvalue::object_type& output, int session);
  void read(const jvalue& input, jvalue::object_type& output, int session);
  void close(const jvalue& input, jvalue::object_type& output, int session);

private:
  Server& server;
};

#endif  /* _CLIENT_HPP_ */
