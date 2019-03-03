#include "client.hpp"
#include "server.hpp"
#include "osport.hpp"

Client::Client(Server& server)
: server(server)
{
}

void Client::run(const Socket::shared_ptr& socket)
{
  std::istream in(socket.get());
  std::ostream out(socket.get());
  for (;;) {
    auto input_value = json5pp::parse(in, false);
    // std::cout << input_value << std::endl;
    const auto& input_object = input_value.as_object();

    auto output_value = json5pp::object({});
    auto& output_object = output_value.as_object();

    auto iter = input_object.find("list");
    if ((iter != input_object.end()) && (iter->second)) {
      list(iter->second, output_object["list"]);
    }
    iter = input_object.find("open");
    if ((iter != input_object.end()) && (iter->second)) {
      open(iter->second, output_object["open"]);
    }
    iter = input_object.find("setup");
    if ((iter != input_object.end()) && (iter->second)) {
      setup(iter->second, output_object["setup"]);
    }
    iter = input_object.find("modem");
    if ((iter != input_object.end()) && (iter->second)) {
      modem(iter->second, output_object["modem"]);
    }
    iter = input_object.find("write");
    if ((iter != input_object.end()) && (iter->second)) {
      write(iter->second, output_object["write"]);
    }
    iter = input_object.find("read");
    if ((iter != input_object.end()) && (iter->second)) {
      read(iter->second, output_object["read"]);
    }
    iter = input_object.find("close");
    if ((iter != input_object.end()) && (iter->second)) {
      close(iter->second, output_object["close"]);
    }

    out << output_value;
  }
}

void Client::list(const jvalue& input, jvalue& output)
{
  std::vector<SerialPortInfo> pi;
  server.os.enumerate(pi);
  std::sort(pi.begin(), pi.end(), [](const auto& a, const auto& b){
    return b.order > a.order;
  });
  output = json5pp::array({});
  auto& array = output.as_array();
  for (const auto& i : pi) {
    array.push_back(json5pp::object({{"path", i.path}, {"name", i.name}}));
  }
}

void Client::open(const jvalue& input, jvalue& output)
{
  static const jvalue true_value(true);

  const std::string& path = input.at("path").as_string();
  const bool readable = input.at("read", true_value);
  const bool writable = input.at("write", true_value);
  const bool shared = input.at("shared");
  const bool port = input.at("port");
}

void Client::setup(const jvalue& input, jvalue& output, bool with_open/* = false*/)
{
  const int session = input.at("session").as_integer();
  const auto& baud = input.at("baud");
  const auto& bits = input.at("bits");
  const auto& parity = input.at("parity");
  const auto& stop = input.at("stop");
  const auto& flow = input.at("flow");
}

void Client::modem(const jvalue& input, jvalue& output)
{
  const int session = input.at("session").as_integer();
}

void Client::write(const jvalue& input, jvalue& output)
{
  const int session = input.at("session").as_integer();
}

void Client::read(const jvalue& input, jvalue& output)
{
  const int session = input.at("session").as_integer();
}

void Client::close(const jvalue& input, jvalue& output)
{
  const int session = input.at("session").as_integer();
}
