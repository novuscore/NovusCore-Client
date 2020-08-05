#pragma once
#include "BufferBackend.h"

namespace Renderer
{
    /*struct IConstantBuffer
    {
        virtual size_t GetSize() = 0;
        virtual void* GetDescriptor(u32 frameIndex) = 0;
        virtual void* GetBuffer(u32 frameIndex) = 0;
    };

    template <typename T>
    struct ConstantBuffer : IConstantBuffer
    {
        T resource;

        virtual size_t GetSize() override
        {
            return sizeof(T);
        }

        void Apply(u32 frameIndex)
        {
            backend->Apply(frameIndex, &resource, GetSize());
        }

        void ApplyAll()
        {
            for (u32 i = 0; i < 2; i++)
            {
                Apply(i);
            }
        }

        virtual void* GetDescriptor(u32 frameIndex) override
        {
            return backend->GetDescriptor(frameIndex);
        }

        virtual void* GetBuffer(u32 frameIndex) override
        {
            return backend->GetBuffer(frameIndex);
        }

        Backend::BufferBackend* backend = nullptr;

    protected:
        ConstantBuffer() {}; // This has to be created through Renderer::CreateConstantBuffer<T>

        friend class Renderer;
    };*/
}