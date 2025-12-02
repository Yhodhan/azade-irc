#include "commands.h"

// --------------------------------------------------------------

Params split_command(const std::string cmd) {
  // store token
  std::string s;

  std::stringstream ss(cmd);

  Params tokens;

  while (getline(ss, s, ' ')) {
    tokens.push_back(s);
  }

  return tokens;
}

// --------------------------------------------------------------

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
  else if (cmd == "PING")
    return Command(PING, command);
  else if (cmd == "MODE")
    return Command(MODE, command);
  else
    return Command(INVALID, Params({}));
}

Command parse_command(const std::string cmd) {
  auto split_cmd = split_command(cmd);
  return get_command(split_cmd);
}
