#include "BackendDispatch.h"
#include "Renderer.h"
#include <tracy/Tracy.hpp>

#include "Commands/Clear.h"
#include "Commands/Draw.h"
#include "Commands/DrawBindless.h"
#include "Commands/DrawIndexedBindless.h"
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
#include "Commands/CopyBuffer.h"
#include "Commands/PipelineBarrier.h"

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

    void BackendDispatch::DrawBindless(Renderer * renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::DrawBindless* actualData = static_cast<const Commands::DrawBindless*>(data);
        renderer->DrawBindless(commandList, actualData->numVertices, actualData->numInstances);
    }

    void BackendDispatch::DrawIndexedBindless(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::DrawIndexedBindless* actualData = static_cast<const Commands::DrawIndexedBindless*>(data);
        renderer->DrawIndexedBindless(commandList, actualData->modelID, actualData->numVertices, actualData->numInstances);
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

    void BackendDispatch::SetComputePipeline(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::SetComputePipeline* actualData = static_cast<const Commands::SetComputePipeline*>(data);
        renderer->SetPipeline(commandList, actualData->pipeline);
    }

    void BackendDispatch::BindDescriptorSet(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::BindDescriptorSet* actualData = static_cast<const Commands::BindDescriptorSet*>(data);
        renderer->BindDescriptorSet(commandList, actualData->slot, actualData->descriptors, actualData->numDescriptors, actualData->frameIndex);
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

    void BackendDispatch::CopyBuffer(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::CopyBuffer* actualData = static_cast<const Commands::CopyBuffer*>(data);
        renderer->CopyBuffer(commandList, actualData->dstBuffer, actualData->dstBufferOffset, actualData->srcBuffer, actualData->srcBufferOffset, actualData->region);
    }

    void BackendDispatch::PipelineBarrier(Renderer* renderer, CommandListID commandList, const void* data)
    {
        ZoneScopedC(tracy::Color::Red3);
        const Commands::PipelineBarrier* actualData = static_cast<const Commands::PipelineBarrier*>(data);
        renderer->PipelineBarrier(commandList, actualData->barrierType, actualData->buffer);
    }
}