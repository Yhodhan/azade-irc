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
}

IrcServer::~IrcServer() { close(this->irc_socket); }
