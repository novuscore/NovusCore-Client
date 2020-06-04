#include "RendererVK.h"
#include "../../../Window/Window.h"
#include <Utils/StringUtils.h>
#include <Utils/DebugHandler.h>
#include "../../../Window/Window.h"

#include "Backend/RenderDeviceVK.h"
#include "Backend/ImageHandlerVK.h"
#include "Backend/TextureHandlerVK.h"
#include "Backend/ModelHandlerVK.h"
#include "Backend/ShaderHandlerVK.h"
#include "Backend/PipelineHandlerVK.h"
#include "Backend/CommandListHandlerVK.h"
#include "Backend/SamplerHandlerVK.h"
#include "Backend/SwapChainVK.h"
#include "Backend/DebugMarkerUtilVK.h"
#include "Backend/ConstantBufferVK.h"

namespace Renderer
{
    RendererVK::RendererVK(TextureDesc& debugTexture)
        : _device(new Backend::RenderDeviceVK())
    {
        _device->Init();
        _imageHandler = new Backend::ImageHandlerVK();
        _textureHandler = new Backend::TextureHandlerVK();
        _modelHandler = new Backend::ModelHandlerVK();
        _shaderHandler = new Backend::ShaderHandlerVK();
        _pipelineHandler = new Backend::PipelineHandlerVK();
        _commandListHandler = new Backend::CommandListHandlerVK();
        _samplerHandler = new Backend::SamplerHandlerVK();

        _textureHandler->LoadDebugTexture(_device, debugTexture);
    }

    void RendererVK::InitWindow(Window* window)
    {
        _device->InitWindow(_shaderHandler, window);
    }

    void RendererVK::Deinit()
    {
        _device->FlushGPU(); // Make sure it has finished rendering

        delete(_device);
        delete(_imageHandler);
        delete(_textureHandler);
        delete(_modelHandler);
        delete(_shaderHandler);
        delete(_pipelineHandler);
        delete(_commandListHandler);
        delete(_samplerHandler);
    }

    ImageID RendererVK::CreateImage(ImageDesc& desc)
    {
        return _imageHandler->CreateImage(_device, desc);
    }

    DepthImageID RendererVK::CreateDepthImage(DepthImageDesc& desc)
    {
        return _imageHandler->CreateDepthImage(_device, desc);
    }

    SamplerID RendererVK::CreateSampler(SamplerDesc& desc)
    {
        return _samplerHandler->CreateSampler(_device, desc);
    }

    GraphicsPipelineID RendererVK::CreatePipeline(GraphicsPipelineDesc& desc)
    {
        return _pipelineHandler->CreatePipeline(_device, _shaderHandler, _imageHandler, desc);
    }

    ComputePipelineID RendererVK::CreatePipeline(ComputePipelineDesc& /*desc*/)
    {
        NC_LOG_FATAL("Not supported yet");
        return ComputePipelineID::Invalid();
    }

    ModelID RendererVK::CreatePrimitiveModel(PrimitiveModelDesc& desc)
    {
        return _modelHandler->CreatePrimitiveModel(_device, desc);
    }

    void RendererVK::UpdatePrimitiveModel(ModelID model, PrimitiveModelDesc& desc)
    {
        _modelHandler->UpdatePrimitiveModel(_device, model, desc);
    }

    TextureID RendererVK::CreateDataTexture(DataTextureDesc& desc)
    {
        return _textureHandler->CreateDataTexture(_device, desc);
    }

    TextureArrayID RendererVK::CreateTextureArray(TextureArrayDesc& desc)
    {
        return _textureHandler->CreateTextureArray(_device, desc);
    }

    ModelID RendererVK::LoadModel(ModelDesc& desc)
    {
        return _modelHandler->LoadModel(_device, desc);
    }

    TextureID RendererVK::LoadTexture(TextureDesc& desc)
    {
        return _textureHandler->LoadTexture(_device, desc);
    }

    TextureID RendererVK::LoadTextureIntoArray(TextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex)
    {
        return _textureHandler->LoadTextureIntoArray(_device, desc, textureArray, arrayIndex);
    }

    VertexShaderID RendererVK::LoadShader(VertexShaderDesc& desc)
    {
        return _shaderHandler->LoadShader(_device, desc);
    }

    PixelShaderID RendererVK::LoadShader(PixelShaderDesc& desc)
    {
        return _shaderHandler->LoadShader(_device, desc);
    }

    ComputeShaderID RendererVK::LoadShader(ComputeShaderDesc& desc)
    {
        return _shaderHandler->LoadShader(_device, desc);
    }

    CommandListID RendererVK::BeginCommandList()
    {
        return _commandListHandler->BeginCommandList(_device);
    }

    void RendererVK::EndCommandList(CommandListID commandListID)
    {
        if (_renderPassOpenCount != 0)
        {
            NC_LOG_FATAL("We found unmatched calls to BeginPipeline in your commandlist, for every BeginPipeline you need to also EndPipeline!");
        }

        _commandListHandler->EndCommandList(_device, commandListID);
    }

    void RendererVK::Clear(CommandListID commandListID, ImageID imageID, Color color)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        VkImage image = _imageHandler->GetImage(imageID);
        
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
        imageSubresourceRange.layerCount = 1;

        // Transition image to TRANSFER_DST_OPTIMAL
        _device->TransitionImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vkCmdClearColorImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColorValue, 1, &imageSubresourceRange);

        // Transition image back to GENERAL
        _device->TransitionImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    }

    void RendererVK::Clear(CommandListID commandListID, DepthImageID imageID, DepthClearFlags clearFlags, f32 depth, u8 stencil)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        VkImage image = _imageHandler->GetImage(imageID);

        VkClearDepthStencilValue clearDepthValue = {};
        VkImageSubresourceRange range = {};

        if (clearFlags == DepthClearFlags::DEPTH_CLEAR_DEPTH || clearFlags == DepthClearFlags::DEPTH_CLEAR_BOTH)
        {
            clearDepthValue.depth = depth;
            range.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        if (clearFlags == DepthClearFlags::DEPTH_CLEAR_STENCIL || clearFlags == DepthClearFlags::DEPTH_CLEAR_BOTH)
        {
            clearDepthValue.stencil = stencil;
            range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        range.layerCount = 1;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.baseMipLevel = 0;

        // Transition image to TRANSFER_DST_OPTIMAL
        _device->TransitionImageLayout(commandBuffer, image, range.aspectMask, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vkCmdClearDepthStencilImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepthValue, 1, &range);

        // Transition image back to DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        _device->TransitionImageLayout(commandBuffer, image, range.aspectMask, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    void RendererVK::Draw(CommandListID commandListID, ModelID modelID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        // Bind vertex buffer
        VkBuffer vertexBuffer = _modelHandler->GetVertexBuffer(modelID);
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

        // Bind index buffer
        VkBuffer indexBuffer = _modelHandler->GetIndexBuffer(modelID);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw
        u32 numIndices = _modelHandler->GetNumIndices(modelID);
        vkCmdDrawIndexed(commandBuffer, numIndices, 1, 0, 0, 0);
    }

    void RendererVK::DrawInstanced(CommandListID commandListID, ModelID modelID, u32 count)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        // Bind vertex buffer
        VkBuffer vertexBuffer = _modelHandler->GetVertexBuffer(modelID);
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

        // Bind index buffer
        VkBuffer indexBuffer = _modelHandler->GetIndexBuffer(modelID);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw
        u32 numIndices = _modelHandler->GetNumIndices(modelID);
        vkCmdDrawIndexed(commandBuffer, numIndices, count, 0, 0, 0);
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

    void RendererVK::SetConstantBuffer(CommandListID commandListID, u32 slot, void* gpuResource, size_t frameIndex)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        GraphicsPipelineID graphicsPipelineID = _commandListHandler->GetBoundGraphicsPipeline(commandListID);

        Backend::DescriptorSetLayoutData& descriptorSetLayoutData = _pipelineHandler->GetDescriptorSetLayoutData(graphicsPipelineID, slot);
        VkDescriptorSetLayout& descriptorSetLayout = _pipelineHandler->GetDescriptorSetLayout(graphicsPipelineID, slot);
        VkPipelineLayout pipelineLayout = _pipelineHandler->GetPipelineLayout(graphicsPipelineID);

        Backend::ConstantBufferBackendVK* constantBuffer = static_cast<Backend::ConstantBufferBackendVK*>(gpuResource);

        if (constantBuffer->descriptorPool == NULL)
        {
            VkDescriptorPoolSize poolSize = {};
            poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSize.descriptorCount = 1;

            VkDescriptorPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = 1;
            poolInfo.pPoolSizes = &poolSize;
            poolInfo.maxSets = constantBuffer->descriptorSet.Num;

            if (vkCreateDescriptorPool(_device->_device, &poolInfo, nullptr, &constantBuffer->descriptorPool) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor pool!");
            }

            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = constantBuffer->descriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &descriptorSetLayout;

            for (int i = 0; i < constantBuffer->descriptorSet.Num; i++)
            {
                if (vkAllocateDescriptorSets(_device->_device, &allocInfo, &constantBuffer->descriptorSet.Get(i)) != VK_SUCCESS)
                {
                    NC_LOG_FATAL("Failed to allocate descriptor sets!");
                }

                VkDescriptorBufferInfo descriptorBufferInfo = {};
                descriptorBufferInfo.buffer = constantBuffer->uniformBuffers.Get(i);
                descriptorBufferInfo.offset = 0;
                descriptorBufferInfo.range = constantBuffer->bufferSize;

                VkWriteDescriptorSet descriptorWrite = {};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = constantBuffer->descriptorSet.Get(i);
                descriptorWrite.dstBinding = 0;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &descriptorBufferInfo;
                descriptorWrite.pImageInfo = nullptr;
                descriptorWrite.pTexelBufferView = nullptr;

                vkUpdateDescriptorSets(_device->_device, 1, &descriptorWrite, 0, nullptr);
            }
        }

        // Bind descriptor set
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, slot, 1, &constantBuffer->descriptorSet.Get(frameIndex), 0, nullptr);
    }

    void RendererVK::BeginPipeline(CommandListID commandListID, GraphicsPipelineID pipelineID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        GraphicsPipelineDesc pipelineDesc = _pipelineHandler->GetDescriptor(pipelineID);
        VkPipeline pipeline = _pipelineHandler->GetPipeline(pipelineID);
        VkRenderPass renderPass = _pipelineHandler->GetRenderPass(pipelineID);
        VkFramebuffer frameBuffer = _pipelineHandler->GetFramebuffer(pipelineID);

        if (_renderPassOpenCount != 0)
        {
            NC_LOG_FATAL("You need to match your BeginPipeline calls with a EndPipeline call before beginning another pipeline!");
        }
        _renderPassOpenCount++;

        // Set up renderpass
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { static_cast<u32>(pipelineDesc.states.viewport.width), static_cast<u32>(pipelineDesc.states.viewport.height) };

        // Start renderpass
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        _commandListHandler->SetBoundGraphicsPipeline(commandListID, pipelineID);
    }

    void RendererVK::EndPipeline(CommandListID commandListID, GraphicsPipelineID /*pipelineID*/)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);

        if (_renderPassOpenCount <= 0)
        {
            NC_LOG_FATAL("You tried to call EndPipeline without first calling BeginPipeline!");
        }
        _renderPassOpenCount--;

        vkCmdEndRenderPass(commandBuffer);
    }

    void RendererVK::SetPipeline(CommandListID /*commandListID*/, ComputePipelineID /*pipelineID*/)
    {
        
    }

    void RendererVK::SetScissorRect(CommandListID /*commandListID*/, ScissorRect /*scissorRect*/)
    {
        
    }

    void RendererVK::SetViewport(CommandListID /*commandListID*/, Viewport /*viewport*/)
    {
        
    }

    void RendererVK::SetSampler(CommandListID commandListID, u32 slot, SamplerID samplerID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        GraphicsPipelineID graphicsPipelineID = _commandListHandler->GetBoundGraphicsPipeline(commandListID);
        VkPipelineLayout pipelineLayout = _pipelineHandler->GetPipelineLayout(graphicsPipelineID);

        VkDescriptorSet samplerDescriptor = _samplerHandler->GetDescriptorSet(samplerID);

        // Bind descriptor set
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, slot, 1, &samplerDescriptor, 0, nullptr);
    }

    void RendererVK::SetTexture(CommandListID commandListID, u32 slot, TextureID textureID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        GraphicsPipelineID graphicsPipelineID = _commandListHandler->GetBoundGraphicsPipeline(commandListID);
        VkPipelineLayout pipelineLayout = _pipelineHandler->GetPipelineLayout(graphicsPipelineID);

        VkDescriptorSet textureDescriptor = _textureHandler->GetDescriptorSet(textureID);

        // Bind descriptor set
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, slot, 1, &textureDescriptor, 0, nullptr);
    }

    void RendererVK::SetTextureArray(CommandListID commandListID, u32 slot, TextureArrayID textureArrayID)
    {
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        GraphicsPipelineID graphicsPipelineID = _commandListHandler->GetBoundGraphicsPipeline(commandListID);
        VkPipelineLayout pipelineLayout = _pipelineHandler->GetPipelineLayout(graphicsPipelineID);

        VkDescriptorSet textureArrayDescriptor = _textureHandler->GetDescriptorSet(textureArrayID);

        // Bind descriptor set
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, slot, 1, &textureArrayDescriptor, 0, nullptr);
    }

    void RendererVK::Present(Window* window, ImageID imageID)
    {
        CommandListID commandListID = _commandListHandler->BeginCommandList(_device);
        VkCommandBuffer commandBuffer = _commandListHandler->GetCommandBuffer(commandListID);
        PushMarker(commandListID, Color::Red, "Present Blitting");

        Backend::SwapChainVK* swapChain = static_cast<Backend::SwapChainVK*>(window->GetSwapChain());

        u32 semaphoreIndex = swapChain->frameIndex;

        // Acquire next swapchain image
        u32 frameIndex;
        vkAcquireNextImageKHR(_device->_device, swapChain->swapChain, UINT64_MAX, swapChain->imageAvailableSemaphores[semaphoreIndex], VK_NULL_HANDLE, &frameIndex);
        _commandListHandler->SetWaitSemaphore(commandListID, swapChain->imageAvailableSemaphores[semaphoreIndex]);

        VkImage image = _imageHandler->GetImage(imageID);

        // Update SRV descriptor
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = _imageHandler->GetColorView(imageID);
        imageInfo.sampler = swapChain->sampler;

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = swapChain->descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = nullptr;
        descriptorWrite.pImageInfo = &imageInfo;
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(_device->_device, 1, &descriptorWrite, 0, nullptr);

        // Set up renderpass
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = swapChain->renderPass;
        renderPassInfo.framebuffer = swapChain->framebuffers[frameIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChain->extent;

        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        // Transition image from GENERAL to SHADER_READ_ONLY_OPTIMAL
        _device->TransitionImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
       
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
       
        // Bind pipeline and descriptors and render
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, swapChain->pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, swapChain->pipelineLayout, 0, 1, &swapChain->descriptorSet, 0, nullptr);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        // Transition image from SHADER_READ_ONLY_OPTIMAL to GENERAL
        _device->TransitionImageLayout(commandBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
        PopMarker(commandListID);

        _commandListHandler->EndCommandList(_device, commandListID);

        // Present
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 0;
        presentInfo.pWaitSemaphores = VK_NULL_HANDLE;

        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain->swapChain;
        presentInfo.pImageIndices = &frameIndex;
        presentInfo.pResults = nullptr; // Optional

        vkQueuePresentKHR(_device->_presentQueue, &presentInfo);
        vkQueueWaitIdle(_device->_presentQueue);

        // Flip frameIndex between 0 and 1
        swapChain->frameIndex = !swapChain->frameIndex;
    }

    void RendererVK::Present(Window* /*window*/, DepthImageID /*image*/)
    {
        
    }

    Backend::ConstantBufferBackend* RendererVK::CreateConstantBufferBackend(size_t size)
    {
        return _device->CreateConstantBufferBackend(size);
    }
}