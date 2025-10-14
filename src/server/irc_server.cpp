#include "irc_server.h"

IrcServer::IrcServer() {
  // socket creation
  this->irc_socket = socket(AF_INET, SOCK_STREAM, 0);

  // specify address
  srv_address.sin_family = AF_INET;
  srv_address.sin_port = htons(this->port);
  srv_address.sin_addr.s_addr = INADDR_ANY;

  int val = bind(this->irc_socket, (struct sockaddr *)&srv_address,
                 sizeof(srv_address));

  if (val == -1) {
    std::cerr << "Error binding the port" << std::endl;
    return;
  }

  listen(this->irc_socket, SOMAXCONN);
}

IrcServer::~IrcServer() { close(this->irc_socket); }

// ------- Accept incoming connections

void IrcServer::accept_client() {
  int fd;
  fd = accept(this->irc_socket, nullptr, nullptr);

  std::cout << "=== Accept client" << std::endl;

  auto client = std::unique_ptr<IrcConnection>(new IrcConnection(fd));
  IrcConnection *client_ptr = client.get();

  this->connections.insert({fd, std::move(client)});


  // launch running client
  std::thread([this, fd, client_ptr] {
    
    client_ptr->work_loop();
    this->connections.erase(fd);

    std::cout << "close connection to client" << std::endl;
  }).detach();
}

// ------- Event Loop

void IrcServer::event_loop() {
  while (true) {
    this->accept_client();
  }
}
