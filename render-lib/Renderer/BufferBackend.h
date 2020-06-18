#pragma once

namespace Renderer
{
    namespace Backend
    {
        struct BufferBackend
        {
            enum Type
            {
                TYPE_CONSTANT_BUFFER,
                TYPE_STORAGE_BUFFER
            };

            virtual ~BufferBackend() {}
            virtual void Apply(u32 frameIndex, void* data, size_t size) = 0;
            virtual void* GetDescriptor(u32 frameIndex) = 0;
            virtual void* GetBuffer(u32 frameIndex) = 0;
        };
    }
}