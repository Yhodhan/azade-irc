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
  return this->is_tls ? SSL_read(this->ssl, buffer, sizeof(buffer) - 1)
                      : read(this->sock, buffer, sizeof(buffer) - 1);
}
void IrcConnection::write_reply(std::string reply) {
  reply += "\r\n";
  this->is_tls ? SSL_write(this->ssl, reply.c_str(), reply.size())
               : write(this->sock, reply.c_str(), reply.size());
}

void IrcConnection::work_loop() {
  char buffer[1024] = {0};

  std::string cmd;
  while (true) {
    ssize_t bytes = this->read_msg(buffer);

    if (bytes <= 0)
      break;

    cmd.append(buffer, bytes);

    size_t pos;
    while ((pos = cmd.find("\r\n")) != std::string::npos) {
      std::string msg = cmd.substr(0, pos);
      cmd.erase(0, pos + 2);
      std::cout << "Command to execute: " << msg << std::endl;
      this->handle_command(msg);
    }
  }
}

void IrcConnection::handle_command(std::string command) {
  Command cmd = parse_command(command);

  switch (cmd.cmd) {
  case CAP:
    this->write_reply(std::string(":azade CAP * LS :"));
    break;
  case JOIN:
    // this->write_reply(std::string("JOIN command"));
    break;
  case NICK:
    command_nick(cmd.params);
    break;
  case USER:
    command_user(cmd.params);
    break;
  default:
    this->write_reply(std::string("INVALID command"));
  }
}

// ------------------
//     Commands
// ------------------
void IrcConnection::command_nick(Params params) {
  this->nick = params[0];
  std::cout << "User nick is: " << this->nick << std::endl;
}

void IrcConnection::command_user(Params params) {

  // std::cout << "params size: " << params.size() << std::endl;
  // for (auto str : params)
  //   std::cout << "str: " << str << std::endl;
  this->user = new User(params[0], params[1], params[2], params[3]);

  std::cout << "User is: " << this->user->username << std::endl;
  std::cout << "User is: " << this->user->hostname << std::endl;
  std::cout << "User is: " << this->user->realname << std::endl;
  std::cout << "User is: " << this->user->servername << std::endl;
}
