#include "BackendDispatch.h"
#include "Renderer.h"

#include "Commands/Clear.h"
#include "Commands/Draw.h"
#include "Commands/PopMarker.h"
#include "Commands/PushMarker.h"
#include "Commands/SetPipeline.h"
#include "Commands/SetScissorRect.h"
#include "Commands/SetViewport.h"
#include "Commands/SetSampler.h"
#include "Commands/SetTextureArray.h"

namespace Renderer
{
    void BackendDispatch::ClearImage(Renderer* renderer, CommandListID commandList, const void* data)
    {
        const Commands::ClearImage* actualData = static_cast<const Commands::ClearImage*>(data);
        renderer->Clear(commandList, actualData->image, actualData->color);
    }
    void BackendDispatch::ClearDepthImage(Renderer* renderer, CommandListID commandList, const void* data)
    {
        const Commands::ClearDepthImage* actualData = static_cast<const Commands::ClearDepthImage*>(data);
        renderer->Clear(commandList, actualData->image, actualData->flags, actualData->depth, actualData->stencil);
    }

    void BackendDispatch::Draw(Renderer* renderer, CommandListID commandList, const void* data)
    {
        const Commands::Draw* actualData = static_cast<const Commands::Draw*>(data);
        renderer->Draw(commandList, actualData->model);
    }

    void BackendDispatch::DrawInstanced(Renderer * renderer, CommandListID commandList, const void* data)
    {
        const Commands::DrawInstanced* actualData = static_cast<const Commands::DrawInstanced*>(data);
        renderer->DrawInstanced(commandList, actualData->model, actualData->count);
    }

    void BackendDispatch::PopMarker(Renderer* renderer, CommandListID commandList, const void* /*data*/)
    {
        renderer->PopMarker(commandList);
    }

    void BackendDispatch::PushMarker(Renderer* renderer, CommandListID commandList, const void* data)
    {
        const Commands::PushMarker* actualData = static_cast<const Commands::PushMarker*>(data);
        renderer->PushMarker(commandList, actualData->color, actualData->marker);
    }

    void BackendDispatch::SetConstantBuffer(Renderer* renderer, CommandListID commandList, const void* data)
    {
        const Commands::SetConstantBuffer* actualData = static_cast<const Commands::SetConstantBuffer*>(data);
        renderer->SetConstantBuffer(commandList, actualData->slot, actualData->gpuResource, actualData->frameIndex);
    }

    void BackendDispatch::BeginGraphicsPipeline(Renderer* renderer, CommandListID commandList, const void* data)
    {
        const Commands::BeginGraphicsPipeline* actualData = static_cast<const Commands::BeginGraphicsPipeline*>(data);
        renderer->BeginPipeline(commandList, actualData->pipeline);
    }

    void BackendDispatch::EndGraphicsPipeline(Renderer* renderer, CommandListID commandList, const void* data)
    {
        const Commands::EndGraphicsPipeline* actualData = static_cast<const Commands::EndGraphicsPipeline*>(data);
        renderer->EndPipeline(commandList, actualData->pipeline);
    }

    void BackendDispatch::SetComputePipeline(Renderer* renderer, CommandListID commandList, const void* data)
    {
        const Commands::SetComputePipeline* actualData = static_cast<const Commands::SetComputePipeline*>(data);
        renderer->SetPipeline(commandList, actualData->pipeline);
    }

    void BackendDispatch::SetScissorRect(Renderer* renderer, CommandListID commandList, const void* data)
    {
        const Commands::SetScissorRect* actualData = static_cast<const Commands::SetScissorRect*>(data);
        renderer->SetScissorRect(commandList, actualData->scissorRect);
    }

    void BackendDispatch::SetViewport(Renderer* renderer, CommandListID commandList, const void* data)
    {
        const Commands::SetViewport* actualData = static_cast<const Commands::SetViewport*>(data);
        renderer->SetViewport(commandList, actualData->viewport);
    }

    void BackendDispatch::SetSampler(Renderer* renderer, CommandListID commandList, const void* data)
    {
        const Commands::SetSampler* actualData = static_cast<const Commands::SetSampler*>(data);
        renderer->SetSampler(commandList, actualData->slot, actualData->sampler);
    }

    void BackendDispatch::SetTexture(Renderer* renderer, CommandListID commandList, const void* data)
    {
        const Commands::SetTexture* actualData = static_cast<const Commands::SetTexture*>(data);
        renderer->SetTexture(commandList, actualData->slot, actualData->texture);
    }
    
    void BackendDispatch::SetTextureArray(Renderer* renderer, CommandListID commandList, const void* data)
    {
        const Commands::SetTextureArray* actualData = static_cast<const Commands::SetTextureArray*>(data);
        renderer->SetTextureArray(commandList, actualData->slot, actualData->textureArray);
    }
}