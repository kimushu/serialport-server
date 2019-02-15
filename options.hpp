#ifndef _OPTIONS_HPP_
#define _OPTIONS_HPP_

class Options
{
public:
  Options() : address("127.0.0.1"), port(0), idfile(nullptr), max_clients(10), verbosity(0) {}
  ~Options() {}

  bool parse(int argc, char *argv[]);

  const char *get_address() const
  {
    return address;
  }

  int get_port() const
  {
    return port;
  }

  const char *get_idfile() const
  {
    return idfile;
  }

  int get_max_clients() const
  {
    return max_clients;
  }

  int get_verbosity() const
  {
    return verbosity;
  }

private:
  const char *address;
  int port;
  const char *idfile;
  int max_clients;
  int verbosity;
};

#endif /* _OPTIONS_HPP_ */
