#pragma once
#include <entity/fwd.hpp>

class BaseSocket;
class ConnectionUpdateSystem
{
public:
    static void Update(entt::registry& registry);

    // Handlers for Network Client
    static void HandleRead(BaseSocket* socket);
    static void HandleConnect(BaseSocket* socket);
    static void HandleDisconnect(BaseSocket* socket);
};