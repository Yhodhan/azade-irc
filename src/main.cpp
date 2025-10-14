#include "server/irc_server.h"
#include <memory>

int main(void) {
  std::unique_ptr<IrcServer> server =
      std::unique_ptr<IrcServer>(new IrcServer());

  std::cout << "=== INIT SERVER" << std::endl;

  server->event_loop();

  std::cout << "=== FINISH SERVER" << std::endl;
  
  return 0;
}
