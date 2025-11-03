#include "connection.h"
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/types.h>

// -----------------------
//      Constructor
// -----------------------

IrcConnection::IrcConnection(int fd, SSL_CTX *ssl_ctx) : client_fd(fd) {
  this->ssl = SSL_new(ssl_ctx);
  SSL_set_fd(ssl, client_fd);

  if (SSL_accept(ssl) <= 0) {
    std::cerr << "TLS Handshake failed" << std::endl;
    ERR_print_errors_fp(stderr);
    handshake_ok = false;
  } else {
    std::cout << "TLS handshake complete" << std::endl;
    handshake_ok = true;
  }
}

// -----------------------
//        Destructor
// -----------------------
IrcConnection::~IrcConnection() {
  SSL_shutdown(this->ssl);
  SSL_free(ssl);
  close(this->client_fd);
}

// -----------------------
//        Getters
// -----------------------
bool IrcConnection::handshake_succesful() const { return this->handshake_ok; }
SSL *IrcConnection::get_ssl() const { return this->ssl; }

// -----------------------
//      Main work loop
// -----------------------
void IrcConnection::work_loop() {
  std::cout << "Client connected" << std::endl;
  char buffer[1024] = {0};

  while (true) {

    ssize_t bytes = SSL_read(this->ssl, buffer, sizeof(buffer) - 1);

    if (bytes < 0) {
      perror("ssl");
      std::cerr << "=== Recv failed: " << errno << std::endl;
      break;
    } else if (bytes == 0) {
      std::cout << "Client Disconnected" << std::endl;
      break;
    }

    buffer[bytes] = 0;
    std::cout << "Message from client: " << buffer << std::endl;
  }
}
