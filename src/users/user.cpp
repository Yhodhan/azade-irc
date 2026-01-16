#include "user.h"

/* -------------------------------------------------- */
/*                 Constructor                        */
/* -------------------------------------------------- */

User::User(int fd) : fd(fd) { this->id = (uint32_t)rand(); }

/* -------------------------------------------------- */
/*                   Destructor                       */
/* -------------------------------------------------- */

User::~User() {}

/* -------------------------------------------------- */
/*                    Getters                         */
/* -------------------------------------------------- */

int User::get_fd() { return this->fd; }
uint32_t User::get_id() { return this->id; }
std::string User::get_nick() { return this->nick; }
std::string User::get_username() { return this->username; }
std::string User::get_hostname() { return this->hostname; }
std::string User::get_servername() { return this->servername; }
std::string User::get_realname() { return this->realname; }
std::unordered_set<std::string> User::get_channels() { return this->channels; }

/* -------------------------------------------------- */
/*                    Setters                         */
/* -------------------------------------------------- */

void User::set_nick(const std::string nick) { this->nick = nick; }
void User::add_channel(std::string channel) { this->channels.insert(channel); }

/* -------------------------------------------------- */
/*                    Change Mode                     */
/* -------------------------------------------------- */

// if enable is true then OR to signal the bit
// otherwise AND with the negation of the mode turns off the signal bit
// mode = 0001 -> ~mode = 1110
// this preserves the other bits unchanged with AND operation.
void User::change_mode(UserMode mode, bool enable) {
  enable ? modes |= mode : modes &= ~(mode);
}
