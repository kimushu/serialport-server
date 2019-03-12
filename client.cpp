#include "client.hpp"
#include "server.hpp"
#include "osport.hpp"

/**
 * @brief Construct a new Client object
 * 
 * @param server A reference to Server object
 */
Client::Client(Server& server)
: server(server)
{
}

/**
 * @brief Start conversation with client
 * 
 * @param socket A socket to client
 */
void Client::run(const Socket::shared_ptr& socket)
{
  static struct {
    const char* name;
    void (Client::*func)(const jvalue& input, jvalue::object_type& output, int session);
  } const operations[] = {
    { "list", &Client::list },
    { "open", &Client::open },
    { "config", &Client::config },
    { "modem", &Client::modem },
    { "write", &Client::write },
    { "read", &Client::read },
    { "close", &Client::close },
    { nullptr }
  };

  std::istream in(socket.get());
  std::ostream out(socket.get());
  for (;;) {
    auto input_value = json5pp::parse(in, false);
    const auto& input_object = input_value.as_object();

    auto output_value = json5pp::object({});
    auto& output_object = output_value.as_object();

    for (auto operation = operations;; ++operation) {
      auto name = operation->name;
      auto func = operation->func;
      if (!name || !func) {
        break;
      }
      auto iter = input_object.find(name);
      if ((iter != input_object.end()) && (iter->second)) {
        const auto& input_item = iter->second;
        auto& output_item = (output_object[name] = json5pp::object({})).as_object();
        const auto& input_sequence = input_item["sequence"];
        if (!input_sequence.is_null()) {
          output_item["sequence"] = input_sequence;
        }
        (this->*func)(input_item, output_item, 0);
      }
    }

    out << output_value;
  }
}

/**
 * @brief Process "list" operation
 * 
 * @param input A reference to input JSON value
 * @param output A reference to object container for output JSON value
 * @param session Not used
 */
void Client::list(const jvalue& input, jvalue::object_type& output, int session)
{
  (void)session;
  auto ports = server.os.enumerate();
  std::sort(ports.begin(), ports.end(), [](const auto& a, const auto& b){
    return b.order > a.order;
  });
  auto& array = (output["result"] = json5pp::array({})).as_array();
  for (const auto& i : ports) {
    array.push_back(json5pp::object({{"path", i.path}, {"name", i.name}}));
  }
}

/**
 * @brief Process "open" operation
 * 
 * @param input A reference to input JSON value
 * @param output A reference to object container for output JSON value
 * @param session Not used
 */
void Client::open(const jvalue& input, jvalue::object_type& output, int session)
{
  (void)session;
  static const jvalue true_value(true);

  const std::string& path = input.at("path").as_string();
  const bool readable = input.at("read", true_value);
  const bool writable = input.at("write", true_value);
  const bool shared = input.at("shared");
  const bool port = input.at("port");
  const bool permanent = input.at("permanent");
}

/**
 * @brief Process "config" operation
 * 
 * @param input A reference to input JSON value
 * @param output A reference to object container for output JSON value
 * @param session Not used
 */
void Client::config(const jvalue& input, jvalue::object_type& output, int session)
{
  bool no_result = true;
  if (session <= 0) {
    no_result = false;
    session = input.at("session").as_integer();
  }
  SerialPortConfig config_change = {0};
  const auto& baud = input.at("baud");
  if (!baud.is_null()) {
    config_change.baud_rate = baud.as_integer();
    config_change.field_mask |= SerialPortConfig::SP_FIELD_BAUD_RATE;
  }
  const auto& bits = input.at("bits");
  if (!bits.is_null()) {
    const auto value = bits.as_integer();
    if ((value < SerialPortConfig::SP_DATABITS_5) ||
        (SerialPortConfig::SP_DATABITS_8 < value)) {
      throw std::invalid_argument("invalid data bits: " + std::to_string(value));
    }
    config_change.data_bits = (SerialPortConfig::DataBitsMode)value;
    config_change.field_mask |= SerialPortConfig::SP_FIELD_DATA_BITS;
  }
  const auto& parity = input.at("parity");
  if (!parity.is_null()) {
    const auto& value = parity.as_string();
    if (value == "none") {
      config_change.parity = SerialPortConfig::SP_PARITY_NONE;
    } else if (value == "odd") {
      config_change.parity = SerialPortConfig::SP_PARITY_ODD;
    } else if (value == "even") {
      config_change.parity = SerialPortConfig::SP_PARITY_EVEN;
    } else if (value == "mark") {
      config_change.parity = SerialPortConfig::SP_PARITY_MARK;
    } else if (value == "space") {
      config_change.parity = SerialPortConfig::SP_PARITY_SPACE;
    } else {
      throw std::invalid_argument("invalid parity mode: " + value);
    }
    config_change.field_mask |= SerialPortConfig::SP_FIELD_PARITY;
  }
  const auto& stop = input.at("stop");
  if (!stop.is_null()) {
    const auto value = stop.as_number();
    if (value == 1.0) {
      config_change.stop_bits = SerialPortConfig::SP_STOPBITS_1;
    } else if (value == 1.5) {
      config_change.stop_bits = SerialPortConfig::SP_STOPBITS_1_5;
    } else if (value == 2.0) {
      config_change.stop_bits = SerialPortConfig::SP_STOPBITS_2;
    } else {
      throw std::invalid_argument("invalid stop bits: " + std::to_string(value));
    }
    config_change.field_mask |= SerialPortConfig::SP_FIELD_STOP_BITS;
  }
  const auto& flow = input.at("flow");
  if (!flow.is_null()) {
    const auto& value = flow.as_string();
    if (value == "none") {
      config_change.flow_control = SerialPortConfig::SP_FLOWCONTROL_NONE;
    } else if (value == "rts/cts") {
      config_change.flow_control = SerialPortConfig::SP_FLOWCONTROL_RTS_CTS;
    } else if (value == "dtr/dsr") {
      config_change.flow_control = SerialPortConfig::SP_FLOWCONTROL_DTR_DSR;
    } else {
      throw std::invalid_argument("invalid flow control: " + value);
    }
    config_change.field_mask |= SerialPortConfig::SP_FIELD_FLOW_CONTROL;
  }

  Server::handle_type handle;
  auto lock = server.get_session_handle(session, handle);
}

/**
 * @brief Process "modem" operation
 * 
 * @param input A reference to input JSON value
 * @param output A reference to object container for output JSON value
 * @param session Not used
 */
void Client::modem(const jvalue& input, jvalue::object_type& output, int session)
{
  if (session <= 0) {
    session = input.at("session").as_integer();
  }
}

/**
 * @brief Process "write" operation
 * 
 * @param input A reference to input JSON value
 * @param output A reference to object container for output JSON value
 * @param session Not used
 */
void Client::write(const jvalue& input, jvalue::object_type& output, int session)
{
  if (session <= 0) {
    session = input.at("session").as_integer();
  }
}

/**
 * @brief Process "read" operation
 * 
 * @param input A reference to input JSON value
 * @param output A reference to object container for output JSON value
 * @param session Not used
 */
void Client::read(const jvalue& input, jvalue::object_type& output, int session)
{
  if (session <= 0) {
    session = input.at("session").as_integer();
  }
}

/**
 * @brief Process "close" operation
 * 
 * @param input A reference to input JSON value
 * @param output A reference to object container for output JSON value
 * @param session Not used
 */
void Client::close(const jvalue& input, jvalue::object_type& output, int session)
{
  if (session <= 0) {
    session = input.at("session").as_integer();
  }
}
