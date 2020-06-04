#pragma once
#include <NovusTypes.h>
#include "ConstantBuffer.h"

namespace Renderer
{
    class Renderer;

    struct ModelCB
    {
        vec4 colorMultiplier = vec4(1, 1, 1, 1); // 16 bytes
        mat4x4 modelMatrix = mat4x4(1.0f); // 64 bytes

        u8 padding[176] = {};
    };

    class InstanceData
    {
    public:
        vec4 colorMultiplier = vec4(1, 1, 1, 1);
        mat4x4 modelMatrix = mat4x4(1.0f);

        void Init(Renderer* renderer);
        void Apply(u32 frameIndex);
        void* GetGPUResource(u32 frameIndex);
    private:
        ConstantBuffer<ModelCB>* _cb = nullptr;
    };
}