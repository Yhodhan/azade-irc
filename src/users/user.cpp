#include "user.h"
#include <cstdint>

User::User() { this->id = (uint32_t)rand(); }

User::~User() {}

void User::set_nick(const std::string nick) { this->nick = nick; }

uint32_t User::get_id() { return this->id; }
