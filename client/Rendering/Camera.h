#pragma once
#include <NovusTypes.h>

class Window;
class InputBinding;
class Camera
{
public:
    Camera(const vec3& pos);
    
    void Init();
    void Update(f32 deltaTime);

    mat4x4 GetViewMatrix() const;
    mat4x4 GetCameraMatrix() const;

private:
    void UpdateCameraVectors();

private:
    Window* _window;
    vec3 _position;

    // Direction vectors
    vec3 _front;
    vec3 _up;
    vec3 _left;
    vec3 _worldUp;

    // Euler Angles
    f32 _yaw = 0.0f;
    f32 _pitch = 0.0f;

    // Mouse position
    vec2 _prevMousePosition;
    bool _captureMouse = false;

    f32 _movementSpeed = 50.0f;
    f32 _mouseSensitivity = 0.05f;
    f32 _lastDeltaTime = 0.0f;
};