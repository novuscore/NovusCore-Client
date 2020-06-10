#include "GeneralHandlers.h"
#include <entt.hpp>
#include "Auth/AuthHandlers.h"
#include <Networking/NetworkPacket.h>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkClient.h>
#include "../../../Utils/EntityUtils.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/Transform.h"

void Client::GeneralHandlers::Setup(MessageHandler* messageHandler)
{
    // Setup other handlers
    AuthHandlers::Setup(messageHandler);
    messageHandler->SetMessageHandler(Opcode::SMSG_CREATE_ENTITY, Client::GeneralHandlers::SMSG_CREATE_ENTITY);
    messageHandler->SetMessageHandler(Opcode::SMSG_UPDATE_ENTITY, Client::GeneralHandlers::SMSG_UPDATE_ENTITY);
}

bool Client::GeneralHandlers::SMSG_CREATE_ENTITY(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
{
    entt::registry* registry = ServiceLocator::GetGameRegistry();

    entt::entity entityId;
    u8 type;
    u32 entryId;

    packet->payload->Get(entityId);
    packet->payload->GetU8(type);
    packet->payload->GetU32(entryId);

    // Create ENTT entity
    entt::entity entity = registry->create(entityId);
    Transform& transform = registry->assign<Transform>(entity);

    packet->payload->Get(transform.position);
    packet->payload->Get(transform.rotation);
    packet->payload->Get(transform.scale);

    Model& model = EntityUtils::CreateModelComponent(*registry, entity, "Data/models/Cube.novusmodel");
    

    return true;
}

bool Client::GeneralHandlers::SMSG_UPDATE_ENTITY(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
{
    entt::registry* registry = ServiceLocator::GetGameRegistry();

    entt::entity entityId;
    packet->payload->Get(entityId);

    Transform& transform = registry->get<Transform>(entityId);
    packet->payload->Get(transform.position);
    packet->payload->Get(transform.rotation);
    packet->payload->Get(transform.scale);
    transform.isDirty = true;

    return true;
}
