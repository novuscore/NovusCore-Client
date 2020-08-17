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

class Window;
class InputBinding;
class Camera
{
public:
    Camera(const vec3& pos);
    
    void Init();
    void Update(f32 deltaTime, float fovInDegrees, float aspectRatioWH);

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
    vec3 GetRotation() const { return vec3(0, _yaw, _pitch); }
    bool IsMouseCaptured() const { return _captureMouse; }

private:
    void UpdateCameraVectors();
    void UpdateFrustumPlanes(const mat4x4& m);

private:
    Window* _window;
    vec3 _position;

    // Rotation Matrix
    mat4x4 _rotationMatrix;
    mat4x4 _viewMatrix;
    mat4x4 _projectionMatrix;
    mat4x4 _viewProjectionMatrix;

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

    vec4 _frustumPlanes[6];
};