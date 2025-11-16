#include "irc_server.h"

// -----------------------
//      Constructors
// -----------------------

IrcServer::IrcServer() { this->init_ssl(); }

void IrcServer::start() {
  // listen to ports
  this->tls_socket = setup_socket(this->tls_port);
  this->plain_socket = setup_socket(this->plain_port);

  std::thread([this] { this->accept_client(this->tls_socket, true); }).detach();
  std::thread([this] {
    this->accept_client(this->plain_socket, false);
  }).detach();

  while (true)
    std::this_thread::sleep_for(std::chrono::hours(1));
}

int IrcServer::setup_socket(int port) {
  // socket creation
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  // specify address
  sockaddr_in addr{};
  srv_address.sin_family = AF_INET;
  srv_address.sin_port = htons(port);
  srv_address.sin_addr.s_addr = INADDR_ANY;

  int val = bind(sock, (struct sockaddr *)&srv_address, sizeof(addr));

  if (val == -1) {
    // do some proper handling error on this
    std::cerr << "Error binding the port" << std::endl;
    exit(1);
  }

  listen(sock, SOMAXCONN);

  return sock;
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

// -----------------------------
//  Accept incoming connections
// -----------------------------

void IrcServer::accept_client(int sock, bool use_tls) {

  int client_fd = accept(sock, nullptr, nullptr);
  // ------------------------------------
  // Create connection object and thread
  // ------------------------------------

  if (use_tls) {
    auto client = std::shared_ptr<IrcConnection>(
        new IrcConnection(client_fd, this->ssl_ctx));

    {
      std::lock_guard<std::mutex> lock(conn_mutex);
      this->connections.insert({client_fd, client});
    }

    std::thread([this, client_fd, client] {
      if (client->handshake_successful()) {
        std::string welcome = "WELCOME TO AZADE SERVER\n";
        SSL_write(client->get_ssl(), welcome.c_str(), welcome.size());
      }

      client->work_loop();
      {
        std::lock_guard<std::mutex> lock(conn_mutex);
        this->connections.erase(client_fd);
      }
    }).detach();
    // ------------------
    // common connection
    // ------------------
  } else {
    auto client = std::shared_ptr<IrcConnection>(new IrcConnection(client_fd));

    {
      std::lock_guard<std::mutex> lock(conn_mutex);
      this->connections.insert({client_fd, client});
    }

    std::thread([this, client_fd, client] {
      std::string welcome = "WELCOME TO AZADE SERVER\n";
      write(client_fd, welcome.c_str(), welcome.size());

      client->work_loop();
      {
        std::lock_guard<std::mutex> lock(conn_mutex);
        this->connections.erase(client_fd);
      }
    }).detach();
  }
}
