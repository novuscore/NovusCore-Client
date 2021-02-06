#pragma once
#include <NovusTypes.h>
#include "Descriptors/RenderGraphDesc.h"
#include "RenderPass.h"
#include "RenderGraphBuilder.h"

#include <Memory/Allocator.h>

namespace Renderer
{
    class Renderer;
    class RenderGraphBuilder;

    struct IRenderGraphData {};

    // Acyclic Graph for rendering
    class RenderGraph
    {
    public:
        ~RenderGraph();

        template <typename PassData>
        void AddPass(std::string name, std::function<bool(PassData&, RenderGraphBuilder&)> onSetup, std::function<void(PassData&, RenderGraphResources&, CommandList&)> onExecute)
        {
            IRenderPass* pass = Memory::Allocator::New<RenderPass<PassData>>(_desc.allocator, name, onSetup, onExecute);
            AddPass(pass);
        }

        void AddSignalSemaphore(GPUSemaphoreID semaphoreID);
        void AddWaitSemaphore(GPUSemaphoreID semaphoreID);

        void Setup();
        void Execute();

        RenderGraphBuilder* GetBuilder() { return _renderGraphBuilder; }

    private:
        RenderGraph(Memory::Allocator* allocator, Renderer* renderer);
        bool Init(RenderGraphDesc& desc);

        void AddPass(IRenderPass* pass);

    private:
        IRenderGraphData* _data;

        RenderGraphDesc _desc;

        Renderer* _renderer;
        RenderGraphBuilder* _renderGraphBuilder;

        friend class Renderer; // To have access to the constructor
        friend class Memory::Allocator;
    };
}