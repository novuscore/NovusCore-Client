#include "MovementSystem.h"
#include <InputManager.h>
#include <Utils/ByteBuffer.h>
#include <Utils/StringUtils.h>
#include <Networking/Opcode.h>
#include "../../Utils/ServiceLocator.h"
#include "../Components/Localplayer.h"
#include "../Components/Transform.h"

void MovementSystem::Update(entt::registry& registry)
{
    InputManager* inputManager = ServiceLocator::GetInputManager();
    auto localplayer = registry.view<Localplayer, Transform>();
    localplayer.each([&registry, &inputManager](const entt::entity& entity, Localplayer& localplayer, Transform& transform)
        {
            MovementFlags originalFlags = transform.moveFlags;
            transform.moveFlags = MovementFlags::NONE;

            bool movingForward = transform.HasMoveFlag(MovementFlags::FORWARD);
            if (inputManager->IsKeyPressed("Move Forward"_h))
            {
                if (!movingForward)
                    transform.AddMoveFlag(MovementFlags::FORWARD);
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

            // Tell server we stopped moving
            if (transform.HasMoveFlag(MovementFlags::NONE))
            {
                // Make sure we only send stop once
                if (transform.moveFlags != originalFlags)
                {
                    std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<128>();
                    buffer->PutU16(Opcode::MSG_MOVE_STOP_ENTITY);
                    buffer->PutU16(32);

                    buffer->Put(entity);
                    buffer->Put(transform.moveFlags);
                    buffer->Put(transform.position);
                    buffer->Put(transform.rotation);
                }
            }
            else
            {
                // Calculate new position

                // Send Packet
                std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<128>();
                buffer->PutU16(Opcode::MSG_MOVE_ENTITY);
                buffer->PutU16(32);

                buffer->Put(entity);
                buffer->Put(transform.moveFlags);
                buffer->Put(transform.position);
                buffer->Put(transform.rotation);
            }
        });

}
