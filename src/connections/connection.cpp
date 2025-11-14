#include "connection.h"

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
ssize_t IrcConnection::read_msg(char *buffer) {
  return this->is_tls ? SSL_read(this->ssl, buffer, sizeof(*buffer) - 1)
                      : read(this->sock, buffer, sizeof(*buffer) - 1);
}
void IrcConnection::write_reply(const std::string reply) {
  this->is_tls ? SSL_write(this->ssl, reply.c_str(), reply.size())
               : write(this->sock, reply.c_str(), reply.size());
}

void IrcConnection::work_loop() {
  char buffer[1024] = {0};

  while (true) {
    ssize_t bytes = this->read_msg(buffer);

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
    std::string cmd(buffer);
    this->handle_command(cmd);
  }
}

void IrcConnection::handle_command(std::string command) {
  Command cmd = parse_command(command);

  switch (cmd.cmd) {
  case CAP:
    this->write_reply(std::string("CAP command"));
  case JOIN:
    this->write_reply(std::string("JOIN command"));
  default:
    this->write_reply(std::string("INVALID command"));
  }
}

// ------------------
//   Parse commands
// ------------------
