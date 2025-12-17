#pragma once

#include "../channels/channel.h"
#include <cstdint>
#include <mutex>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

class User;

using UserMap = std::unordered_map<int, std::shared_ptr<User>>;

struct Users {
 std::mutex mtx;
 UserMap users_map;
};

enum UserMode {
  MODE_INVISIBLE = 1 << 0,
  MODE_WALLOPS = 1 << 1,
  MODE_RESTRICTED = 1 << 2,
  MODE_OPERATOR = 1 << 3,
  UNKNOWN,
};

class User {
public:
  User();
  ~User();
  std::string get_nick();
  void set_nick(const std::string nick);
  uint32_t get_id();
  
  std::string get_username();
  std::string get_hostname();
  std::string get_servername();
  std::string get_realname();
  void change_mode(UserMode mode, bool enable);

private:
  uint32_t id;
  std::string nick = "";
  std::string username;
  std::string hostname;
  std::string servername;
  std::string realname;
  uint8_t modes = 0;   

  std::vector<Channel> channels;
};
