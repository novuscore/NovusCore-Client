#pragma once
#include "NetworkClient.h"
#include <NovusTypes.h>
#include <Utils/Message.h>
#include <Utils/DebugHandler.h>
#include "NetworkPacket.h"
#include "Opcodes.h"

#include "../Utils/ServiceLocator.h"
#include "../ECS/Components/Network/ConnectionSingleton.h"

void NetworkClient::Listen()
{
    _baseSocket->AsyncRead();
}
bool NetworkClient::Connect(tcp::endpoint endpoint)
{
    try
    {
        _baseSocket->socket()->connect(endpoint);
        HandleConnect();
    }
    catch (std::exception e)
    {
        return false;
    }

    _baseSocket->AsyncRead();
    return true;
}
bool NetworkClient::Connect(std::string address, u16 port)
{
    return Connect(tcp::endpoint(asio::ip::address::from_string(address), port));
}
void NetworkClient::HandleConnect()
{
    /* Send Initial Packet */

    std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<512>();
    ClientLogonChallenge logonChallenge;
    logonChallenge.majorVersion = 3;
    logonChallenge.patchVersion = 3;
    logonChallenge.minorVersion = 5;
    logonChallenge.buildType = BuildType::Internal;
    logonChallenge.gameBuild = 12340;
    logonChallenge.gameName = "WoW";
    logonChallenge.username = username;

    // If StartAuthentication fails, it means A failed to generate and thus we cannot connect
    if (!srp.StartAuthentication())
        return;

    buffer->PutU16(Opcode::CMSG_LOGON_CHALLENGE);
    buffer->PutU16(0);

    u16 payloadSize = logonChallenge.Serialize(buffer) + static_cast<u16>(srp.aBuffer->Size);
    buffer->PutBytes(srp.aBuffer->GetDataPointer(), srp.aBuffer->Size);

    buffer->Put<u16>(payloadSize, 2);
    Send(buffer.get());

    Listen();
}
void NetworkClient::HandleDisconnect()
{
}
void NetworkClient::HandleRead()
{
    std::shared_ptr<ByteBuffer> buffer = _baseSocket->GetReceiveBuffer();
    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();

    ConnectionSingleton* connectionSingleton = &gameRegistry->ctx<ConnectionSingleton>();

    while (buffer->GetActiveSize())
    {
        u16 opcode = 0;
        u16 size = 0;

        buffer->GetU16(opcode);
        buffer->GetU16(size);

        if (size > NETWORK_BUFFER_SIZE)
        {
            _baseSocket->Close(asio::error::shut_down);
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

    _baseSocket->AsyncRead();
}