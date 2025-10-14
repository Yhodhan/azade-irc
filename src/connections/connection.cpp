#include "connection.h"
#include <sys/types.h>

IrcConnection::IrcConnection(int fd) : client_fd(fd) {}

IrcConnection::~IrcConnection() {}

void IrcConnection::work_loop() {
  std::cout << "Client connected" << std::endl;
  char buffer[1024] = {0};
  while (true) {

    ssize_t bytes = recv(this->client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes < 0) {
      perror("Recv");
      std::cerr << "=== Recv failed: " << errno << std::endl;
      break;
    } else if (bytes == 0) {
      std::cout << "Client Disconnected" << std::endl;
      break;
    }

    buffer[bytes] = 0;
    std::cout << "Message from client: " << buffer << std::endl;
  }
  close(this->client_fd);
}
