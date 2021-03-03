#include "BackendDispatch.h"
#include "Renderer.h"
#include <tracy/Tracy.hpp>

#include "Commands/Clear.h"
#include "Commands/Draw.h"
#include "Commands/DrawIndirect.h"
#include "Commands/DrawIndexed.h"
#include "Commands/DrawIndexedIndirect.h"
#include "Commands/DrawIndexedIndirectCount.h"
#include "Commands/Dispatch.h"
#include "Commands/DispatchIndirect.h"
#include "Commands/PopMarker.h"
#include "Commands/PushMarker.h"
#include "Commands/SetPipeline.h"
#include "Commands/SetScissorRect.h"
#include "Commands/SetViewport.h"
#include "Commands/SetVertexBuffer.h"
#include "Commands/SetIndexBuffer.h"
#include "Commands/SetBuffer.h"
#include "Commands/BindDescriptorSet.h"
#include "Commands/MarkFrameStart.h"
#include "Commands/BeginTrace.h"
#include "Commands/EndTrace.h"
#include "Commands/AddSignalSemaphore.h"
#include "Commands/AddWaitSemaphore.h"
#include "Commands/CopyImage.h"
#include "Commands/CopyBuffer.h"
#include "Commands/FillBuffer.h"
#include "Commands/PipelineBarrier.h"
#include "Commands/ImageBarrier.h"
#include "Commands/DepthImageBarrier.h"
#include "Commands/DrawImgui.h"
#include "Commands/PushConstant.h"

namespace Renderer
{
    void BackendDispatch::ClearImage(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::ClearImage* actualData = static_cast<const Commands::ClearImage*>(data);
        renderer->Clear(commandList, actualData->image, actualData->color);
    }

    void BackendDispatch::ClearDepthImage(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::ClearDepthImage* actualData = static_cast<const Commands::ClearDepthImage*>(data);
        renderer->Clear(commandList, actualData->image, actualData->flags, actualData->depth, actualData->stencil);
    }

    void BackendDispatch::Draw(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::Draw* actualData = static_cast<const Commands::Draw*>(data);
        renderer->Draw(commandList, actualData->vertexCount, actualData->instanceCount, actualData->vertexOffset, actualData->instanceOffset);
    }

    void BackendDispatch::DrawIndirect(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::DrawIndirect* actualData = static_cast<const Commands::DrawIndirect*>(data);
        renderer->DrawIndirect(commandList, actualData->argumentBuffer, actualData->argumentBufferOffset, actualData->drawCount);
    }

    void BackendDispatch::DrawIndexed(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::DrawIndexed* actualData = static_cast<const Commands::DrawIndexed*>(data);
        renderer->DrawIndexed(commandList, actualData->indexCount, actualData->instanceCount, actualData->indexOffset, actualData->vertexOffset, actualData->instanceOffset);
    }

    void BackendDispatch::DrawIndexedIndirect(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::DrawIndexedIndirect* actualData = static_cast<const Commands::DrawIndexedIndirect*>(data);
        renderer->DrawIndexedIndirect(commandList, actualData->argumentBuffer, actualData->argumentBufferOffset, actualData->drawCount);
    }

    void BackendDispatch::DrawIndexedIndirectCount(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::DrawIndexedIndirectCount* actualData = static_cast<const Commands::DrawIndexedIndirectCount*>(data);
        renderer->DrawIndexedIndirectCount(commandList, actualData->argumentBuffer, actualData->argumentBufferOffset, actualData->drawCountBuffer, actualData->drawCountBufferOffset, actualData->maxDrawCount);
    }

    void BackendDispatch::Dispatch(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::Dispatch* actualData = static_cast<const Commands::Dispatch*>(data);
        renderer->Dispatch(commandList, actualData->threadGroupCountX, actualData->threadGroupCountY, actualData->threadGroupCountZ);
    }

    void BackendDispatch::DispatchIndirect(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::DispatchIndirect* actualData = static_cast<const Commands::DispatchIndirect*>(data);
        renderer->DispatchIndirect(commandList, actualData->argumentBuffer, actualData->argumentBufferOffset);
    }

    void BackendDispatch::MarkFrameStart(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::MarkFrameStart* actualData = static_cast<const Commands::MarkFrameStart*>(data);
        renderer->MarkFrameStart(commandList, actualData->frameIndex);
    }

    void BackendDispatch::BeginTrace(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::BeginTrace* actualData = static_cast<const Commands::BeginTrace*>(data);
        renderer->BeginTrace(commandList, actualData->sourceLocation);
    }

    void BackendDispatch::EndTrace(Renderer* renderer, CommandListID commandList, const void* /*data*/)
    {
        ZoneScopedC(tracy::Color::Red3);
        renderer->EndTrace(commandList);
    }

    void BackendDispatch::PopMarker(Renderer* renderer, CommandListID commandList, const void* /*data*/)
    {
        ZoneScopedC(tracy::Color::Red3)
        renderer->PopMarker(commandList);
    }

    void BackendDispatch::PushMarker(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::PushMarker* actualData = static_cast<const Commands::PushMarker*>(data);
        renderer->PushMarker(commandList, actualData->color, actualData->marker);
    }

    void BackendDispatch::BeginGraphicsPipeline(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::BeginGraphicsPipeline* actualData = static_cast<const Commands::BeginGraphicsPipeline*>(data);
        renderer->BeginPipeline(commandList, actualData->pipeline);
    }

    void BackendDispatch::EndGraphicsPipeline(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::EndGraphicsPipeline* actualData = static_cast<const Commands::EndGraphicsPipeline*>(data);
        renderer->EndPipeline(commandList, actualData->pipeline);
    }

    void BackendDispatch::BeginComputePipeline(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::BeginComputePipeline* actualData = static_cast<const Commands::BeginComputePipeline*>(data);
        renderer->BeginPipeline(commandList, actualData->pipeline);
    }

    void BackendDispatch::EndComputePipeline(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::EndComputePipeline* actualData = static_cast<const Commands::EndComputePipeline*>(data);
        renderer->EndPipeline(commandList, actualData->pipeline);
    }

    void BackendDispatch::BindDescriptorSet(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::BindDescriptorSet* actualData = static_cast<const Commands::BindDescriptorSet*>(data);
        renderer->BindDescriptorSet(commandList, actualData->slot, actualData->descriptors, actualData->numDescriptors);
    }

    void BackendDispatch::SetScissorRect(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::SetScissorRect* actualData = static_cast<const Commands::SetScissorRect*>(data);
        renderer->SetScissorRect(commandList, actualData->scissorRect);
    }

    void BackendDispatch::SetViewport(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::SetViewport* actualData = static_cast<const Commands::SetViewport*>(data);
        renderer->SetViewport(commandList, actualData->viewport);
    }

    void BackendDispatch::SetVertexBuffer(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::SetVertexBuffer* actualData = static_cast<const Commands::SetVertexBuffer*>(data);
        renderer->SetVertexBuffer(commandList, actualData->slot, actualData->bufferID);
    }

    void BackendDispatch::SetIndexBuffer(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::SetIndexBuffer* actualData = static_cast<const Commands::SetIndexBuffer*>(data);
        renderer->SetIndexBuffer(commandList, actualData->bufferID, actualData->indexFormat);
    }

    void BackendDispatch::SetBuffer(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::SetBuffer* actualData = static_cast<const Commands::SetBuffer*>(data);
        renderer->SetBuffer(commandList, actualData->slot, actualData->buffer);
    }

    void BackendDispatch::AddSignalSemaphore(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::AddSignalSemaphore* actualData = static_cast<const Commands::AddSignalSemaphore*>(data);
        renderer->AddSignalSemaphore(commandList, actualData->semaphore);
    }

    void BackendDispatch::AddWaitSemaphore(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::AddWaitSemaphore* actualData = static_cast<const Commands::AddWaitSemaphore*>(data);
        renderer->AddWaitSemaphore(commandList, actualData->semaphore);
    }

    void BackendDispatch::CopyImage(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::CopyImage* actualData = static_cast<const Commands::CopyImage*>(data);
        renderer->CopyImage(commandList, actualData->dstImage, actualData->dstPos, actualData->dstMipLevel, actualData->srcImage, actualData->srcPos, actualData->srcMipLevel, actualData->size);
    }

    void BackendDispatch::CopyBuffer(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::CopyBuffer* actualData = static_cast<const Commands::CopyBuffer*>(data);
        renderer->CopyBuffer(commandList, actualData->dstBuffer, actualData->dstBufferOffset, actualData->srcBuffer, actualData->srcBufferOffset, actualData->region);
    }

    void BackendDispatch::FillBuffer(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::FillBuffer* actualData = static_cast<const Commands::FillBuffer*>(data);
        renderer->FillBuffer(commandList, actualData->dstBuffer, actualData->dstBufferOffset, actualData->size, actualData->data);
    }

    void BackendDispatch::PipelineBarrier(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::PipelineBarrier* actualData = static_cast<const Commands::PipelineBarrier*>(data);
        renderer->PipelineBarrier(commandList, actualData->barrierType, actualData->buffer);
    }

    void BackendDispatch::ImageBarrier(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::ImageBarrier* actualData = static_cast<const Commands::ImageBarrier*>(data);
        renderer->ImageBarrier(commandList, actualData->image);
    }

    void BackendDispatch::DepthImageBarrier(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::DepthImageBarrier* actualData = static_cast<const Commands::DepthImageBarrier*>(data);
        renderer->DepthImageBarrier(commandList, actualData->image);
    }

    void BackendDispatch::DrawImgui(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedNC("Imgui Draw", tracy::Color::Red3);

        renderer->DrawImgui(commandList);
    }

    void BackendDispatch::PushConstant(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::PushConstant* actualData = static_cast<const Commands::PushConstant*>(data);
        renderer->PushConstant(commandList, actualData->data, actualData->offset, actualData->size);
    }
}