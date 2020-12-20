#pragma once
#include <entity/fwd.hpp>
#include <asio/io_service.hpp>

struct ConnectionSingleton;
namespace NetworkUtils
{
    void InitNetwork(entt::registry* registry, std::shared_ptr<asio::io_service> asioService);
    void DeInitNetwork(entt::registry* registry, std::shared_ptr<asio::io_service> asioService);
}