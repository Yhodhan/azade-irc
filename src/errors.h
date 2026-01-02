#pragma once

#include <iostream> 
#include <ostream>

enum IrcNumeric {
    /* ----- Registration / General ----- */
    RPL_WELCOME            = 001,
    RPL_YOURHOST           = 002,
    RPL_CREATED            = 003,
    RPL_MYINFO             = 004,
    RPL_UMODEIS            = 221,
    ERR_NOSUCHNICK         = 401,
    ERR_NOSUCHCHANNEL      = 403,
    ERR_CANNOTSENDTOCHAN   = 404,
    ERR_NOORIGIN           = 409, 
    ERR_UNKNOWNCOMMAND     = 421,
    ERR_NOMOTD             = 422,
    ERR_NONICKNAMEGIVEN    = 431,
    ERR_ERRONEUSNICKNAME   = 432,
    ERR_NICKNAMEINUSE      = 433,
    ERR_NICKCOLLISION      = 436,
    ERR_USERNOTINCHANNEL   = 441,
    ERR_NOTONCHANNEL       = 442,
    ERR_NOTREGISTERED      = 451,
    ERR_NEEDMOREPARAMS     = 461,
    ERR_ALREADYREGISTERED  = 462,
    ERR_PASSWDMISMATCH     = 464,
    ERR_UNKNOWNMODE        = 472,
    ERR_CHANOPRIVSNEEDED   = 482,
    ERR_UNKNOWNMODEFLAG    = 501,
    ERR_USERSDONTMATCH     = 502
};
