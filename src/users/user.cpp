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

bool User::has_nick() { return this->nick != ""; }

void User::registry() { this->registered = true; }

std::string User::get_nick() { return this->nick; }

bool User::is_tls_ready() { return this->tls_ready; }

bool User::is_registered() { return this->registered; }

void User::set_tls(bool ready) { this->tls_ready = ready; }

std::string User::get_username() { return this->username; }

std::string User::get_hostname() { return this->hostname; }

std::string User::get_servername() { return this->servername; }

std::string User::get_realname() { return this->realname; }

std::unordered_set<std::string> User::get_channels() { return this->channels; }

bool User::has_user_fields() {
  return this->username != "" && this->realname != "";
}

/* -------------------------------------------------- */
/*                    Setters                         */
/* -------------------------------------------------- */

void User::set_nick(const std::string nick) { this->nick = nick; }

void User::add_channel(std::string channel) { this->channels.insert(channel); }

void User::set_username(const std::string &username) {
  this->username = username;
}

void User::set_hostname(const std::string &hostname) {
  this->hostname = hostname;
}

void User::set_realname(const std::string &realname) {
  this->realname = realname;
}

void User::set_servername(const std::string &servername) {
  this->servername = servername;
}

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
