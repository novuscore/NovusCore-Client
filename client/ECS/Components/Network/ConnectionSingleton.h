#pragma once
#include <NovusTypes.h>
#include <Utils/ConcurrentQueue.h>
#include <Networking/NetworkPacket.h>
#include <Networking/NetworkClient.h>

struct ConnectionSingleton
{
public:
    ConnectionSingleton() : packetQueue(256) { }

    std::shared_ptr<NetworkClient> connection;
    moodycamel::ConcurrentQueue<NetworkPacket*> packetQueue;
};