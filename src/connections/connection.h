#pragma once

#include "../commands/commands.h"
#include "../users/user.h"
#include <arpa/inet.h>
#include <cstdint>
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

class IrcConnection;

using ConnectionsMap = std::unordered_map<int, std::shared_ptr<IrcConnection>>;

struct Connections {
  std::mutex mtx;
  ConnectionsMap connections;
};

class IrcConnection {

public:
  IrcConnection(int fd, uint32_t id, std::shared_ptr<Connections> conns,
                std::shared_ptr<Users> users);

  IrcConnection(int fd, uint32_t id, SSL_CTX *ssl_ctx,
                std::shared_ptr<Connections> conns,
                std::shared_ptr<Users> users);

  ~IrcConnection();

  SSL *get_ssl() const;
  void work_loop();
  bool get_is_tls() const;
  bool handshake_successful() const;
  uint32_t get_user_id() const;

protected:
  ssize_t read_msg(char *buffer);
  void write_reply(const std::string reply);
  void handle_command(std::string cmd);
  bool user_exist(Params params);
  // Handle commands
  void command_cap(Params params);
  void command_join(Params params);
  void command_nick(Params params);
  void command_user(Params params);

private:
  int sock;
  SSL *ssl = NULL;
  bool handshake_ok = false;
  bool is_tls = false;
  uint32_t user_id;

  std::shared_ptr<Connections> conns;
  std::shared_ptr<Users> users;
};
