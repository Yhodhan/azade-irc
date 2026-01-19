#include "commands.h"

/* -------------------------------------------------- */
/*                 Split command                      */
/* -------------------------------------------------- */

Params split_command(const std::string cmd) {
  std::string s;
  std::stringstream ss(cmd);
  Params tokens;

  while (getline(ss, s, ' '))
    tokens.push_back(s);

  return tokens;
}

/* -------------------------------------------------- */
/*                  Get command                       */
/* -------------------------------------------------- */

Command get_command(Params command) {
  std::string cmd = command[0];
  command.erase(command.begin());

  if (cmd == "CAP")
    return Command(CAP, command);
  else if (cmd == "JOIN")
    return Command(JOIN, command);
  else if (cmd == "NICK")
    return Command(NICK, command);
  else if (cmd == "USER")
    return Command(USER, command);
  else if (cmd == "MODE")
    return Command(MODE, command);
  else if (cmd == "LIST")
    return Command(LIST, command);
  else if (cmd == "QUIT")
    return Command(QUIT, command);
  else if (cmd == "TOPIC")
    return Command(TOPIC, command);
  else if (cmd == "PRIVMSG")
    return Command(PRIVMSG, command);
  else
    return Command(PING, command);
}

/* -------------------------------------------------- */
/*                 Parse command                      */
/* -------------------------------------------------- */

Command parse_command(const std::string cmd) {
  auto split_cmd = split_command(cmd);
  return get_command(split_cmd);
}
