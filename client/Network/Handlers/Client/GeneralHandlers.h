#pragma once
#include <memory>

class MessageHandler;
class NetworkClient;
struct NetworkPacket;
namespace Client
{
    class GeneralHandlers
    {
    public:
        static void Setup(MessageHandler*);
        static bool SMSG_CREATE_ENTITY(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool SMSG_UPDATE_ENTITY(std::shared_ptr<NetworkClient>, NetworkPacket*);
    };
}