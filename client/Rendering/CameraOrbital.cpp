#include "CameraOrbital.h"
#include <windows.h>
#include <InputManager.h>
#include <filesystem>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <GLFW/glfw3.h>
#include <Window/Window.h>
#include <Utils/DebugHandler.h>
#include <Utils/FileReader.h>
#include "../Utils/ServiceLocator.h"
#include "../Utils/MapUtils.h"

namespace fs = std::filesystem;

void CameraOrbital::Init()
{
    InputManager* inputManager = ServiceLocator::GetInputManager();
    inputManager->RegisterMouseScrollCallback("CameraOrbital Mouse Scroll", [this](Window* window, f32 xPos, f32 yPos)
    {
        if (!IsActive())
            return;

        _distance = glm::clamp(_distance - yPos, 5.f, 30.f);
    });

    inputManager->RegisterMousePositionCallback("CameraOrbital MouseLook", [this, inputManager](Window* window, f32 xPos, f32 yPos)
    {
        if (!IsActive())
            return;

        if (_captureMouse)
        {
            vec2 mousePosition = vec2(xPos, yPos);
            if (_captureMouseHasMoved)
            {
                vec2 deltaPosition = _prevMousePosition - mousePosition;

                _yaw -= deltaPosition.x * _mouseSensitivity;
                _pitch = Math::Clamp(_pitch + (deltaPosition.y * _mouseSensitivity), -89.0f, 89.0f);;

                /* TODO: Add proper collision for the camera so we don't go through the ground
                         the below code will do a quick test for the pitch but not the yaw.
                         We also need to use "distToCollision" to possibly add an offset.

                f32 tmpPitch = Math::Clamp(_pitch + (deltaPosition.y * _mouseSensitivity), -89.0f, 89.0f);
                f32 dist = tmpPitch - _pitch;

                mat4x4 rotationMatrix = glm::yawPitchRoll(glm::radians(_yaw), glm::radians(_pitch), 0.0f);
                vec3 t = vec3(_rotationMatrix * vec4(vec3(0, 0, _distance), 0.0f));
                vec3 position = _position + t;

                Geometry::AABoundingBox box;
                box.min = position - vec3(0.5f, 2.5f, 0.5f);
                box.max = position + vec3(0.5f, 2.5f, 0.5f);

                Geometry::Triangle triangle;
                f32 height = 0;
                f32 distToCollision = 0;

                if (!Terrain::MapUtils::Intersect_AABB_TERRAIN_SWEEP(box, triangle, height, dist, distToCollision))
                {
                    _pitch = tmpPitch;
                }*/
            }
            else
                _captureMouseHasMoved = true;

            _prevMousePosition = mousePosition;
        }
    });
    inputManager->RegisterKeybind("CameraOrbital Left Mouseclick", GLFW_MOUSE_BUTTON_1, KEYBIND_ACTION_CLICK, KEYBIND_MOD_ANY, [this, inputManager](Window* window, std::shared_ptr<Keybind> keybind)
    {
        if (!IsActive())
            return false;

        if (inputManager->IsKeyPressed("CameraOrbital Right Mouseclick"_h))
            return false;

        if (keybind->state == GLFW_PRESS)
        {
            if (!_captureMouse)
            {
                _captureMouse = true;
                _prevMousePosition = vec2(inputManager->GetMousePositionX(), inputManager->GetMousePositionY());

                glfwSetInputMode(window->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        else
        {
            if (_captureMouse)
            {
                _captureMouse = false;
                _captureMouseHasMoved = false;

                glfwSetInputMode(window->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }

        return true;
    });
    inputManager->RegisterKeybind("CameraOrbital Right Mouseclick", GLFW_MOUSE_BUTTON_2, KEYBIND_ACTION_CLICK, KEYBIND_MOD_ANY, [this, inputManager](Window* window, std::shared_ptr<Keybind> keybind)
    {
        if (!IsActive())
            return false;

        if (inputManager->IsKeyPressed("CameraOrbital Left Mouseclick"_h))
            return false;

        if (keybind->state == GLFW_PRESS)
        {
            if (!_captureMouse)
            {
                _captureMouse = true;
                _prevMousePosition = vec2(inputManager->GetMousePositionX(), inputManager->GetMousePositionY());

                glfwSetInputMode(window->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        else
        {
            if (_captureMouse)
            {
                _captureMouse = false;
                _captureMouseHasMoved = false;

                glfwSetInputMode(window->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }

        return true;
    });

    UpdateCameraVectors();
}

void CameraOrbital::Enabled()
{
}

void CameraOrbital::Disabled()
{
    _captureMouse = false;
    _captureMouseHasMoved = false;
    glfwSetInputMode(_window->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void CameraOrbital::Update(f32 deltaTime, float fovInDegrees, float aspectRatioWH)
{
    // Compute matrices
    _rotationMatrix = glm::yawPitchRoll(glm::radians(_yaw), glm::radians(_pitch), 0.0f);

    vec3 t = vec3(_rotationMatrix * vec4(vec3(0, 0, _distance), 0.0f));
    vec3 position = _position + t;

    _viewMatrix = glm::lookAt(position, _position, worldUp);
    _projectionMatrix = glm::perspective(glm::radians(fovInDegrees), aspectRatioWH, _farClip, _nearClip);
    _viewProjectionMatrix = _projectionMatrix * _viewMatrix;

    UpdateCameraVectors();
    UpdateFrustumPlanes(glm::transpose(_viewProjectionMatrix));
}
