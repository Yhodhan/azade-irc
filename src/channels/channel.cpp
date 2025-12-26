#include "channel.h"

Channel::Channel(std::string name) : name(name){}

Channel::~Channel() {}

void Channel::add_user(UserId id) {
    users.insert(id);
}
