#include "irc_server.h"
#include <openssl/err.h>

IrcServer::IrcServer() {
  // socket creation
  this->irc_socket = socket(AF_INET, SOCK_STREAM, 0);

  // specify address
  sockaddr_in addr{};
  srv_address.sin_family = AF_INET;
  srv_address.sin_port = htons(this->port);
  srv_address.sin_addr.s_addr = INADDR_ANY;

  int val =
      bind(this->irc_socket, (struct sockaddr *)&srv_address, sizeof(addr));

  if (val == -1) {
    std::cerr << "Error binding the port" << std::endl;
    return;
  }

  listen(this->irc_socket, SOMAXCONN);

  this->init_ssl();
}

// -------  SSL encryption

void IrcServer::init_ssl() {
  SSL_library_init();
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

// -------  Welcoming message

void IrcServer::welcome_message(int client_fd) {
  std::string welcome = "WELCOME TO AZADE SERVER\n";

  send(client_fd, welcome.c_str(), welcome.size(), 0);
}

IrcServer::~IrcServer() { close(this->irc_socket); }

// ------- Accept incoming connections

void IrcServer::accept_client() {
  int client_fd = accept(this->irc_socket, nullptr, nullptr);

  std::cout << "=== TSL handshake done" << std::endl;
  // ------------------------------------
  // Create connection object and thread
  // ------------------------------------

  this->welcome_message(client_fd);

  auto client = std::shared_ptr<IrcConnection>(
      new IrcConnection(client_fd, this->ssl_ctx));

  this->connections.insert({client_fd, client});

  std::thread([this, client_fd, client] {
    client->work_loop();
    this->connections.erase(client_fd);
    std::cout << "close connection to client" << std::endl;
  }).detach();
}

// ------- Event Loop

void IrcServer::event_loop() {
  while (true) {
    this->accept_client();
  }
}
