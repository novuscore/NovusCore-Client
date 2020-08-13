#pragma once
#include "Renderer.h"
#include "FrameResource.h"
#include "Descriptors/BufferDesc.h"

namespace Renderer
{
    template <typename T>
    struct Buffer
    {
        T resource;

        Buffer(Renderer* renderer, const std::string& name, BufferUsage usage, BufferCPUAccess cpuAccess)
            : _renderer(renderer)
        {
            BufferDesc desc;
            desc.name = name;
            desc.usage = usage;
            desc.cpuAccess = cpuAccess;
            desc.size = sizeof(T);

            for (u32 i = 0; i < _buffers.Num; ++i)
            {
                _buffers.Get(i) = renderer->CreateBuffer(desc);
            }
        }

        size_t GetSize()
        {
            return sizeof(T);
        }

        void Apply(u32 frameIndex)
        {
            const BufferID buffer = GetBuffer(frameIndex);
            void* mappedMemory = _renderer->MapBuffer(buffer);
            memcpy(mappedMemory, &resource, sizeof(resource));
            _renderer->UnmapBuffer(buffer);
        }

        void ApplyAll()
        {
            for (u32 i = 0; i < _buffers.Num; i++)
            {
                Apply(i);
            }
        }

        BufferID GetBuffer(u32 frameIndex)
        {
            return _buffers.Get(frameIndex);
        }

    private:
        Renderer* _renderer;
        FrameResource<BufferID, 2> _buffers;
    };
}