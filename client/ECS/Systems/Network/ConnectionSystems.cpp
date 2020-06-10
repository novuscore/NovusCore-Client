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
    MessageHandler* messageHandler = ServiceLocator::GetNetworkMessageHandler();
    ConnectionSingleton& connectionSingleton = registry.ctx<ConnectionSingleton>();

    ZoneScopedNC("InternalPacketHandlerSystem::Update", tracy::Color::Blue)

        NetworkPacket* packet;
    while (connectionSingleton.packetQueue.try_dequeue(packet))
    {
        if (!messageHandler->CallHandler(connectionSingleton.connection, packet))
        {
            connectionSingleton.packetQueue.enqueue(packet);
            continue;
        }

        delete packet;
    }
}

void ConnectionUpdateSystem::HandleRead(BaseSocket* socket)
{
    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    std::shared_ptr<ByteBuffer> buffer = socket->GetReceiveBuffer();

    ConnectionSingleton* connectionSingleton = &gameRegistry->ctx<ConnectionSingleton>();

    while (buffer->GetActiveSize())
    {
        u16 opcode = 0;
        u16 size = 0;

        buffer->GetU16(opcode);
        buffer->GetU16(size);

        if (size > NETWORK_BUFFER_SIZE)
        {
            socket->Close(asio::error::shut_down);
            return;
        }

        NetworkPacket* packet = new NetworkPacket();
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
                    packet->payload = ByteBuffer::Borrow<NETWORK_BUFFER_SIZE>();
                    packet->payload->Size = size;
                    packet->payload->WrittenData = size;
                    std::memcpy(packet->payload->GetDataPointer(), buffer->GetReadPointer(), size);
                }
            }

            connectionSingleton->packetQueue.enqueue(packet);
        }

        buffer->ReadData += size;
    }

    socket->AsyncRead();
}

void ConnectionUpdateSystem::HandleConnect(BaseSocket* socket)
{
    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    AuthenticationSingleton& authentication = gameRegistry->ctx<AuthenticationSingleton>();
    /* Send Initial Packet */

    std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<512>();
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

    buffer->PutU16(Opcode::CMSG_LOGON_CHALLENGE);
    buffer->PutU16(0);

    u16 payloadSize = logonChallenge.Serialize(buffer, authentication.srp.aBuffer);

    buffer->Put<u16>(payloadSize, 2);
    socket->Send(buffer.get());
    socket->AsyncRead();
}

void ConnectionUpdateSystem::HandleDisconnect(BaseSocket* socket)
{
}