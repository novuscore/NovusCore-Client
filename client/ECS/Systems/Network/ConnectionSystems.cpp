#include "ConnectionSystems.h"
#include <entt.hpp>
#include <tracy/Tracy.hpp>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkClient.h>
#include "../../Components/Network/ConnectionSingleton.h"
#include "../../Components/Network/AuthenticationSingleton.h"
#include "../../../Utils/ServiceLocator.h"

void ConnectionUpdateSystem::Update(entt::registry& registry)
{
    ZoneScopedNC("ConnectionUpdateSystem::Update", tracy::Color::Blue)
    ConnectionSingleton& connectionSingleton = registry.ctx<ConnectionSingleton>();

    if (connectionSingleton.authConnection)
    {
        std::shared_ptr<NetworkPacket> packet = nullptr;

        MessageHandler* authSocketMessageHandler = ServiceLocator::GetAuthSocketMessageHandler();
        while (connectionSingleton.authPacketQueue.try_dequeue(packet))
        {
#ifdef NC_Debug
            NC_LOG_SUCCESS("[Network/Socket]: CMD: %u, Size: %u", packet->header.opcode, packet->header.size);
#endif // NC_Debug

            if (!authSocketMessageHandler->CallHandler(connectionSingleton.authConnection, packet))
            {
                connectionSingleton.authConnection->Close(asio::error::shut_down);
                connectionSingleton.authConnection = nullptr;
                return;
            }
        }
    }

    if (connectionSingleton.gameConnection)
    {
        std::shared_ptr<NetworkPacket> packet = nullptr;

        MessageHandler* gameSocketMessageHandler = ServiceLocator::GetGameSocketMessageHandler();
        while (connectionSingleton.gamePacketQueue.try_dequeue(packet))
        {
#ifdef NC_Debug
            NC_LOG_SUCCESS("[Network/Socket]: CMD: %u, Size: %u", packet->header.opcode, packet->header.size);
#endif // NC_Debug

            if (!gameSocketMessageHandler->CallHandler(connectionSingleton.gameConnection, packet))
            {
                connectionSingleton.gameConnection->Close(asio::error::shut_down);
                connectionSingleton.gameConnection = nullptr;
                return;
            }
        }
    }
}

void ConnectionUpdateSystem::AuthSocket_HandleConnect(BaseSocket* socket, bool connected)
{
    // The client initially will connect to a region server, from there on the client receives
    // an IP address / port from that region server to the proper authentication server.

    if (connected)
    {
        /* Send Initial Packet */
        std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<512>();
        buffer->Put(Opcode::MSG_REQUEST_ADDRESS);
        buffer->PutU16(0);
        socket->Send(buffer);

        socket->AsyncRead();
    }
}
void ConnectionUpdateSystem::AuthSocket_HandleRead(BaseSocket* socket)
{
    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    std::shared_ptr<Bytebuffer> buffer = socket->GetReceiveBuffer();

    ConnectionSingleton* connectionSingleton = &gameRegistry->ctx<ConnectionSingleton>();

    while (buffer->GetActiveSize())
    {
        Opcode opcode = Opcode::INVALID;
        u16 size = 0;

        buffer->Get(opcode);
        buffer->GetU16(size);

        if (size > NETWORK_BUFFER_SIZE)
        {
            socket->Close(asio::error::shut_down);
            return;
        }

        std::shared_ptr<NetworkPacket> packet = NetworkPacket::Borrow();
        {
            // Header
            {
                packet->header.opcode = opcode;
                packet->header.size = size;
            }

            // Payload
            {
                if (size)
                {
                    packet->payload = Bytebuffer::Borrow<NETWORK_BUFFER_SIZE>();
                    packet->payload->size = size;
                    packet->payload->writtenData = size;
                    std::memcpy(packet->payload->GetDataPointer(), buffer->GetReadPointer(), size);
                }
            }

            connectionSingleton->authPacketQueue.enqueue(packet);
        }

        buffer->readData += size;
    }

    socket->AsyncRead();
}
void ConnectionUpdateSystem::AuthSocket_HandleDisconnect(BaseSocket* socket)
{
}

void ConnectionUpdateSystem::GameSocket_HandleConnect(BaseSocket* socket, bool connected)
{
    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    AuthenticationSingleton& authentication = gameRegistry->ctx<AuthenticationSingleton>();
    
    /* Send Initial Packet */
    std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<512>();
    ClientLogonChallenge logonChallenge;
    logonChallenge.majorVersion = 3;
    logonChallenge.patchVersion = 3;
    logonChallenge.minorVersion = 5;
    logonChallenge.buildType = BuildType::Internal;
    logonChallenge.gameBuild = 12340;
    logonChallenge.gameName = "WoW";
    logonChallenge.username = "nix1";

    authentication.username = "nix1";
    authentication.srp.username = "nix1";
    authentication.srp.password = "test";

    // If StartAuthentication fails, it means A failed to generate and thus we cannot connect
    if (!authentication.srp.StartAuthentication())
        return;

    buffer->Put(Opcode::CMSG_LOGON_CHALLENGE);
    buffer->PutU16(0);

    u16 payloadSize = logonChallenge.Serialize(buffer, authentication.srp.aBuffer);

    buffer->Put<u16>(payloadSize, 2);
    socket->Send(buffer);
    socket->AsyncRead();
}
void ConnectionUpdateSystem::GameSocket_HandleRead(BaseSocket* socket)
{
    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    std::shared_ptr<Bytebuffer> buffer = socket->GetReceiveBuffer();

    ConnectionSingleton* connectionSingleton = &gameRegistry->ctx<ConnectionSingleton>();

    while (buffer->GetActiveSize())
    {
        Opcode opcode = Opcode::INVALID;
        u16 size = 0;

        buffer->Get(opcode);
        buffer->GetU16(size);

        if (size > NETWORK_BUFFER_SIZE)
        {
            socket->Close(asio::error::shut_down);
            return;
        }

        std::shared_ptr<NetworkPacket> packet = NetworkPacket::Borrow();
        {
            // Header
            {
                packet->header.opcode = opcode;
                packet->header.size = size;
            }

            // Payload
            {
                if (size)
                {
                    packet->payload = Bytebuffer::Borrow<NETWORK_BUFFER_SIZE>();
                    packet->payload->size = size;
                    packet->payload->writtenData = size;
                    std::memcpy(packet->payload->GetDataPointer(), buffer->GetReadPointer(), size);
                }
            }

            connectionSingleton->gamePacketQueue.enqueue(packet);
        }

        buffer->readData += size;
    }

    socket->AsyncRead();
}
void ConnectionUpdateSystem::GameSocket_HandleDisconnect(BaseSocket* socket)
{
}