#pragma once
#include <NovusTypes.h>
#include "Camera.h"

class CameraFreeLook : public Camera
{
public:
    CameraFreeLook() : Camera(false) { _position = vec3(0.0f, 0.0f, 0.0f); }
    CameraFreeLook(const vec3& pos) : Camera(false) { _position = pos; }
    
    void Init() override;
    void Enabled() override;
    void Disabled() override;
    void Update(f32 deltaTime, float fovInDegrees, float aspectRatioWH) override;
};