#include "server/irc_server.h"
#include <memory>

int main(void) {
  std::unique_ptr<IrcServer> server =
      std::unique_ptr<IrcServer>(new IrcServer());

  return 0;
}
