#pragma once

#include "../channels/channel.h"
#include "../numerics.h"
#include "../users/user.h"
#include "../commands/commands.h"
#include <arpa/inet.h>
#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

// #define TLS_PORT 6697
#define PORT 6667
#define BUF_SIZE 4096

constexpr int MAX_EVENTS = 100;
constexpr int MAX_TIMEOUT = 3000;

class IrcServer {
public:
  IrcServer();
  ~IrcServer();

  void start(void);
  void event_loop(void);
  void shutdown(void);
  static void on_sigint(int);
  void install_signal_handler(IrcServer *server);

  /* --------------------------------*/
  /*            Getters              */
  /* --------------------------------*/
  UserMap get_users();
  User *get_user(int fd);
  void close_user(int fd);
  User *get_user_by_id(uint32_t fd);
  bool user_exists(int fd, Params &params);
  bool channel_exist(const std::string &name);
  Channel *get_channel(const std::string &ch_name);
  std::map<std::string, Channel *> get_channels();

  /* --------------------------------*/
  /*            Responses            */
  /* --------------------------------*/
  void write_reply(int fd, std::string reply);
  void broadcast(int from_fd, Channel *channel, const std::string &msg);
  void send_numeric(int fd, IrcNumeric code, const std::string &target,
                    const std::string &msg);

  /* --------------------------------*/
  /*         Signal Variables        */
  /* --------------------------------*/
  static IrcServer *instance;

private:
  void init_ssl(void);
  void setup_poll(void);
  int setup_socket(int port);
  void handle_msg(struct epoll_event *event);
  int poll_wait(struct epoll_event **events);
  void handle_command(int fd, std::string command);
  ssize_t read_msg(int fd, char *buffer, size_t size);
  void print_error(const std::string &msg, bool with_errno = false);
  void accept_client(int sock, bool use_tls);

  /* --------------------------------*/
  /*           Variables             */
  /* --------------------------------*/

  int epollfd;
  SSL *ssl = NULL;
  int sockfd;
  sockaddr_in srv_address;
  int port = PORT;

  std::map<std::string, Channel *> channels;
  SSL_CTX *ssl_ctx = nullptr;

  UserMap users;
  UserIdMap users_id;
  std::atomic<bool> running;
  std::map<int, std::string> cmdBuffers;

  /* --------------------------------*/
  /*          Exceptions             */
  /* --------------------------------*/

  class socketException : public std::exception {
  public:
    virtual const char *what() const throw();
  };

  class bindException : public std::exception {
  public:
    virtual const char *what() const throw();
  };

  class AcceptException : public std::exception {
  public:
    virtual const char *what() const throw();
  };

  class pollException : public std::exception {
  public:
    virtual const char *what() const throw();
  };

  class pollAddException : public std::exception {
  public:
    virtual const char *what() const throw();
  };

  class pollWaitException : public std::exception {
  public:
    virtual const char *what() const throw();
  };

  class readFdError : public std::exception {
  public:
    virtual const char *what() const throw();
  };
};
