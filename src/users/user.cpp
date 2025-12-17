#include "user.h"

User::User() { this->id = (uint32_t)rand(); }

User::~User() {}

std::string User::get_nick() { return this->nick; }
void User::set_nick(const std::string nick) { this->nick = nick; }

uint32_t User::get_id() { return this->id; }

// if enable is true then OR to signal the bit
// otherwise AND with the negation of the mode turns off the signal bit
// mode = 0001 -> ~mode = 1110
// this preserves the other bits unchanged with AND operation. 
void User::change_mode(UserMode mode, bool enable) { 
    enable ? modes |= mode
           : modes &= ~(mode);
}
// ------------------
//  Getter functions
// ------------------

std::string User::get_username() { return this->username; }
std::string User::get_hostname() { return this->hostname; }
std::string User::get_servername() { return this->servername; }
std::string User::get_realname() { return this->realname; }