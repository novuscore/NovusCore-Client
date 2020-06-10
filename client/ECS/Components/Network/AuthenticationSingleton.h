#pragma once
#include <NovusTypes.h>
#include <Utils/srp.h>

struct AuthenticationSingleton
{
    std::string username = "";
    SRPUser srp;
};