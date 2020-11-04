#include "MovementSystem.h"
#include <Math/Geometry.h>
#include <Utils/ByteBuffer.h>
#include <Utils/StringUtils.h>
#include <Networking/Opcode.h>
#include <InputManager.h>
#include "../../Utils/ServiceLocator.h"
#include "../../Utils/MapUtils.h"
#include "../../Rendering/CameraOrbital.h"
#include "../Components/Singletons/TimeSingleton.h"
#include "../Components/Network/ConnectionSingleton.h"
#include "../Components/Singletons/LocalplayerSingleton.h"
#include "../Components/Transform.h"

#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <GLFW/glfw3.h>

void MovementSystem::Init(entt::registry& registry)
{
    InputManager* inputManager = ServiceLocator::GetInputManager();

    inputManager->RegisterKeybind("MovementSystem Increase Speed", GLFW_KEY_PAGE_UP, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [](Window* window, std::shared_ptr<Keybind> keybind)
    {
        CameraOrbital* camera = ServiceLocator::GetCameraOrbital();
        if (!camera->IsActive())
            return false;

        entt::registry* registry = ServiceLocator::GetGameRegistry();

        LocalplayerSingleton& localplayerSingleton = registry->ctx<LocalplayerSingleton>();
        if (localplayerSingleton.entity == entt::null)
            return false;

        Transform& transform = registry->get<Transform>(localplayerSingleton.entity);
        transform.movementData.speed += 7.1111f;

        return true;
    });
    inputManager->RegisterKeybind("MovementSystem Decrease Speed", GLFW_KEY_PAGE_DOWN, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [](Window* window, std::shared_ptr<Keybind> keybind)
    {
        CameraOrbital* camera = ServiceLocator::GetCameraOrbital();
        if (!camera->IsActive())
            return false;

        entt::registry* registry = ServiceLocator::GetGameRegistry();

        LocalplayerSingleton& localplayerSingleton = registry->ctx<LocalplayerSingleton>();
        if (localplayerSingleton.entity == entt::null)
            return false;

        Transform& transform = registry->get<Transform>(localplayerSingleton.entity);
        transform.movementData.speed = glm::max(transform.movementData.speed - 7.1111f, 7.1111f);

        return true;
    });
    inputManager->RegisterKeybind("MovementSystem Auto Run", GLFW_KEY_HOME, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [](Window* window, std::shared_ptr<Keybind> keybind)
    {
        CameraOrbital* camera = ServiceLocator::GetCameraOrbital();
        if (!camera->IsActive())
            return false;

        entt::registry* registry = ServiceLocator::GetGameRegistry();

        LocalplayerSingleton& localplayerSingleton = registry->ctx<LocalplayerSingleton>();
        if (localplayerSingleton.entity == entt::null)
            return false;

        localplayerSingleton.autoRun = !localplayerSingleton.autoRun;
        return true;
    });
}

void MovementSystem::Update(entt::registry& registry)
{
    LocalplayerSingleton& localplayerSingleton = registry.ctx<LocalplayerSingleton>();
    if (localplayerSingleton.entity == entt::null)
        return;

    CameraOrbital* camera = ServiceLocator::GetCameraOrbital();
    if (!camera->IsActive())
        return;

    TimeSingleton& timeSingleton = registry.ctx<TimeSingleton>();
    Transform& transform = registry.get<Transform>(localplayerSingleton.entity);

    // Here we save our original movement flags to know if we have "changed" direction, and have to update the server, otherwise we can continue sending heartbeats
    MovementData& movementData = transform.movementData;
    MovementFlags originalFlags = transform.movementData.flags;
    movementData.flags = MovementFlags::NONE;

    InputManager* inputManager = ServiceLocator::GetInputManager();

    f32 terrainHeight = Terrain::MapUtils::GetHeightFromWorldPosition(transform.position);
    bool isGrounded = transform.position.y <= terrainHeight;
    bool isRightClickDown = inputManager->IsKeyPressed("CameraOrbital Right Mouseclick"_h);
    if (isRightClickDown)
    {
        transform.yaw = camera->GetYaw();
        
        // Only set Pitch if we are flying
        //transform.pitch = camera->GetPitch();

        transform.rotationMatrix = glm::yawPitchRoll(glm::radians(transform.yaw), glm::radians(transform.pitch), 0.0f);
        transform.UpdateVectors();
    }

    if (isGrounded)
    {
        transform.velocityDirection = vec3(0, 0, 0);
        transform.velocity = vec3(0, 0, 0);

        // Clip to Terrain
        transform.position.y = terrainHeight;

        movementData.AddMoveFlag(MovementFlags::GROUNDED);

        bool moveForwardIsPressed = inputManager->IsKeyPressed("Move Forward"_h);
        bool isLeftClickDown = inputManager->IsKeyPressed("CameraOrbital Left Mouseclick"_h);
        if (moveForwardIsPressed || (isLeftClickDown && isRightClickDown) || localplayerSingleton.autoRun)
        {
            if (localplayerSingleton.autoRun && (moveForwardIsPressed || (isRightClickDown && isLeftClickDown)))
            {
                localplayerSingleton.autoRun = false;
            }

            movementData.AddMoveFlag(MovementFlags::FORWARD);
            transform.velocityDirection += transform.front;
        }
        if (inputManager->IsKeyPressed("Move Backward"_h))
        {
            if (localplayerSingleton.autoRun && movementData.HasMoveFlag(MovementFlags::FORWARD))
            {
                localplayerSingleton.autoRun = false;
            }

            movementData.AddMoveFlag(MovementFlags::BACKWARD);
            transform.velocityDirection -= transform.front;
        }

        if (inputManager->IsKeyPressed("Move Left"_h))
        {
            movementData.AddMoveFlag(MovementFlags::LEFT);
            transform.velocityDirection += transform.left;
        }
        if (inputManager->IsKeyPressed("Move Right"_h))
        {
            movementData.AddMoveFlag(MovementFlags::RIGHT);
            transform.velocityDirection -= transform.left;
        }

        bool isJumping = inputManager->IsKeyPressed("Move Jump"_h);
        if (isJumping)
        {
            transform.velocityDirection += transform.up;
        }

        if (glm::length2(transform.velocityDirection) != 0)
        {
            transform.velocity = glm::normalize(transform.velocityDirection) * movementData.speed;
        }

        // Apply Jump after calculating velocity
        if (isJumping)
        {
            transform.velocity.y += 7.1111f;
        }

        // If we are moving in directions that counter themselves unset them
        if (movementData.HasMoveFlag(MovementFlags::ALL))
        {
            movementData.RemoveMoveFlag(MovementFlags::ALL);
        }
        else if (movementData.HasMoveFlag(MovementFlags::VERTICAL))
        {
            movementData.RemoveMoveFlag(MovementFlags::VERTICAL);
        }
        else if (movementData.HasMoveFlag(MovementFlags::HORIZONTAL))
        {
            movementData.RemoveMoveFlag(MovementFlags::HORIZONTAL);
        }

        if (movementData.flags == MovementFlags::NONE)
        {
            // Tell server we stopped moving (Only if our flags differ)
            if (movementData.flags != originalFlags)
            {
                ConnectionSingleton& connectionSingleton = registry.ctx<ConnectionSingleton>();

                std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<128>();
                buffer->Put(Opcode::MSG_MOVE_STOP_ENTITY);
                buffer->PutU16(32);

                buffer->Put(localplayerSingleton.entity);
                buffer->Put(movementData.flags);
                buffer->Put(transform.position);
                buffer->Put(transform.GetRotation());
                connectionSingleton.gameConnection->Send(buffer);
            }
        }
        else
        {
            movementData.RemoveMoveFlag(MovementFlags::NONE);

            // Send Packet
            ConnectionSingleton& connectionSingleton = registry.ctx<ConnectionSingleton>();

            std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<128>();
            if (movementData.flags != originalFlags)
            {
                buffer->Put(Opcode::MSG_MOVE_ENTITY);
            }
            else
            {
                buffer->Put(Opcode::MSG_MOVE_HEARTBEAT_ENTITY);
            }

            buffer->PutU16(32);

            buffer->Put(localplayerSingleton.entity);
            buffer->Put(movementData.flags);
            buffer->Put(transform.position);
            buffer->Put(transform.GetRotation());

            connectionSingleton.gameConnection->Send(buffer);
        }
    }
    else
    {
        transform.velocityDirection.y -= 1.f * timeSingleton.deltaTime;
        transform.velocity = glm::normalize(transform.velocityDirection) * movementData.speed;
    }

    if (glm::length2(transform.velocity) != 0)
    {
        vec3 newPosition = transform.position + (transform.velocity * timeSingleton.deltaTime);

        Geometry::Triangle triangle;
        f32 terrainHeight = 0;

        if (Terrain::MapUtils::GetTriangleFromWorldPosition(newPosition, triangle, terrainHeight))
        {
            newPosition.y = glm::max(newPosition.y, terrainHeight);

            if (isGrounded)
            {
                f32 angle = triangle.GetSteepnessAngle();

                // TODO: Properly Push the player down the slope
                if (angle <= 50 || newPosition.y <= transform.position.y)
                {
                    transform.position = newPosition;
                }
            }
            else
            {
                transform.position = newPosition;
            }
        }

        if (!isGrounded)
        {
            //f32 velocityDecay = (7.1111f * 0.5f) * timeSingleton.deltaTime;
            //transform.velocity.x = glm::max(transform.velocity.x - velocityDecay, 0.f);
            //transform.velocity.y = glm::max(transform.velocity.y - velocityDecay, 0.f);
            //transform.velocity.z = glm::max(transform.velocity.z - velocityDecay, 0.f);
        }
    }

    
    camera->SetPosition(transform.position);
}
