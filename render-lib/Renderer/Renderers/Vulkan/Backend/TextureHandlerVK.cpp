#include "TextureHandlerVK.h"
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include "RenderDeviceVK.h"
#include "FormatConverterVK.h"
#include "DebugMarkerUtilVK.h"
#include <gli/gli.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "vkformat/vk_format.h"
#include "vk_format_utils.h"

namespace Renderer
{
    namespace Backend
    {
        TextureHandlerVK::TextureHandlerVK()
        {

        }

        TextureHandlerVK::~TextureHandlerVK()
        {

        }

        void TextureHandlerVK::LoadDebugTexture(RenderDeviceVK* device, const TextureDesc& desc)
        {
            _debugTexture.debugName = desc.path;
            
            u8* pixels;
            pixels = ReadFile(desc.path, _debugTexture.width, _debugTexture.height, _debugTexture.format);
            if (!pixels)
            {
                NC_LOG_FATAL("Failed to load debug texture!");
            }

            CreateTexture(device, _debugTexture, pixels);
        }

        TextureID TextureHandlerVK::LoadTexture(RenderDeviceVK* device, const TextureDesc& desc)
        {
            size_t nextHandle = _textures.size();

            // Make sure we haven't exceeded the limit of the ImageID type, if this hits you need to change type of ImageID to something bigger
            assert(nextHandle < TextureID::MaxValue());
            using type = type_safe::underlying_type<TextureID>;

            Texture texture;
            texture.debugName = desc.path;

            u8* pixels;
            pixels = ReadFile(desc.path, texture.width, texture.height, texture.format);
            if (!pixels)
            {
                NC_LOG_FATAL("Failed to load texture!");
            }

            CreateTexture(device, texture, pixels);

            _textures.push_back(texture);
            return TextureID(static_cast<type>(nextHandle));
        }

        TextureID TextureHandlerVK::LoadTextureIntoArray(RenderDeviceVK* device, const TextureDesc& desc, TextureArrayID textureArrayID, u32& arrayIndex)
        {
            using textureArrayType = type_safe::underlying_type<TextureArrayID>;
            assert(static_cast<textureArrayType>(textureArrayID) < _textureArrays.size());

            TextureID textureID = LoadTexture(device, desc);

            using textureType = type_safe::underlying_type<TextureID>;
            Texture& texture = _textures[static_cast<textureType>(textureID)];

            TextureArray& textureArray = _textureArrays[static_cast<textureArrayType>(textureArrayID)];
            arrayIndex = static_cast<u32>(textureArray.textures.size());
            textureArray.textures.push_back(textureID);

            VkDescriptorImageInfo descriptorInfo = {};
            descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorInfo.imageView = texture.imageView;

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.pNext = NULL;
            descriptorWrite.dstSet = textureArray.descriptorSet;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrite.pImageInfo = &descriptorInfo;
            descriptorWrite.dstArrayElement = arrayIndex;
            descriptorWrite.dstBinding = 0;

            vkUpdateDescriptorSets(device->_device, 1, &descriptorWrite, 0, NULL);

            return textureID;
        }

        TextureID TextureHandlerVK::CreateDataTexture(RenderDeviceVK* device, const DataTextureDesc& desc)
        {
            assert(desc.width > 0);
            assert(desc.height > 0);
            assert(desc.data != nullptr);

            size_t nextHandle = _textures.size();

            // Make sure we haven't exceeded the limit of the TextureID type, if this hits you need to change type of ImageID to something bigger
            assert(nextHandle < TextureID::MaxValue());
            using type = type_safe::underlying_type<TextureID>;

            Texture texture;
            texture.debugName = desc.debugName;

            texture.width = desc.width;
            texture.height = desc.height;
            texture.format = FormatConverterVK::ToVkFormat(desc.format);

            CreateTexture(device, texture, desc.data);

            _textures.push_back(texture);
            return TextureID(static_cast<type>(nextHandle));
        }

        TextureArrayID TextureHandlerVK::CreateTextureArray(RenderDeviceVK* device, const TextureArrayDesc& desc)
        {
            assert(desc.size > 0);

            size_t nextHandle = _textureArrays.size();

            // Make sure we haven't exceeded the limit of the TextureArrayID type, if this hits you need to change type of TextureArrayID to something bigger
            assert(nextHandle < TextureArrayID::MaxValue());
            using type = type_safe::underlying_type<TextureArrayID>;

            TextureArray textureArray;
            textureArray.textures.reserve(desc.size);

            // Create descriptor set layout
            VkDescriptorSetLayoutBinding descriptorLayout = {};
            descriptorLayout.binding = 0;
            descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorLayout.descriptorCount = desc.size;
            descriptorLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            descriptorLayout.pImmutableSamplers = NULL;

            VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = {};
            descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorLayoutInfo.pNext = NULL;
            descriptorLayoutInfo.bindingCount = 1;
            descriptorLayoutInfo.pBindings = &descriptorLayout;

            if (vkCreateDescriptorSetLayout(device->_device, &descriptorLayoutInfo, NULL, &textureArray.descriptorSetLayout) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor set layout for texture!");
            }

            // Create descriptor pool
            VkDescriptorPoolSize poolSize = {};
            poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            poolSize.descriptorCount = 1;

            VkDescriptorPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = 1;
            poolInfo.pPoolSizes = &poolSize;
            poolInfo.maxSets = 1;

            if (vkCreateDescriptorPool(device->_device, &poolInfo, nullptr, &textureArray.descriptorPool) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor pool for texture!");
            }

            // Create descriptor set
            VkDescriptorSetAllocateInfo descriptorAllocInfo;
            descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorAllocInfo.pNext = NULL;
            descriptorAllocInfo.descriptorPool = textureArray.descriptorPool;
            descriptorAllocInfo.descriptorSetCount = 1;
            descriptorAllocInfo.pSetLayouts = &textureArray.descriptorSetLayout;

            if (vkAllocateDescriptorSets(device->_device, &descriptorAllocInfo, &textureArray.descriptorSet) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor set for texture!");
            }

            // Now we need to update each descriptor set of the array to point to our debug texture
            // This is needed because if we bind an array where only some indices of the descriptor set has been updated it will complain, even if we promise really honestly never to use the non-updated ones
            std::vector<VkDescriptorImageInfo> descriptorInfos(desc.size);
            for (u32 i = 0; i < desc.size; i++)
            {
                descriptorInfos[i].sampler = nullptr;
                descriptorInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                descriptorInfos[i].imageView = _debugTexture.imageView;
            }

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.pNext = NULL;
            descriptorWrite.dstSet = textureArray.descriptorSet;
            descriptorWrite.descriptorCount = desc.size;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrite.pImageInfo = descriptorInfos.data();
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.dstBinding = 0;

            vkUpdateDescriptorSets(device->_device, 1, &descriptorWrite, 0, NULL);

            _textureArrays.push_back(textureArray);
            return TextureArrayID(static_cast<type>(nextHandle));
        }

        VkImageView TextureHandlerVK::GetImageView(const TextureID id)
        {
            using type = type_safe::underlying_type<TextureID>;

            // Lets make sure this id exists
            assert(_textures.size() > static_cast<type>(id));
            return _textures[static_cast<type>(id)].imageView;
        }

        VkDescriptorSet TextureHandlerVK::GetDescriptorSet(const TextureID id)
        {
            using type = type_safe::underlying_type<TextureID>;

            // Lets make sure this id exists
            assert(_textures.size() > static_cast<type>(id));
            return _textures[static_cast<type>(id)].descriptorSet;
        }

        VkDescriptorSet TextureHandlerVK::GetDescriptorSet(const TextureArrayID id)
        {
            using type = type_safe::underlying_type<TextureArrayID>;

            // Lets make sure this id exists
            assert(_textureArrays.size() > static_cast<type>(id));
            return _textureArrays[static_cast<type>(id)].descriptorSet;
        }

        u8* TextureHandlerVK::ReadFile(const std::string& filename, i32& width, i32& height, VkFormat& format)
        {
            format = VK_FORMAT_R8G8B8A8_UNORM;
            int channels;
            stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            u8* textureMemory = nullptr;

            if (!pixels)
            {
                gli::texture texture = gli::load(filename);
                assert(!texture.empty());

                gli::gl gl(gli::gl::PROFILE_GL33);
                gli::gl::format const gliFormat = gl.translate(texture.format(), texture.swizzles());

                width = texture.extent().x;
                height = texture.extent().y;

                format = vkGetFormatFromOpenGLInternalFormat(gliFormat.Internal);
                size_t textureMemorySize = Math::RoofToInt(static_cast<f64>(width) * static_cast<f64>(height) * FormatTexelSize(format));
                
                textureMemory = new u8[textureMemorySize];
                memcpy(textureMemory, texture.data(), textureMemorySize);
            }
            else
            {
                size_t textureMemorySize = width * height * 4;

                textureMemory = new u8[textureMemorySize];
                memcpy(textureMemory, pixels, textureMemorySize);
            }

            return textureMemory;
        }

        void TextureHandlerVK::CreateTexture(RenderDeviceVK* device, Texture& texture, u8* pixels)
        {
            // Create staging buffer
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;

            VkDeviceSize imageSize = Math::RoofToInt(static_cast<f64>(texture.width) * static_cast<f64>(texture.height) * FormatTexelSize(texture.format));

            device->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            void* data;
            vkMapMemory(device->_device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(device->_device, stagingBufferMemory);

            delete[] pixels;

            // Create image
            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = static_cast<u32>(texture.width);
            imageInfo.extent.height = static_cast<u32>(texture.height);
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = texture.format;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.flags = 0; // Optional

            if (vkCreateImage(device->_device, &imageInfo, nullptr, &texture.image) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create texture image!");
            }

            DebugMarkerUtilVK::SetObjectName(device->_device, (u64)texture.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, texture.debugName.c_str());

            // Allocate and bind memory
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device->_device, texture.image, &memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = device->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(device->_device, &allocInfo, nullptr, &texture.memory) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to allocate texture image memory!");
            }

            vkBindImageMemory(device->_device, texture.image, texture.memory, 0);

            // Copy data from stagingBuffer into image
            device->TransitionImageLayout(texture.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            device->CopyBufferToImage(stagingBuffer, texture.image, static_cast<u32>(texture.width), static_cast<u32>(texture.height));
            device->TransitionImageLayout(texture.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // Create color view
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = texture.image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = texture.format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device->_device, &viewInfo, nullptr, &texture.imageView) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create texture image view!");
            }

            DebugMarkerUtilVK::SetObjectName(device->_device, (u64)texture.imageView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, texture.debugName.c_str());

            // Create descriptor set layout
            VkDescriptorSetLayoutBinding descriptorLayout = {};
            descriptorLayout.binding = 0;
            descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorLayout.descriptorCount = 1;
            descriptorLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            descriptorLayout.pImmutableSamplers = NULL;

            VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = {};
            descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorLayoutInfo.pNext = NULL;
            descriptorLayoutInfo.bindingCount = 1;
            descriptorLayoutInfo.pBindings = &descriptorLayout;

            if (vkCreateDescriptorSetLayout(device->_device, &descriptorLayoutInfo, NULL, &texture.descriptorSetLayout) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor set layout for texture!");
            }

            // Create descriptor pool
            VkDescriptorPoolSize poolSize = {};
            poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            poolSize.descriptorCount = 1;

            VkDescriptorPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = 1;
            poolInfo.pPoolSizes = &poolSize;
            poolInfo.maxSets = 1;

            if (vkCreateDescriptorPool(device->_device, &poolInfo, nullptr, &texture.descriptorPool) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor pool for texture!");
            }

            // Create descriptor set
            VkDescriptorSetAllocateInfo descriptorAllocInfo;
            descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorAllocInfo.pNext = NULL;
            descriptorAllocInfo.descriptorPool = texture.descriptorPool;
            descriptorAllocInfo.descriptorSetCount = 1;
            descriptorAllocInfo.pSetLayouts = &texture.descriptorSetLayout;

            if (vkAllocateDescriptorSets(device->_device, &descriptorAllocInfo, &texture.descriptorSet) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor set for texture!");
            }

            VkDescriptorImageInfo descriptorInfo = {};
            descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorInfo.imageView = texture.imageView;

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.pNext = NULL;
            descriptorWrite.dstSet = texture.descriptorSet;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrite.pImageInfo = &descriptorInfo;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.dstBinding = 0;

            vkUpdateDescriptorSets(device->_device, 1, &descriptorWrite, 0, NULL);
        }

    }
}