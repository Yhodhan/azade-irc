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

class IrcConnection {

public:
  IrcConnection(int fd);
  IrcConnection(int fd, SSL_CTX *ssl_ctx);
  ~IrcConnection();

  bool handshake_successful() const;
  SSL *get_ssl() const;

  void work_loop();

protected:
  void handle_tls();
  void handle_plain();

private:
  int sock;
  SSL *ssl = NULL;
  std::string nick;
  bool handshake_ok;
  bool is_tls;
};
