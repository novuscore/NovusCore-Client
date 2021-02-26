#include "RendererVK.h"
#include "../../../Window/Window.h"
#include <Utils/StringUtils.h>
#include <Utils/DebugHandler.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include "Backend/RenderDeviceVK.h"
#include "Backend/BufferHandlerVK.h"
#include "Backend/ImageHandlerVK.h"
#include "Backend/TextureHandlerVK.h"
#include "Backend/ShaderHandlerVK.h"
#include "Backend/PipelineHandlerVK.h"
#include "Backend/CommandListHandlerVK.h"
#include "Backend/SamplerHandlerVK.h"
#include "Backend/SemaphoreHandlerVK.h"
#include "Backend/SwapChainVK.h"
#include "Backend/DebugMarkerUtilVK.h"
#include "Backend/DescriptorSetBuilderVK.h"
#include "Backend/FormatConverterVK.h"

#include "imgui/imgui_impl_vulkan.h"

namespace Renderer
{
    RendererVK::RendererVK(TextureDesc& debugTexture)
        : _device(new Backend::RenderDeviceVK())
    {
        // Create handlers
        _bufferHandler = new Backend::BufferHandlerVK();
        _imageHandler = new Backend::ImageHandlerVK();
        _textureHandler = new Backend::TextureHandlerVK();
        _shaderHandler = new Backend::ShaderHandlerVK();
        _pipelineHandler = new Backend::PipelineHandlerVK();
        _commandListHandler = new Backend::CommandListHandlerVK();
        _samplerHandler = new Backend::SamplerHandlerVK();
        _semaphoreHandler = new Backend::SemaphoreHandlerVK();

        // Init
        _device->Init();
        _bufferHandler->Init(_device);
        _imageHandler->Init(_device);
        _textureHandler->Init(_device, _bufferHandler);
        _shaderHandler->Init(_device);
        _pipelineHandler->Init(_device, _shaderHandler, _imageHandler);
        _commandListHandler->Init(_device);
        _samplerHandler->Init(_device);
        _semaphoreHandler->Init(_device);

        _textureHandler->LoadDebugTexture(debugTexture);

        CreateDummyPipeline();
    }

    void RendererVK::InitWindow(Window* window)
    {
        _device->InitWindow(_imageHandler, _semaphoreHandler, window);
    }

    void RendererVK::Deinit()
    {
        _device->FlushGPU(); // Make sure it has finished rendering

        delete(_device);
        delete(_bufferHandler);
        delete(_imageHandler);
        delete(_textureHandler);
        delete(_shaderHandler);
        delete(_pipelineHandler);
        delete(_commandListHandler);
        delete(_samplerHandler);
        delete(_semaphoreHandler);
    }

    void RendererVK::ReloadShaders(bool forceRecompileAll)
    {
        // Make sure the device is not rendering
        vkDeviceWaitIdle(_device->_device);

        _shaderHandler->ReloadShaders(forceRecompileAll);
        _pipelineHandler->DiscardPipelines();

        CreateDummyPipeline();
    }

    BufferID RendererVK::CreateBuffer(BufferDesc& desc)
    {
        return _bufferHandler->CreateBuffer(desc);
    }

    BufferID RendererVK::CreateTemporaryBuffer(BufferDesc& desc, u32 framesLifetime)
    {
        return _bufferHandler->CreateTemporaryBuffer(desc, framesLifetime);
    }

    void RendererVK::QueueDestroyBuffer(BufferID buffer)
    {
        _destroyLists[_destroyListIndex].buffers.push_back(buffer);
    }

    ImageID RendererVK::CreateImage(ImageDesc& desc)
    {
        return _imageHandler->CreateImage(desc);
    }

    DepthImageID RendererVK::CreateDepthImage(DepthImageDesc& desc)
    {
        return _imageHandler->CreateDepthImage(desc);
    }

    SamplerID RendererVK::CreateSampler(SamplerDesc& desc)
    {
        return _samplerHandler->CreateSampler(desc);
    }

    GPUSemaphoreID RendererVK::CreateGPUSemaphore()
    {
        return _semaphoreHandler->CreateGPUSemaphore();
    }

    GraphicsPipelineID RendererVK::CreatePipeline(GraphicsPipelineDesc& desc)
    {
        return _pipelineHandler->CreatePipeline(desc);
    }

    ComputePipelineID RendererVK::CreatePipeline(ComputePipelineDesc& desc)
    {
        return _pipelineHandler->CreatePipeline(desc);
    }

    TextureArrayID RendererVK::CreateTextureArray(TextureArrayDesc& desc)
    {
        return _textureHandler->CreateTextureArray(desc);
    }

    TextureID RendererVK::CreateDataTexture(DataTextureDesc& desc)
    {
        return _textureHandler->CreateDataTexture(desc);
    }

    TextureID RendererVK::CreateDataTextureIntoArray(DataTextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex)
    {
        return _textureHandler->CreateDataTextureIntoArray(desc, textureArray, arrayIndex);
    }

    TextureID RendererVK::LoadTexture(TextureDesc& desc)
    {
        return _textureHandler->LoadTexture(desc);
    }

    TextureID RendererVK::LoadTextureIntoArray(TextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex)
    {
        return _textureHandler->LoadTextureIntoArray(desc, textureArray, arrayIndex);
    }

    VertexShaderID RendererVK::LoadShader(VertexShaderDesc& desc)
    {
        return _shaderHandler->LoadShader(desc);
    }

    PixelShaderID RendererVK::LoadShader(PixelShaderDesc& desc)
    {
        return _shaderHandler->LoadShader(desc);
    }

    ComputeShaderID RendererVK::LoadShader(ComputeShaderDesc& desc)
    {
        return _shaderHandler->LoadShader(desc);
    }

    void RendererVK::UnloadTexture(TextureID textureID)
    {
        _device->FlushGPU(); // Make sure we have finished rendering

        _textureHandler->UnloadTexture(textureID);
    }

    void RendererVK::UnloadTexturesInArray(TextureArrayID textureArrayID, u32 unloadStartIndex)
    {
        _device->FlushGPU(); // Make sure we have finished rendering

        _textureHandler->UnloadTexturesInArray(textureArrayID, unloadStartIndex);
    }

    static VmaBudget sBudgets[16] = { 0 };

    void RendererVK::FlipFrame(u32 frameIndex)
    {
        ZoneScopedC(tracy::Color::Red3);

        // Reset old commandbuffers
        _commandListHandler->FlipFrame();

        // Wait on frame fence
        {
            ZoneScopedNC("Wait For Fence", tracy::Color::Red3);

            VkFence frameFence = _commandListHandler->GetCurrentFence();

            u64 timeout = 5000000000; // 5 seconds in nanoseconds
            VkResult result = vkWaitForFences(_device->_device, 1, &frameFence, true, timeout);

            if (result == VK_TIMEOUT)
            {
                DebugHandler::PrintFatal("Waiting for frame fence took longer than 5 seconds, something is wrong!");
            }

            vkResetFences(_device->_device, 1, &frameFence);
        }

        _commandListHandler->ResetCommandBuffers();
        _bufferHandler->OnFrameStart();

        vmaSetCurrentFrameIndex(_device->_allocator, frameIndex);
        vmaGetBudget(_device->_allocator, sBudgets);
    }


    ImageDesc RendererVK::GetImageDesc(ImageID ID)
    {
        return _imageHandler->GetImageDesc(ID);
    }


    uvec2 RendererVK::GetImageDimension(const ImageID id, u32 mipLevel)
    {
        return _imageHandler->GetDimension(id, mipLevel);
    }

    DepthImageDesc RendererVK::GetDepthImageDesc(DepthImageID ID)
    {
        return _imageHandler->GetDepthImageDesc(ID);
    }

    CommandListID RendererVK::BeginCommandList()
    {
        return _commandListHandler->BeginCommandList();
    }

    void RendererVK::EndCommandList(CommandListID commandListID)
    {
        if (_renderPassOpenCount != 0)
        {
            DebugHandler::PrintFatal("We found unmatched calls to BeginPipeline in your commandlist, for every BeginPipeline you need to also EndPipeline!");
        }

        _commandListHandler->EndCommandList(commandListID, VK_NULL_HANDLE);
    }

    void RendererVK::Clear(CommandListID commandListID, ImageID imageID, Color color)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        VkImage image = _imageHandler->GetImage(imageID);

        ImageDesc desc = _imageHandler->GetImageDesc(imageID);
        
        VkClearColorValue clearColorValue = {};
        clearColorValue.float32[0] = color.r;
        clearColorValue.float32[1] = color.g;
        clearColorValue.float32[2] = color.b;
        clearColorValue.float32[3] = color.a;

        VkImageSubresourceRange imageSubresourceRange;
        imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageSubresourceRange.baseMipLevel = 0;
        imageSubresourceRange.levelCount = 1;
        imageSubresourceRange.baseArrayLayer = 0;
        imageSubresourceRange.layerCount = desc.depth;

        // Transition image to TRANSFER_DST_OPTIMAL
        _device->TransitionImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, desc.depth, 1);

        vkCmdClearColorImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColorValue, 1, &imageSubresourceRange);

        // Transition image back to GENERAL
        _device->TransitionImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, desc.depth, 1);
    }

    void RendererVK::Clear(CommandListID commandListID, DepthImageID imageID, DepthClearFlags clearFlags, f32 depth, u8 stencil)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        VkImage image = _imageHandler->GetImage(imageID);

        VkClearDepthStencilValue clearDepthValue = {};
        VkImageSubresourceRange range = {};

        if (clearFlags == DepthClearFlags::DEPTH || clearFlags == DepthClearFlags::BOTH)
        {
            clearDepthValue.depth = depth;
            range.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        if (clearFlags == DepthClearFlags::STENCIL || clearFlags == DepthClearFlags::BOTH)
        {
            clearDepthValue.stencil = stencil;
            range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        range.layerCount = 1;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.baseMipLevel = 0;

        // Transition image to TRANSFER_DST_OPTIMAL
        _device->TransitionImageLayout(commandBuffer, image, range.aspectMask, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 1);

        vkCmdClearDepthStencilImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepthValue, 1, &range);

        // Transition image back to DEPTH_STENCIL_READ_ONLY_OPTIMAL
        _device->TransitionImageLayout(commandBuffer, image, range.aspectMask, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 1, 1);
    }

    void RendererVK::Draw(CommandListID commandListID, u32 numVertices, u32 numInstances, u32 vertexOffset, u32 instanceOffset)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        if (_renderPassOpenCount <= 0)
        {
            DebugHandler::PrintFatal("You tried to draw without first calling BeginPipeline!");
        }

        vkCmdDraw(commandBuffer, numVertices, numInstances, vertexOffset, instanceOffset);
    }

    void RendererVK::DrawIndexed(CommandListID commandListID, u32 numIndices, u32 numInstances, u32 indexOffset, u32 vertexOffset, u32 instanceOffset)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        if (_renderPassOpenCount <= 0)
        {
            DebugHandler::PrintFatal("You tried to draw without first calling BeginPipeline!");
        }

        vkCmdDrawIndexed(commandBuffer, numIndices, numInstances, indexOffset, vertexOffset, instanceOffset);
    }

    void RendererVK::DrawIndexedIndirect(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset, u32 drawCount)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        if (_renderPassOpenCount <= 0)
        {
            DebugHandler::PrintFatal("You tried to draw without first calling BeginPipeline!");
        }

        VkBuffer vkArgumentBuffer = _bufferHandler->GetBuffer(argumentBuffer);

        vkCmdDrawIndexedIndirect(commandBuffer, vkArgumentBuffer, argumentBufferOffset, drawCount, sizeof(VkDrawIndexedIndirectCommand));
    }

    void RendererVK::DrawIndexedIndirectCount(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset, BufferID drawCountBuffer, u32 drawCountBufferOffset, u32 maxDrawCount)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        if (_renderPassOpenCount <= 0)
        {
            DebugHandler::PrintFatal("You tried to draw without first calling BeginPipeline!");
        }

        VkBuffer vkArgumentBuffer = _bufferHandler->GetBuffer(argumentBuffer);
        VkBuffer vkDrawCountBuffer = _bufferHandler->GetBuffer(drawCountBuffer);

        Backend::RenderDeviceVK::fnVkCmdDrawIndexedIndirectCountKHR(commandBuffer, vkArgumentBuffer, argumentBufferOffset, vkDrawCountBuffer, drawCountBufferOffset, maxDrawCount, sizeof(VkDrawIndexedIndirectCommand));
    }

    void RendererVK::Dispatch(CommandListID commandListID, u32 threadGroupCountX, u32 threadGroupCountY, u32 threadGroupCountZ)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        vkCmdDispatch(commandBuffer, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    }

    void RendererVK::DispatchIndirect(CommandListID commandListID, BufferID argumentBuffer, u32 argumentBufferOffset)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        VkBuffer vkArgumentBuffer = _bufferHandler->GetBuffer(argumentBuffer);

        vkCmdDispatchIndirect(commandBuffer, vkArgumentBuffer, argumentBufferOffset);
    }

    void RendererVK::PopMarker(CommandListID commandListID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        Backend::DebugMarkerUtilVK::PopMarker(commandBuffer);
    }

    void RendererVK::PushMarker(CommandListID commandListID, Color color, std::string name)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        Backend::DebugMarkerUtilVK::PushMarker(commandBuffer, color, name);
    }

    void RendererVK::BeginPipeline(CommandListID commandListID, GraphicsPipelineID pipelineID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        const GraphicsPipelineDesc& pipelineDesc = _pipelineHandler->GetDescriptor(pipelineID);
        VkPipeline pipeline = _pipelineHandler->GetPipeline(pipelineID);
        VkRenderPass renderPass = _pipelineHandler->GetRenderPass(pipelineID);
        VkFramebuffer frameBuffer = _pipelineHandler->GetFramebuffer(pipelineID);

        if (_renderPassOpenCount != 0)
        {
            DebugHandler::PrintFatal("You need to match your BeginPipeline calls with a EndPipeline call before beginning another pipeline!");
        }
        _renderPassOpenCount++;

        uvec2 renderSize = _device->GetMainWindowSize();

        // Transition depth stencil to DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        if (pipelineDesc.depthStencil != RenderPassMutableResource::Invalid())
        {
            DepthImageID depthImageID = pipelineDesc.MutableResourceToDepthImageID(pipelineDesc.depthStencil);
            const DepthImageDesc& depthImageDesc = _imageHandler->GetDepthImageDesc(depthImageID);

            u32 imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            // TODO: If we add stencil support we need to selectively add VK_IMAGE_ASPECT_STENCIL_BIT to imageAspect if the depthStencil has a stencil

            VkImage depthImage = _imageHandler->GetImage(depthImageID);
            _device->TransitionImageLayout(commandBuffer, depthImage, imageAspect, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);
        }
        
        // Set up renderpass
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { renderSize.x, renderSize.y };

        // Start renderpass
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        _commandListHandler->SetBoundGraphicsPipeline(commandListID, pipelineID);
    }

    void RendererVK::EndPipeline(CommandListID commandListID, GraphicsPipelineID pipelineID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        if (_renderPassOpenCount <= 0)
        {
            DebugHandler::PrintFatal("You tried to call EndPipeline without first calling BeginPipeline!");
        }
        _renderPassOpenCount--;

        vkCmdEndRenderPass(commandBuffer);
        _commandListHandler->SetBoundGraphicsPipeline(commandListID, GraphicsPipelineID::Invalid());

        // Transition depth stencil to DEPTH_STENCIL_READ_ONLY_OPTIMAL
        const GraphicsPipelineDesc& pipelineDesc = _pipelineHandler->GetDescriptor(pipelineID);
        if (pipelineDesc.depthStencil != RenderPassMutableResource::Invalid())
        {
            DepthImageID depthImageID = pipelineDesc.MutableResourceToDepthImageID(pipelineDesc.depthStencil);
            const DepthImageDesc& depthImageDesc = _imageHandler->GetDepthImageDesc(depthImageID);

            u32 imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            // TODO: If we add stencil support we need to selectively add VK_IMAGE_ASPECT_STENCIL_BIT to imageAspect if the depthStencil has a stencil

            VkImage depthImage = _imageHandler->GetImage(depthImageID);
            _device->TransitionImageLayout(commandBuffer, depthImage, imageAspect, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 1, 1);
        }
    }

    void RendererVK::BeginPipeline(CommandListID commandListID, ComputePipelineID pipelineID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        VkPipeline pipeline = _pipelineHandler->GetPipeline(pipelineID);

        if (_renderPassOpenCount != 0)
        {
            DebugHandler::PrintFatal("You need to match your BeginPipeline calls with a EndPipeline call before beginning another pipeline!");
        }
        _renderPassOpenCount++;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        _commandListHandler->SetBoundComputePipeline(commandListID, pipelineID);
    }

    void RendererVK::EndPipeline(CommandListID commandListID, ComputePipelineID pipelineID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        if (_renderPassOpenCount <= 0)
        {
            DebugHandler::PrintFatal("You tried to call EndPipeline without first calling BeginPipeline!");
        }
        _renderPassOpenCount--;

        VkPipeline pipeline = _pipelineHandler->GetPipeline(pipelineID);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        _commandListHandler->SetBoundComputePipeline(commandListID, ComputePipelineID::Invalid());
    }

    void RendererVK::SetScissorRect(CommandListID commandListID, ScissorRect scissorRect)
    {
        _lastScissorRect = scissorRect;

        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        VkRect2D vkScissorRect = {};
        vkScissorRect.offset = { scissorRect.left, scissorRect.top };
        vkScissorRect.extent = { static_cast<u32>(scissorRect.right - scissorRect.left), static_cast<u32>(scissorRect.bottom - scissorRect.top) };

        vkCmdSetScissor(commandBuffer, 0, 1, &vkScissorRect);
    }

    void RendererVK::SetViewport(CommandListID commandListID, Viewport viewport)
    {
        _lastViewport = viewport;

        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        VkViewport vkViewport = {};
        vkViewport.x = viewport.topLeftX;
        vkViewport.y = viewport.height - viewport.topLeftY;
        vkViewport.width = viewport.width;
        vkViewport.height = -viewport.height;
        vkViewport.minDepth = viewport.minDepth;
        vkViewport.maxDepth = viewport.maxDepth;

        vkCmdSetViewport(commandBuffer, 0, 1, &vkViewport);
    }

    void RendererVK::SetVertexBuffer(CommandListID commandListID, u32 slot, BufferID bufferID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        // Bind vertex buffer
        VkBuffer vertexBuffer = _bufferHandler->GetBuffer(bufferID);
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, slot, 1, &vertexBuffer, offsets);
    }

    void RendererVK::SetIndexBuffer(CommandListID commandListID, BufferID bufferID, IndexFormat indexFormat)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        // Bind index buffer
        VkBuffer indexBuffer = _bufferHandler->GetBuffer(bufferID);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, Backend::FormatConverterVK::ToVkIndexType(indexFormat));
    }

    void RendererVK::SetBuffer(CommandListID commandListID, u32 slot, BufferID buffer)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        // Bind buffer
        VkBuffer vkBuffer = _bufferHandler->GetBuffer(buffer);
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, slot, 1, &vkBuffer, offsets);
    }

    bool RendererVK::ReflectDescriptorSet(const std::string& name, u32 nameHash, u32 type, i32& set, const std::vector<Backend::BindInfo>& bindInfos, u32& outBindInfoIndex, VkDescriptorSetLayoutBinding* outDescriptorLayoutBinding)
    {
        // Try to find a BindInfo with a matching name
        for(u32 i = 0; i < bindInfos.size(); i++)
        {
            auto& bindInfo = bindInfos[i];

            // If the name and type matches
            if (nameHash == bindInfo.nameHash && type == bindInfo.descriptorType)
            {
                // If we have a set, make sure it's the correct one
                if (set != -1)
                {
                    if (set != bindInfo.set)
                    {
                        DebugHandler::PrintError("While creating DescriptorSet, we found BindInfo with matching name (%s) and type (%u), but it didn't match the location (%i != %i)", bindInfo.name, bindInfo.descriptorType, bindInfo.set, set);
                    }
                }
                else
                {
                    set = bindInfo.set;
                }

                // Fill out descriptor set layout
                outDescriptorLayoutBinding->binding = bindInfo.binding;
                outDescriptorLayoutBinding->descriptorType = bindInfo.descriptorType;
                outDescriptorLayoutBinding->descriptorCount = bindInfo.count;
                outDescriptorLayoutBinding->stageFlags = bindInfo.stageFlags;
                outDescriptorLayoutBinding->pImmutableSamplers = NULL;

                outBindInfoIndex = i;

                return true;
            }
        }

        DebugHandler::PrintError("While creating DescriptorSet we encountered binding (%s) of type (%u) which did not have a matching BindInfo in the bound shaders", name.c_str(), type);
        return false;
    }

    void RendererVK::BindDescriptor(Backend::DescriptorSetBuilderVK* builder, void* imageInfosArraysVoid, Descriptor& descriptor)
    {
        std::vector<std::vector<VkDescriptorImageInfo>>& imageInfosArrays = *static_cast<std::vector<std::vector<VkDescriptorImageInfo>>*>(imageInfosArraysVoid);

        if (descriptor.descriptorType == DescriptorType::DESCRIPTOR_TYPE_SAMPLER)
        {
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.sampler = _samplerHandler->GetSampler(descriptor.samplerID);

            builder->BindSampler(descriptor.nameHash, imageInfo);
        }
        else if (descriptor.descriptorType == DescriptorType::DESCRIPTOR_TYPE_TEXTURE)
        {
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = _textureHandler->GetImageView(descriptor.textureID);

            builder->BindImage(descriptor.nameHash, imageInfo);
        }
        else if (descriptor.descriptorType == DescriptorType::DESCRIPTOR_TYPE_TEXTURE_ARRAY)
        {
            const std::vector<TextureID>& textureIDs = _textureHandler->GetTextureIDsInArray(descriptor.textureArrayID);
            std::vector<VkDescriptorImageInfo>& imageInfos = imageInfosArrays.emplace_back();

            u32 textureArraySize = _textureHandler->GetTextureArraySize(descriptor.textureArrayID);
            imageInfos.reserve(textureArraySize);
            
            u32 numTextures = static_cast<u32>(textureIDs.size());

            // From 0 to numTextures, add our actual textures
            bool texturesAreOnionTextures = false;
            for (auto textureID : textureIDs)
            {
                VkDescriptorImageInfo& imageInfo = imageInfos.emplace_back();
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = _textureHandler->GetImageView(textureID);
                imageInfo.sampler = VK_NULL_HANDLE;

                texturesAreOnionTextures = _textureHandler->IsOnionTexture(textureID);
            }

            // from numTextures to textureArraySize, add debug texture
            VkDescriptorImageInfo imageInfoDebugTexture;
            imageInfoDebugTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            if (texturesAreOnionTextures)
            {
                imageInfoDebugTexture.imageView = _textureHandler->GetDebugOnionTextureImageView();
            }
            else
            {
                imageInfoDebugTexture.imageView = _textureHandler->GetDebugTextureImageView();
            }
            
            imageInfoDebugTexture.sampler = VK_NULL_HANDLE;

            for (u32 i = numTextures; i < textureArraySize; i++)
            {
                imageInfos.push_back(imageInfoDebugTexture);
            }
            
            builder->BindImageArray(descriptor.nameHash, imageInfos.data(), static_cast<i32>(imageInfos.size()));
        }
        else if (descriptor.descriptorType == DescriptorType::DESCRIPTOR_TYPE_IMAGE)
        {
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageInfo.imageView = _imageHandler->GetColorView(descriptor.imageID,descriptor.imageMipLevel);

            builder->BindImage(descriptor.nameHash, imageInfo);
        }
        else if (descriptor.descriptorType == DescriptorType::DESCRIPTOR_TYPE_DEPTH_IMAGE)
        {
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            imageInfo.imageView = _imageHandler->GetDepthView(descriptor.depthImageID);

            builder->BindImage(descriptor.nameHash, imageInfo);
        }
        else if (descriptor.descriptorType == DescriptorType::DESCRIPTOR_TYPE_STORAGE_IMAGE)
        {
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageInfo.imageView = _imageHandler->GetColorView(descriptor.imageID, descriptor.imageMipLevel);

            builder->BindStorageImage(descriptor.nameHash, imageInfo);
        }
        else if (descriptor.descriptorType == DescriptorType::DESCRIPTOR_TYPE_BUFFER)
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = _bufferHandler->GetBuffer(descriptor.bufferID);
            bufferInfo.range = _bufferHandler->GetBufferSize(descriptor.bufferID);

            builder->BindBuffer(descriptor.nameHash, bufferInfo);
        }
    }

    void RendererVK::RecreateSwapChain(Backend::SwapChainVK* swapChain)
    {
        _device->RecreateSwapChain(_imageHandler, _semaphoreHandler, swapChain);
        _pipelineHandler->OnWindowResize();
        _imageHandler->OnWindowResize();
    }

    void RendererVK::CreateDummyPipeline()
    {
        // Load dummy pipeline containing our global descriptorset
        VertexShaderDesc dummyShaderDesc;
        dummyShaderDesc.path = "globalDataDummy.vs.hlsl";

        GraphicsPipelineDesc dummyPipelineDesc;
        dummyPipelineDesc.states.vertexShader = _shaderHandler->LoadShader(dummyShaderDesc);

        _globalDummyPipeline = _pipelineHandler->CreatePipeline(dummyPipelineDesc);
    }

    void RendererVK::DestroyObjects(ObjectDestroyList& destroyList)
    {
        for (const BufferID buffer : destroyList.buffers)
        {
            _bufferHandler->DestroyBuffer(buffer);
        }

        destroyList.buffers.clear();
    }

    void RendererVK::BindDescriptorSet(CommandListID commandListID, DescriptorSetSlot slot, Descriptor* descriptors, u32 numDescriptors)
    {
        ZoneScopedNC("RendererVK::BindDescriptorSet", tracy::Color::Red3);

        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        GraphicsPipelineID graphicsPipelineID = _commandListHandler->GetBoundGraphicsPipeline(commandListID);
        ComputePipelineID computePipelineID = _commandListHandler->GetBoundComputePipeline(commandListID);

        if (graphicsPipelineID != GraphicsPipelineID::Invalid())
        {
            std::vector<std::vector<VkDescriptorImageInfo>> imageInfosArrays; // These need to live until builder->BuildDescriptor()
            imageInfosArrays.reserve(8);

            Backend::DescriptorSetBuilderVK* builder = _pipelineHandler->GetDescriptorSetBuilder(graphicsPipelineID);

            for (u32 i = 0; i < numDescriptors; i++)
            {
                ZoneScopedNC("BindDescriptor", tracy::Color::Red3);
                Descriptor& descriptor = descriptors[i];
                BindDescriptor(builder, &imageInfosArrays, descriptor);
            }

            VkDescriptorSet descriptorSet = builder->BuildDescriptor(static_cast<i32>(slot), Backend::DescriptorLifetime::PerFrame);

            VkPipelineLayout pipelineLayout = _pipelineHandler->GetPipelineLayout(graphicsPipelineID);

            // Bind descriptor set
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, slot, 1, &descriptorSet, 0, nullptr);
        } 
        else if (computePipelineID != ComputePipelineID::Invalid())
        {
            std::vector<std::vector<VkDescriptorImageInfo>> imageInfosArrays; // These need to live until builder->BuildDescriptor()
            imageInfosArrays.reserve(8);

            Backend::DescriptorSetBuilderVK* builder = _pipelineHandler->GetDescriptorSetBuilder(computePipelineID);

            for (u32 i = 0; i < numDescriptors; i++)
            {
                ZoneScopedNC("BindDescriptor", tracy::Color::Red3);
                Descriptor& descriptor = descriptors[i];
                BindDescriptor(builder, &imageInfosArrays, descriptor);
            }

            VkDescriptorSet descriptorSet = builder->BuildDescriptor(static_cast<i32>(slot), Backend::DescriptorLifetime::PerFrame);

            VkPipelineLayout pipelineLayout = _pipelineHandler->GetPipelineLayout(computePipelineID);

            // Bind descriptor set
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, slot, 1, &descriptorSet, 0, nullptr);
        }
    }

    void RendererVK::MarkFrameStart(CommandListID commandListID, u32 frameIndex)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        // Collect tracy timings
        TracyVkCollect(_device->_tracyContext, commandBuffer);

        // Add a marker specifying the frameIndex
        Backend::DebugMarkerUtilVK::PushMarker(commandBuffer, Color(1,1,1,1), std::to_string(frameIndex));
        Backend::DebugMarkerUtilVK::PopMarker(commandBuffer);

        // Free up any old descriptors
        _device->_descriptorMegaPool->SetFrame(frameIndex);
    }

#if !TRACY_ENABLE
    void RendererVK::BeginTrace(CommandListID /*commandListID*/, const tracy::SourceLocationData* /*sourceLocation*/)
    {
#else
    void RendererVK::BeginTrace(CommandListID commandListID, const tracy::SourceLocationData* sourceLocation)
    {

        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        tracy::VkCtxManualScope*& tracyScope = _commandListHandler->GetTracyScope(commandListID);

        if (tracyScope != nullptr)
        {
            DebugHandler::PrintFatal("Tried to begin GPU trace on a commandlist that already had a begun GPU trace");
        }

        tracyScope = new tracy::VkCtxManualScope(_device->_tracyContext, sourceLocation, true);
        tracyScope->Start(commandBuffer);
#endif
    }

#if !TRACY_ENABLE
    void RendererVK::EndTrace(CommandListID /*commandListID*/)
    {
#else
    void RendererVK::EndTrace(CommandListID commandListID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        tracy::VkCtxManualScope*& tracyScope = _commandListHandler->GetTracyScope(commandListID);

        if (tracyScope == nullptr)
        {
            DebugHandler::PrintFatal("Tried to end GPU trace on a commandlist that didn't have a running trace");
        }

        tracyScope->End();
        delete tracyScope;
        tracyScope = nullptr;
#endif
    }

    void RendererVK::AddSignalSemaphore(CommandListID commandListID, GPUSemaphoreID semaphoreID)
    {
        VkSemaphore semaphore = _semaphoreHandler->GetVkSemaphore(semaphoreID);
        _commandListHandler->AddSignalSemaphore(commandListID, semaphore);
    }

    void RendererVK::AddWaitSemaphore(CommandListID commandListID, GPUSemaphoreID semaphoreID)
    {
        VkSemaphore semaphore = _semaphoreHandler->GetVkSemaphore(semaphoreID);
        _commandListHandler->AddWaitSemaphore(commandListID, semaphore);
    }

    void RendererVK::CopyImage(CommandListID commandListID, ImageID dstImageID, uvec2 dstPos, u32 dstMipLevel, ImageID srcImageID, uvec2 srcPos, u32 srcMipLevel, uvec2 size)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        VkImage dstImage = _imageHandler->GetImage(dstImageID);
        VkImage srcImage = _imageHandler->GetImage(srcImageID);

        VkImageCopy imageCopy = {};
        imageCopy.dstOffset.x = dstPos.x;
        imageCopy.dstOffset.y = dstPos.y;
        imageCopy.dstSubresource.mipLevel = dstMipLevel;
        imageCopy.dstSubresource.layerCount = 1;
        imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        imageCopy.srcOffset.x = srcPos.x;
        imageCopy.srcOffset.y = srcPos.y;
        imageCopy.srcSubresource.mipLevel = srcMipLevel;
        imageCopy.srcSubresource.layerCount = 1;
        imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        imageCopy.extent.width = size.x;
        imageCopy.extent.height = size.y;
        imageCopy.extent.depth = 1;
        
        vkCmdCopyImage(commandBuffer, srcImage, VK_IMAGE_LAYOUT_GENERAL, dstImage, VK_IMAGE_LAYOUT_GENERAL, 1, &imageCopy);
    }

    void RendererVK::CopyBuffer(CommandListID commandListID, BufferID dstBuffer, u64 dstOffset, BufferID srcBuffer, u64 srcOffset, u64 range)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        VkBuffer vkDstBuffer = _bufferHandler->GetBuffer(dstBuffer);
        VkBuffer vkSrcBuffer = _bufferHandler->GetBuffer(srcBuffer);

        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;
        copyRegion.size = range;
        vkCmdCopyBuffer(commandBuffer, vkSrcBuffer, vkDstBuffer, 1, &copyRegion);
    }

    void RendererVK::PipelineBarrier(CommandListID commandListID, PipelineBarrierType type, BufferID buffer)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        VkPipelineStageFlags srcStageMask;
        VkPipelineStageFlags dstStageMask;

        VkBufferMemoryBarrier bufferBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        bufferBarrier.buffer = _bufferHandler->GetBuffer(buffer);
        bufferBarrier.size = VK_WHOLE_SIZE;

        switch (type)
        {
        case PipelineBarrierType::TransferDestToIndirectArguments:
            srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStageMask = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
            bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            bufferBarrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            break;

        case PipelineBarrierType::TransferDestToComputeShaderRW:
            srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
            break;

        case PipelineBarrierType::TransferDestToVertexBuffer:
            srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            break;

        case PipelineBarrierType::TransferDestToTransferSrc:
            srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            bufferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case PipelineBarrierType::ComputeWriteToVertexShaderRead:
            srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
            bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;

        case PipelineBarrierType::ComputeWriteToPixelShaderRead:
            srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;

        case PipelineBarrierType::ComputeWriteToComputeShaderRead:
            srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
            break;

        case PipelineBarrierType::ComputeWriteToIndirectArguments:
            srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            dstStageMask = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
            bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            bufferBarrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            break;

        case PipelineBarrierType::ComputeWriteToVertexBuffer:
            srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            dstStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            break;

        case PipelineBarrierType::ComputeWriteToTransferSrc:
            srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            bufferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case PipelineBarrierType::AllCommands:
            srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            bufferBarrier.srcAccessMask = VK_ACCESS_FLAG_BITS_MAX_ENUM;;
            bufferBarrier.dstAccessMask = VK_ACCESS_FLAG_BITS_MAX_ENUM;;
            break;
        }

        vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
    }

    void RendererVK::ImageBarrier(CommandListID commandListID, ImageID image)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        const VkImage& vkImage = _imageHandler->GetImage(image);
        const ImageDesc& imageDesc = _imageHandler->GetImageDesc(image);

        _device->TransitionImageLayout(commandBuffer, vkImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, imageDesc.depth, imageDesc.mipLevels);
    }

    void RendererVK::DepthImageBarrier(CommandListID commandListID, DepthImageID image)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        const VkImage& vkImage = _imageHandler->GetImage(image);
        const DepthImageDesc& imageDesc = _imageHandler->GetDepthImageDesc(image);

        u32 imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        // TODO: If we add stencil support we need to selectively add VK_IMAGE_ASPECT_STENCIL_BIT to imageAspect if the depthStencil has a stencil

        _device->TransitionImageLayout(commandBuffer, vkImage, imageAspect, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 1, 1);
    }

    void RendererVK::PushConstant(CommandListID commandListID, void* data, u32 offset, u32 size)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        GraphicsPipelineID graphicsPipelineID = _commandListHandler->GetBoundGraphicsPipeline(commandListID);

        if (graphicsPipelineID != GraphicsPipelineID::Invalid())
        {
            VkPipelineLayout layout = _pipelineHandler->GetPipelineLayout(graphicsPipelineID);
            vkCmdPushConstants(commandBuffer, layout, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, offset, size, data);
        }

        ComputePipelineID computePipelineID = _commandListHandler->GetBoundComputePipeline(commandListID);
        if (computePipelineID != ComputePipelineID::Invalid())
        {
            VkPipelineLayout layout = _pipelineHandler->GetPipelineLayout(computePipelineID);
            vkCmdPushConstants(commandBuffer, layout, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT, offset, size, data);
        }
    }

    void RendererVK::Present(Window* window, ImageID imageID, GPUSemaphoreID semaphoreID)
    {
        CommandListID commandListID = _commandListHandler->BeginCommandList();
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        
        // Tracy profiling
        PushMarker(commandListID, Color::Red, "Present Blitting");
        tracy::VkCtxManualScope*& tracyScope = _commandListHandler->GetTracyScope(commandListID);

        {
            ZoneScopedNC("Present::TracyScope", tracy::Color::Red);
            if (tracyScope != nullptr)
            {
                DebugHandler::PrintFatal("Tried to begin GPU trace on a commandlist that already had a begun GPU trace");
            }

#if TRACY_ENABLE
            TracySourceLocation(presentBlitting, "PresentBlitting", tracy::Color::Yellow2);
            tracyScope = new tracy::VkCtxManualScope(_device->_tracyContext, &presentBlitting, true);
            tracyScope->Start(commandBuffer);
#endif
        }

        Backend::SwapChainVK* swapChain = static_cast<Backend::SwapChainVK*>(window->GetSwapChain());
        u32 semaphoreIndex = swapChain->frameIndex;

        VkFence frameFence = _commandListHandler->GetCurrentFence();

        // Acquire next swapchain image
        VkResult result;
        u32 frameIndex;
        {
            ZoneScopedNC("Present::AcquireNextImage", tracy::Color::Red);
            VkSemaphore imageAvailableSemaphore = _semaphoreHandler->GetVkSemaphore(swapChain->imageAvailableSemaphores.Get(semaphoreIndex));

            result = vkAcquireNextImageKHR(_device->_device, swapChain->swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &frameIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                RecreateSwapChain(swapChain);
                return;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            {
                DebugHandler::PrintFatal("Failed to acquire swap chain image!");
            }
        }
        
        {
            ZoneScopedNC("Present::AddSemaphores", tracy::Color::Red);

            if (semaphoreID != GPUSemaphoreID::Invalid())
            {
                VkSemaphore semaphore = _semaphoreHandler->GetVkSemaphore(semaphoreID);
                _commandListHandler->AddWaitSemaphore(commandListID, semaphore); // Wait for the provided semaphore to finish
            }

            VkSemaphore imageAvailableSemaphore = _semaphoreHandler->GetVkSemaphore(swapChain->imageAvailableSemaphores.Get(semaphoreIndex));
            VkSemaphore blitFinishedSemaphore = _semaphoreHandler->GetVkSemaphore(swapChain->blitFinishedSemaphores.Get(semaphoreIndex));

            _commandListHandler->AddWaitSemaphore(commandListID, imageAvailableSemaphore); // Wait for swapchain image to be available
            _commandListHandler->AddSignalSemaphore(commandListID, blitFinishedSemaphore); // Signal that blitting is done
        }
        
        VkImage image = _imageHandler->GetImage(imageID);

        // Create sampler
        SamplerDesc samplerDesc;
        samplerDesc.enabled = true;
        samplerDesc.filter = SamplerFilter::MIN_MAG_MIP_LINEAR;
        samplerDesc.addressU = TextureAddressMode::WRAP;
        samplerDesc.addressV = TextureAddressMode::WRAP;
        samplerDesc.addressW = TextureAddressMode::CLAMP;
        samplerDesc.shaderVisibility = ShaderVisibility::PIXEL;

        SamplerID samplerID = _samplerHandler->CreateSampler(samplerDesc);
        
        {
            ZoneScopedNC("Present::Blit", tracy::Color::Red);

            // Load shaders
            VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "Blitting/blit.vs.hlsl";

            ImageDesc imageDesc = _imageHandler->GetImageDesc(imageID);
            ImageComponentType componentType = ToImageComponentType(imageDesc.format);
            std::string componentTypeName = "";

            switch (componentType)
            {
                case ImageComponentType::FLOAT:
                    componentTypeName = "float";
                    break;
                case ImageComponentType::SINT:
                case ImageComponentType::SNORM:
                    componentTypeName = "int";
                    break;
                case ImageComponentType::UINT:
                case ImageComponentType::UNORM:
                    componentTypeName = "uint";
                    break;
            }

            u8 componentCount = ToImageComponentCount(imageDesc.format);
            if (componentCount > 1)
            {
                componentTypeName += std::to_string(componentCount);
            }

            PixelShaderDesc pixelShaderDesc;
            pixelShaderDesc.path = "Blitting/blit.ps.hlsl";
            pixelShaderDesc.AddPermutationField("TEX_TYPE", componentTypeName);

            GraphicsPipelineDesc pipelineDesc;

            // We define simple passthrough functions here because we don't have a rendergraph that keeps track of resources while presenting
            pipelineDesc.ResourceToImageID = [](RenderPassResource resource)
            {
                return ImageID(static_cast<RenderPassResource::type>(resource));
            };

            pipelineDesc.ResourceToDepthImageID = [](RenderPassResource resource)
            {
                return DepthImageID(static_cast<RenderPassResource::type>(resource));
            };

            pipelineDesc.MutableResourceToImageID = [](RenderPassMutableResource resource)
            {
                return ImageID(static_cast<RenderPassResource::type>(resource));
            };

            pipelineDesc.MutableResourceToDepthImageID = [](RenderPassMutableResource resource)
            {
                return DepthImageID(static_cast<RenderPassResource::type>(resource));
            };

            pipelineDesc.states.vertexShader = _shaderHandler->LoadShader(vertexShaderDesc);
            pipelineDesc.states.pixelShader = _shaderHandler->LoadShader(pixelShaderDesc);

            pipelineDesc.states.rasterizerState.cullMode = CullMode::BACK;
            pipelineDesc.states.rasterizerState.frontFaceMode = FrontFaceState::COUNTERCLOCKWISE;

            pipelineDesc.renderTargets[0] = RenderPassMutableResource(static_cast<ImageID::type>(swapChain->imageIDs.Get(frameIndex)));

            GraphicsPipelineID pipelineID = _pipelineHandler->CreatePipeline(pipelineDesc);
            VkPipelineLayout pipelineLayout = _pipelineHandler->GetPipelineLayout(pipelineID);
            Backend::DescriptorSetBuilderVK* builder = _pipelineHandler->GetDescriptorSetBuilder(pipelineID);
            
            BeginPipeline(commandListID, pipelineID);

            SetViewport(commandListID, _lastViewport);
            SetScissorRect(commandListID, _lastScissorRect);

            // Set descriptors
            VkDescriptorImageInfo samplerInfo = {};
            samplerInfo.sampler = _samplerHandler->GetSampler(samplerID);
            builder->BindSampler("_sampler"_h, samplerInfo);

            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageInfo.imageView = _imageHandler->GetColorView(imageID);
            builder->BindImage("_texture"_h, imageInfo);

            VkDescriptorSet descriptorSet = builder->BuildDescriptor(0, Backend::DescriptorLifetime::PerFrame);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

            struct BlitConstant
            {
                vec4 colorMultiplier;
                vec4 additiveColor;
            } blitConstant;
            blitConstant.colorMultiplier = vec4(1, 1, 1, 1);
            blitConstant.additiveColor = vec4(0, 0, 0, 0);

            vkCmdPushConstants(commandBuffer, pipelineLayout, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BlitConstant), &blitConstant);

            vkCmdDraw(commandBuffer, 3, 1, 0, 0);

            EndPipeline(commandListID, pipelineID);

#if TRACY_ENABLE
            tracyScope->End();
            tracyScope = nullptr;
#endif

            _commandListHandler->EndCommandList(commandListID, frameFence);
        }

        // Present
        {
            ImageID swapchainImageID = swapChain->imageIDs.Get(frameIndex);
            VkImage image = _imageHandler->GetImage(swapchainImageID);

            VkSemaphore blitFinishedSemaphore = _semaphoreHandler->GetVkSemaphore(swapChain->blitFinishedSemaphores.Get(semaphoreIndex));

            ZoneScopedNC("Present::Present", tracy::Color::Red);
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &blitFinishedSemaphore; // Wait for blitting to finish

            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapChain->swapChain;
            presentInfo.pImageIndices = &frameIndex;
            presentInfo.pResults = nullptr; // Optional

            {
                ZoneScopedNC("Present::vkQueuePresentKHR", tracy::Color::Red);
                result = vkQueuePresentKHR(_device->_presentQueue, &presentInfo);
            }
            

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            {
                DebugHandler::PrintWarning("Recreated swapchain!");
                RecreateSwapChain(swapChain);
                return;
            }
            else if (result != VK_SUCCESS)
            {
                DebugHandler::PrintFatal("Failed to present swap chain image!");
            }
        }

        {
            ZoneScopedNC("Present::Cleanup", tracy::Color::Red);
            // Flip frameIndex between 0 and 1
            swapChain->frameIndex = !swapChain->frameIndex;

            _destroyListIndex = (_destroyListIndex + 1) % _destroyLists.size();
            DestroyObjects(_destroyLists[_destroyListIndex]);
        }
    }

    void RendererVK::Present(Window* /*window*/, DepthImageID /*image*/, GPUSemaphoreID /*semaphoreID*/)
    {
        
    }

    void RendererVK::CopyBuffer(BufferID dstBuffer, u64 dstOffset, BufferID srcBuffer, u64 srcOffset, u64 range)
    {
        VkBuffer vkDstBuffer = _bufferHandler->GetBuffer(dstBuffer);
        VkBuffer vkSrcBuffer = _bufferHandler->GetBuffer(srcBuffer);
        _device->CopyBuffer(vkDstBuffer, dstOffset, vkSrcBuffer, srcOffset, range);

        DestroyObjects(_destroyLists[_destroyListIndex]);
    }

    void RendererVK::FillBuffer(CommandListID commandListID, BufferID dstBuffer, u64 dstOffset, u64 size, u32 data)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        VkBuffer vkDstBuffer = _bufferHandler->GetBuffer(dstBuffer);

        vkCmdFillBuffer(commandBuffer, vkDstBuffer, dstOffset, size, data);
    }

    void* RendererVK::MapBuffer(BufferID buffer)
    {
        void* mappedMemory;

        VkResult result = vmaMapMemory(_device->_allocator, _bufferHandler->GetBufferAllocation(buffer), &mappedMemory);
        if (result != VK_SUCCESS)
        {
            DebugHandler::PrintError("vmaMapMemory failed!\n");
            return nullptr;
        }
        return mappedMemory;
    }
    
    void RendererVK::UnmapBuffer(BufferID buffer)
    {
        vmaUnmapMemory(_device->_allocator, _bufferHandler->GetBufferAllocation(buffer));
    }

    const std::string& RendererVK::GetGPUName()
    {
        return _device->GetGPUName();
    }

    size_t RendererVK::GetVRAMUsage()
    {
        size_t usage = sBudgets[0].usage;

        return usage;
    }

    size_t RendererVK::GetVRAMBudget()
    {
        size_t budget = sBudgets[0].budget;

        return budget;
    }

    void RendererVK::InitImgui()
    {
        _device->InitializeImguiVulkan();
    }

    void RendererVK::DrawImgui(CommandListID commandListID)
    {
        VkCommandBuffer cmd = _commandListHandler->GetCommandBuffer(commandListID);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    }

    u32 RendererVK::GetNumImages()
    {
        return _imageHandler->GetNumImages();
    }

    u32 RendererVK::GetNumDepthImages()
    {
        return _imageHandler->GetNumDepthImages();
    }
}