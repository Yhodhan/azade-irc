#pragma once

#include "../channels/channel.h"
#include "../connections/connection.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define TLS_PORT 6697
#define PLAIN_PORT 6667

constexpr int MAX_EVENTS = 10;

class IrcServer {
public:
  IrcServer();
  ~IrcServer();

  void start(void);
  void event_loop();
  void accept_client(int sock, bool use_tls);

protected:
  void init_ssl(void);
  int poll_wait(void);
  void setup_poll(void);
  int  setup_socket(int port);
  void print_error(const std::string msg, bool with_errno = false);
  void handle_connection(struct epoll_event *event);

private:
  int epoll;
  int tls_socket;
  int plain_socket;
  sockaddr_in srv_address;
  int tls_port = TLS_PORT;
  int plain_port = PLAIN_PORT;

  struct epoll_event event, events[MAX_EVENTS];

  std::vector<Channel> channels;
  SSL_CTX *ssl_ctx = nullptr;
  // connections data structures
  std::shared_ptr<Connections> conns;
  std::shared_ptr<Users> users;
  
  // Define exceptions
  class socketException : public std::exception
  { public: virtual const char *what() const throw(); };

  class bindException : public std::exception
  { public: virtual const char *what() const throw(); };

  class pollException : public std::exception
  { public: virtual const char *what() const throw(); };

  class pollWaitException : public std::exception
  { public: virtual const char *what() const throw(); };
};
