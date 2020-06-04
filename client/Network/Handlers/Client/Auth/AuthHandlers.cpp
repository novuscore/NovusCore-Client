#include "AuthHandlers.h"
#include "../../../MessageHandler.h"
#include "../../../NetworkClient.h"
#include "../../../NetworkPacket.h"
#include <Utils/ByteBuffer.h>

// @TODO: Remove Temporary Includes when they're no longer needed
#include <Utils/DebugHandler.h>

void Client::AuthHandlers::Setup(MessageHandler* messageHandler)
{
    messageHandler->SetMessageHandler(Opcode::SMSG_LOGON_CHALLENGE, Client::AuthHandlers::HandshakeHandler);
    messageHandler->SetMessageHandler(Opcode::SMSG_LOGON_RESPONSE, Client::AuthHandlers::HandshakeResponseHandler);
}
bool Client::AuthHandlers::HandshakeHandler(std::shared_ptr<NetworkClient> client, NetworkPacket* packet)
{
    ServerLogonChallenge logonChallenge;
    logonChallenge.Deserialize(packet->payload);

    NC_LOG_MESSAGE("Received SMSG_LOGON_CHALLENGE");
    NC_LOG_MESSAGE("Status (%u)", logonChallenge.status);

    // If "ProcessChallenge" fails, we have either hit a bad memory allocation or a SRP-6a safety check, thus we should close the connection
    if (!client->srp.ProcessChallenge(logonChallenge.s, logonChallenge.B))
    {
        client->Close(asio::error::no_data);
        return true;
    }

    std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<36>();
    ClientLogonResponse clientResponse;

    std::memcpy(clientResponse.M1, client->srp.M, 32);

    buffer->PutU16(Opcode::CMSG_LOGON_RESPONSE);
    buffer->PutU16(0);

    u16 payloadSize = clientResponse.Serialize(buffer);
    buffer->Put<u16>(payloadSize, 2);
    client->Send(buffer.get());
    return true;
}
bool Client::AuthHandlers::HandshakeResponseHandler(std::shared_ptr<NetworkClient> client, NetworkPacket* packet)
{
    // Handle handshake response
    NC_LOG_MESSAGE("Received SMSG_LOGON_RESPONSE");
    ServerLogonResponse logonResponse;
    logonResponse.Deserialize(packet->payload);

    if (!client->srp.VerifySession(logonResponse.HAMK))
    {
        NC_LOG_WARNING("Unsuccessful Login");
        client->Close(asio::error::no_permission);
        return true;
    }

    NC_LOG_SUCCESS("Successful Login");
    return true;
}