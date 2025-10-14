#include "../connections/connection.h"
#include <arpa/inet.h>
#include <cstring>
#include <thread>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

class IrcServer {
public:
  IrcServer();
  ~IrcServer();

  void accept_client();

  void event_loop();

private:
  int irc_socket;
  sockaddr_in srv_address;
  int port = 6667;

  // connections data structures
  std::unordered_map<int, std::unique_ptr<IrcConnection>> connections;
};
