#pragma once
#include <NovusTypes.h>

namespace Renderer
{
    // This struct needs to be 256 bytes padded
    struct InstanceData
    {
        vec4 colorMultiplier = vec4(1,1,1,1); // 16 bytes
        mat4x4 modelMatrix = mat4x4(1.0f); // 64 bytes
        u8 padding[176] = {};
    };
}