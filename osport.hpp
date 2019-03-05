#ifndef _OSPORT_HPP_
#define _OSPORT_HPP_

#include <string>
#include "socket.hpp"
#include <memory>

struct SerialPortInfo
{
  std::string path;
  std::string name;
  int vendor_id;
  int product_id;
  int order;

  SerialPortInfo(const std::string& path, const std::string& name)
  : path(path), name(name)
  {
  }
};

struct SerialPortSetup
{
  int baud_rate;              ///< Baud rate in bps
  enum ParityMode
  {
    SP_PARITY_NONE,
    SP_PARITY_ODD,
    SP_PARITY_EVEN,
    SP_PARITY_MARK,
    SP_PARITY_SPACE,
  } parity;                   ///< Parity mode
  enum DataBitsMode
  {
    SP_DATABITS_5 = 5,
    SP_DATABITS_6,
    SP_DATABITS_7,
    SP_DATABITS_8,
  } data_bits;                ///< Data bits
  enum StopBitsMode
  {
    SP_STOPBITS_1,
    SP_STOPBITS_1_5,
    SP_STOPBITS_2,
  }
  enum FlowControlMode
  {
    SP_FLOWCONTROL_NONE,
    SP_FLOWCONTROL_RTS_CTS,
    SP_FLOWCONTROL_DTR_DSR,
  } flow_control
};

/**
 * @brief An abstract class to declare OS portable features
 */
class OsPort
{
protected:
  /**
   * @brief Construct a new OsPort object.
   */
  OsPort() = default;

public:
  /**
   * @brief Type alias definition for shared pointer to this class
   */
  using shared_ptr = std::shared_ptr<OsPort>;

  /**
   * @brief Create a new OsPort derived object.
   * 
   * @return A shared pointer to OsPort object
   */
  static shared_ptr create();

  /**
   * @brief Destroy the OsPort object.
   */
  virtual ~OsPort() = default;

  /**
   * @brief getopt() function
   * 
   * @param argc A number of arguments
   * @param argv A list of pointer to each argument
   * @param options A string containing the legitimate option characters
   * @param optarg A reference of area to save optarg
   * @param optind A reference of area to save optind
   * @return Character code
   */
  virtual int getopt(int argc, char *argv[], const char *options, char*& optarg, int& optind) = 0;

  /**
   * @brief getpid() function
   * 
   * @return Current process ID
   */
  virtual int getpid() = 0;

  /**
   * @brief Create a Socket object for TCP connection
   * 
   * @return A shared pointer to Socket object
   */
  virtual Socket::shared_ptr create_socket_tcp() = 0;

  /**
   * @brief Enumerate serial ports
   * 
   * @return A vector
   */
  virtual std::vector<SerialPortInfo> enumerate() = 0;
};

#endif /* _OSPORT_HPP_ */
