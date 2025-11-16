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
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define TLS_PORT 6697
#define PLAIN_PORT 6667

class IrcServer {
public:
  IrcServer();
  ~IrcServer();

  void start();
  void event_loop();
  void accept_client(int sock, bool use_tls);

protected:
  void init_ssl();
  int setup_socket(int port);

private:
  int tls_socket;
  int plain_socket;
  sockaddr_in srv_address;
  int tls_port = TLS_PORT;
  int plain_port = PLAIN_PORT;

  std::vector<Channel> channels;
  SSL_CTX *ssl_ctx = nullptr;
  // connections data structures
  std::shared_ptr<Connections> conns;
};
