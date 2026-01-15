#pragma once

#include <bits/stdc++.h>
#include <string>
#include <vector>

#define Params std::vector<std::string>

enum CmdType {
  CAP,
  JOIN,
  NICK,
  USER,
  PING,
  MODE,
  LIST,
  QUIT,
  TOPIC,
  PRIVMSG,
};

struct Command {
  CmdType cmd;
  Params params;

  Command(CmdType command, Params parameters)
      : cmd(command), params(parameters) {}
};

Command parse_command(const std::string cmd);
