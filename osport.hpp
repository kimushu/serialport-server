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

struct SerialPortConfig
{
  enum FieldMask
  {
    SP_FIELD_BAUD_RATE    = (1<<0),
    SP_FIELD_PARITY       = (1<<1),
    SP_FIELD_DATA_BITS    = (1<<2),
    SP_FIELD_STOP_BITS    = (1<<3),
    SP_FIELD_FLOW_CONTROL = (1<<4),
    SP_FIELD_XON_CHAR     = (1<<5),
    SP_FIELD_XOFF_CHAR    = (1<<6),
    SP_FIELD_ERROR_CHAR   = (1<<7),
  };
  int field_mask;             ///< Combination of SP_FIELD_*
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
  } stop_bits;                ///< Stop bits
  enum FlowControlMode
  {
    SP_FLOWCONTROL_NONE,
    SP_FLOWCONTROL_RTS_CTS,
    SP_FLOWCONTROL_DTR_DSR,
  } flow_control;             ///< Flow control
  char xon_char;              ///< XON character
  char xoff_char;             ///< XOFF character
  char error_char;            ///< Parity error character
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
   * @brief Type alias definition for port handle
   */
  using handle_type = void*;

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

  /**
   * @brief Open port
   * 
   * @param path Path of port
   * @return Port handle
   */
  virtual handle_type open_port(const char* path) = 0;

  /**
   * @brief Configure port
   * 
   * @param handle Port handle
   * @param set Config to change
   * @param get Reference to store current config
   */
  virtual void configure_port(handle_type handle, const SerialPortConfig& set, SerialPortConfig& get) = 0;

  /**
   * @brief Close port
   * 
   * @param handle Port handle
   */
  virtual void close_port(handle_type handle) = 0;

};

#endif /* _OSPORT_HPP_ */
