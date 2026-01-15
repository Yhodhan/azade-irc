#include "commands.h"
#include "../server/irc_server.h"

/* -------------------------------------------------- */
/*                 Split command                      */
/* -------------------------------------------------- */

Params split_command(const std::string cmd) {

  std::string s;

  std::stringstream ss(cmd);

  Params tokens;

  while (getline(ss, s, ' ')) {
    tokens.push_back(s);
  }

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
  else if (cmd == "PING")
    return Command(PING, command);
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
    return Command(INVALID, Params({}));
}

/* -------------------------------------------------- */
/*                 Parse command                      */
/* ------------------------------------
-------------- */

Command parse_command(const std::string cmd) {
  auto split_cmd = split_command(cmd);
  return get_command(split_cmd);
}

/* -------------------------------------------------- */
/*                   Commands                         */
/* -------------------------------------------------- */

/* --------------------------------*/
/*          Cap Command            */
/* --------------------------------*/

void command_cap(int fd, Params &params, IrcServer *server) {
  std::string msg;
  auto user = server->get_user(fd);
  auto nick = user->get_nick();

  if (nick == "")
    msg = std::string(":azade CAP * LS :");
  else
    msg = std::string(":azade CAP ") + nick + " LS :";

  server->write_reply(fd, msg);
}

/* --------------------------------*/
/*          Nick Command           */
/* --------------------------------*/

void command_nick(int fd, Params &params, IrcServer *server) {
  auto user = server->get_user(fd);

  for (auto const &[key, val] : server->get_users()) {
    if (val->get_nick() == params[0]) {
      server->send_numeric(fd, ERR_NICKCOLLISION, "NICK",
                           "ERROR NICK ALREADY IN USE");
      return;
    }
  }

  user->set_nick(params[0]);
}

/* --------------------------------*/
/*          User Command           */
/* --------------------------------*/

bool user_exists(int fd, Params &params, IrcServer *server) {
  // if (server->users[fd]->get_fd() == fd) {
  //   write_reply(fd, "461 USER :User already registered");
  //   return true;
  // }
  // else if (server->users[fd]->get_nick() == params[1]) {
  //   write_reply(fd, "461 USER :Nick already registered");
  // }

  return false;
}

void command_user(int fd, Params &params, IrcServer *server) {
  auto user = server->get_user(fd);
  auto nick = user->get_nick();

  if (params.size() < 4) {
    server->send_numeric(fd, ERR_NEEDMOREPARAMS, nick, "Not enough parameters");
    return;
  }

  if (user_exists(fd, params, server))
    return;

  server->send_numeric(fd, RPL_WELCOME, nick,
                       "Welcome to the Azade IRC Server");
  server->send_numeric(fd, RPL_YOURHOST, nick,
                       "Your host is azade, running version 0.1");
  server->send_numeric(fd, RPL_CREATED, nick,
                       "server server was created today");
  server->send_numeric(fd, RPL_MYINFO, nick, "azade 0.1 o o");
}

/* --------------------------------*/
/*          Ping Command           */
/* --------------------------------*/

void command_ping(int fd, Params &params, IrcServer *server) {
  if (params.empty()) {
    auto nick = server->get_user(fd)->get_nick();
    server->send_numeric(fd, ERR_NEEDMOREPARAMS, nick, "No origin specified");
    return;
  }

  server->write_reply(fd, "PONG " + params[0]);
}

/* --------------------------------*/
/*          Join Command           */
/* --------------------------------*/

std::string channel_name(std::string chl) {
  auto c = chl[0];
  std::string channel;

  if (c == '#' || c == '&')
    channel = chl.substr(1, chl.size());
  else
    channel = chl;

  return channel;
}

bool channel_exist(const std::string &name, IrcServer *server) {
  auto channels = server->get_channels();
  auto it = channels.find(name);
  return it == server->get_channels().end() ? false : true;
}

void command_join(int fd, Params &params, IrcServer *server) {
  auto user = server->get_user(fd);
  auto nick = user->get_nick();

  if (params.empty()) {
    server->send_numeric(fd, ERR_NEEDMOREPARAMS, nick, "Not enough parameters");
    return;
  }

  auto name = channel_name(params[0]);

  if (server->get_channels().empty() || !channel_exist(name, server))
    server->get_channels()[name] = new Channel(name);

  auto channel = server->get_channels()[name];
  channel->add_user(user->get_id());
  user->add_channel(name);

  /* Send the topic */
  auto msg = "Topic: " + channel->get_topic();
  server->send_numeric(fd, RPL_TOPIC, nick, msg);

  /* Broadcast new join */
  msg = "User " + nick + " Has join the channel!";
  server->broadcast(0, channel, msg);
}

/* --------------------------------*/
/*          List Command           */
/* --------------------------------*/

void command_list(int fd, Params &params, IrcServer *server) {
  auto user = server->get_user(fd);
  auto nick = user->get_nick();

  server->send_numeric(fd, RPL_LISTSTART, nick, "Channel :Users Name");

  if (params.empty()) {
    for (auto const &[name, channel] : server->get_channels()) {
      if (channel->is_private())
        continue;

      std::string msg = name + " " + std::to_string(channel->user_count()) +
                        " " + channel->get_topic();
      server->send_numeric(fd, RPL_LIST, nick, msg);
    }
  }

  else {
    for (auto ch_name : params) {
      auto name = channel_name(ch_name);

      if (!channel_exist(name, server))
        continue;

      auto channel = server->get_channels()[name];
      std::string msg = name + " " + std::to_string(channel->user_count()) +
                        " " + channel->get_topic();
      server->send_numeric(fd, RPL_LIST, nick, msg);
    }
  }

  server->send_numeric(fd, RPL_LISTEND, nick, "End of /LIST");
}

/* --------------------------------*/
/*          Mode Command           */
/* --------------------------------*/

UserMode char_to_flag(char flag) {
  switch (flag) {
  case 'w': return MODE_WALLOPS;
  case 'o': return MODE_OPERATOR;
  case 'i': return MODE_INVISIBLE;
  case 'r': return MODE_RESTRICTED;
  default: return UNKNOWN;
  }
}

void command_mode(int fd, Params &params, IrcServer *server) {
  auto user = server->get_user(fd);
  auto nick = user->get_nick();

  if (params.size() < 2) {
    server->send_numeric(fd, ERR_NEEDMOREPARAMS, nick, "Not enough parameters");
    return;
  }

  if (params[0] != nick) {
    server->send_numeric(fd, ERR_USERSDONTMATCH, nick, "User do not match");
    return;
  }

  auto modes = params[1];
  bool enable;

  for (char c : modes) {
    if (c == '+') {enable = true; continue;}
    if (c == '-') {enable = false; continue;}

    auto mode = char_to_flag(c);
    if (mode == UNKNOWN) {
      server->send_numeric(fd, ERR_UNKNOWNMODEFLAG, nick, "Not a valid mode");
      return;
    }

    user->change_mode(mode, enable);
  }
}

/* --------------------------------*/
/*          Quit Command           */
/* --------------------------------*/

void command_quit(int fd, Params &params, IrcServer *server) {
  auto user = server->get_user(fd);
  auto channels = user ->get_channels();

  std::string msg;
  if (params.empty())
    msg = "";
  else {
    for (int i = 1; i < params.size(); i++)
      msg = msg + " " + params[i];
  }

  /* Broadcast Quit message */
  for (auto ch_name : channels) {
    auto channel = server->get_channels()[ch_name];
    server->broadcast(fd, channel, msg);
    channel->remove_user(user->get_id());
  }

  server->close_user(fd);
}

/* --------------------------------*/
/*          Topic Command          */
/* --------------------------------*/

void command_topic(int fd, Params &params, IrcServer *server) {
  auto user = server->get_user(fd);
  auto nick = user->get_nick();

  if (params.empty())
    server->send_numeric(fd, ERR_NEEDMOREPARAMS, nick, "Not enough parameters");

  else if (params.size() == 1) {
    auto ch_name = channel_name(params[0]);

    if (!channel_exist(ch_name, server)) {
      server->send_numeric(fd, ERR_NOSUCHCHANNEL, nick,
                           ch_name + " No such channel");
      return;
    }

    auto channel = server->get_channels()[ch_name];

    if (!channel->has_user(user->get_id())) {
      server->send_numeric(fd, ERR_NOTONCHANNEL, nick, ch_name + "");
      return;
    }

    auto topic = server->get_channels()[ch_name]->get_topic();

    if (topic.empty())
      server->send_numeric(fd, RPL_NOTOPIC, nick, ch_name + " " + topic);
    else
      server->send_numeric(fd, RPL_TOPIC, nick, ch_name + " " + topic);
  }

  else {
    auto ch_name = channel_name(params[0]);
    auto channel = server->get_channels()[ch_name];
    auto new_topic = params[1];
    channel->set_topic(new_topic);
  }
}

/* --------------------------------*/
/*          PRIVMSG Command        */
/* --------------------------------*/

std::string build_msg(std::string nick, Channel *channel,
                      const std::string &msg) {
  auto irc_msg = ":" + nick + " PRIVMSG " + channel->get_name() + msg;

  return irc_msg;
}

void command_privmsg(int fd, Params &params, IrcServer *server) {
  auto user = server->get_user(fd);
  auto nick = user->get_nick();

  if (params.size() < 2) {
    server->send_numeric(fd, ERR_NOTEXTTOSEND, nick, "No message to send");
    return;
  }

  auto ch_name = channel_name(params[0]);
  if (!channel_exist(ch_name, server)) {
    server->send_numeric(fd, ERR_NOSUCHCHANNEL, nick,
                         ch_name + " No such channel");
    return;
  }

  /* unified split messages */
  std::string msg = "";

  for (int i = 1; i < params.size(); i++)
    msg = msg + " " + params[i];

  auto channel = server->get_channels()[ch_name];
  auto irc_msg = build_msg(user->get_nick(), channel, msg);

  server->broadcast(fd, channel, irc_msg);
}
