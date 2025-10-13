#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

class IrcServer {
public:
  IrcServer();
  ~IrcServer();

private:
  int irc_socket;
  sockaddr_in srv_address;
  int port = 6667;

  // connections data structures
  std::unordered_map<int, int> conexions;
};
