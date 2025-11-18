#include "user.h"

User::User() { this->id = (uint32_t)rand(); }

User::~User() {}

void User::set_nick(const std::string nick) { this->nick = nick; }

uint32_t User::get_id() { return this->id; }

// ------------------
//  Getter functions
// ------------------

std::string User::get_username() { return this->username; }
std::string User::get_hostname() { return this->hostname; }
std::string User::get_servername() { return this->servername; }
std::string User::get_realname() { return this->realname; }
