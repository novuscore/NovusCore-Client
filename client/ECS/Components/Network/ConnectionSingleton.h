#pragma once
#include <NovusTypes.h>
#include <Utils/ConcurrentQueue.h>
#include <Networking/NetworkPacket.h>
#include <Networking/NetworkClient.h>

struct ConnectionSingleton
{
public:
    ConnectionSingleton() : authPacketQueue(256), gamePacketQueue(256) { }

    std::shared_ptr<NetworkClient> authConnection;
    std::shared_ptr<NetworkClient> gameConnection;

    moodycamel::ConcurrentQueue<std::shared_ptr<NetworkPacket>> authPacketQueue;
    moodycamel::ConcurrentQueue<std::shared_ptr<NetworkPacket>> gamePacketQueue;
};