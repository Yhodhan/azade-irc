#include "connection.h"

// -----------------------
//      Constructors
// -----------------------

IrcConnection::IrcConnection(int fd, uint32_t id,
                             std::shared_ptr<Connections> conns,
                             std::shared_ptr<Users> users)
    : sock(fd), user_id(id), conns(conns) {}

IrcConnection::IrcConnection(int fd, uint32_t id, SSL_CTX *ssl_ctx,
                             std::shared_ptr<Connections> conns,
                             std::shared_ptr<Users> users)
    : sock(fd), user_id(id), conns(conns), users(users) {
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
uint32_t IrcConnection::get_user_id() const { return this->user_id; }

// -----------------------
//      Main work loop
// -----------------------

ssize_t IrcConnection::read_msg(char *buffer, size_t size) {
  return this->is_tls ? SSL_read(this->ssl, buffer, size - 1)
                      : read(this->sock, buffer, size - 1);
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
    ssize_t bytes = this->read_msg(buffer, sizeof(buffer));

    if (bytes <= 0) {
      std::cout << "Closed connection" << std::endl;
      break;
    }

    cmd.append(buffer, bytes);

    size_t pos;
    while ((pos = cmd.find("\r\n")) != std::string::npos) {
      std::string msg = cmd.substr(0, pos);
      cmd.erase(0, pos + 2);
      std::cout << "Command to execute: " << msg << std::endl;
      this->handle_command(msg);
    }
  }
  std::cout << "Out of work loop" << std::endl;
}

void IrcConnection::handle_command(std::string command) {
  Command cmd = parse_command(command);
  switch (cmd.cmd) {
  case CAP:
    command_cap(cmd.params);
    break;
  case JOIN:
    command_join(cmd.params);
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

std::shared_ptr<User> IrcConnection::get_user() { 
  return this->users->users_map[this->user_id];
}
// ------------------
//     Commands
// ------------------

// ------------------
//    Cap command
void IrcConnection::command_cap(Params params) {
  std::string msg;
  auto user = this->get_user();
  auto nick = user->get_nick();

  if (nick == "")
    msg = std::string(":azade CAP * LS :");
  else 
    msg = std::string(":azade CAP ") + nick + " LS :";

  std::cout << "CAP reply is: " << msg << std::endl;
  this->write_reply(msg);
}

void IrcConnection::command_join(Params params) { (void)params; }

// ------------------
//    Nick command
void IrcConnection::command_nick(Params params) {
  // get nick
  {
   std::lock_guard<std::mutex> lock(this->users->mtx);
   auto user = this->users->users_map[this->user_id].get();
   user->set_nick(params[0]);
   std::cout << "User nick is: " << user->get_nick() << std::endl;
  }
}

bool IrcConnection::user_exists() {
  if(this->users->users_map.find(this->user_id)
     != this->users->users_map.end())
      return true;
  else
   return false;
}

// ------------------
//   User command
void IrcConnection::command_user(Params params) {
  if (params.size() < 4) {
    write_reply("461 USER :Not enough parameters");
    return;
  }

  //if (this->user_exists()) {
  //  write_reply("433 USER :User already registered");
  //  return;
  //}
  auto user = this->get_user();
  auto nick = user->get_nick();

  write_reply("001 " + nick + " :Welcome to the Azade IRC Server");
  write_reply("002 " + nick + " :Your host is azade, running version 0.1");
  write_reply("003 " + nick + " :This server was created today");
  write_reply("004 " + nick + " azade 0.1 o o");
}
