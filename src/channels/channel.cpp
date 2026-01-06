#include "channel.h"

Channel::Channel(std::string name) : name(name) {}

Channel::~Channel() {}

void Channel::add_user(UserId id) { users.insert(id); }
void Channel::set_topic(std::string new_topic) { this->topic = new_topic; }

// Getters

bool Channel::is_private() { return this->_private; }
int Channel::user_count() { return this->users.size(); }
std::string Channel::get_topic() { return this->topic; }

bool Channel::has_user(UserId id) {
  auto it = this->users.find(id);
  return it == this->users.end() ? false : true;
}
