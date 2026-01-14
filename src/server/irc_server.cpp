#include "irc_server.h"

/* --------------------------------*/
/*          Constructor            */
/* --------------------------------*/
IrcServer *IrcServer::instance = nullptr;
IrcServer::IrcServer() : running(true) {
  // this- >init_ssl();
}

/* --------------------------------*/
/*          Destructor             */
/* --------------------------------*/

IrcServer::~IrcServer() {
  epoll_ctl(this->epollfd, EPOLL_CTL_DEL, this->sockfd, nullptr);

  /* Delete all allocated users */
  for (auto &[fd, user] : this->users) {
    close(fd);
    epoll_ctl(this->epollfd, EPOLL_CTL_DEL, fd, nullptr);
    delete user;
  }

  /* Delete all allocated channels */
  for (auto &[key, channel] : this->channels)
    delete channel;
 
  close(this->sockfd);
  close(this->epollfd);
}

/* --------------------------------*/
/*        SSL encryption           */
/* --------------------------------*/

void IrcServer::init_ssl() {
  SSL_library_init();

  ERR_load_crypto_strings();
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();

  if (!SSL_CTX_use_certificate_file(ssl_ctx, "server.crt", SSL_FILETYPE_PEM) ||
      !SSL_CTX_use_PrivateKey_file(ssl_ctx, "server.key", SSL_FILETYPE_PEM) ||
      !SSL_CTX_check_private_key(ssl_ctx)) {
    ERR_print_errors_fp(stderr);
    exit(1);
  }

  this->ssl = SSL_new(this->ssl_ctx);
}

/* --------------------------------*/
/*        Signal management        */
/* --------------------------------*/

void IrcServer::on_sigint(int) {
  if (instance)
    instance->running = false;
}

void IrcServer::install_signal_handler(IrcServer *server) {
  IrcServer::instance = server;
  signal(SIGINT, server->on_sigint);
}

/* --------------------------------*/
/*      Read/Write operations      */
/* --------------------------------*/

void IrcServer::send_numeric(int fd, IrcNumeric code, const std::string &target,
                             const std::string &msg) {
  std::ostringstream oss;
  oss << ":" << "azade" << " " << std::setw(3) << std::setfill('0') << code
      << " " << target << " :" << msg;

  write_reply(fd, oss.str());
}

void IrcServer::write_reply(int fd, std::string reply) {
  reply += "\r\n";
  write(fd, reply.c_str(), reply.size());
}

ssize_t IrcServer::read_msg(int fd, char *buffer, size_t size) {
  return recv(fd, buffer, size, 0);
}

/* --------------------------------*/
/*            Getters              */
/* --------------------------------*/

User *IrcServer::get_user(int fd) { return this->users[fd]; }
User *IrcServer::get_user_by_id(uint32_t id) { return this->users_id[id]; }

Channel *IrcServer::get_channel(const std::string &ch_name) {
  auto name = this->channel_name(ch_name);
  return this->channels[ch_name];
}

/* --------------------------------*/
/*           Start server          */
/* --------------------------------*/

void IrcServer::start(void) {
  try {
    /* create the sockets of the server */
    this->sockfd = setup_socket(this->port);
    /* create event poll */
    this->setup_poll();
  }
  /* Killer exceptions */
 catch (const IrcServer::bindException &e)    { print_error(e.what(), true); return; }
 catch (const IrcServer::pollException &e)    { print_error(e.what(), true); return; }
 catch (const IrcServer::socketException &e)  { print_error(e.what(), true); return; }
 catch (const IrcServer::pollAddException &e) { print_error(e.what(), true); return; }
}

/* --------------------------------*/
/*         Shutdown server         */
/* --------------------------------*/

void IrcServer::shutdown(void) {
  // Broadcast termination signal
  for (auto &[fd, user] : this->users) {
    write_reply(fd, "ERROR :Server shutting down");
  }
}

/* --------------------------------*/
/*           Work loop             */
/* --------------------------------*/
void IrcServer::event_loop(void) {
  try {
    struct epoll_event events[MAX_EVENTS];
    struct epoll_event *events_ptr;

    int num_events;

    while (running) {
      events_ptr = &(events[0]);
      num_events = this->poll_wait(&events_ptr);
      for (int i = 0; i < num_events; i++) {
        /* its a connection */
        if (events[i].data.fd == this->sockfd)
          accept_client(this->sockfd, false);
        else
          handle_msg(&events[i]);
      }
    }
  }
  /* Killer exceptions */
  catch (const IrcServer::readFdError &e)       {print_error(e.what(), true); return;}
  catch (const IrcServer::AcceptException &e)   {print_error(e.what(), true); return;}
  catch (const IrcServer::pollWaitException &e) {print_error(e.what(), true); return;}
}

/* --------------------------------*/
/*          Handle MSG             */
/* --------------------------------*/

void IrcServer::handle_msg(struct epoll_event *event) {
  int fd = event->data.fd;
  char buffer[BUF_SIZE] = {0};
  std::string &cmd = this->cmdBuffers[fd];

  ssize_t bytes = this->read_msg(fd, buffer, sizeof(buffer));

  if (bytes < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) return;
    throw IrcServer::readFdError();
  }

  else if (bytes == 0) {
    this->close_user(fd);
    return;
  }

  cmd.append(buffer, bytes);
  size_t pos;
  while ((pos = cmd.find("\r\n")) != std::string::npos) {
    std::string msg = cmd.substr(0, pos);
    cmd.erase(0, pos + 2);

    this->handle_command(fd, msg);
  }
}

/* --------------------------------*/
/*        Create the socket        */
/* --------------------------------*/

int IrcServer::setup_socket(int port) {
  /* Socket creation */
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1)
    throw IrcServer::socketException();

  if (fcntl(sock_fd, F_SETFL, O_NONBLOCK) == -1)
    throw IrcServer::socketException();

  /* Specify address */
  sockaddr_in addr{};
  srv_address.sin_family = AF_INET;
  srv_address.sin_port = htons(port);
  srv_address.sin_addr.s_addr = INADDR_ANY;

  if (bind(sock_fd, (struct sockaddr *)&srv_address, sizeof(addr)) == -1)
    throw IrcServer::bindException();

  if (listen(sock_fd, SOMAXCONN) == -1)
    throw IrcServer::bindException();

  return sock_fd;
}

/* --------------------------------*/
/*     Create event poll event     */
/* --------------------------------*/

void IrcServer::setup_poll(void) {
  struct epoll_event ev;
  this->epollfd = epoll_create1(0);

  if (this->epollfd == -1)
    throw IrcServer::pollException();

  /* Add the fd to the polling events */
  int ectlfd;
  ev.events = EPOLLIN;
  ev.data.fd = this->sockfd;
  ectlfd = epoll_ctl(this->epollfd, EPOLL_CTL_ADD, this->sockfd, &ev);

  if (ectlfd == -1)
    throw IrcServer::pollAddException();
}

/* --------------------------------*/
/*      Set poll queue event       */
/* --------------------------------*/

int IrcServer::poll_wait(struct epoll_event **events) {
  int num_events = epoll_wait(this->epollfd, *events, MAX_EVENTS, MAX_TIMEOUT);

  if (num_events == -1)
    throw IrcServer::pollWaitException();
  return num_events;
}

/* --------------------------------*/
/*          Accept client          */
/* --------------------------------*/

void IrcServer::accept_client(int sock, bool use_tls) {
  struct epoll_event ev;

  int user_fd = accept(sock, nullptr, nullptr);

  if (user_fd < 0)
    throw IrcServer::AcceptException();

  if (fcntl(user_fd, F_SETFL, O_NONBLOCK) == -1)
    throw IrcServer::socketException();

  auto user = new User(user_fd);
  this->users[user_fd] = user;
  this->users_id[user->get_id()] = user; // check this is properly release

  /* ----------------------- */
  /*       Add to epoll      */
  /* ----------------------- */
  memset(&ev, 0, sizeof(struct epoll_event));
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = user_fd;

  if (epoll_ctl(this->epollfd, EPOLL_CTL_ADD, user_fd, &ev) == -1)
    throw IrcServer::pollAddException();
}

/* --------------------------------*/
/*        Close user socket        */
/* --------------------------------*/

void IrcServer::close_user(int fd) {
  close(fd);
  epoll_ctl(this->epollfd, EPOLL_CTL_DEL, fd, nullptr);
  this->users.erase(fd);
  this->cmdBuffers.erase(fd);
}

/* --------------------------------*/
/*           Command               */
/* --------------------------------*/

void IrcServer::handle_command(int fd, std::string command) {
  Command cmd = parse_command(command);
  switch (cmd.cmd) {
  case CAP:
    command_cap(fd, cmd.params); break;
  case JOIN:
    command_join(fd, cmd.params); break;
  case NICK:
    command_nick(fd, cmd.params); break;
  case USER:
    command_user(fd, cmd.params); break;
  case PING:
    command_ping(fd, cmd.params); break;
  case MODE:
    command_mode(fd, cmd.params); break;
  case LIST:
    command_list(fd, cmd.params); break;
  case QUIT:
    command_quit(fd, cmd.params); break;
  case TOPIC:
    command_topic(fd, cmd.params); break;
  case PRIVMSG:
    command_privmsg(fd, cmd.params); break;
  default:
    this->write_reply(fd, std::string("INVALID command"));
  }
}

/* --------------------------------*/
/*          Cap Command            */
/* --------------------------------*/

void IrcServer::command_cap(int fd, Params &params) {
  std::string msg;
  auto user = this->get_user(fd);
  auto nick = user->get_nick();

  if (nick == "")
    msg = std::string(":azade CAP * LS :");
  else
    msg = std::string(":azade CAP ") + nick + " LS :";

  this->write_reply(fd, msg);
}

/* --------------------------------*/
/*          Nick Command           */
/* --------------------------------*/

void IrcServer::command_nick(int fd, Params &params) {
  auto user = this->users[fd];

  for (auto const &[key, val] : this->users) {
    if (val->get_nick() == params[0]) {
      send_numeric(fd, ERR_NICKCOLLISION, "NICK", "ERROR NICK ALREADY IN USE");
      return;
    }
  }

  user->set_nick(params[0]);
}

/* --------------------------------*/
/*          User Command           */
/* --------------------------------*/

bool IrcServer::user_exists(int fd, Params &params) {
  // if (this->users[fd]->get_fd() == fd) {
  //   write_reply(fd, "461 USER :User already registered");
  //   return true;
  // }
  // else if (this->users[fd]->get_nick() == params[1]) {
  //   write_reply(fd, "461 USER :Nick already registered");
  // }

  return false;
}

void IrcServer::command_user(int fd, Params &params) {
  auto user = this->get_user(fd);
  auto nick = user->get_nick();

  if (params.size() < 4) {
    send_numeric(fd, ERR_NEEDMOREPARAMS, nick, "Not enough parameters");
    return;
  }

  if (this->user_exists(fd, params))
    return;

  send_numeric(fd, RPL_WELCOME, nick, "Welcome to the Azade IRC Server");
  send_numeric(fd, RPL_YOURHOST, nick,
               "Your host is azade, running version 0.1");
  send_numeric(fd, RPL_CREATED, nick, "This server was created today");
  send_numeric(fd, RPL_MYINFO, nick, "azade 0.1 o o");
}

/* --------------------------------*/
/*          Ping Command           */
/* --------------------------------*/

void IrcServer::command_ping(int fd, Params &params) {
  if (params.empty()) {
    auto nick = this->get_user(fd)->get_nick();
    send_numeric(fd, ERR_NEEDMOREPARAMS, nick, "No origin specified");
    return;
  }

  write_reply(fd, "PONG " + params[0]);
}

/* --------------------------------*/
/*          Join Command           */
/* --------------------------------*/

std::string IrcServer::channel_name(std::string chl) {
  auto c = chl[0];
  std::string channel;

  if (c == '#' || c == '&')
    channel = chl.substr(1, chl.size());
  else
    channel = chl;

  return channel;
}

bool IrcServer::channel_exist(const std::string &name) {
  auto it = this->channels.find(name);
  return it == this->channels.end() ? false : true;
}

void IrcServer::command_join(int fd, Params &params) {
  auto user = this->get_user(fd);
  auto nick = user->get_nick();

  if (params.empty()) {
    send_numeric(fd, ERR_NEEDMOREPARAMS, nick, "Not enough parameters");
    return;
  }

  auto name = channel_name(params[0]);

  if (this->channels.empty() || !this->channel_exist(name))
    this->channels[name] = new Channel(name);
 
  auto channel = this->channels[name];
  channel->add_user(user->get_id());

  /* Send the topic */
  auto msg = "Topic: " + channel->get_topic();
  send_numeric(fd, RPL_TOPIC, nick, msg);
}

/* --------------------------------*/
/*          List Command           */
/* --------------------------------*/

void IrcServer::command_list(int fd, Params &params) {
  auto user = this->get_user(fd);
  auto nick = user->get_nick();

  send_numeric(fd, RPL_LISTSTART, nick, "Channel :Users Name");

  if (params.empty()) {
    for (auto const &[name, channel] : this->channels) {
      if (channel->is_private())
        continue;

      std::string msg = name + " " + std::to_string(channel->user_count()) +
                        " " + channel->get_topic();
      send_numeric(fd, RPL_LIST, nick, msg);
    }
  }

  else {
    for (auto ch_name : params) {
      auto name = channel_name(ch_name);

      if (!channel_exist(name))
        continue;

      auto channel = this->channels[name];
      std::string msg = name + " " + std::to_string(channel->user_count()) +
                        " " + channel->get_topic();
      send_numeric(fd, RPL_LIST, nick, msg);
    }
  }

  send_numeric(fd, RPL_LISTEND, nick, "End of /LIST");
}

/* --------------------------------*/
/*          Mode Command           */
/* --------------------------------*/

UserMode char_to_flag(char flag) {
  switch (flag) {
  case 'w': return MODE_WALLOPS;
  case 'o': return MODE_OPERATOR;
  case 'i': return MODE_INVISIBLE;
  case 'r': return MODE_RESTRICTED;
  default: return UNKNOWN;
  }
}

void IrcServer::command_mode(int fd, Params &params) {
  auto user = this->get_user(fd);
  auto nick = user->get_nick();

  if (params.size() < 2) {
    send_numeric(fd, ERR_NEEDMOREPARAMS, nick, "Not enough parameters");
    return;
  }

  if (params[0] != nick) {
    send_numeric(fd, ERR_USERSDONTMATCH, nick, "User do not match");
    return;
  }

  auto modes = params[1];
  bool enable;

  for (char c : modes) {
    if (c == '+') {enable = true; continue;}
    if (c == '-') {enable = false; continue;}

    auto mode = char_to_flag(c);
    if (mode == UNKNOWN) {
      send_numeric(fd, ERR_UNKNOWNMODEFLAG, nick, "Not a valid mode");
      return;
    }

    user->change_mode(mode, enable);
  }
}

/* --------------------------------*/
/*          Quit Command           */
/* --------------------------------*/

void IrcServer::command_quit(int fd, Params &params) {
  close(fd);
  epoll_ctl(this->epollfd, EPOLL_CTL_DEL, fd, nullptr);
  this->users.erase(fd);
  this->cmdBuffers.erase(fd);

  if (params.size() == 0)
    return;
  // else broadcast the message
}

/* --------------------------------*/
/*          Topic Command          */
/* --------------------------------*/

void IrcServer::command_topic(int fd, Params &params) {
  auto user = this->get_user(fd);
  auto nick = user->get_nick();

  if (params.empty())
    send_numeric(fd, ERR_NEEDMOREPARAMS, nick, "Not enough parameters");

  else if (params.size() == 1) {
    auto ch_name = channel_name(params[0]);

    if (!channel_exist(ch_name)) {
      send_numeric(fd, ERR_NOSUCHCHANNEL, nick, ch_name + " No such channel");
      return;
    }

    auto channel = this->channels[ch_name];

    if (!channel->has_user(user->get_id())) {
      send_numeric(fd, ERR_NOTONCHANNEL, nick, ch_name + "");
      return;
    }

    auto topic = this->channels[ch_name]->get_topic();

    if (topic.empty())
      send_numeric(fd, RPL_NOTOPIC, nick, ch_name + " " + topic);
    else
      send_numeric(fd, RPL_TOPIC, nick, ch_name + " " + topic);
  }

  else {
    auto ch_name = channel_name(params[0]);
    auto channel = this->channels[ch_name];
    auto new_topic = params[1];
    channel->set_topic(new_topic);
  }
}

/* --------------------------------*/
/*          PRIVMSG Command        */
/* --------------------------------*/

std::string build_msg(std::string nick, Channel *channel,
                      const std::string &msg) {
  auto irc_msg = ":" + nick + " PRIVMSG " + channel->get_name() + msg;

  return irc_msg;
}

void IrcServer::command_privmsg(int fd, Params &params) {
  auto user = this->get_user(fd);
  auto nick = user->get_nick();

  if (params.size() < 2) {
    send_numeric(fd, ERR_NOTEXTTOSEND, nick, "No message to send");
    return;
  }

  auto ch_name = channel_name(params[0]);
  if (!this->channel_exist(ch_name)) {
    send_numeric(fd, ERR_NOSUCHCHANNEL, nick, ch_name + " No such channel");
    return;
  }

  /* unified split messages */
  std::string msg = "";

  for (int i = 1; i < params.size(); i++)
    msg = msg + " " + params[i];
  
  auto channel = this->channels[ch_name];
  auto irc_msg = build_msg(user->get_nick(), channel, msg);

  broadcast(fd, channel, irc_msg);
}

/* --------------------------------*/
/*          Broadcast              */
/* --------------------------------*/

void IrcServer::broadcast(int from_fd, Channel *channel, const std::string &msg) {
  for (auto user_id : channel->get_users()) {
    auto user = this->get_user_by_id(user_id);
    auto fd = user->get_fd();

    if (from_fd == fd)
      continue;
    
    write_reply(fd, msg);
  }
}

/* --------------------------------*/
/*          Exceptions             */
/* --------------------------------*/

const char *IrcServer::socketException::what() const throw() {
  return ("Socket creation or mode error: ");
}

const char *IrcServer::bindException::what() const throw() {
  return ("Bind error: ");
}

const char *IrcServer::AcceptException::what() const throw() {
  return ("Accept error: ");
}

const char *IrcServer::pollException::what() const throw() {
  return ("Poll error: ");
}

const char *IrcServer::pollAddException::what() const throw() {
  return ("Poll add error: ");
}

const char *IrcServer::pollWaitException::what() const throw() {
  return ("Poll wait error: ");
}

const char *IrcServer::readFdError::what() const throw() {
  return ("Read fd error: ");
}

/* --------------------------------*/
/*          Error Printer          */
/* --------------------------------*/

void IrcServer::print_error(const std::string &msg, bool with_errno) {
  std::cout << msg;
  if (with_errno)
    std::cout << strerror(errno);
  std::cout << std::endl;
}
