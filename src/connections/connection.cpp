#include "connection.h"

// -----------------------
//      Constructors
// -----------------------

IrcConnection::IrcConnection(int fd, std::shared_ptr<Connections> conns)
    : sock(fd), conns(conns) {}

IrcConnection::IrcConnection(int fd, SSL_CTX *ssl_ctx,
                             std::shared_ptr<Connections> conns)
    : sock(fd), conns(conns) {
  this->ssl = SSL_new(ssl_ctx);
  SSL_set_fd(ssl, sock);

  if (SSL_accept(ssl) <= 0) {
    std::cerr << "TLS handshake failed" << std::endl;
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

SSL *IrcConnection::get_ssl() const { return this->ssl; }
bool IrcConnection::get_is_tls() const { return this->is_tls; }
bool IrcConnection::handshake_successful() const { return this->handshake_ok; }

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

  if (params.size() < 4) {
    write_reply("461 USER :Not enough parameters");
    return;
  }

//  if (user_exist(params)) {
//    write_reply("433 USER :User already registered");
//    return;
//  }

  this->username = params[0];
  this->hostname = params[1];
  this->realname = params[2];
  this->servername = params[3];
}

// ------------------
//  Helper functions
// ------------------

std::string IrcConnection::get_username() { return this->username; }
std::string IrcConnection::get_hostname() { return this->hostname; }
std::string IrcConnection::get_servername() { return this->servername; }
std::string IrcConnection::get_realname() { return this->realname; }
