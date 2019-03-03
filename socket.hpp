#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

class Socket : public std::streambuf
{
protected:
  Socket() = default;

public:
  typedef std::shared_ptr<Socket> shared_ptr;

  virtual ~Socket() = default;

  virtual void bind(const std::string& address, int port) = 0;
  virtual void get_address(std::string& address, int& port) = 0;
  virtual void listen() = 0;
  virtual shared_ptr accept() = 0;
  virtual void close() = 0;
  virtual std::string error_string() = 0;
  virtual int recv_bytes(void *buffer, int length) = 0;
  virtual int send_bytes(const void *buffer, int length) = 0;

protected:
  virtual int_type underflow() override
  {
    if (!gptr() || (gptr() >= egptr())) {
      auto remainder = std::min<int>(egptr() - eback(), 1);
      if (remainder > 0) {
        std::memmove(read_buffer, gptr() - remainder, remainder);
      }
      int len = recv_bytes(read_buffer + remainder, sizeof(read_buffer) - 1 - remainder);
      if (len == 0) {
        return traits_type::eof();
      } else if (len < 0) {
        throw std::runtime_error("socket recv failed: " + error_string());
      }
      setg(read_buffer, read_buffer + remainder, read_buffer + remainder + len);
    }
    return *gptr();
  }

  virtual int_type overflow(int_type c = traits_type::eof()) override
  {
    if (traits_type::eq_int_type(c, traits_type::eof())) {
      return c;
    }
    char ch = c;
    if (xsputn(&ch, 1) < 0) {
      return traits_type::eof();
    }
    return 1;
  }

  virtual std::streamsize xsputn(const char_type* s, std::streamsize count) override
  {
    int len = send_bytes(s, (int)count);
    if (len < 0) {
      throw std::runtime_error("socket send failed: " + error_string());
    }
    return len;
  }

private:
  char read_buffer[1024 + 1];
};

#endif /* _SOCKET_HPP_ */
