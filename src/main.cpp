#include "server/irc_server.h"

int main(void) {
  std::unique_ptr<IrcServer> server =
      std::unique_ptr<IrcServer>(new IrcServer());

  std::cout << "=== INIT SERVER" << std::endl;

  server->start();

  std::cout << "=== FINISH SERVER" << std::endl;
  
  return 0;
}
