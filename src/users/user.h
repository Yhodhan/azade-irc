#pragma once

#include "../channels/channel.h"
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

class User;

using UserMap = std::map<int, User *>;
using UserIdMap = std::map<uint32_t, User *>;

enum UserMode {
  MODE_INVISIBLE = 1 << 0,
  MODE_WALLOPS = 1 << 1,
  MODE_RESTRICTED = 1 << 2,
  MODE_OPERATOR = 1 << 3,
  UNKNOWN = 0,
};

class User {
public:
  User(int fd);
  ~User();

  int get_fd();
  void registry();
  uint32_t get_id();
  bool is_tls_ready();
  bool is_registered();
  std::string get_nick();
  void set_tls(bool ready);
  std::string get_username();
  std::string get_hostname();
  std::string get_realname();
  std::string get_servername();
  void add_channel(std::string channel);
  void set_nick(const std::string nick);
  void change_mode(UserMode mode, bool enable);
  std::unordered_set<std::string> get_channels();

private:
  int fd;
  uint32_t id;
  uint8_t modes = 0;
  std::string nick = "";
  std::string username;
  std::string hostname;
  std::string servername;
  std::string realname;
  bool tls_ready = false;
  bool registered = false;

  std::unordered_set<std::string> channels;
};
