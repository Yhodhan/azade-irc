#include "../connections/connection.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

class IrcServer {
public:
  IrcServer();
  ~IrcServer();

  void accept_client();
  void event_loop();

protected:
  void welcome_message(int client_fd);
  void init_ssl();

private:
  int irc_socket;
  sockaddr_in srv_address;
  int port = 6697;

   SSL_CTX *ssl_ctx = nullptr;
  // connections data structures
  std::unordered_map<int, std::shared_ptr<IrcConnection>> connections;
};
