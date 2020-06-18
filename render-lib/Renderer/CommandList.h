#pragma once
#include <NovusTypes.h>
#include "BackendDispatch.h"
#include "Descriptors/CommandListDesc.h"
#include <vector>
#include <Memory/StackAllocator.h>
#include <Containers/DynamicArray.h>

// Commands
#include "Commands/Clear.h"
#include "Commands/Draw.h"
#include "Commands/DrawBindless.h"
#include "Commands/DrawIndexedBindless.h"
#include "Commands/PopMarker.h"
#include "Commands/PushMarker.h"
#include "Commands/SetConstantBuffer.h"
#include "Commands/SetStorageBuffer.h"
#include "Commands/SetPipeline.h"
#include "Commands/SetScissorRect.h"
#include "Commands/SetViewport.h"
#include "Commands/SetSampler.h"
#include "Commands/SetTexture.h"
#include "Commands/SetTextureArray.h"
#include "Commands/SetVertexBuffer.h"
#include "Commands/SetIndexBuffer.h"
#include "Commands/SetBuffer.h"

namespace Renderer
{
    class CommandList
    {
    public:
        CommandList(Renderer* renderer, Memory::Allocator* allocator)
            : _renderer(renderer)
            , _allocator(allocator)
            , _markerScope(0)
            , _functions(allocator, 32)
            , _data(allocator, 32)
        {

        }

        void PushMarker(std::string marker, Color color);
        void PopMarker();

        void BeginPipeline(GraphicsPipelineID pipelineID);
        void EndPipeline(GraphicsPipelineID pipelineID);

        void SetScissorRect(u32 left, u32 right, u32 top, u32 bottom);
        void SetViewport(f32 topLeftX, f32 topLeftY, f32 width, f32 height, f32 minDepth, f32 maxDepth);
        void SetConstantBuffer(u32 slot, void* descriptor, size_t frameIndex);
        void SetStorageBuffer(u32 slot, void* descriptor, size_t frameIndex);
        void SetSampler(u32 slot, SamplerID sampler);
        void SetTexture(u32 slot, TextureID texture);
        void SetTextureArray(u32 slot, TextureArrayID textureArray);
        void SetVertexBuffer(u32 slot, ModelID model);
        void SetIndexBuffer(ModelID model);
        void SetBuffer(u32 slot, void* buffer);

        void Clear(ImageID imageID, Color color);
        void Clear(DepthImageID imageID, f32 depth, DepthClearFlags flags = DepthClearFlags::DEPTH_CLEAR_DEPTH, u8 stencil = 0);

        void Draw(ModelID modelID);
        void DrawBindless(u32 numVertices, u32 numInstances);
        void DrawIndexedBindless(ModelID modelID, u32 numVertices, u32 numInstances);

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