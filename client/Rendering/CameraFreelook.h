#pragma once
#include <NovusTypes.h>
#include "Camera.h"

class CameraFreeLook : public Camera
{
public:
    CameraFreeLook(const vec3& pos) : Camera(false) { _position = pos; }
    
    void Init() override;
    void Enabled() override;
    void Disabled() override;
    void Update(f32 deltaTime, float fovInDegrees, float aspectRatioWH) override;
};