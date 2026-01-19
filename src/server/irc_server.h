#pragma once

#include "../channels/channel.h"
#include "../commands/commands.h"
#include "../numerics.h"
#include "../users/user.h"
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
#include <unordered_map>
#include <vector>

#define TLS_PORT 6697
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
  /*         Signal Variables        */
  /* --------------------------------*/
  static IrcServer *instance;
  using Handler = void (IrcServer::*)(int, Params &);

private:
  void init_ssl(void);
  void setup_poll(void);
  void init_command_map();
  int setup_socket(int port);
  void set_user_tls(User *user);
  void handle_msg(struct epoll_event *event);
  int poll_wait(struct epoll_event **events);
  void write_reply(int fd, std::string reply);
  void handle_command(int fd, std::string command);
  ssize_t read_msg(int fd, char *buffer, size_t size);
  void print_error(const std::string &msg, bool with_errno = false);
  void send_numeric(int fd, IrcNumeric code, const std::string &target,
                    const std::string &msg);

  User *get_user(int fd);
  void close_user(int fd);
  bool is_tls_connection(int fd);
  User *get_user_by_id(uint32_t fd);
  void handle_tls_user(int fd, User *user);
  bool user_exists(int fd, Params &params);
  std::string channel_name(std::string chl);
  void accept_client(int sock, bool use_tls);
  bool channel_exist(const std::string &name);
  Channel *get_channel(const std::string &ch_name);
  void dispatch_command(int fd, std::string &command);
  void broadcast(int from_fd, Channel *channel, const std::string &msg);
  /* --------------------------------*/
  /*           Commands              */
  /* --------------------------------*/

  void command_cap(int fd, Params &params);
  void command_join(int fd, Params &params);
  void command_nick(int fd, Params &params);
  void command_user(int fd, Params &params);
  void command_ping(int fd, Params &params);
  void command_mode(int fd, Params &params);
  void command_list(int fd, Params &params);
  void command_quit(int fd, Params &params);
  void command_topic(int fd, Params &params);
  void command_privmsg(int fd, Params &params);

  /* --------------------------------*/
  /*           Variables             */
  /* --------------------------------*/

  int tlsfd;
  int sockfd;
  int epollfd;
  int port = PORT;
  int tls_port = TLS_PORT;
  sockaddr_in srv_address;

  SSL_CTX *ssl_ctx = nullptr;
  std::map<std::string, Channel *> channels;

  UserMap users;
  UserIdMap users_id;
  std::atomic<bool> running;
  std::map<int, std::string> cmdBuffers;
  std::unordered_map<CmdType, Handler> handlers;
  std::unordered_map<int, SSL *> tls_connections;

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

  class acceptException : public std::exception {
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

  class sslError : public std::exception {
  public:
    virtual const char *what() const throw();
  };

  class sslReadError : public std::exception {
  public:
    virtual const char *what() const throw();
  };

  class sslWriteError : public std::exception {
  public:
    virtual const char *what() const throw();
  };
};
