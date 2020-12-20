#include "NetworkUtils.h"
#include "../ECS/Components/Network/AuthenticationSingleton.h"
#include "../ECS/Components/Network/ConnectionSingleton.h"
#include "../ECS/Systems/Network/ConnectionSystems.h"

namespace NetworkUtils
{
    void InitNetwork(entt::registry* registry, std::shared_ptr<asio::io_service> asioService)
    {
        ConnectionSingleton& connectionSingleton = registry->set<ConnectionSingleton>();
        AuthenticationSingleton& authenticationSingleton = registry->set<AuthenticationSingleton>();

        // Init Auth Socket
        {
            connectionSingleton.authConnection = std::make_shared<NetworkClient>(new asio::ip::tcp::socket(*asioService.get()));
            connectionSingleton.authConnection->SetReadHandler(std::bind(&ConnectionUpdateSystem::AuthSocket_HandleRead, std::placeholders::_1));
            connectionSingleton.authConnection->SetConnectHandler(std::bind(&ConnectionUpdateSystem::AuthSocket_HandleConnect, std::placeholders::_1, std::placeholders::_2));
            connectionSingleton.authConnection->SetDisconnectHandler(std::bind(&ConnectionUpdateSystem::AuthSocket_HandleDisconnect, std::placeholders::_1));
        }

        // Init Game Socket
        {
            connectionSingleton.gameConnection = std::make_shared<NetworkClient>(new asio::ip::tcp::socket(*asioService.get()));
            connectionSingleton.gameConnection->SetReadHandler(std::bind(&ConnectionUpdateSystem::GameSocket_HandleRead, std::placeholders::_1));
            connectionSingleton.gameConnection->SetConnectHandler(std::bind(&ConnectionUpdateSystem::GameSocket_HandleConnect, std::placeholders::_1, std::placeholders::_2));
            connectionSingleton.gameConnection->SetDisconnectHandler(std::bind(&ConnectionUpdateSystem::GameSocket_HandleDisconnect, std::placeholders::_1));
        }
        
    }
    void DeInitNetwork(entt::registry* registry, std::shared_ptr<asio::io_service> asioService)
    {
        ConnectionSingleton& connectionSingleton = registry->ctx<ConnectionSingleton>();
        if (!connectionSingleton.authConnection->IsClosed())
        {
            connectionSingleton.authConnection->Close(asio::error::shut_down);
        }

        if (!connectionSingleton.gameConnection->IsClosed())
        {
            connectionSingleton.gameConnection->Close(asio::error::shut_down);
        }

        asioService->stop();
    }
}
