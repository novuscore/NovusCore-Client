#pragma once
#include <memory>

class MessageHandler;
class NetworkClient;
struct NetworkPacket;
namespace AuthSocket
{
    class AuthHandlers
    {
    public:
        static void Setup(MessageHandler*);
        static bool HandleSendAddress(std::shared_ptr<NetworkClient>, std::shared_ptr<NetworkPacket>&);
        static bool HandshakeHandler(std::shared_ptr<NetworkClient>, std::shared_ptr<NetworkPacket>&);
        static bool HandshakeResponseHandler(std::shared_ptr<NetworkClient>, std::shared_ptr<NetworkPacket>&);
    };
}