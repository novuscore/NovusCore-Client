#include "CameraFreelook.h"
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
#include <CVar/CVarSystem.h>

namespace fs = std::filesystem;

AutoCVar_Float CVAR_CameraSpeed("camera.speed", "Camera Freelook Speed", 7.1111f);

void CameraFreeLook::Init()
{
    InputManager* inputManager = ServiceLocator::GetInputManager();
    inputManager->RegisterKeybind("CameraFreeLook Forward", GLFW_KEY_W, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);
    inputManager->RegisterKeybind("CameraFreeLook Backward", GLFW_KEY_S, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);
    inputManager->RegisterKeybind("CameraFreeLook Left", GLFW_KEY_A, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);
    inputManager->RegisterKeybind("CameraFreeLook Right", GLFW_KEY_D, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);
    inputManager->RegisterKeybind("CameraFreeLook Up", GLFW_KEY_SPACE, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);
    inputManager->RegisterKeybind("CameraFreeLook Down", GLFW_KEY_LEFT_CONTROL, KEYBIND_ACTION_PRESS | KEYBIND_ACTION_REPEAT, KEYBIND_MOD_ANY);

    inputManager->RegisterKeybind("CameraFreeLook ToggleMouseCapture", GLFW_KEY_TAB, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        if (!IsActive())
            return false;

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
    inputManager->RegisterKeybind("CameraFreeLook Right Mouseclick", GLFW_MOUSE_BUTTON_2, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this, inputManager](Window* window, std::shared_ptr<Keybind> keybind)
    {
        if (!IsActive())
            return false;

        if (!_captureMouse)
        {
            _captureMouse = true;
            _prevMousePosition = vec2(inputManager->GetMousePositionX(), inputManager->GetMousePositionY());

            glfwSetInputMode(window->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            NC_LOG_MESSAGE("Mouse captured because of mouseclick!");
        }
        return true;
    });
    inputManager->RegisterMousePositionCallback("CameraFreeLook MouseLook", [this](Window* window, f32 xPos, f32 yPos)
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
                _pitch = Math::Clamp(_pitch - (deltaPosition.y * _mouseSensitivity), -89.0f, 89.0f);
            }
            else
                _captureMouseHasMoved = true;

            _prevMousePosition = mousePosition;
        }
    });

    inputManager->RegisterKeybind("IncreaseCameraSpeed", GLFW_KEY_PAGE_UP, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        if (!IsActive())
            return false;

        f32 newSpeed = CVAR_CameraSpeed.GetFloat() + 10.0f;
        CVAR_CameraSpeed.Set(newSpeed);
        return true;
    });
    inputManager->RegisterKeybind("DecreaseCameraSpeed", GLFW_KEY_PAGE_DOWN, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        if (!IsActive())
            return false;

        f32 newSpeed = CVAR_CameraSpeed.GetFloat() - 10.0f;
        newSpeed = glm::max(newSpeed, 7.1111f);
        CVAR_CameraSpeed.Set(newSpeed);
        return true;
    });
    
    inputManager->RegisterKeybind("SaveCameraDefault", GLFW_KEY_F9, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        if (!IsActive())
            return false;

        SaveToFile("freelook.cameradata");
        return true;
    });  
    inputManager->RegisterKeybind("LoadCameraDefault", GLFW_KEY_F10, KEYBIND_ACTION_PRESS, KEYBIND_MOD_ANY, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        if (!IsActive())
            return false;

        LoadFromFile("freelook.cameradata");
        return true;
    });

    UpdateCameraVectors();
}

void CameraFreeLook::Enabled()
{
    _captureMouseHasMoved = false;
    glfwSetInputMode(_window->GetWindow(), GLFW_CURSOR, _captureMouse ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void CameraFreeLook::Disabled()
{
    glfwSetInputMode(_window->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void CameraFreeLook::Update(f32 deltaTime, float fovInDegrees, float aspectRatioWH)
{
    if (!IsActive())
        return;

    InputManager* inputManager = ServiceLocator::GetInputManager();
    f32 speed = CVAR_CameraSpeed.GetFloat();

    // Movement
    if (inputManager->IsKeyPressed("CameraFreeLook Forward"_h))
    {
        _position += _front * speed * deltaTime;
    }
    if (inputManager->IsKeyPressed("CameraFreeLook Backward"_h))
    {
        _position -= _front * speed * deltaTime;
    }
    if (inputManager->IsKeyPressed("CameraFreeLook Left"_h))
    {
        _position += _left * speed * deltaTime;
    }
    if (inputManager->IsKeyPressed("CameraFreeLook Right"_h))
    {
        _position -= _left * speed * deltaTime;
    }
    if (inputManager->IsKeyPressed("CameraFreeLook Up"_h))
    {
        _position += worldUp * speed * deltaTime;
    }
    if (inputManager->IsKeyPressed("CameraFreeLook Down"_h))
    {
        _position -= worldUp * speed * deltaTime;
    }

    // Compute matrices
    _rotationMatrix = glm::yawPitchRoll(glm::radians(_yaw), glm::radians(_pitch), 0.0f);
    const mat4x4 cameraMatrix = glm::translate(mat4x4(1.0f), _position) * _rotationMatrix;
    _viewMatrix = glm::inverse(cameraMatrix);

    _projectionMatrix = glm::perspective(glm::radians(fovInDegrees), aspectRatioWH, _farClip, _nearClip);
    _viewProjectionMatrix = _projectionMatrix * _viewMatrix;

    UpdateCameraVectors();
    UpdateFrustumPlanes(glm::transpose(_viewProjectionMatrix));
}