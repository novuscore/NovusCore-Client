#pragma once
#include <NovusTypes.h>

enum class FrustumPlane
{
    Left,
    Right,
    Bottom,
    Top,
    Near,
    Far,
};

struct CameraSaveData
{
    vec3 position;
    f32 yaw;
    f32 pitch;
    f32 movement;
};

constexpr vec3 worldUp = vec3(0, 1, 0);

class Window;
class Camera
{
public:
    Camera(bool isActive);

    virtual void Init() = 0;
    virtual void Enabled() = 0;
    virtual void Disabled() = 0;
    virtual void Update(f32 deltaTime, float fovInDegrees, float aspectRatioWH) = 0;

    void SetActive(bool state) { _active = state; }
    bool IsActive() { return _active; }

    void SetNearClip(f32 value) { _nearClip = value; }
    f32 GetNearClip() { return _nearClip; }

    void SetFarClip(f32 value) { _farClip = value; }
    f32 GetFarClip() { return _farClip; }

    bool LoadFromFile(std::string filename);
    bool SaveToFile(std::string filename);

    __forceinline const mat4x4& GetViewMatrix() const { return _viewMatrix; }
    __forceinline const mat4x4& GetProjectionMatrix() const { return _projectionMatrix; }
    __forceinline const mat4x4& GetViewProjectionMatrix() const { return _viewProjectionMatrix; }
    __forceinline const vec4* GetFrustumPlanes() const { return _frustumPlanes; }

    void SetPosition(vec3 position)
    {
        _position = position;
    }
    vec3 GetPosition() const { return _position; }
    
    void SetPreviousMousePosition(vec2 position)
    {
        _prevMousePosition = position;
    }
    vec2 GetPreviousMousePosition() const { return _prevMousePosition; }

    void SetYaw(f32 value) { _yaw = value; }
    f32 GetYaw() { return _yaw; }
    void SetPitch(f32 value) { _pitch = value; }
    f32 GetPitch() { return _pitch; }
    vec3 GetRotation() const { return vec3(0, _yaw, _pitch); }

    void SetMouseCaptured(bool state) { _captureMouse = state; }
    bool IsMouseCaptured() const { return _captureMouse; }

    void SetCapturedMouseMoved(bool state) { _captureMouseHasMoved = state; }
    bool GetCapturedMouseMoved() const { return _captureMouseHasMoved; }

protected:
    void UpdateCameraVectors();
    void UpdateFrustumPlanes(const mat4x4& m);

protected:
    Window* _window = nullptr;

    bool _active;
    f32 _nearClip = 1.0f;
    f32 _farClip = 100000.0f;

    vec3 _position = vec3(0, 0, 0);
    
    // Euler Angles
    f32 _yaw = 0.0f;
    f32 _pitch = 0.0f;

    // Rotation Matrix
    mat4x4 _rotationMatrix = mat4x4();
    mat4x4 _viewMatrix = mat4x4();
    mat4x4 _projectionMatrix = mat4x4();
    mat4x4 _viewProjectionMatrix = mat4x4();

    // Direction vectors
    vec3 _front = vec3(0, 0, 0);
    vec3 _up = vec3(0, 0, 0);
    vec3 _left = vec3(0, 0, 0);

    // Mouse States
    vec2 _prevMousePosition = vec2(0, 0);
    bool _captureMouse = false;
    bool _captureMouseHasMoved = false;
    f32 _movementSpeed = 50.0f;
    f32 _mouseSensitivity = 0.05f;

    vec4 _frustumPlanes[6] = 
    { 
        vec4(0, 0, 0, 0), vec4(0, 0, 0, 0), vec4(0, 0, 0, 0),
        vec4(0, 0, 0, 0), vec4(0, 0, 0, 0), vec4(0, 0, 0, 0) 
    };
};