#pragma once
#include <memory>

class MessageHandler;
class NetworkClient;
struct NetworkPacket;
namespace GameSocket
{
    class GameHandlers
    {
    public:
        static void Setup(MessageHandler*);
        static bool HandleCreatePlayer(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool HandleCreateEntity(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool HandleUpdateEntity(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool HandleDeleteEntity(std::shared_ptr<NetworkClient>, NetworkPacket*);
    };
}