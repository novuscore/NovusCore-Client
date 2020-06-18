#pragma once
#include <NovusTypes.h>
#include "Descriptors/RenderGraphDesc.h"
#include "RenderPass.h"
#include <Memory/StackAllocator.h>
#include <Containers/DynamicArray.h>

namespace Memory
{
    class Allocator;
}

namespace Renderer
{
    class Renderer;
    class RenderGraphBuilder;

    // Acyclic Graph for rendering
    class RenderGraph
    {
    public:
        ~RenderGraph();

        template <typename PassData>
        void AddPass(std::string name, std::function<bool(PassData&, RenderGraphBuilder&)> onSetup, std::function<void(PassData&, CommandList&)> onExecute)
        {
            IRenderPass* pass = Memory::Allocator::New<RenderPass<PassData>>(_desc.allocator, name, onSetup, onExecute);
            _passes.Insert(pass);
        }

        void Setup();
        void Execute();

        RenderGraphBuilder* GetBuilder() { return _renderGraphBuilder; }

        void InitializePipelineDesc(GraphicsPipelineDesc& desc) const;

    private:
        RenderGraph(Memory::Allocator* allocator, Renderer* renderer)
            : _renderer(renderer)
            , _renderGraphBuilder(nullptr)
            , _passes(allocator, 32)
            , _executingPasses(allocator, 32)
        {
        
        } // This gets friend-created by Renderer
        bool Init(RenderGraphDesc& desc);

    private:
        RenderGraphDesc _desc;

        //std::vector<IRenderPass*> _passes;
        //std::vector<IRenderPass*> _executingPasses;

        DynamicArray<IRenderPass*> _passes;
        DynamicArray<IRenderPass*> _executingPasses;

        Renderer* _renderer;
        RenderGraphBuilder* _renderGraphBuilder;

        friend class Renderer; // To have access to the constructor
    };
}