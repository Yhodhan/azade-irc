#pragma once

#include "../channels/channel.h"
#include "../commands/commands.h"
#include "../users/user.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <fcntl.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>
#include <vector>

//#define TLS_PORT 6697
#define PORT 6667
#define BUF_SIZE 4096

constexpr int MAX_EVENTS = 100;
constexpr int MAX_TIMEOUT = 3000;

class IrcServer {
public:
  IrcServer();
  ~IrcServer();

  void start(void);
  void event_loop();

private:

  void init_ssl(void);
  void setup_poll(void);
  int setup_socket(int port);
  int poll_wait(struct epoll_event **events);
  void handle_msg(struct epoll_event *event);
  void print_error(const std::string msg, bool with_errno = false);
  void write_reply(int fd, std::string reply);
  ssize_t read_msg(int fd, char *buffer, size_t size);
  void handle_command(int fd, std::string command);

  void accept_client(int sock, bool use_tls);

  void command_cap(int fd, Params params);
  void command_join(int fd, Params params);
  void command_nick(int fd, Params params);
  void command_user(int fd, Params params);
  void command_ping(int fd, Params params);
  void command_mode(int fd, Params params);
  void command_quit(int fd, Params params);
  User* get_user(int fd);

  int epollfd;
  //int tls_socket;
  SSL *ssl = NULL;
  int sockfd;
  sockaddr_in srv_address;
  //int tls_port = TLS_PORT;
  int port = PORT;

  std::vector<Channel> channels;
  SSL_CTX *ssl_ctx = nullptr;

  UserMap users;
  std::map<int, std::string> cmdBuffers;

  // Define exceptions
  class socketException : public std::exception
  { public: virtual const char *what() const throw(); };

  class bindException : public std::exception
  { public: virtual const char *what() const throw(); };

  class AcceptException : public std::exception
  { public: virtual const char *what() const throw(); };

  class pollException : public std::exception
  { public: virtual const char *what() const throw(); };

  class pollAddException : public std::exception
  { public: virtual const char *what() const throw(); };

  class pollWaitException : public std::exception
  { public: virtual const char *what() const throw(); };

  class readFdError: public std::exception
  { public: virtual const char *what() const throw(); };
};
