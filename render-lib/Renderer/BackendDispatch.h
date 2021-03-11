#pragma once
#include <NovusTypes.h>
#include "Descriptors/CommandListDesc.h"

namespace Renderer
{
    class Renderer;
    typedef void (*BackendDispatchFunction)(Renderer*, CommandListID, const void*);

    class BackendDispatch
    {
    public:
        static void ClearImage(Renderer* renderer, CommandListID commandList, const void* data);
        static void ClearDepthImage(Renderer* renderer, CommandListID commandList, const void* data);

        static void Draw(Renderer* renderer, CommandListID commandList, const void* data);
        static void DrawIndirect(Renderer* renderer, CommandListID commandList, const void* data);
        static void DrawIndexed(Renderer* renderer, CommandListID commandList, const void* data);
        static void DrawIndexedIndirect(Renderer* renderer, CommandListID commandList, const void* data);
        static void DrawIndexedIndirectCount(Renderer* renderer, CommandListID commandList, const void* data);

        static void Dispatch(Renderer* renderer, CommandListID commandList, const void* data);
        static void DispatchIndirect(Renderer* renderer, CommandListID commandList, const void* data);

        static void MarkFrameStart(Renderer* renderer, CommandListID commandList, const void* data);
        static void BeginTrace(Renderer* renderer, CommandListID commandList, const void* data);
        static void EndTrace(Renderer* renderer, CommandListID commandList, const void* data);

        static void PopMarker(Renderer* renderer, CommandListID commandList, const void* data);
        static void PushMarker(Renderer* renderer, CommandListID commandList, const void* data);

        static void BeginGraphicsPipeline(Renderer* renderer, CommandListID commandList, const void* data);
        static void EndGraphicsPipeline(Renderer* renderer, CommandListID commandList, const void* data);
        static void BeginComputePipeline(Renderer* renderer, CommandListID commandList, const void* data);
        static void EndComputePipeline(Renderer* renderer, CommandListID commandList, const void* data);

        static void BindDescriptorSet(Renderer* renderer, CommandListID commandList, const void* data);

        static void SetScissorRect(Renderer* renderer, CommandListID commandList, const void* data);
        static void SetViewport(Renderer* renderer, CommandListID commandList, const void* data);

        static void SetVertexBuffer(Renderer* renderer, CommandListID commandList, const void* data);
        static void SetIndexBuffer(Renderer* renderer, CommandListID commandList, const void* data);
        static void SetBuffer(Renderer* renderer, CommandListID commandList, const void* data);

        static void AddSignalSemaphore(Renderer* renderer, CommandListID commandList, const void* data);
        static void AddWaitSemaphore(Renderer* renderer, CommandListID commandList, const void* data);

        static void CopyImage(Renderer* renderer, CommandListID commandList, const void* data);

        static void CopyBuffer(Renderer* renderer, CommandListID commandList, const void* data);
        static void FillBuffer(Renderer* renderer, CommandListID commandList, const void* data);
        static void UpdateBuffer(Renderer* renderer, CommandListID commandList, const void* data);

        static void PipelineBarrier(Renderer* renderer, CommandListID commandList, const void* data);
        static void ImageBarrier(Renderer* renderer, CommandListID commandList, const void* data);
        static void DepthImageBarrier(Renderer* renderer, CommandListID commandList, const void* data);

        static void DrawImgui(Renderer* renderer, CommandListID commandList, const void* data);

        static void PushConstant(Renderer* renderer, CommandListID commandList, const void* data);
    };
}