#include "irc_server.h"
#include <netinet/in.h>

IrcServer::IrcServer() {
  // socket creation
  this->irc_socket = socket(AF_INET, SOCK_STREAM, 0);

  // specify address

}

IrcServer::~IrcServer() { close(this->irc_socket); }
