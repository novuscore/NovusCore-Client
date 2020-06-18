#include "../BackendDispatch.h"
#include "Clear.h"
#include "Draw.h"
#include "DrawBindless.h"
#include "DrawIndexedBindless.h"
#include "PopMarker.h"
#include "PushMarker.h"
#include "SetConstantBuffer.h"
#include "SetStorageBuffer.h"
#include "SetPipeline.h"
#include "SetScissorRect.h"
#include "SetViewport.h"
#include "SetSampler.h"
#include "SetTexture.h"
#include "SetTextureArray.h"
#include "SetVertexBuffer.h"
#include "SetIndexBuffer.h"
#include "SetBuffer.h"

namespace Renderer
{
    namespace Commands
    {
        const BackendDispatchFunction ClearImage::DISPATCH_FUNCTION = &BackendDispatch::ClearImage;
        const BackendDispatchFunction ClearDepthImage::DISPATCH_FUNCTION = &BackendDispatch::ClearDepthImage;
        const BackendDispatchFunction Draw::DISPATCH_FUNCTION = &BackendDispatch::Draw;
        const BackendDispatchFunction DrawBindless::DISPATCH_FUNCTION = &BackendDispatch::DrawBindless;
        const BackendDispatchFunction DrawIndexedBindless::DISPATCH_FUNCTION = &BackendDispatch::DrawIndexedBindless;
        const BackendDispatchFunction PopMarker::DISPATCH_FUNCTION = &BackendDispatch::PopMarker;
        const BackendDispatchFunction PushMarker::DISPATCH_FUNCTION = &BackendDispatch::PushMarker;
        const BackendDispatchFunction SetConstantBuffer::DISPATCH_FUNCTION = &BackendDispatch::SetConstantBuffer;
        const BackendDispatchFunction SetStorageBuffer::DISPATCH_FUNCTION = &BackendDispatch::SetStorageBuffer;
        const BackendDispatchFunction BeginGraphicsPipeline::DISPATCH_FUNCTION = &BackendDispatch::BeginGraphicsPipeline;
        const BackendDispatchFunction EndGraphicsPipeline::DISPATCH_FUNCTION = &BackendDispatch::EndGraphicsPipeline;
        const BackendDispatchFunction SetComputePipeline::DISPATCH_FUNCTION = &BackendDispatch::SetComputePipeline;
        const BackendDispatchFunction SetScissorRect::DISPATCH_FUNCTION = &BackendDispatch::SetScissorRect;
        const BackendDispatchFunction SetViewport::DISPATCH_FUNCTION = &BackendDispatch::SetViewport;
        const BackendDispatchFunction SetSampler::DISPATCH_FUNCTION = &BackendDispatch::SetSampler;
        const BackendDispatchFunction SetTexture::DISPATCH_FUNCTION = &BackendDispatch::SetTexture;
        const BackendDispatchFunction SetTextureArray::DISPATCH_FUNCTION = &BackendDispatch::SetTextureArray;
        const BackendDispatchFunction SetVertexBuffer::DISPATCH_FUNCTION = &BackendDispatch::SetVertexBuffer;
        const BackendDispatchFunction SetIndexBuffer::DISPATCH_FUNCTION = &BackendDispatch::SetIndexBuffer;
        const BackendDispatchFunction SetBuffer::DISPATCH_FUNCTION = &BackendDispatch::SetBuffer;
    }
}
