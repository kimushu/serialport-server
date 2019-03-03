#include "socket.hpp"
#include "osport.hpp"
#include <stdexcept>
#include <string>
#include <cassert>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "winsock2.h"
#include "setupapi.h"

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

  virtual void enumerate(std::vector<SerialPortInfo>& list) override
  {
    list.clear();

    HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT,
      nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hDevInfoSet == INVALID_HANDLE_VALUE) {
      return;
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
  }

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
};

OsPort::shared_ptr OsPort::create()
{
  return shared_ptr(new Win32OsPort());
}
