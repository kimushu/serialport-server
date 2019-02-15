#include "options.hpp"
#include <getopt.h>
#include <iostream>

bool Options::parse(int argc, char *argv[])
{
  int ch;

  optind = 1;
  opterr = 0;
  while ((ch = getopt(argc, argv, "a:p:i:m:vh")) != -1)
  {
    switch (ch)
    {
    case 'a':
      // -a <address>
      address = optarg;
      break;
    case 'p':
      // -p <number>
      port = atoi(optarg);
      break;
    case 'i':
      // -i <idfile>
      idfile = optarg;
      break;
    case 'm':
      // -m <number>
      max_clients = atoi(optarg);
      break;
    case 'v':
      ++verbosity;
      break;
    case 'h':
      std::cerr << "Usage: " << argv[0] << " [<options>]\n\n"
        "Options:\n"
        "  -a <address>      Specify bind address (default: 127.0.0.1)\n"
        "  -p <number>       Specify port (default: assign an arbitrary unused port)\n"
        "  -i <file>         Specify file to write IDs [PID:Address:Port] (default: stdout)\n"
        "  -m <number>       Specify maximum number of clients (default: 10)\n"
        "  -h                Print this help message\n"
        << std::endl;
      return false;
    default:
      throw std::invalid_argument("unknown argument: " + std::string(argv[optind - 1]));
    }
  }

  return true;
}
