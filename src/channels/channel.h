#pragma once 

#include <cstdint>
#include <string>
#include <unordered_set>

typedef uint32_t UserId;

class Channel {
public:
  Channel(std::string name);
  ~Channel();
  
  void add_user(UserId id);

private:
  std::string name;
  std::unordered_set<UserId> users;
};
