#include "channel.h"

/* --------------------------------*/
/*          Constructor            */
/* --------------------------------*/

Channel::Channel(std::string name) : name(name) {}

/* --------------------------------*/
/*          Destructor             */
/* --------------------------------*/

Channel::~Channel() {}

/* --------------------------------*/
/*            Getters              */
/* --------------------------------*/

bool Channel::is_private() { return this->_private; }
int Channel::user_count() { return this->users.size(); }
std::string Channel::get_topic() { return this->topic; }
std::string Channel::get_name() { return "#" + this->name; }
std::unordered_set<UserId> Channel::get_users() { return this->users; }

/* --------------------------------*/
/*            Setters              */
/* --------------------------------*/

void Channel::add_user(UserId id) { users.insert(id); }
void Channel::set_topic(std::string new_topic) { this->topic = new_topic; }

bool Channel::has_user(UserId id) {
  auto it = this->users.find(id);
  return it == this->users.end() ? false : true;
}
