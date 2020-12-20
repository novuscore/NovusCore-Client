#include "Camera.h"
#include <filesystem>
#include <Utils/FileReader.h>
#include "../Utils/ServiceLocator.h"

namespace fs = std::filesystem;

AutoCVar_Float CVAR_CameraNearClip("camera.nearClip", "Sets the near clip of the camera", 1.0f);
AutoCVar_Float CVAR_CameraFarClip("camera.farClip", "Sets the far clip of the camera", 100000.0f);

Camera::Camera(bool isActive) : _active(isActive), _window(nullptr) { }

bool Camera::LoadFromFile(std::string filename)
{
    fs::path filePath = fs::current_path().append("Data/CameraSaves").append(filename).make_preferred();
    if (!fs::exists(filePath))
    {
        printf("Failed to OPEN Camera Save file. Check admin permissions\n");
        return false;
    }

    FileReader reader(filePath.string(), filename);
    if (!reader.Open())
        return false;

    size_t length = reader.Length();
    if (length < sizeof(CameraSaveData))
        return false;

    Bytebuffer buffer(nullptr, length);
    reader.Read(&buffer, length);
    reader.Close();

    CameraSaveData saveData;
    if (!buffer.Get(saveData))
        return false;

    _position = saveData.position;
    _yaw = saveData.yaw;
    _pitch = saveData.pitch;
    _movementSpeed = saveData.movement;

    UpdateCameraVectors();

    return true;
}

bool Camera::SaveToFile(std::string filename)
{
    // Create a file
    fs::path outputPath = fs::current_path().append("Data/CameraSaves").append(filename).make_preferred();
    fs::create_directories(outputPath.parent_path());

    std::ofstream output(outputPath, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    if (!output)
    {
        printf("Failed to create Camera Save file. Check admin permissions\n");
        return false;
    }

    output.write(reinterpret_cast<char const*>(&_position), sizeof(vec3)); // Write camera position
    output.write(reinterpret_cast<char const*>(&_yaw), sizeof(f32)); // Write camera yaw
    output.write(reinterpret_cast<char const*>(&_pitch), sizeof(f32)); // Write camera pitch
    output.write(reinterpret_cast<char const*>(&_movementSpeed), sizeof(f32)); // Write camera speed
    output.close();
    return true;
}

void Camera::UpdateCameraVectors()
{
    _left = -_rotationMatrix[0];
    _up = _rotationMatrix[1];
    _front = -_rotationMatrix[2];
}

void Camera::UpdateFrustumPlanes(const mat4x4& m)
{
    _frustumPlanes[(size_t)FrustumPlane::Left] = (m[3] + m[0]);
    _frustumPlanes[(size_t)FrustumPlane::Right] = (m[3] - m[0]);
    _frustumPlanes[(size_t)FrustumPlane::Bottom] = (m[3] + m[1]);
    _frustumPlanes[(size_t)FrustumPlane::Top] = (m[3] - m[1]);
    _frustumPlanes[(size_t)FrustumPlane::Near] = (m[3] + m[2]);
    _frustumPlanes[(size_t)FrustumPlane::Far] = (m[3] - m[2]);
}