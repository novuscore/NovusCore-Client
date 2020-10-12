#pragma once
#include <NovusTypes.h>
#include "BackendDispatch.h"
#include <vector>
#include <Memory/StackAllocator.h>
#include <Containers/DynamicArray.h>

#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
#include <Renderer/DescriptorSet.h>

#include "Descriptors/BufferDesc.h"
#include "Descriptors/ModelDesc.h"
#include "Descriptors/CommandListDesc.h"
#include "Descriptors/GraphicsPipelineDesc.h"
#include "Descriptors/ComputePipelineDesc.h"
#include "Descriptors/GPUSemaphoreDesc.h"

#define COMMANDLIST_DEBUG_IMMEDIATE_MODE 0 // This makes it easier to debug the renderer by providing better callstacks if it asserts or crashes inside of render-lib

#if TRACY_ENABLE
#define GPU_SCOPED_PROFILER_ZONE(commandList, name) \
    TracySourceLocation(name, #name, tracy::Color::Yellow2); \
    Renderer::ScopedGPUProfilerZone nameZone(commandList, &name);
#else
namespace tracy
{
    struct SourceLocationData;
}
#define GPU_SCOPED_PROFILER_ZONE(commandList, name) ((void)0)
#endif

namespace Renderer
{
    class DescriptorSet;
    class CommandList;

    struct ScopedGPUProfilerZone
    {
        ScopedGPUProfilerZone(CommandList& commandList, const tracy::SourceLocationData* sourceLocation);
        ~ScopedGPUProfilerZone();

    private:
        CommandList& _commandList;
    };

    class CommandList
    {
    public:
        CommandList(Renderer* renderer, Memory::Allocator* allocator);

        void MarkFrameStart(u32 frameIndex);

        void BeginTrace(const tracy::SourceLocationData* sourceLocation);
        void EndTrace();

        void PushMarker(std::string marker, Color color);
        void PopMarker();

        void BeginPipeline(GraphicsPipelineID pipelineID);
        void EndPipeline(GraphicsPipelineID pipelineID);

        void BeginPipeline(ComputePipelineID pipelineID);
        void EndPipeline(ComputePipelineID pipelineID);

        void BindDescriptorSet(DescriptorSetSlot slot, DescriptorSet* descriptorSet, u32 frameIndex);

        void SetScissorRect(u32 left, u32 right, u32 top, u32 bottom);
        void SetViewport(f32 topLeftX, f32 topLeftY, f32 width, f32 height, f32 minDepth, f32 maxDepth);

        void SetVertexBuffer(u32 slot, BufferID buffer);
        void SetIndexBuffer(BufferID buffer, IndexFormat indexFormat);
        void SetBuffer(u32 slot, BufferID buffer);

        void Clear(ImageID imageID, Color color);
        void Clear(DepthImageID imageID, f32 depth, DepthClearFlags flags = DepthClearFlags::DEPTH_CLEAR_DEPTH, u8 stencil = 0);

        void DrawBindless(u32 numVertices, u32 numInstances);
        void DrawIndexedBindless(ModelID modelID, u32 numVertices, u32 numInstances);
        void Draw(u32 numVertices, u32 numInstances, u32 vertexOffset, u32 instanceOffset);
        void DrawIndexed(u32 numIndices, u32 numInstances, u32 indexOffset, u32 vertexOffset, u32 instanceOffset);
        void DrawIndexedIndirect(BufferID argumentBuffer, u32 argumentBufferOffset, u32 drawCount);
        void DrawIndexedIndirectCount(BufferID argumentBuffer, u32 argumentBufferOffset, BufferID drawCountBuffer, u32 drawCountBufferOffset, u32 maxDrawCount);

        void Dispatch(u32 numThreadGroupsX, u32 numThreadGroupsY, u32 numThreadGroupsZ);
        void DispatchIndirect(BufferID argumentBuffer, u32 argumentBufferOffset);

        void AddSignalSemaphore(GPUSemaphoreID semaphoreID);
        void AddWaitSemaphore(GPUSemaphoreID semaphoreID);

        void CopyBuffer(BufferID dstBuffer, u64 dstBufferOffset, BufferID srcBuffer, u64 srcBufferOffset, u64 region);
        void FillBuffer(BufferID dstBuffer, u64 dstBufferOffset, u64 size, u32 data);

        void PipelineBarrier(PipelineBarrierType type, BufferID buffer);

        void DrawImgui();

        void PushConstant(void* data, u32 offset, u32 size);

    private:
        // Execute gets friend-called from RenderGraph
        void Execute();

        template<typename Command>
        Command* AddCommand()
        {
            Command* command = AllocateCommand<Command>();

            AddFunction(Command::DISPATCH_FUNCTION);
            AddData(command);

            return command;
        }

        template<typename Command>
        Command* AllocateCommand()
        {
            assert(_allocator != nullptr);

            return Memory::Allocator::New<Command>(_allocator);
        }

        void AddFunction(const BackendDispatchFunction& function)
        {
            _functions.Insert(function);
        }

        void AddData(void* data)
        {
            _data.Insert(data);
        }

    private:
        Memory::Allocator* _allocator;
        Renderer* _renderer;
        u32 _markerScope;

        DynamicArray<BackendDispatchFunction> _functions;
        DynamicArray<void*> _data;

        bool _isTracing = false;

#if COMMANDLIST_DEBUG_IMMEDIATE_MODE
        CommandListID _immediateCommandList = CommandListID::Invalid();
#endif

        friend class RenderGraph;
    };

    class ScopedMarker
    {
    public:
        ScopedMarker(CommandList& commandList, std::string marker, const Color& color)
            : _commandList(commandList)
        {
            _commandList.PushMarker(marker, color);
        }
        ~ScopedMarker()
        {
            _commandList.PopMarker();
        }

    private:
        CommandList& _commandList;
    };
}