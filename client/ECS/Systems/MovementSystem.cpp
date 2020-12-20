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
#include "../Components/Singletons/LocalplayerSingleton.h"
#include "../Components/Network/ConnectionSingleton.h"
#include "../Components/Rendering/DebugBox.h"
#include "../Components/Transform.h"

#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <GLFW/glfw3.h>

void MovementSystem::Init(entt::registry& registry)
{
    InputManager* inputManager = ServiceLocator::GetInputManager();

    inputManager->RegisterKeybind("MovementSystem Forward", GLFW_KEY_W, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);
    inputManager->RegisterKeybind("MovementSystem Backward", GLFW_KEY_S, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);
    inputManager->RegisterKeybind("MovementSystem Left", GLFW_KEY_A, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);
    inputManager->RegisterKeybind("MovementSystem Right", GLFW_KEY_D, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);
    inputManager->RegisterKeybind("MovementSystem Jump", GLFW_KEY_SPACE, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);

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
        transform.moveSpeed += 7.1111f;

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
        transform.moveSpeed = glm::max(transform.moveSpeed - 7.1111f, 7.1111f);

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

        localplayerSingleton.movementProperties.autoRun = !localplayerSingleton.movementProperties.autoRun;
        return true;
    });

    LocalplayerSingleton& localplayerSingleton = registry.set<LocalplayerSingleton>();
    localplayerSingleton.movementProperties.canJump = true;
    localplayerSingleton.movementProperties.canChangeDirection = true;

    // This allows us to move around in the world "offline" (The server will automatically override this when connecting
    localplayerSingleton.entity = registry.create();

    registry.emplace<DebugBox>(localplayerSingleton.entity);
    Transform& transform = registry.emplace<Transform>(localplayerSingleton.entity);
    transform.position = vec3(-9249.f, 87.f, 79.f);
    transform.scale = vec3(0.5f, 0.5f, 2.f); // "Ish" scale for humans
}

void MovementSystem::Update(entt::registry& registry)
{
    LocalplayerSingleton& localplayerSingleton = registry.ctx<LocalplayerSingleton>();
    if (localplayerSingleton.entity == entt::null)
        return;

    CameraOrbital* camera = ServiceLocator::GetCameraOrbital();
    if (!camera->IsActive())
        return;

    InputManager* inputManager = ServiceLocator::GetInputManager();
    TimeSingleton& timeSingleton = registry.ctx<TimeSingleton>();
    Transform& transform = registry.get<Transform>(localplayerSingleton.entity);

    // Here we save our original movement flags to know if we have "changed" direction, and have to update the server, otherwise we can continue sending heartbeats
    Transform::MovementFlags& movementFlags = transform.movementFlags;
    Transform::MovementFlags originalFlags = transform.movementFlags;
    movementFlags.value = 0;

    Geometry::Triangle triangle;
    f32 terrainHeight = 0.f;
    Terrain::MapUtils::GetTriangleFromWorldPosition(transform.position, triangle, terrainHeight);

    bool isGrounded = transform.position.z <= terrainHeight;
    bool isRightClickDown = inputManager->IsKeyPressed("CameraOrbital Right Mouseclick"_h);
    if (isRightClickDown)
    {
        transform.yaw = camera->GetYaw();
        
        // Only set Pitch if we are flying
        //transform.pitch = camera->GetPitch();
    }

    transform.UpdateRotationMatrix();

    bool isJumping = false;

    // Handle Input
    {
        vec3 moveDirection = vec3(0.f, 0.f, 0.f);
        i8 moveAlongVerticalAxis = 0;
        i8 moveAlongHorizontalAxis = 0;
        bool isMoving = false;

        moveAlongVerticalAxis += inputManager->IsKeyPressed("MovementSystem Forward"_h);
        moveAlongVerticalAxis -= inputManager->IsKeyPressed("MovementSystem Backward"_h);

        moveAlongHorizontalAxis += inputManager->IsKeyPressed("MovementSystem Left"_h);
        moveAlongHorizontalAxis -= inputManager->IsKeyPressed("MovementSystem Right"_h);

        isMoving = moveAlongVerticalAxis + moveAlongHorizontalAxis;

        // Handle Move Direction
        {
            // FORWARD
            if (moveAlongVerticalAxis == 1)
            {
                moveDirection += transform.front;
            }
            // BACKWARD
            else if (moveAlongVerticalAxis == -1)
            {
                moveDirection -= transform.front;
            }

            // LEFT
            if (moveAlongHorizontalAxis == 1)
            {
                moveDirection += transform.left;
            }
            // RIGHT
            else if (moveAlongHorizontalAxis == -1)
            {
                moveDirection -= transform.left;
            }

            // JUMP
            bool isPressingJump = inputManager->IsKeyPressed("MovementSystem Jump"_h);
            isJumping = isPressingJump && localplayerSingleton.movementProperties.canJump;

            // This ensures we have to stop pressing "Move Jump" and press it again to jump
            localplayerSingleton.movementProperties.canJump = !isPressingJump;

            f32 moveDirectionLength = glm::length2(moveDirection);
            if (moveDirectionLength != 0)
            {
                moveDirection = glm::normalize(moveDirection);
            }
        }

        if (isGrounded)
        {
            localplayerSingleton.movementProperties.canChangeDirection = true;
            transform.velocity = vec3(0.f, 0.f, 0.f);
            transform.fallSpeed = 19.5f;

            // Clip to Terrain
            {
                transform.position.z = terrainHeight;
                movementFlags.GROUNDED = !isJumping;
            }

            movementFlags.FORWARD = moveAlongVerticalAxis == 1;
            movementFlags.BACKWARD = moveAlongVerticalAxis == -1;
            movementFlags.LEFT = moveAlongHorizontalAxis == 1;
            movementFlags.RIGHT = moveAlongHorizontalAxis == -1;
            transform.velocity = moveDirection * transform.moveSpeed;

            if (isJumping)
            {
                transform.velocity += transform.up * 8.f;

                // Ensure we can only manipulate direction of the jump if we did not produce a direction when we initially jumped
                localplayerSingleton.movementProperties.canChangeDirection = moveAlongVerticalAxis == 0 && moveAlongHorizontalAxis == 0;
            }
        }
        else
        {
            movementFlags.GROUNDED = false;

            // Check if we can change direction
            if (isMoving && localplayerSingleton.movementProperties.canChangeDirection)
            {
                localplayerSingleton.movementProperties.canChangeDirection = false;

                f32 moveSpeed = transform.moveSpeed;

                // If we were previously standing still reduce our moveSpeed by half
                if (originalFlags.value == 0 || originalFlags.value == 0x16)
                    moveSpeed *= 0.33f;

                movementFlags.FORWARD = moveAlongVerticalAxis == 1;
                movementFlags.BACKWARD = moveAlongVerticalAxis == -1;
                movementFlags.LEFT = moveAlongHorizontalAxis == 1;
                movementFlags.RIGHT = moveAlongHorizontalAxis == -1;

                vec3 newVelocity = moveDirection * moveSpeed;
                transform.velocity.x = newVelocity.x;
                transform.velocity.y = newVelocity.y;
            }
            else
            {
                // We rebuild our movementFlags every frame to check if we need to send an update to the server, this ensures we keep the correct movement flags
                movementFlags.FORWARD = originalFlags.FORWARD;
                movementFlags.BACKWARD = originalFlags.BACKWARD;
                movementFlags.LEFT = originalFlags.LEFT;
                movementFlags.RIGHT = originalFlags.RIGHT;
            }

            transform.fallSpeed += transform.fallAcceleration * timeSingleton.deltaTime;
            transform.velocity -= transform.up * transform.fallSpeed * timeSingleton.deltaTime;
        }
    }

    f32 sqrVelocity = glm::length2(transform.velocity);
    if (sqrVelocity != 0)
    {
        vec3 newPosition = transform.position + (transform.velocity * timeSingleton.deltaTime);
        Terrain::MapUtils::GetTriangleFromWorldPosition(newPosition, triangle, terrainHeight);

        f32 diff = newPosition.z - terrainHeight;
        if (!isJumping && originalFlags.GROUNDED && diff < 0.25f)
        {
            newPosition.z = terrainHeight;
        }
        else
        {
            newPosition.z = glm::max(newPosition.z, terrainHeight);
        }

        if (isGrounded)
        {
            f32 angle = triangle.GetSteepnessAngle();

            // TODO: Properly Push the player down the slope
            if (angle <= 50 || newPosition.z <= transform.position.z)
            {
                transform.position = newPosition;
            }
        }
        else
        {
            transform.position = newPosition;
        }
    }
    
    camera->SetPosition(transform.position);
}
