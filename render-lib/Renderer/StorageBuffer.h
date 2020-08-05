#pragma once
#include "BufferBackend.h"

namespace Renderer
{
    /*struct IStorageBuffer
    {
        virtual size_t GetSize() = 0;
        virtual void* GetDescriptor(u32 frameIndex) = 0;
        virtual void* GetBuffer(u32 frameIndex) = 0;
    };

    template <typename T>
    struct StorageBuffer : IStorageBuffer
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
        StorageBuffer() {}; // This has to be created through Renderer::CreateStorageBuffer<T>

        friend class Renderer;
    };*/
}