#pragma once
#include <NovusTypes.h>
#include "Camera.h"

class CameraOrbital : public Camera
{
public:
    CameraOrbital() : Camera(true) { }
    
    void Init() override;
    void Enabled() override;
    void Disabled() override;
    void Update(f32 deltaTime, float fovInDegrees, float aspectRatioWH) override;

    void SetZoomLevel(u8 zoomLevel) { _zoomLevel = zoomLevel; }
    u8 GetZoomLevel() { return _zoomLevel; }

    void SetDistance(f32 distance) { _distance = distance; }
    f32 GetDistance() { return _distance; }

private:
    u8 _zoomLevel = 1;
    f32 _distance = 15;
};