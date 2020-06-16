#include "GeneralHandlers.h"
#include <entt.hpp>
#include "Auth/AuthHandlers.h"
#include <Networking/NetworkPacket.h>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkClient.h>
#include "../../../Utils/EntityUtils.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/Transform.h"
#include "../../../ECS/Components/LocalplayerSingleton.h"

void Client::GeneralHandlers::Setup(MessageHandler* messageHandler)
{
    // Setup other handlers
    AuthHandlers::Setup(messageHandler);
    messageHandler->SetMessageHandler(Opcode::SMSG_CREATE_PLAYER, Client::GeneralHandlers::SMSG_CREATE_PLAYER);
    messageHandler->SetMessageHandler(Opcode::SMSG_CREATE_ENTITY, Client::GeneralHandlers::SMSG_CREATE_ENTITY);
    messageHandler->SetMessageHandler(Opcode::SMSG_UPDATE_ENTITY, Client::GeneralHandlers::SMSG_UPDATE_ENTITY);
    messageHandler->SetMessageHandler(Opcode::SMSG_DELETE_ENTITY, Client::GeneralHandlers::SMSG_DELETE_ENTITY);
}

bool Client::GeneralHandlers::SMSG_CREATE_PLAYER(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
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
    Transform& transform = registry->assign<Transform>(entity);

    packet->payload->Get(transform.position);
    packet->payload->Get(transform.rotation);
    packet->payload->Get(transform.scale);
    transform.isDirty = true;

    Model& model = EntityUtils::CreateModelComponent(*registry, entity, "Data/models/Cube.novusmodel");
    return true;
}
bool Client::GeneralHandlers::SMSG_CREATE_ENTITY(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
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
    Transform& transform = registry->assign<Transform>(entity);

    packet->payload->Get(transform.position);
    packet->payload->Get(transform.rotation);
    packet->payload->Get(transform.scale);
    transform.isDirty = true;

    Model& model = EntityUtils::CreateModelComponent(*registry, entity, "Data/models/Cube.novusmodel");

    return true;
}

bool Client::GeneralHandlers::SMSG_UPDATE_ENTITY(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
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

bool Client::GeneralHandlers::SMSG_DELETE_ENTITY(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
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
