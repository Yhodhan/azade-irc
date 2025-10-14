#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

class IrcConnection {

public:
  IrcConnection(int fd);
  ~IrcConnection();

  void work_loop();

private:
  int client_fd;
  std::string nick;
};
