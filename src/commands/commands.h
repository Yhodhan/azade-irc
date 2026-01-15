#pragma once

#include "../numerics.h"
#include <bits/stdc++.h>
#include <string>
#include <vector>

#define Params std::vector<std::string>
class IrcServer;

enum COMMAND {
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
  INVALID,
};

struct Command {
  COMMAND cmd;
  Params params;

  Command(COMMAND command, Params parameters)
      : cmd(command), params(parameters) {}
};

std::string channel_name(std::string chl);
Command parse_command(const std::string cmd);

void command_cap(int fd, Params &params, IrcServer *server);
void command_nick(int fd, Params &params, IrcServer *server);
void command_user(int fd, Params &params, IrcServer *server);
void command_ping(int fd, Params &params, IrcServer *server);
void command_join(int fd, Params &params, IrcServer *server);
void command_list(int fd, Params &params, IrcServer *server);
void command_mode(int fd, Params &params, IrcServer *server);
void command_quit(int fd, Params &params, IrcServer *server);
void command_topic(int fd, Params &params, IrcServer *server);
void command_privmsg(int fd, Params &params, IrcServer *server);
