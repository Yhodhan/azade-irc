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
  void set_topic(std::string new_topic);

  // getters
  bool is_private();
  int user_count();
  std::string get_topic();
  bool has_user(UserId id);

private:
  std::string name;
  std::string topic;
  std::unordered_set<UserId> users;
  bool _private = false;
};
