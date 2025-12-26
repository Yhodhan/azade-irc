#include "irc_server.h"
#include <sys/epoll.h>

// -----------------------
//      Constructors
// -----------------------

IrcServer::IrcServer() {
  //this->init_ssl();
}

// -----------------------
//       Destructor
// -----------------------
IrcServer::~IrcServer() {
  //close(this->tls_socket);
  close(this->sockfd);
}

// -----------------------
//     SSL encryption
// -----------------------
void IrcServer::init_ssl() {
  SSL_library_init();

  ERR_load_crypto_strings();
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();

  this->ssl_ctx = SSL_CTX_new(TLS_server_method());
  if (!ssl_ctx) {
    ERR_print_errors_fp(stderr);
    exit(1);
  }

  if (!SSL_CTX_use_certificate_file(ssl_ctx, "server.crt", SSL_FILETYPE_PEM) ||
      !SSL_CTX_use_PrivateKey_file(ssl_ctx, "server.key", SSL_FILETYPE_PEM) ||
      !SSL_CTX_check_private_key(ssl_ctx)) {
    ERR_print_errors_fp(stderr);
    exit(1);
  }

  this->ssl = SSL_new(this->ssl_ctx);
}

// -----------------------
//        Work loop 
// -----------------------
void IrcServer::start(void) {
  try {
    struct epoll_event events[MAX_EVENTS];
    struct epoll_event *events_ptr;

    // create the sockets of the server
    this->sockfd = setup_socket(this->port);
    //this->tls_socket = setup_socket(this->tls_port);
    //SSL_set_fd(ssl, this->tls_socket);

    // create event poll
    this->setup_poll();
    int num_events;

    while (true) {
      events_ptr = &(events[0]);
      num_events = this->poll_wait(&events_ptr);
      for (int i = 0; i < num_events; i++) {        
        // its a connection
        if (events[i].data.fd == this->sockfd) 
          accept_client(this->sockfd, false);
        else 
          handle_msg(&events[i]);
      }
    }
  }
  // Killer exceptions
  catch (IrcServer::socketException &e){ print_error(e.what(), true); return; }
  catch (IrcServer::bindException &e)  { print_error(e.what(), true); return; }
  catch (IrcServer::AcceptException &e){ print_error(e.what(), true); return; }
  catch (IrcServer::pollException &e)  { print_error(e.what(), true); return; }
  catch (IrcServer::pollAddException &e){ print_error(e.what(), true); return; }
  catch (IrcServer::pollWaitException &e){ print_error(e.what(), true); return; }
  catch (IrcServer::readFdError &e){ print_error(e.what(), true); return; }
}

void IrcServer::handle_msg(struct epoll_event *event) {
  int fd = event->data.fd;
  char buffer[BUF_SIZE] = {0};
  std::string& cmd = this->cmdBuffers[fd];

  while (true) {
    std::cout << "read message" << std::endl;
    ssize_t bytes = this->read_msg(fd, buffer, sizeof(buffer));

    if (bytes > 0) {
      cmd.append(buffer, bytes);

      size_t pos;
      while ((pos = cmd.find("\r\n")) != std::string::npos) {
        std::string msg = cmd.substr(0, pos);

        cmd.erase(0, pos + 2);
        std::cout << "Command to execute: " << msg << std::endl;

        this->handle_command(fd, msg);
      }
    }
    else if (bytes == 0) {
      close(fd); 
      epoll_ctl(this->epollfd, EPOLL_CTL_DEL, fd, nullptr);
      this->users.erase(fd);
      this->cmdBuffers.erase(fd);
    }
    else {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
          return; // normal epoll behavior
      throw IrcServer::readFdError();
    }

  }
 }

ssize_t IrcServer::read_msg(int fd, char *buffer, size_t size) {
  return recv(fd, buffer, size, 0);
}

void IrcServer::write_reply(int fd, std::string reply) {
  reply += "\r\n";
  write(fd, reply.c_str(), reply.size());
}

// ---------------------------------
//        Create the socket
// ---------------------------------
int IrcServer::setup_socket(int port) {
  // socket creation
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) 
    throw IrcServer::socketException();

  if (fcntl(sock_fd, F_SETFL, O_NONBLOCK) == -1) 
    throw IrcServer::socketException();

  // specify address
  sockaddr_in addr{};
  srv_address.sin_family = AF_INET;
  srv_address.sin_port = htons(port);
  srv_address.sin_addr.s_addr = INADDR_ANY;

  if (bind(sock_fd, (struct sockaddr *)&srv_address, \
  sizeof(addr)) == -1) 
    throw IrcServer::bindException();

  if (listen(sock_fd, SOMAXCONN) == -1)
    throw IrcServer::bindException();
  
  return sock_fd;  
}

// ---------------------------------
//     Create the event poll queue 
// ---------------------------------
void IrcServer::setup_poll(void) {
  // create a epoll for handling events
  struct epoll_event ev;
  this->epollfd = epoll_create1(0);

  if (this->epollfd == -1) 
    throw IrcServer::pollException();

  // add the fd to the polling events
  int ectlfd;
  ev.events = EPOLLIN;
  ev.data.fd = this->sockfd; 
  ectlfd = epoll_ctl(this->epollfd, EPOLL_CTL_ADD, this->sockfd, &ev); 

  if (ectlfd == -1) 
    throw IrcServer::pollAddException();

  //ev.data.fd = this->tls_socket; 
  //ectlfd = epoll_ctl(this->epollfd, EPOLL_CTL_ADD, this->tls_socket, &ev); 
//
  //if (ectlfd == -1) 
  //  throw IrcServer::pollAddException();
}

// ---------------------------------
//       Set poll queue event 
// ---------------------------------
int IrcServer::poll_wait(struct epoll_event **events) {
  int num_events = epoll_wait(this->epollfd, *events, MAX_EVENTS, MAX_TIMEOUT);
  if (num_events == -1) 
    throw IrcServer::pollWaitException();
  return num_events;
}

// -----------------------
//       Error Printer 
// -----------------------
void IrcServer::print_error(const std::string msg, bool with_errno) {
  std::cout << msg;
  if (with_errno)
    std::cout << strerror(errno);
  std::cout << std::endl;
}

// -----------------------------
//  Accept incoming connections
// -----------------------------

// ------------------------------------
//          Accept client  
// ------------------------------------
void IrcServer::accept_client(int sock, bool use_tls) {
  std::cout << "accept client" << std::endl;
  struct epoll_event ev;

  int user_fd = accept(sock, nullptr, nullptr);
  if (user_fd < 0) 
    throw IrcServer::AcceptException();

  if (fcntl(user_fd, F_SETFL, O_NONBLOCK) == -1) 
    throw IrcServer::socketException();

  // store user 
  this->users[user_fd] = new User(user_fd);

  // -----------------------
  //       Add to epoll 
  // -----------------------
  memset(&ev, 0, sizeof(struct epoll_event));
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = user_fd;

  if (epoll_ctl(this->epollfd, EPOLL_CTL_ADD, user_fd, &ev) == -1)
    throw IrcServer::pollAddException();
}

// ------------------
//       Getters
// ------------------
User* IrcServer::get_user(int fd) { 
  return this->users[fd];
}

// -----------------------
//        Commands 
// -----------------------

void IrcServer::handle_command(int fd, std::string command) {
  std::cout << "Inside handle command: " << command << std::endl;

  Command cmd = parse_command(command);
  switch (cmd.cmd) {
  case CAP:
    command_cap(fd, cmd.params);
    break;
  case JOIN:
    command_join(fd, cmd.params);
    break;
  case NICK:
    command_nick(fd, cmd.params);
    break;
  case USER:
    command_user(fd, cmd.params);
    break;
  case PING:
    command_ping(fd, cmd.params);
    break;
  case MODE:
    command_mode(fd, cmd.params);
    break;
  case QUIT:
    command_quit(fd, cmd.params);
  break;
  default:
    this->write_reply(fd, std::string("INVALID command"));
  }
}

// ------------------
//    Cap command
// ------------------

void IrcServer::command_cap(int fd, Params params) {
  std::cout << "handle cap command" << std::endl;
  std::string msg;
  auto user = this->get_user(fd);
  auto nick = user->get_nick();

  if (nick == "")
    msg = std::string(":azade CAP * LS :");
  else 
    msg = std::string(":azade CAP ") + nick + " LS :";

  std::cout << "Reply cap command: " << std::endl;
  this->write_reply(fd, msg);
}

// ------------------
//    Nick command
// ------------------

void IrcServer::command_nick(int fd, Params params) {
   auto user = this->users[fd];
   user->set_nick(params[0]);
   std::cout << "User nick is: " << user->get_nick() << std::endl;
}

// ------------------
//    User command
// ------------------

void IrcServer::command_user(int fd, Params params) {
  if (params.size() < 4) {
    write_reply(fd, "461 USER :Not enough parameters");
    return;
  }

//  if (this->user_exists()) {
//    write_reply("433 USER :User already registered");
//    return;
//  }

  auto user = this->get_user(fd);
  auto nick = user->get_nick();

  write_reply(fd, "001 " + nick + " :Welcome to the Azade IRC Server");
  write_reply(fd, "002 " + nick + " :Your host is azade, running version 0.1");
  write_reply(fd, "003 " + nick + " :This server was created today");
  write_reply(fd, "004 " + nick + " azade 0.1 o o");
}

// ------------------
//   Mode command
// ------------------

void IrcServer::command_ping(int fd, Params params) {
  if (params.empty()) {
    auto nick = this->get_user(fd)->get_nick();
    write_reply(fd, "409 " + nick + " :No origin specified");
  }

  write_reply(fd, "PONG " + params[0]);
}

// ------------------
//    Join command
// ------------------

void IrcServer::command_join(int fd, Params params) {

}

// ------------------
//   Mode command
// ------------------

UserMode char_to_flag(char flag) {
  switch (flag) {
    case 'w': return MODE_WALLOPS; 
    case 'o': return MODE_OPERATOR; 
    case 'i': return MODE_INVISIBLE; 
    case 'r': return MODE_RESTRICTED;
    default : return UNKNOWN;
  }
}

void IrcServer::command_mode(int fd, Params params) { 
  auto user = this->get_user(fd);
  auto nick = user->get_nick();

  if (params.size() < 2) {
    write_reply(fd, "461 " + nick + ":Not enough parameters");
    return;
  }

  if (params[0] != nick) {
    write_reply(fd, "502 " + nick + ":User do not match");
    return;
  }

  auto modes = params[1];
  bool enable;

  for (char c : modes) {
    if (c == '+') { enable = true; continue;} 
    if (c == '-') { enable = false; continue;} 
  
    auto mode = char_to_flag(c);
    if (mode == UNKNOWN) {
      write_reply(fd, "501" + nick + ":Not a valid mode");
      return;
    }

    user->change_mode(mode, enable);
  }
}

// ------------------
//    Quit Command 
// ------------------
void IrcServer::command_quit(int fd, Params params) {	
  //this->client_quit = true;  
  close(fd); 
  epoll_ctl(this->epollfd, EPOLL_CTL_DEL, fd, nullptr);
  this->users.erase(fd);
  this->cmdBuffers.erase(fd);

  if (params.size() == 0)
    return;
  // else broadcast the message
}

/* --------------------------------*/
/*          Exceptions             */
/* --------------------------------*/

const char	*IrcServer::socketException::what() const throw()
{ return ("Socket creation or mode error: "); }

const char	*IrcServer::bindException::what() const throw()
{ return ("Bind error: "); }

const char	*IrcServer::AcceptException::what() const throw()
{ return ("Accept error: "); }

const char	*IrcServer::pollException::what() const throw()
{ return ("Poll error: "); }

const char	*IrcServer::pollAddException::what() const throw()
{ return ("Poll add error: "); }

const char	*IrcServer::pollWaitException::what() const throw()
{ return ("Poll wait error: "); }

const char	*IrcServer::readFdError::what() const throw()
{ return ("Read fd error: "); }