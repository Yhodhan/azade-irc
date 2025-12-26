#pragma once

#include <cstdint>
#include <mutex>
#include <vector>
#include <string>
#include <map>
#include <memory>

class User;

using UserMap = std::map<int, User *>;

enum UserMode {
  MODE_INVISIBLE  = 1 << 0,
  MODE_WALLOPS    = 1 << 1,
  MODE_RESTRICTED = 1 << 2,
  MODE_OPERATOR   = 1 << 3,
  UNKNOWN         = 0,
};

class User {
public:
  User(int fd);
  ~User();

  int get_fd();
  uint32_t get_id();
  std::string get_nick();
  void set_nick(const std::string nick);  
  std::string get_username();
  std::string get_hostname();
  std::string get_realname();
  std::string get_servername();
  void change_mode(UserMode mode, bool enable);

private:
  int fd;
  uint32_t id;
  uint8_t  modes = 0;   
  std::string nick = "";
  std::string username;
  std::string hostname;
  std::string servername;
  std::string realname;
};