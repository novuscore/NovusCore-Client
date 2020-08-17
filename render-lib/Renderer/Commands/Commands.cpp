#include "../BackendDispatch.h"
#include "Clear.h"
#include "Draw.h"
#include "DrawBindless.h"
#include "DrawIndexed.h"
#include "DrawIndexedBindless.h"
#include "DrawIndexedIndirect.h"
#include "DrawIndexedIndirectCount.h"
#include "Dispatch.h"
#include "DispatchIndirect.h"
#include "PopMarker.h"
#include "PushMarker.h"
#include "SetPipeline.h"
#include "SetScissorRect.h"
#include "SetViewport.h"
#include "SetVertexBuffer.h"
#include "SetIndexBuffer.h"
#include "SetBuffer.h"
#include "BindDescriptorSet.h"
#include "MarkFrameStart.h"
#include "BeginTrace.h"
#include "EndTrace.h"
#include "AddSignalSemaphore.h"
#include "AddWaitSemaphore.h"
#include "CopyBuffer.h"
#include "PipelineBarrier.h"
#include "DrawImgui.h"
#include "PushConstant.h"

namespace Renderer
{
    namespace Commands
    {
        const BackendDispatchFunction ClearImage::DISPATCH_FUNCTION = &BackendDispatch::ClearImage;
        const BackendDispatchFunction ClearDepthImage::DISPATCH_FUNCTION = &BackendDispatch::ClearDepthImage;
        const BackendDispatchFunction Draw::DISPATCH_FUNCTION = &BackendDispatch::Draw;
        const BackendDispatchFunction DrawBindless::DISPATCH_FUNCTION = &BackendDispatch::DrawBindless;
        const BackendDispatchFunction DrawIndexedBindless::DISPATCH_FUNCTION = &BackendDispatch::DrawIndexedBindless;
        const BackendDispatchFunction DrawIndexed::DISPATCH_FUNCTION = &BackendDispatch::DrawIndexed;
        const BackendDispatchFunction DrawIndexedIndirect::DISPATCH_FUNCTION = &BackendDispatch::DrawIndexedIndirect;
        const BackendDispatchFunction DrawIndexedIndirectCount::DISPATCH_FUNCTION = &BackendDispatch::DrawIndexedIndirectCount;
        const BackendDispatchFunction Dispatch::DISPATCH_FUNCTION = &BackendDispatch::Dispatch;
        const BackendDispatchFunction DispatchIndirect::DISPATCH_FUNCTION = &BackendDispatch::DispatchIndirect;
        const BackendDispatchFunction PopMarker::DISPATCH_FUNCTION = &BackendDispatch::PopMarker;
        const BackendDispatchFunction PushMarker::DISPATCH_FUNCTION = &BackendDispatch::PushMarker;
        const BackendDispatchFunction BeginGraphicsPipeline::DISPATCH_FUNCTION = &BackendDispatch::BeginGraphicsPipeline;
        const BackendDispatchFunction EndGraphicsPipeline::DISPATCH_FUNCTION = &BackendDispatch::EndGraphicsPipeline;
        const BackendDispatchFunction SetComputePipeline::DISPATCH_FUNCTION = &BackendDispatch::SetComputePipeline;
        const BackendDispatchFunction SetScissorRect::DISPATCH_FUNCTION = &BackendDispatch::SetScissorRect;
        const BackendDispatchFunction SetViewport::DISPATCH_FUNCTION = &BackendDispatch::SetViewport;
        const BackendDispatchFunction SetVertexBuffer::DISPATCH_FUNCTION = &BackendDispatch::SetVertexBuffer;
        const BackendDispatchFunction SetIndexBuffer::DISPATCH_FUNCTION = &BackendDispatch::SetIndexBuffer;
        const BackendDispatchFunction SetBuffer::DISPATCH_FUNCTION = &BackendDispatch::SetBuffer;
        const BackendDispatchFunction BindDescriptorSet::DISPATCH_FUNCTION = &BackendDispatch::BindDescriptorSet;
        const BackendDispatchFunction MarkFrameStart::DISPATCH_FUNCTION = &BackendDispatch::MarkFrameStart;
        const BackendDispatchFunction BeginTrace::DISPATCH_FUNCTION = &BackendDispatch::BeginTrace;
        const BackendDispatchFunction EndTrace::DISPATCH_FUNCTION = &BackendDispatch::EndTrace;
        const BackendDispatchFunction AddSignalSemaphore::DISPATCH_FUNCTION = &BackendDispatch::AddSignalSemaphore;
        const BackendDispatchFunction AddWaitSemaphore::DISPATCH_FUNCTION = &BackendDispatch::AddWaitSemaphore;
        const BackendDispatchFunction CopyBuffer::DISPATCH_FUNCTION = &BackendDispatch::CopyBuffer;
        const BackendDispatchFunction PipelineBarrier::DISPATCH_FUNCTION = &BackendDispatch::PipelineBarrier;
        const BackendDispatchFunction DrawImgui::DISPATCH_FUNCTION = &BackendDispatch::DrawImgui;
        const BackendDispatchFunction PushConstant::DISPATCH_FUNCTION = &BackendDispatch::PushConstant;
    }
}
