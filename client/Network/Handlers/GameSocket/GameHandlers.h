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
        static bool HandleCreatePlayer(std::shared_ptr<NetworkClient>, std::shared_ptr<NetworkPacket>&);
        static bool HandleCreateEntity(std::shared_ptr<NetworkClient>, std::shared_ptr<NetworkPacket>&);
        static bool HandleUpdateEntity(std::shared_ptr<NetworkClient>, std::shared_ptr<NetworkPacket>&);
        static bool HandleDeleteEntity(std::shared_ptr<NetworkClient>, std::shared_ptr<NetworkPacket>&);
    };
}