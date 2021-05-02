#include "Renderer.h"
#include "RenderGraph.h"

#include <Memory/Allocator.h>

namespace Renderer
{
    Renderer::~Renderer()
    {
    }

    RenderGraph& Renderer::CreateRenderGraph(RenderGraphDesc& desc)
    {
        RenderGraph* renderGraph = Memory::Allocator::New<RenderGraph>(desc.allocator, desc.allocator, this);
        renderGraph->Init(desc);

        return *renderGraph;
    }

    BufferID Renderer::CreateBuffer(BufferID bufferID, BufferDesc& desc)
    {
        if (bufferID != BufferID::Invalid())
        {
            QueueDestroyBuffer(bufferID);
        }

        return CreateBuffer(desc);
    }

    BufferID Renderer::CreateAndFillBuffer(BufferID bufferID, BufferDesc desc, void* data, size_t dataSize)
    {
        if (bufferID != BufferID::Invalid())
        {
            QueueDestroyBuffer(bufferID);
        }

        return CreateAndFillBuffer(desc, data, dataSize);
    }

    BufferID Renderer::CreateAndFillBuffer(BufferDesc desc, void* data, size_t dataSize)
    {
        // Create actual buffer
        desc.usage |= BufferUsage::TRANSFER_DESTINATION; // If we're supposed to stage into it, we have to make sure it's a transfer destination
        BufferID bufferID = CreateBuffer(desc);

        // Create staging buffer
        desc.name += "Staging";
        desc.usage = BufferUsage::TRANSFER_SOURCE; // The staging buffer needs to be transfer source
        desc.cpuAccess = BufferCPUAccess::WriteOnly;

        BufferID stagingBuffer = CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = MapBuffer(stagingBuffer);
        memcpy(dst, data, dataSize);
        UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        CopyBuffer(bufferID, 0, stagingBuffer, 0, desc.size);

        return bufferID;
    }

    BufferID Renderer::CreateAndFillBuffer(BufferID bufferID, BufferDesc desc, const std::function<void(void*)>& callback)
    {
        if (bufferID != BufferID::Invalid())
        {
            QueueDestroyBuffer(bufferID);
        }
        
        return CreateAndFillBuffer(desc, callback);
    }

    BufferID Renderer::CreateAndFillBuffer(BufferDesc desc, const std::function<void(void*)>& callback)
    {
        // Create actual buffer
        desc.usage |= BufferUsage::TRANSFER_DESTINATION; // If we're supposed to stage into it, we have to make sure it's a transfer destination
        BufferID bufferID = CreateBuffer(desc);

        // Create staging buffer
        desc.name += "Staging";
        desc.usage = BufferUsage::TRANSFER_SOURCE; // The staging buffer needs to be transfer source
        desc.cpuAccess = BufferCPUAccess::WriteOnly;

        BufferID stagingBuffer = CreateBuffer(desc);

        // Upload to staging buffer
        void* dst = MapBuffer(stagingBuffer);
        callback(dst);
        UnmapBuffer(stagingBuffer);

        // Queue destroy staging buffer
        QueueDestroyBuffer(stagingBuffer);
        // Copy from staging buffer to buffer
        CopyBuffer(bufferID, 0, stagingBuffer, 0, desc.size);

        return bufferID;
    }
}