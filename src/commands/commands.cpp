#include "commands.h"

// --------------------------------------------------------------

std::vector<std::string> split_command(const std::string cmd) {
  // store token
  std::string s;

  std::stringstream ss(cmd);

  std::vector<std::string> tokens;

  while (getline(ss, s, ' ')) {
    tokens.push_back(s);
  }

  return tokens;
}

// --------------------------------------------------------------

Command get_command(std::vector<std::string> command) {
  std::string cmd = command[0];

  if (cmd == "CAP") {
    command.erase(command.begin());
    return Command(CAP, command);
  } else if (cmd == "JOIN") {
    command.erase(command.begin());
    return Command(JOIN, command);
  } else {
    return Command(INVALID, std::vector<std::string>({}));
  }
}

Command parse_command(const std::string cmd) {
  auto split_cmd = split_command(cmd);
  return get_command(split_cmd);
}
