#include "server/irc_server.h"

int main(void) {  
  IrcServer *server = new IrcServer();
     
  std::cout << "=== INIT SERVER" << std::endl;
  server->install_signal_handler(server);

  server->start();
  server->event_loop();

  server->shutdown();
  std::cout << "=== FINISH SERVER" << std::endl;

  delete server;
  return 0;
}
