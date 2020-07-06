#include "GameHandlers.h"
#include <entt.hpp>
#include <Networking/NetworkPacket.h>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkClient.h>
#include "../../../Utils/EntityUtils.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/Transform.h"
#include "../../../ECS/Components/LocalplayerSingleton.h"

namespace GameSocket
{
    void GameHandlers::Setup(MessageHandler* messageHandler)
    {
        // Setup other handlers
        messageHandler->SetMessageHandler(Opcode::SMSG_CREATE_PLAYER, { ConnectionStatus::CONNECTED, sizeof(entt::entity) + sizeof(u8) + sizeof(u32), GameHandlers::HandleCreatePlayer });
        messageHandler->SetMessageHandler(Opcode::SMSG_CREATE_ENTITY, { ConnectionStatus::CONNECTED, sizeof(entt::entity) + sizeof(u8) + sizeof(u32), GameHandlers::HandleCreateEntity });
        messageHandler->SetMessageHandler(Opcode::SMSG_UPDATE_ENTITY, { ConnectionStatus::CONNECTED, sizeof(entt::entity) + sizeof(vec3) + sizeof(vec3) + sizeof(vec3), GameHandlers::HandleUpdateEntity });
        messageHandler->SetMessageHandler(Opcode::SMSG_DELETE_ENTITY, { ConnectionStatus::CONNECTED, sizeof(entt::entity), GameHandlers::HandleDeleteEntity });
    }

    bool GameHandlers::HandleCreatePlayer(std::shared_ptr<NetworkClient> networkClient, std::shared_ptr<NetworkPacket>& packet)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        LocalplayerSingleton& localplayerSingleton = registry->ctx<LocalplayerSingleton>();

        u8 type;
        u32 entryId;

        packet->payload->Get(localplayerSingleton.entity);
        packet->payload->GetU8(type);
        packet->payload->GetU32(entryId);

        // Create ENTT entity
        entt::entity entity = registry->create(localplayerSingleton.entity);
        Transform& transform = registry->emplace<Transform>(entity);

        packet->payload->Get(transform.position);
        packet->payload->Get(transform.rotation);
        packet->payload->Get(transform.scale);
        transform.isDirty = true;

        Model& model = EntityUtils::CreateModelComponent(*registry, entity, "Data/models/Cube.novusmodel");
        return true;
    }
    bool GameHandlers::HandleCreateEntity(std::shared_ptr<NetworkClient> networkClient, std::shared_ptr<NetworkPacket>& packet)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        LocalplayerSingleton& localplayerSingleton = registry->ctx<LocalplayerSingleton>();

        entt::entity entityId = entt::null;
        u8 type;
        u32 entryId;

        packet->payload->Get(entityId);

        if (localplayerSingleton.entity == entityId)
            return true;

        packet->payload->GetU8(type);
        packet->payload->GetU32(entryId);

        // Create ENTT entity
        entt::entity entity = registry->create(entityId);
        Transform& transform = registry->emplace<Transform>(entity);

        packet->payload->Get(transform.position);
        packet->payload->Get(transform.rotation);
        packet->payload->Get(transform.scale);
        transform.isDirty = true;

        Model& model = EntityUtils::CreateModelComponent(*registry, entity, "Data/models/Cube.novusmodel");

        return true;
    }
    bool GameHandlers::HandleUpdateEntity(std::shared_ptr<NetworkClient> networkClient, std::shared_ptr<NetworkPacket>& packet)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        LocalplayerSingleton& localplayerSingleton = registry->ctx<LocalplayerSingleton>();

        entt::entity entityId = entt::null;
        packet->payload->Get(entityId);

        if (localplayerSingleton.entity == entityId)
            return true;

        Transform& transform = registry->get<Transform>(entityId);
        packet->payload->Get(transform.position);
        packet->payload->Get(transform.rotation);
        packet->payload->Get(transform.scale);
        transform.isDirty = true;

        return true;
    }
    bool GameHandlers::HandleDeleteEntity(std::shared_ptr<NetworkClient> networkClient, std::shared_ptr<NetworkPacket>& packet)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        LocalplayerSingleton& localplayerSingleton = registry->ctx<LocalplayerSingleton>();

        entt::entity entityId = entt::null;
        packet->payload->Get(entityId);

        if (localplayerSingleton.entity == entityId)
            return true;

        registry->destroy(entityId);
        return true;
    }
}