#include "socket.hpp"
#include "osport.hpp"
#include <stdexcept>
#include <string>
#include <cassert>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "winsock2.h"
#include "setupapi.h"
#include "windows.h"

static std::string MultiByteToUtf8(const std::string& src)
{
  if (src.empty()) {
    return "";
  }
  std::wstring buf;
  buf.resize(src.size());
  int wchars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                &src[0], src.size(), &buf[0], buf.size());
  if (wchars == 0) {
    return "";
  }
  std::string buf8;
  buf8.resize(wchars * 3);
  int chars = WideCharToMultiByte(CP_UTF8, WC_SEPCHARS,
                &buf[0], wchars, &buf8[0], buf8.size(), nullptr, nullptr);
  buf8.resize(chars);
  return buf8;
}

class Win32Socket : public Socket
{
public:
  static void startup()
  {
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(1, 1), &wsa_data) != 0) {
      throw std::runtime_error("cannot initialize WinSock: " + error_string_static());
    }
  }

  static void cleanup()
  {
    WSACleanup();
  }

  Win32Socket(int af, int type, int protocol)
  {
    socket = ::socket(af, type, protocol);
    if (socket < 0) {
      throw std::runtime_error("cannot create socket: " + error_string());
    }
  }

  Win32Socket(SOCKET socket) : socket(socket)
  {
    assert(socket >= 0);
  }

  virtual ~Win32Socket()
  {
    close();
  }

  virtual void bind(const std::string& address, int port) override
  {
    assert(socket >= 0);

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr(address.c_str());
    saddr.sin_port = htons(port);

    if (::bind(socket, (const struct sockaddr *)&saddr, sizeof(saddr)) != 0) {
      throw std::runtime_error("cannot bind address: " + error_string());
    }
  }

  virtual void get_address(std::string& address, int& port) override
  {
    assert(socket >= 0);

    struct sockaddr_in saddr;
    int saddrlen = sizeof(saddr);
    if (getsockname(socket, (struct sockaddr *)&saddr, &saddrlen) != 0) {
      throw std::runtime_error("cannot get socket address: " + error_string());
    }
    address = inet_ntoa(saddr.sin_addr);
    port = ntohs(saddr.sin_port);
  }

  virtual void listen() override
  {
    assert(socket >= 0);

    if (::listen(socket, 5) != 0) {
      throw std::runtime_error("cannot listen socket: " + error_string());
    }
  }

  virtual Socket::shared_ptr accept() override
  {
    assert(socket >= 0);
    
    SOCKET client = ::accept(socket, nullptr, nullptr);
    if (client < 0) {
      throw std::runtime_error("cannot accept socket: " + error_string());
    }
    return Socket::shared_ptr(new Win32Socket(client));
  }

  virtual void close() override
  {
    if (socket >= 0) {
      closesocket(socket);
      socket = -1;
    }
  }

protected:

  virtual int recv_bytes(void *buffer, int length) override
  {
    assert(socket >= 0);

    return ::recv(socket, (char *)buffer, length, 0);
  }

  virtual int send_bytes(const void *buffer, int length) override
  {
    assert(socket >= 0);

    return ::send(socket, (const char *)buffer, length, 0);
  }

private:
  virtual std::string error_string() override
  {
    return Win32Socket::error_string_static();
  }

  static std::string error_string_static()
  {
    return std::to_string(WSAGetLastError());
  }

  SOCKET socket;
};

class Win32RegKey
{
public:
  Win32RegKey(HKEY hKey) : hKey(hKey) {}
  ~Win32RegKey()
  {
    if (*this) {
      RegCloseKey(hKey);
      hKey = (HKEY)INVALID_HANDLE_VALUE;
    }
  }

  operator bool() const { return (hKey != INVALID_HANDLE_VALUE); }

  bool QueryValueString(const char *name, std::string& buffer)
  {
    if (!*this) {
      return false;
    }

    DWORD len = 0;
    DWORD type;
    int ret = RegQueryValueExA(hKey, name, nullptr, &type, nullptr, &len);
    if ((ret != ERROR_SUCCESS) || (type != REG_SZ)) {
      return false;
    }
    buffer.resize(len);
    if (RegQueryValueExA(hKey, name, nullptr, &type, (PBYTE)&buffer[0], &len) != ERROR_SUCCESS) {
      return false;
    }
    buffer.resize(len - 1);
    return true;
  }

protected:
  HKEY hKey;
};

class Win32OsPort : public OsPort
{
protected:
  std::string get_error_string() const
  {
    char *buffer;
    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr,
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&buffer, 0, nullptr);
    std::string result(buffer);
    LocalFree(buffer);
    return result;
  }

public:
  Win32OsPort()
  {
    SetConsoleOutputCP(CP_UTF8);
    Win32Socket::startup();
  }

  virtual ~Win32OsPort()
  {
    Win32Socket::cleanup();
  }

  virtual int getopt(int argc, char *argv[], const char *options, char*& optarg, int& optind) override
  {
    static char *nextchar;

    if (optind == 0) {
      optind = 1;
      nextchar = nullptr;
    }

    for (;;) {
      char ch;
      if (nextchar && (ch = nextchar[0]) != '\0') {
        auto opt = strchr(options, ch);
        if (!opt) {
          return '?';
        }
        if (opt[1] == ':') {
          if (nextchar[1] != '\0') {
            optarg = nextchar + 1;
          } else {
            optarg = argv[++optind];
          }
          ++optind;
          nextchar = nullptr;
        } else {
          optarg = nullptr;
          ++nextchar;
        }
        return ch;
      }

      if (optind >= argc) {
        // No more argument
        return -1;
      }

      if (argv[optind][0] != '-') {
        // Stop at non-option argument
        return -1;
      }

      nextchar = &argv[optind][1];
      if (nextchar[0] == '-') {
        // Stop at "--" option
        return -1;
      }
    }
  }

  virtual int getpid() override
  {
    return GetCurrentProcessId();
  }

  virtual Socket::shared_ptr create_socket_tcp() override
  {
    return Socket::shared_ptr(new Win32Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
  }

  virtual std::vector<SerialPortInfo> enumerate() override
  {
    std::vector<SerialPortInfo> list;

    HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT,
      nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hDevInfoSet == INVALID_HANDLE_VALUE) {
      return list;
    }

    for (int index = 0;; ++index) {
      SP_DEVINFO_DATA did = { 0 };
      did.cbSize = sizeof(did);

      // Get port name
      if (SetupDiEnumDeviceInfo(hDevInfoSet, index, &did) == 0) {
        break;
      }
      Win32RegKey key(SetupDiOpenDevRegKey(hDevInfoSet, &did, DICS_FLAG_GLOBAL, 0,
        DIREG_DEV, KEY_QUERY_VALUE));
      std::string name;
      if (!key) {
        continue;
      }
      if (!key.QueryValueString("PortName", name)) {
        continue;
      }
      if (name.substr(0, 3) != "COM") {
        continue;
      }
      std::size_t pos;
      int order = std::stoi(name.substr(3), &pos);
      if (pos != (name.size() - 3)) {
        continue;
      }

      std::string desc;
      do {
        // Get port description
        DWORD dataType;
        DWORD length;
        if (!SetupDiGetDeviceRegistryPropertyA(hDevInfoSet, &did, SPDRP_DEVICEDESC,
            &dataType, nullptr, 0, &length)) {
          if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            break;
          }
        }
        desc.resize(length);
        if (!SetupDiGetDeviceRegistryPropertyA(hDevInfoSet, &did, SPDRP_DEVICEDESC,
            &dataType, (PBYTE)&desc[0], length, &length)) {
          break;
        }
        if (dataType != REG_SZ) {
          break;
        }
        desc.resize(length - 1);
        desc = MultiByteToUtf8(desc);
      } while (0);
      list.emplace_back(name, desc);
      list.back().order = order;
    }

    return list;
  }

  /**
   * @brief Open port
   * 
   * @param path Path of port
   * @return Port handle
   */
  virtual handle_type open_port(const char* path) override
  {
    std::string full_path = R"(\\.\)";
    full_path += path;

    HANDLE hCom = CreateFile(
      full_path.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      0,
      nullptr,
      OPEN_EXISTING,
      FILE_FLAG_OVERLAPPED,
      nullptr
    );
    if (hCom == INVALID_HANDLE_VALUE) {
      // TODO: exceptions
      return nullptr;
    }

    return (handle_type)hCom;
  }

  /**
   * @brief Configure port
   * 
   * @param handle Port handle
   * @param set Config to change
   * @param get Reference to store current config
   */
  virtual void configure_port(handle_type handle, const SerialPortConfig& set, SerialPortConfig& get) override
  {
    HANDLE hCom = (HANDLE)handle;
    DCB dcb;

    // Read current state
    if (!GetCommState(hCom, &dcb)) {
      throw std::runtime_error("cannot get old state: " + get_error_string());
    }
    
    // Change state
    if (set.field_mask & SerialPortConfig::SP_FIELD_BAUD_RATE) {
      dcb.BaudRate = set.baud_rate;
    }
    if (set.field_mask & SerialPortConfig::SP_FIELD_DATA_BITS) {
      switch (set.data_bits) {
      case SerialPortConfig::SP_DATABITS_5:
        dcb.ByteSize = 5;
        break;
      case SerialPortConfig::SP_DATABITS_6:
        dcb.ByteSize = 6;
        break;
      case SerialPortConfig::SP_DATABITS_7:
        dcb.ByteSize = 7;
        break;
      case SerialPortConfig::SP_DATABITS_8:
        dcb.ByteSize = 8;
        break;
      default:
        throw std::invalid_argument("unsupported data bits: " + std::to_string(set.data_bits));
      }
    }
    if (set.field_mask & SerialPortConfig::SP_FIELD_PARITY) {
      switch (set.parity) {
      case SerialPortConfig::SP_PARITY_NONE:
        dcb.Parity = NOPARITY;
        break;
      case SerialPortConfig::SP_PARITY_ODD:
        dcb.Parity = ODDPARITY;
        break;
      case SerialPortConfig::SP_PARITY_EVEN:
        dcb.Parity = EVENPARITY;
        break;
      case SerialPortConfig::SP_PARITY_MARK:
        dcb.Parity = MARKPARITY;
        break;
      case SerialPortConfig::SP_PARITY_SPACE:
        dcb.Parity = SPACEPARITY;
        break;
      default:
        throw std::invalid_argument("unsupported parity mode: " + std::to_string(set.parity));
      }
    }
    if (set.field_mask & SerialPortConfig::SP_FIELD_STOP_BITS) {
      switch (set.stop_bits) {
      case SerialPortConfig::SP_STOPBITS_1:
        dcb.StopBits = ONESTOPBIT;
        break;
      case SerialPortConfig::SP_STOPBITS_1_5:
        dcb.StopBits = ONE5STOPBITS;
        break;
      case SerialPortConfig::SP_STOPBITS_2:
        dcb.StopBits = TWOSTOPBITS;
        break;
      default:
        throw std::invalid_argument("unsupported stop bits: " + std::to_string(set.stop_bits));
      }
    }
    if (set.field_mask & SerialPortConfig::SP_FIELD_XON_CHAR) {
      dcb.XonChar = set.xon_char;
    }
    if (set.field_mask & SerialPortConfig::SP_FIELD_XOFF_CHAR) {
      dcb.XoffChar = set.xoff_char;
    }
    if (set.field_mask & SerialPortConfig::SP_FIELD_ERROR_CHAR) {
      dcb.ErrorChar = set.error_char;
    }

    if (!SetCommState(hCom, &dcb)) {
      throw std::runtime_error("cannot set state: " + get_error_string());
    }

    if (!GetCommState(hCom, &dcb)) {
      throw std::runtime_error("cannot get new state: " + get_error_string());
    }

    get.baud_rate = dcb.BaudRate;
    switch (dcb.ByteSize) {
    case 5:
      get.data_bits = SerialPortConfig::SP_DATABITS_5;
      break;
    case 6:
      get.data_bits = SerialPortConfig::SP_DATABITS_6;
      break;
    case 7:
      get.data_bits = SerialPortConfig::SP_DATABITS_7;
      break;
    case 8:
      get.data_bits = SerialPortConfig::SP_DATABITS_8;
      break;
    default:
      throw std::invalid_argument("unknown data bits: " + std::to_string(dcb.ByteSize));
    }
    switch (dcb.Parity) {
    case NOPARITY:
      get.parity = SerialPortConfig::SP_PARITY_NONE;
      break;
    case ODDPARITY:
      get.parity = SerialPortConfig::SP_PARITY_ODD;
      break;
    case EVENPARITY:
      get.parity = SerialPortConfig::SP_PARITY_EVEN;
      break;
    case MARKPARITY:
      get.parity = SerialPortConfig::SP_PARITY_MARK;
      break;
    case SPACEPARITY:
      get.parity = SerialPortConfig::SP_PARITY_SPACE;
      break;
    default:
      throw std::invalid_argument("unknown parity: " + std::to_string(dcb.Parity));
    }
    switch (dcb.StopBits) {
    case ONESTOPBIT:
      get.stop_bits = SerialPortConfig::SP_STOPBITS_1;
      break;
    case ONE5STOPBITS:
      get.stop_bits = SerialPortConfig::SP_STOPBITS_1_5;
      break;
    case TWOSTOPBITS:
      get.stop_bits = SerialPortConfig::SP_STOPBITS_2;
      break;
    default:
      throw std::invalid_argument("unknown stop bits: " + std::to_string(dcb.StopBits));
    }
    get.xon_char = dcb.XonChar;
    get.xoff_char = dcb.XoffChar;
    get.error_char = dcb.ErrorChar;
  }

  /**
   * @brief Close port
   * 
   * @param handle Port handle
   */
  virtual void close_port(handle_type handle) override
  {

  }

};

OsPort::shared_ptr OsPort::create()
{
  return shared_ptr(new Win32OsPort());
}
