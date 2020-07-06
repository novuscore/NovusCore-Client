#include "MovementSystem.h"
#include <InputManager.h>
#include <Utils/ByteBuffer.h>
#include <Utils/StringUtils.h>
#include <Networking/Opcode.h>
#include "../../Utils/ServiceLocator.h"
#include "../../Rendering/Camera.h"
#include "../Components/Singletons/TimeSingleton.h"
#include "../Components/Network/ConnectionSingleton.h"
#include "../Components/LocalplayerSingleton.h"
#include "../Components/Transform.h"

void MovementSystem::Update(entt::registry& registry)
{
    LocalplayerSingleton& localplayerSingleton = registry.ctx<LocalplayerSingleton>();
    if (localplayerSingleton.entity == entt::null)
        return;

    InputManager* inputManager = ServiceLocator::GetInputManager();
    Transform& transform = registry.get<Transform>(localplayerSingleton.entity);

    MovementFlags originalFlags = transform.moveFlags;
    transform.moveFlags = MovementFlags::NONE;

    bool movingForward = transform.HasMoveFlag(MovementFlags::FORWARD);
    if (inputManager->IsKeyPressed("Move Forward"_h))
    {
        if (!movingForward)
        {
            transform.AddMoveFlag(MovementFlags::FORWARD);
        }
    }
    else
    {
        if (movingForward)
            transform.RemoveMoveFlag(MovementFlags::FORWARD);
    }

    bool movingBackward = transform.HasMoveFlag(MovementFlags::BACKWARD);
    if (inputManager->IsKeyPressed("Move Backward"_h))
    {
        if (!movingBackward)
            transform.AddMoveFlag(MovementFlags::BACKWARD);
    }
    else
    {
        if (movingBackward)
            transform.RemoveMoveFlag(MovementFlags::BACKWARD);
    }

    bool movingLeft = transform.HasMoveFlag(MovementFlags::LEFT);
    if (inputManager->IsKeyPressed("Move Left"_h))
    {
        if (!movingLeft)
            transform.AddMoveFlag(MovementFlags::LEFT);
    }
    else
    {
        if (movingLeft)
            transform.RemoveMoveFlag(MovementFlags::LEFT);
    }

    bool movingRight = transform.HasMoveFlag(MovementFlags::RIGHT);
    if (inputManager->IsKeyPressed("Move Right"_h))
    {
        if (!movingRight)
            transform.AddMoveFlag(MovementFlags::RIGHT);
    }
    else
    {
        if (movingRight)
            transform.RemoveMoveFlag(MovementFlags::RIGHT);
    }


    // If we are moving in directions that counter themselves unset them
    if (transform.HasMoveFlag(MovementFlags::ALL))
    {
        transform.RemoveMoveFlag(MovementFlags::ALL);
    }
    else if (transform.HasMoveFlag(MovementFlags::VERTICAL))
    {
        transform.RemoveMoveFlag(MovementFlags::VERTICAL);
    }
    else if (transform.HasMoveFlag(MovementFlags::HORIZONTAL))
    {
        transform.RemoveMoveFlag(MovementFlags::HORIZONTAL);
    }

    if (transform.moveFlags != MovementFlags::NONE)
        transform.RemoveMoveFlag(MovementFlags::NONE);

    // Tell server we stopped moving
    if (transform.HasMoveFlag(MovementFlags::NONE))
    {
        // Make sure we only send stop once
        if (transform.moveFlags != originalFlags)
        {
            Camera* camera = ServiceLocator::GetCamera();

            ConnectionSingleton& connectionSingleton = registry.ctx<ConnectionSingleton>();
            std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<128>();
            buffer->Put(Opcode::MSG_MOVE_STOP_ENTITY);
            buffer->PutU16(32);

            vec3 position = camera->GetPosition();
            vec3 rotation = camera->GetRotation();

            buffer->Put(localplayerSingleton.entity);
            buffer->Put(transform.moveFlags);
            buffer->Put(position);
            buffer->Put(rotation);
            connectionSingleton.gameConnection->Send(buffer);

            transform.position = position;
            transform.rotation = rotation;
            transform.isDirty = true;
        }
    }
    else
    {
        // Calculate new position
        Camera* camera = ServiceLocator::GetCamera();
        TimeSingleton& timeSingleton = registry.ctx<TimeSingleton>();
        //transform.position.x += 1 * timeSingleton.deltaTime;

        // Send Packet
        ConnectionSingleton& connectionSingleton = registry.ctx<ConnectionSingleton>();

        std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<128>();
        if (transform.moveFlags != originalFlags)
        {
            buffer->Put(Opcode::MSG_MOVE_ENTITY);
        }
        else
        {
            buffer->Put(Opcode::MSG_MOVE_HEARTBEAT_ENTITY);
        }

        buffer->PutU16(32);

        vec3 position = camera->GetPosition();
        vec3 rotation = camera->GetRotation();

        buffer->Put(localplayerSingleton.entity);
        buffer->Put(transform.moveFlags);
        buffer->Put(position);
        buffer->Put(rotation);
        connectionSingleton.gameConnection->Send(buffer);

        transform.position = position;
        transform.rotation = rotation;
        transform.isDirty = true;
    }
}
