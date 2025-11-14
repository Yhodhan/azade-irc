#pragma once

#include <bits/stdc++.h>
#include <string>
#include <vector>

enum COMMAND {
  CAP,
  JOIN,
  INVALID,
};

struct Command {
  COMMAND cmd;
  std::vector<std::string> params;

  Command(COMMAND command, std::vector<std::string> parameters)
      : cmd(command), params(parameters) {}
};

Command parse_command(const std::string cmd);
