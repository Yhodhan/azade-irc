#pragma once

#include "../channels/channel.h"
#include <cstdint>
#include <mutex>
#include <vector>
#include <string>
#include <unordered_map>

class User;

using UserMap = std::unordered_map<int, User>;

struct Users {
 std::mutex mtx;
 UserMap users;
};

class User {
public:
  User();
  ~User();
  void set_nick(const std::string nick);
  uint32_t get_id();
  
private:
  uint32_t id;
  std::string nick;
  std::vector<Channel> channels;
};
