#pragma once

#include "../commands/commands.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct User {
  std::string username;
  std::string hostname;
  std::string servername;
  std::string realname;

  User(const std::string user, const std::string host, const std::string server,
       const std::string realname)
      : username(user), hostname(host), servername(server), realname(realname) {
  }
};

class IrcConnection {

public:
  IrcConnection(int fd);
  IrcConnection(int fd, SSL_CTX *ssl_ctx);
  ~IrcConnection();

  bool handshake_successful() const;
  SSL *get_ssl() const;
  void work_loop();

protected:
  ssize_t read_msg(char *buffer);
  void write_reply(const std::string reply);
  void handle_command(std::string cmd);
  // Handle commands
  void command_nick(Params params);
  void command_user(Params params);

private:
  int sock;
  SSL *ssl = NULL;
  std::string nick;
  bool handshake_ok = false;
  bool is_tls = false;
  User *user = NULL;
};
