#include "connection.h"
#include <cstring>
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
  std::string reply;
  while (true) {
    ssize_t bytes = this->is_tls
                        ? SSL_read(this->ssl, buffer, sizeof(buffer) - 1)
                        : read(this->sock, buffer, sizeof(buffer) - 1);
    if (bytes < 0) {
      perror("Reading bytes");
      std::cerr << "=== Recv failed: " << errno << std::endl;
      break;
    } else if (bytes == 0) {
      std::cerr << "=== Client Disconnected" << std::endl;
      break;
    }

    buffer[bytes] = 0;
    std::cout << "Message from client: " << buffer << std::endl;
    // here should parse the commands, return answers
    reply = this->parse_command(buffer);

    this->is_tls ? SSL_write(this->ssl, reply.c_str(), reply.size())
                 : write(this->sock, reply.c_str(), reply.size());
  }
}

std::string IrcConnection::parse_command(char command[]) {
  std::string cmd(command);
  if (cmd == "CAP LS 302") {
    return std::string(":azade-server CAP * LS: \r\n");
  }
  return "";
}
