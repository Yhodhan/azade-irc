#include "connection.h"
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <sys/types.h>

// -----------------------
//      Constructors
// -----------------------

IrcConnection::IrcConnection(int sock) : sock(sock) {}

IrcConnection::IrcConnection(int sock, SSL_CTX *ssl_ctx) : sock(sock) {
  this->ssl = SSL_new(ssl_ctx);
  SSL_set_fd(ssl, sock);

  if (SSL_accept(ssl) <= 0) {
    std::cerr << "TLS Handshake failed" << std::endl;
    ERR_print_errors_fp(stderr);
    this->handshake_ok = false;
    this->is_tls = false;
  } else {
    std::cout << "TLS handshake complete" << std::endl;
    this->handshake_ok = true;
    this->is_tls = true;
  }
}

// -----------------------
//        Destructor
// -----------------------

IrcConnection::~IrcConnection() {
  SSL_shutdown(this->ssl);
  SSL_free(this->ssl);
  close(this->sock);
}

// -----------------------
//        Getters
// -----------------------

bool IrcConnection::handshake_successful() const { return this->handshake_ok; }
SSL *IrcConnection::get_ssl() const { return this->ssl; }

// -----------------------
//      Main work loop
// -----------------------

void IrcConnection::work_loop() {
  char buffer[1024] = {0};
  ssize_t bytes = this->is_tls ? SSL_read(this->ssl, buffer, sizeof(buffer) - 1)
                               : read(this->sock, buffer, sizeof(buffer) - 1);
  while (true) {
    if (bytes < 0) {
      perror("Reading bytes");
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
