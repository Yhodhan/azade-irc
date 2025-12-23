#include "irc_server.h"

// -----------------------
//      Constructors
// -----------------------

IrcServer::IrcServer() {
  this->init_ssl();
  this->conns = std::make_shared<Connections>();
  this->users = std::make_shared<Users>();
}

void IrcServer::start(void) {
  // listen to ports
  try {
    // create the sockets of the server
    this->tls_socket = setup_socket(this->tls_port);
    this->plain_socket = setup_socket(this->plain_port);

    // create event poll
    this->setup_poll();

    int num_events;

    // cycle the fds to handle events
    while (true) {
      num_events = this->poll_wait();
      for (int i = 0; i < num_events; i++) {        
        // its a connection
        if (this->events[i].data.fd == this->plain_socket) 
          accept_client(this->plain_socket, false);

        if (this->events[i].data.fd == this->tls_socket) 
          accept_client(this->tls_socket, true);

        else 
          handle_connection(&this->events[i]);
      }
    }
  }

  // Killer exceptions
  catch (IrcServer::socketException &e){ print_error(e.what(), true); return; }
  catch (IrcServer::bindException &e){ print_error(e.what(), true); return; }
  catch (IrcServer::pollException &e){ print_error(e.what(), true); return; }
  catch (IrcServer::pollWaitException &e){ print_error(e.what(), true); return; }
}

void IrcServer::handle_connection(struct epoll_event *event) {

}

int IrcServer::setup_socket(int port) {
  // socket creation
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) 
    throw IrcServer::socketException();

  // specify address
  sockaddr_in addr{};
  srv_address.sin_family = AF_INET;
  srv_address.sin_port = htons(port);
  srv_address.sin_addr.s_addr = INADDR_ANY;

  if (bind(sockfd, (struct sockaddr *)&srv_address, \
  sizeof(addr)) == -1) 
    throw IrcServer::bindException();

  if (listen(sockfd, SOMAXCONN) == -1)
    throw IrcServer::bindException();

  return sockfd;  
}

void IrcServer::setup_poll(void) {
  // create a epoll for handling events
  this->epoll = epoll_create1(0);

  if (epoll == -1) 
    throw IrcServer::pollException();

  // add the fd to the polling events
  int ectlfd;
  event.events = EPOLLIN;
  event.data.fd = this->plain_socket; 
  ectlfd = epoll_ctl(epoll, EPOLL_CTL_ADD, this->plain_socket, &event); 

  if (ectlfd == -1) 
    throw IrcServer::pollException();

  event.data.fd = this->tls_socket; 
  ectlfd = epoll_ctl(epoll, EPOLL_CTL_ADD, this->tls_socket, &event); 

  if (ectlfd == -1) 
    throw IrcServer::pollException();
}

int IrcServer::poll_wait(void) {
  int num_events = epoll_wait(epoll, events, MAX_EVENTS, -1);
  if (num_events == -1) 
    throw IrcServer::pollWaitException();
  return num_events;
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
}

// -----------------------
//       Destructor
// -----------------------

IrcServer::~IrcServer() {
  close(this->tls_socket);
  close(this->plain_socket);
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
void welcome_msg(std::shared_ptr<IrcConnection> client, int fd) {
  std::string welcome = "WELCOME TO AZADE SERVER\n";
  if (client->get_is_tls()) {
    if (client->handshake_successful()) {
      SSL_write(client->get_ssl(), welcome.c_str(), welcome.size());
    }
  } else {
    write(fd, welcome.c_str(), welcome.size());
  }
}

void IrcServer::accept_client(int sock, bool use_tls) {

  std::cout << "enter accept client" << std::endl;
  int client_fd = accept(sock, nullptr, nullptr);
  // ------------------------------------
  // Create connection object and thread
  // ------------------------------------
  if (client_fd < 0) {
    perror("Accept");
    return;
  }

  std::cout << "Connection Accepted\n";

  std::shared_ptr<IrcConnection> client;
  std::shared_ptr<User> user = std::make_shared<User>();

  if (use_tls)
    client = std::make_shared<IrcConnection>(
        client_fd, user->get_id(), this->ssl_ctx, this->conns, this->users);
  else
    client = std::make_shared<IrcConnection>(client_fd, user->get_id(),
                                             this->conns, this->users);

  // -----------------------
  //    Store Connections
  // -----------------------
  {
    std::lock_guard<std::mutex> lock(conns->mtx);
    this->conns->connections.insert({client_fd, client});
  }

  // -----------------------
  //       Store Users
  // -----------------------
  {
    std::lock_guard<std::mutex> lock(users->mtx);
    this->users->users_map.insert({user->get_id(), std::move(user)});
  }

  // -----------------------
  //  Create working thread
  // -----------------------
  std::thread([conns = this->conns, users = this->users, client_fd, client] {
    //welcome_msg(client, client_fd);

    client->work_loop();
    {
      std::lock_guard<std::mutex> lock(users->mtx);
      users->users_map.erase(client->get_user_id());
    }
    {
      std::lock_guard<std::mutex> lock(conns->mtx);
      conns->connections.erase(client_fd);
    }
  }).detach();
}

/* --------------------------------*/
/*          Exceptions             */
/* --------------------------------*/

const char	*IrcServer::socketException::what() const throw()
{ return ("Socket creation or mode error: "); }

const char	*IrcServer::bindException::what() const throw()
{ return ("Bind error: "); }

const char	*IrcServer::pollException::what() const throw()
{ return ("Poll error: "); }

const char	*IrcServer::pollWaitException::what() const throw()
{ return ("Poll wail error: "); }