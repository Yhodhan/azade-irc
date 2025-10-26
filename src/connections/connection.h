#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

class IrcConnection {

public:
  IrcConnection(int fd, SSL_CTX * ssl_ctx);
  ~IrcConnection();

  void work_loop();

private:
  int client_fd;
  SSL* ssl = NULL;
  std::string nick;
};
