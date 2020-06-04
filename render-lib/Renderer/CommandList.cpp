#pragma once
#include "CommandList.h"
#include "Renderer.h"

namespace Renderer
{
    void CommandList::Execute()
    {
        assert(_markerScope == 0); // We need to pop all markers that we push

        CommandListID commandList = _renderer->BeginCommandList();

        // Execute each command
        for (int i = 0; i < _functions.Count(); i++)
        {
            _functions[i](_renderer, commandList, _data[i]);
        }

        _renderer->EndCommandList(commandList);
    }

    void CommandList::PushMarker(std::string marker, Color color)
    {
        Commands::PushMarker* command = AddCommand<Commands::PushMarker>();
        assert(marker.length() < 16); // Max length of marker names is enforced to 15 chars since we have to store the string internally
        strcpy_s(command->marker, marker.c_str());
        command->color = color;

        _markerScope++;
    }

    void CommandList::PopMarker()
    {
        AddCommand<Commands::PopMarker>();

        assert(_markerScope > 0); // We tried to pop a marker we never pushed
        _markerScope--;
    }

    void CommandList::BeginPipeline(GraphicsPipelineID pipelineID)
    {
        Commands::BeginGraphicsPipeline* command = AddCommand<Commands::BeginGraphicsPipeline>();
        command->pipeline = pipelineID;
    }

    void CommandList::EndPipeline(GraphicsPipelineID pipelineID)
    {
        Commands::EndGraphicsPipeline* command = AddCommand<Commands::EndGraphicsPipeline>();
        command->pipeline = pipelineID;
    }

    void CommandList::SetScissorRect(u32 left, u32 right, u32 top, u32 bottom)
    {
        Commands::SetScissorRect* command = AddCommand<Commands::SetScissorRect>();
        command->scissorRect.left = left;
        command->scissorRect.right = right;
        command->scissorRect.top = top;
        command->scissorRect.bottom = bottom;
    }

    void CommandList::SetViewport(f32 topLeftX, f32 topLeftY, f32 width, f32 height, f32 minDepth, f32 maxDepth)
    {
        Commands::SetViewport* command = AddCommand<Commands::SetViewport>();
        command->viewport.topLeftX = topLeftX;
        command->viewport.topLeftY = topLeftY;
        command->viewport.width = width;
        command->viewport.height = height;
        command->viewport.minDepth = minDepth;
        command->viewport.maxDepth = maxDepth;
    }

    void CommandList::SetConstantBuffer(u32 slot, void* gpuResource, size_t frameIndex)
    {
        Commands::SetConstantBuffer* command = AddCommand<Commands::SetConstantBuffer>();
        command->slot = slot;
        command->gpuResource = gpuResource;
        command->frameIndex = frameIndex;
    }

    void CommandList::SetSampler(u32 slot, SamplerID sampler)
    {
        Commands::SetSampler* command = AddCommand<Commands::SetSampler>();
        command->slot = slot;
        command->sampler = sampler;
    }

    void CommandList::SetTexture(u32 slot, TextureID texture)
    {
        Commands::SetTexture* command = AddCommand<Commands::SetTexture>();
        command->slot = slot;
        command->texture = texture;
    }

    void CommandList::SetTextureArray(u32 slot, TextureArrayID textureArray)
    {
        Commands::SetTextureArray* command = AddCommand<Commands::SetTextureArray>();
        command->slot = slot;
        command->textureArray = textureArray;
    }

    void CommandList::Clear(ImageID imageID, Color color)
    {
        Commands::ClearImage* command = AddCommand<Commands::ClearImage>();                                                                                                       
        command->image = imageID;
        command->color = color;
    }

    void CommandList::Clear(DepthImageID imageID, f32 depth, DepthClearFlags flags, u8 stencil)
    {
        Commands::ClearDepthImage* command = AddCommand<Commands::ClearDepthImage>();                                                                                                      
        command->image = imageID;
        command->depth = depth;
        command->flags = flags;
        command->stencil = stencil;
    }

    void CommandList::Draw(ModelID modelID)
    {
        assert(modelID != ModelID::Invalid());
        Commands::Draw* command = AddCommand<Commands::Draw>();
        command->model = modelID;
    }
    void CommandList::DrawInstanced(ModelID modelID, u32 count)
    {
        assert(modelID != ModelID::Invalid());
        assert(count > 0);
        Commands::DrawInstanced* command = AddCommand<Commands::DrawInstanced>();
        command->model = modelID;
        command->count = count;
    }
}