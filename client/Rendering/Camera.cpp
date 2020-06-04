#include "Camera.h"
#include <windows.h>
#include <InputManager.h>
#include "../Utils/ServiceLocator.h"
#include <glm/gtx/rotate_vector.hpp>
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
            _pitch += deltaPosition.y * _mouseSensitivity;

            _prevMousePosition = mousePosition;
        }
    });

    inputManager->RegisterKeybind("Left Mouseclick", GLFW_MOUSE_BUTTON_1, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this, inputManager](Window* window, std::shared_ptr<Keybind> keybind)
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

    inputManager->RegisterKeybind("IncreaseCameraSpeed", GLFW_KEY_KP_ADD, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        _movementSpeed += 5.0f;
        return true;
    });

    inputManager->RegisterKeybind("DecreaseCameraSpeed", GLFW_KEY_KP_SUBTRACT, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        _movementSpeed -= 5.0f;
        if (_movementSpeed < 7.3333f)
        {
            _movementSpeed = 7.3333f;
        }
        return true;
    });

    UpdateCameraVectors();
}

void Camera::Update(f32 deltaTime)
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
        _position += _up * _movementSpeed * deltaTime;
    }
    if (inputManager->IsKeyPressed("Camera Down"_h))
    {
        _position -= _up * _movementSpeed * deltaTime;
    }

    // Constrain pitch
    if (_pitch > 89.0f)
    {
        _pitch = 89.0f;
    }
    if (_pitch < -89.0f)
    {
        _pitch = -89.0f;
    }

    UpdateCameraVectors();
}

mat4x4 Camera::GetViewMatrix() const
{
    return glm::lookAt(_position, _position + _front, _up);
}

mat4x4 Camera::GetCameraMatrix() const
{
    return glm::inverse(GetViewMatrix());
}

void Camera::UpdateCameraVectors()
{
    const f32 cosPitch = Math::Cos(glm::radians(_pitch));

    // Calculate the new front vector
    vec3 front;
    front.x = Math::Cos(glm::radians(_yaw)) * cosPitch;
    front.y = Math::Sin(glm::radians(_pitch));
    front.z = Math::Sin(glm::radians(_yaw)) * cosPitch;

    _front = glm::normalize(front);

    // Recalculate the left and up vector
    _left = glm::normalize(glm::cross(_front, _worldUp));
    _up = glm::normalize(glm::cross(_left, _front));
}

