#pragma once 

#include "../channels/channel.h"
#include "../connections/connection.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

class IrcServer {
public:
  IrcServer();
  ~IrcServer();

  void accept_client(int sock, bool use_tls);
  void event_loop();
  void start();

protected:
  void init_ssl();
  int setup_socket(int port);

private:
  int tls_socket;
  int plain_socket;
  sockaddr_in srv_address;
  int tls_port = 6697;
  int plain_port = 6667;

  std::vector<Channel> channels;
  SSL_CTX *ssl_ctx = nullptr;
  // connections data structures
  std::unordered_map<int, std::shared_ptr<IrcConnection>> connections;
};
