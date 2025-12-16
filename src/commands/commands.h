#pragma once

#include <bits/stdc++.h>
#include <string>
#include <vector>

#define Params std::vector<std::string>

enum COMMAND {
  CAP,
  JOIN,
  NICK,
  USER,
  PING,
  MODE,
  QUIT,
  INVALID,
};

struct Command {
  COMMAND cmd;
  Params params;

  Command(COMMAND command, Params parameters)
      : cmd(command), params(parameters) {}
};

Command parse_command(const std::string cmd);
