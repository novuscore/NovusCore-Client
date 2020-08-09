#include "Camera.h"
#include <windows.h>
#include <InputManager.h>
#include "../Utils/ServiceLocator.h"
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <GLFW/glfw3.h>
#include <Window/Window.h>
#include <Utils/DebugHandler.h>

Camera::Camera(const vec3& pos)
{
    _position = pos;
    _worldUp = vec3(0.0f, 1.0f, 0.0f);
}

void Camera::Init()
{
    _window = ServiceLocator::GetWindow();

    InputManager* inputManager = ServiceLocator::GetInputManager();

    inputManager->RegisterKeybind("Camera Forward", GLFW_KEY_W, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);
    inputManager->RegisterKeybind("Camera Backward", GLFW_KEY_S, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);
    inputManager->RegisterKeybind("Camera Left", GLFW_KEY_A, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);
    inputManager->RegisterKeybind("Camera Right", GLFW_KEY_D, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);
    inputManager->RegisterKeybind("Camera Up", GLFW_KEY_SPACE, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);
    inputManager->RegisterKeybind("Camera Down", GLFW_KEY_LEFT_CONTROL, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);

    inputManager->RegisterKeybind("Camera Rotate Up", GLFW_KEY_DOWN, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);
    inputManager->RegisterKeybind("Camera Rotate Down", GLFW_KEY_UP, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);
    inputManager->RegisterKeybind("Camera Rotate Left", GLFW_KEY_LEFT, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);
    inputManager->RegisterKeybind("Camera Rotate Right", GLFW_KEY_RIGHT, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);

    inputManager->RegisterKeybind("ToggleMouseCapture", GLFW_KEY_TAB, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        _captureMouse = !_captureMouse;

        GLFWwindow* glfwWindow = _window->GetWindow();
        if (_captureMouse)
        {
            glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            NC_LOG_MESSAGE("Mouse captured because of tab!");
        }
        else
        {
            glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            NC_LOG_MESSAGE("Mouse released because of tab!");
        }
        return true;
    });

    inputManager->RegisterMousePositionCallback("MouseLook", [this](Window* window, f32 xPos, f32 yPos)
    {
        if (_captureMouse)
        {
            vec2 mousePosition = vec2(xPos, yPos);
            vec2 deltaPosition = _prevMousePosition - mousePosition;

            _yaw += deltaPosition.x * _mouseSensitivity;
            _pitch -= deltaPosition.y * _mouseSensitivity;

            _prevMousePosition = mousePosition;
        }
    });

    inputManager->RegisterKeybind("Right Mouseclick", GLFW_MOUSE_BUTTON_2, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this, inputManager](Window* window, std::shared_ptr<Keybind> keybind)
    {
        if (!_captureMouse)
        {
            _captureMouse = true;
            _prevMousePosition = vec2(inputManager->GetMousePositionX(), inputManager->GetMousePositionY());

            glfwSetInputMode(window->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            NC_LOG_MESSAGE("Mouse captured because of mouseclick!");
        }
        return true;
    });

    inputManager->RegisterKeybind("IncreaseCameraSpeed", GLFW_KEY_PAGE_UP, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        _movementSpeed += 5.0f;
        return true;
    });

    inputManager->RegisterKeybind("DecreaseCameraSpeed", GLFW_KEY_PAGE_DOWN, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        _movementSpeed -= 5.0f;
        if (_movementSpeed < 7.3333f)
        {
            _movementSpeed = 7.3333f;
        }
        return true;
    });

    UpdateCameraVectors();

    ServiceLocator::SetCamera(this);
}

void Camera::Update(f32 deltaTime, float fovInDegrees, float aspectRatioWH)
{
    InputManager* inputManager = ServiceLocator::GetInputManager();
    _lastDeltaTime = deltaTime;

    // Movement
    if (inputManager->IsKeyPressed("Camera Forward"_h))
    {
        _position += _front * _movementSpeed * deltaTime;
    }
    if (inputManager->IsKeyPressed("Camera Backward"_h))
    {
        _position -= _front * _movementSpeed * deltaTime;
    }
    if (inputManager->IsKeyPressed("Camera Left"_h))
    {
        _position += _left * _movementSpeed * deltaTime;
    }
    if (inputManager->IsKeyPressed("Camera Right"_h))
    {
        _position -= _left * _movementSpeed * deltaTime;
    }
    if (inputManager->IsKeyPressed("Camera Up"_h))
    {
        _position += _worldUp * _movementSpeed * deltaTime;
    }
    if (inputManager->IsKeyPressed("Camera Down"_h))
    {
        _position -= _worldUp * _movementSpeed * deltaTime;
    }

    // Constrain pitch
    _pitch = Math::Clamp(_pitch, -89.0f, 89.0f);

    // Compute matrices
    _rotationMatrix = glm::yawPitchRoll(glm::radians(_yaw), glm::radians(_pitch), 0.0f);
    const mat4x4 cameraMatrix = glm::translate(mat4x4(1.0f), _position) * _rotationMatrix;
    _viewMatrix = glm::inverse(cameraMatrix);

    const f32 nearClip = 1.0f;
    const f32 farClip = 100000.0f;

    _projectionMatrix = glm::perspective(glm::radians(fovInDegrees), aspectRatioWH, nearClip, farClip);
    _viewProjectionMatrix = _projectionMatrix * _viewMatrix;

    UpdateCameraVectors();
    UpdateFrustumPlanes(glm::transpose(_viewProjectionMatrix));
}

void Camera::UpdateCameraVectors()
{
    _left = -_rotationMatrix[0];
    _up = _rotationMatrix[1];
    _front = -_rotationMatrix[2];
}

void Camera::UpdateFrustumPlanes(const mat4x4& m)
{
    _frustumPlanes[(size_t)FrustumPlane::Left]   = (m[3] + m[0]);
    _frustumPlanes[(size_t)FrustumPlane::Right]  = (m[3] - m[0]);
    _frustumPlanes[(size_t)FrustumPlane::Bottom] = (m[3] + m[1]);
    _frustumPlanes[(size_t)FrustumPlane::Top]    = (m[3] - m[1]);
    _frustumPlanes[(size_t)FrustumPlane::Near]   = (m[3] + m[2]);
    _frustumPlanes[(size_t)FrustumPlane::Far]    = (m[3] - m[2]);
}
