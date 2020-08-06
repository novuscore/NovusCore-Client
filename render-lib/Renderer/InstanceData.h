#pragma once
#include <NovusTypes.h>


namespace Renderer
{
    class Renderer;

    struct ModelCB
    {
        vec4 colorMultiplier = vec4(1, 1, 1, 1); // 16 bytes
        mat4x4 modelMatrix = mat4x4(1.0f); // 64 bytes

        u8 padding[176] = {};
    };

    struct InstanceDataBackend
    {
        //ConstantBuffer<ModelCB>* cb = nullptr;
        void* optional = nullptr;
    };

    class InstanceData
    {
    public:
        vec4 colorMultiplier = vec4(1, 1, 1, 1);
        mat4x4 modelMatrix = mat4x4(1.0f);

        void Init(Renderer* renderer);
        void Apply(u32 frameIndex);
        void ApplyAll();
        void* GetDescriptor(u32 frameIndex);
        void* GetBuffer(u32 frameIndex);

        template<typename T>
        void SetOptional(T* optional)
        {
            _backend.optional = static_cast<void*>(optional);
        }

        template<typename T>
        T* GetOptional()
        {
            return static_cast<T*>(_backend.optional);
        }

    private:
        InstanceDataBackend _backend;
    };
}