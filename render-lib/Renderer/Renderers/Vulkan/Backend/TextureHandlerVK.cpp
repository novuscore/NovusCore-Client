#include "TextureHandlerVK.h"

#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <Utils/XXHash64.h>
#include <vulkan/vulkan.h>
#include <gli/gli.hpp>
#include <vector>
#include <queue>

#include "vk_mem_alloc.h"
#include "RenderDeviceVK.h"
#include "FormatConverterVK.h"
#include "DebugMarkerUtilVK.h"
#include "BufferHandlerVK.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "vkformat/vk_format.h"
#include "vk_format_utils.h"

namespace Renderer
{
    namespace Backend
    {
        struct Texture
        {
            bool loaded = true;
            u64 hash;

            TextureID::type textureIndex;

            i32 width;
            i32 height;
            i32 layers;
            i32 mipLevels;

            VkFormat format;
            size_t fileSize;

            VmaAllocation allocation;
            VkImage image;
            VkImageView imageView;

            std::string debugName = "";
        };

        struct TextureArray
        {
            u32 size;
            std::vector<TextureID> textures;
            std::vector<u64> textureHashes;
        };

        struct TextureHandlerVKData : ITextureHandlerVKData
        {
            std::vector<Texture> textures;
            std::queue<Texture*> freeTextureQueue;

            std::vector<TextureArray> textureArrays;
        };

        void TextureHandlerVK::Init(RenderDeviceVK* device, BufferHandlerVK* bufferHandler)
        {
            _data = new TextureHandlerVKData();
            _device = device;
            _bufferHandler = bufferHandler;

            DataTextureDesc dataTextureDesc;
            dataTextureDesc.width = 1;
            dataTextureDesc.height = 1;
            dataTextureDesc.layers = 256;
            dataTextureDesc.format = ImageFormat::R8G8B8A8_UNORM;
            dataTextureDesc.data = new u8[1 * 1 * 256 * 4]{ 1 };

            _debugOnionTexture = CreateDataTexture(dataTextureDesc);

            delete[] dataTextureDesc.data;
        }

        void TextureHandlerVK::LoadDebugTexture(const TextureDesc& desc)
        {
            _debugTexture = LoadTexture(desc);
        }

        TextureID TextureHandlerVK::LoadTexture(const TextureDesc& desc)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);

            // Check the cache, we only want to do this for LOADED textures though, never CREATED data textures
            size_t nextID;
            u64 cacheDescHash = CalculateDescHash(desc);
            if (TryFindExistingTexture(cacheDescHash, nextID))
            {
                TextureID::type id = static_cast<TextureID::type>(nextID);
                Texture& texture = data.textures[id];
                if (texture.loaded)
                {
                    return TextureID(id); // We already loaded this texture
                }
            }

            // TODO: Check the clearlist before allocating a new one

            size_t nextHandle = data.textures.size();

            // Make sure we haven't exceeded the limit of the ImageID type, if this hits you need to change type of ImageID to something bigger
            if (nextHandle >= TextureID::MaxValue())
            {
                DebugHandler::PrintFatal("We exceeded the limit of the TextureID type!");
            }
            
            Texture texture;
            texture.hash = cacheDescHash;
            texture.debugName = desc.path;

            texture.textureIndex = static_cast<TextureID::type>(nextHandle);

            u8* pixels;
            pixels = ReadFile(desc.path, texture.width, texture.height, texture.layers, texture.mipLevels, texture.format, texture.fileSize);
            if (!pixels)
            {
                DebugHandler::PrintFatal("Failed to load texture! (%s)", desc.path.c_str());
            }

            CreateTexture(texture, pixels);

            data.textures.push_back(texture);
            return TextureID(static_cast<TextureID::type>(nextHandle));
        }

        TextureID TextureHandlerVK::LoadTextureIntoArray(const TextureDesc& desc, TextureArrayID textureArrayID, u32& arrayIndex)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            TextureID textureID;

            // Check the cache, we only want to do this for LOADED textures though, never CREATED data textures
            size_t nextID;
            u64 descHash = CalculateDescHash(desc);

            if (descHash == 0) // What are the odds? All data textures has a 0 hash so we don't wanna go ahead with this, figure out why this happens.
            {
                DebugHandler::PrintFatal("Calculated texture descriptor hash was 0, this is a big issue! (%s)", desc.path.c_str());
            }

            if (TryFindExistingTextureInArray(textureArrayID, descHash, nextID, textureID))
            {
                arrayIndex = static_cast<u32>(nextID);
                return textureID; // This texture already exists in this array
            }

            TextureArrayID::type id = static_cast<TextureArrayID::type>(textureArrayID);

            // Otherwise load it
            if (id >= data.textureArrays.size())
            {
                DebugHandler::PrintFatal("Tried to load into a TextureArrayID which doesn't exist! (%u)", id);
            }

            textureID = LoadTexture(desc);

            Texture& texture = data.textures[static_cast<TextureID::type>(textureID)];

            TextureArray& textureArray = data.textureArrays[static_cast<TextureArrayID::type>(textureArrayID)];
            arrayIndex = static_cast<u32>(textureArray.textures.size());
            textureArray.textures.push_back(textureID);
            textureArray.textureHashes.push_back(descHash);

            return textureID;
        }

        void TextureHandlerVK::UnloadTexture(const TextureID textureID)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            Texture& texture = data.textures[static_cast<TextureID::type>(textureID)];

            if (!texture.loaded)
            {
                return;
            }

            texture.loaded = false;
            texture.hash = 0;

            vmaFreeMemory(_device->_allocator, texture.allocation);
            vkDestroyImage(_device->_device, texture.image, nullptr);
            vkDestroyImageView(_device->_device, texture.imageView, nullptr);

            data.freeTextureQueue.push(&texture);
        }

        void TextureHandlerVK::UnloadTexturesInArray(const TextureArrayID textureArrayID, u32 unloadStartIndex)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            TextureArray& textureArray = data.textureArrays[static_cast<TextureArrayID::type>(textureArrayID)];

            for (u32 i = unloadStartIndex; i < textureArray.textures.size(); i++)
            {
                UnloadTexture(textureArray.textures[i]);
            }

            textureArray.textureHashes.resize(unloadStartIndex);
            textureArray.textures.resize(unloadStartIndex);
        }

        TextureArrayID TextureHandlerVK::CreateTextureArray(const TextureArrayDesc& desc)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            if (desc.size == 0)
            {
                DebugHandler::PrintFatal("Tried to create a texture array with a size of zero!");
            }

            size_t nextHandle = data.textureArrays.size();

            // Make sure we haven't exceeded the limit of the TextureArrayID type, if this hits you need to change type of TextureArrayID to something bigger
            if (nextHandle >= TextureArrayID::MaxValue())
            {
                DebugHandler::PrintFatal("We exceeded the limit of the TextureArrayID type!");
            }

            TextureArray textureArray;
            textureArray.textures.reserve(desc.size);
            textureArray.size = desc.size;

            data.textureArrays.push_back(textureArray);
            return TextureArrayID(static_cast<TextureArrayID::type>(nextHandle));
        }

        TextureID TextureHandlerVK::CreateDataTexture(const DataTextureDesc& desc)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            if (desc.width == 0 || desc.height == 0 || desc.layers == 0)
            {
                DebugHandler::PrintFatal("Invalid DataTexture dimensions! (width %u, height %u, layers %u) (%s)", desc.width, desc.height, desc.layers, desc.debugName.c_str());
            }

            if (desc.data == nullptr)
            {
                DebugHandler::PrintFatal("Tried to create a DataTexture with the data being a nullptr! (%s)", desc.debugName.c_str());
            }

            size_t nextHandle = data.textures.size();

            if (nextHandle >= TextureID::MaxValue())
            {
                DebugHandler::PrintFatal("We exceeded the limit of the TextureID type!");
            }

            Texture texture;
            texture.debugName = desc.debugName;

            texture.width = desc.width;
            texture.height = desc.height;
            texture.layers = desc.layers;
            texture.mipLevels = 1;
            texture.format = FormatConverterVK::ToVkFormat(desc.format);
            texture.fileSize = Math::RoofToInt(static_cast<f64>(texture.width) * static_cast<f64>(texture.height) * static_cast<f64>(texture.layers) * FormatTexelSize(texture.format));

            CreateTexture(texture, desc.data);

            data.textures.push_back(texture);
            return TextureID(static_cast<TextureID::type>(nextHandle));
        }

        TextureID TextureHandlerVK::CreateDataTextureIntoArray(const DataTextureDesc& desc, TextureArrayID textureArrayID, u32& arrayIndex)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            if (static_cast<TextureArrayID::type>(textureArrayID) >= data.textureArrays.size())
            {
                DebugHandler::PrintFatal("Tried to create DataTexture (%s) into invalid array", desc.debugName.c_str());
            }

            TextureID textureID = CreateDataTexture(desc);

            Texture& texture = data.textures[static_cast<TextureID::type>(textureID)];

            TextureArray& textureArray = data.textureArrays[static_cast<TextureArrayID::type>(textureArrayID)];
            arrayIndex = static_cast<u32>(textureArray.textures.size());
            textureArray.textures.push_back(textureID);
            textureArray.textureHashes.push_back(0);

            return textureID;
        }

        const std::vector<TextureID>& TextureHandlerVK::GetTextureIDsInArray(const TextureArrayID textureArrayID)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            TextureArrayID::type id = static_cast<TextureArrayID::type>(textureArrayID);

            // Lets make sure this id exists
            if (data.textureArrays.size() <= id)
            {
                DebugHandler::PrintFatal("Tried to access invalid TextureArrayID: %u", id);
            }

            return data.textureArrays[static_cast<TextureArrayID::type>(textureArrayID)].textures;
        }

        bool TextureHandlerVK::IsOnionTexture(const TextureID textureID)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            TextureID::type id = static_cast<TextureID::type>(textureID);

            // Lets make sure this id exists
            if (data.textures.size() <= id)
            {
                DebugHandler::PrintFatal("Tried to access invalid TextureID: %u", id);
            }

            return data.textures[static_cast<TextureID::type>(textureID)].layers != 1;
        }

        VkImageView TextureHandlerVK::GetImageView(const TextureID textureID)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            TextureID::type id = static_cast<TextureID::type>(textureID);

            // Lets make sure this id exists
            if (data.textures.size() <= id)
            {
                DebugHandler::PrintFatal("Tried to access invalid TextureID: %u", id);
            }

            return data.textures[static_cast<TextureID::type>(textureID)].imageView;
        }

        VkImageView TextureHandlerVK::GetDebugTextureImageView()
        {
            return GetImageView(_debugTexture);
        }

        VkImageView TextureHandlerVK::GetDebugOnionTextureImageView()
        {
            return GetImageView(_debugOnionTexture);
        }

        u32 TextureHandlerVK::GetTextureArraySize(const TextureArrayID textureArrayID)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            TextureArrayID::type id = static_cast<TextureArrayID::type>(textureArrayID);

            // Lets make sure this id exists
            if (data.textureArrays.size() <= id)
            {
                DebugHandler::PrintFatal("Tried to access invalid TextureArrayID: %u", id);
            }

            return data.textureArrays[static_cast<TextureArrayID::type>(textureArrayID)].size;
        }

        u64 TextureHandlerVK::CalculateDescHash(const TextureDesc& desc)
        {
            u64 hash = XXHash64::hash(desc.path.c_str(), desc.path.size(), 0);
            return hash;
        }

        bool TextureHandlerVK::TryFindExistingTexture(u64 descHash, size_t& id)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            id = 0;

            for (auto& texture : data.textures)
            {
                if (descHash == texture.hash)
                {
                    return true;
                }
                id++;
            }

            return false;
        }

        bool TextureHandlerVK::TryFindExistingTextureInArray(TextureArrayID textureArrayID, u64 descHash, size_t& arrayIndex, TextureID& textureID)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            TextureArrayID::type id = static_cast<TextureArrayID::type>(textureArrayID);
            if (data.textureArrays.size() <= id)
            {
                DebugHandler::PrintFatal("Tried to access invalid TextureArrayID: %u", id);
            }

            TextureArray& array = data.textureArrays[id];

            for (arrayIndex = 0; arrayIndex < array.textureHashes.size(); arrayIndex++)
            {
                if (descHash == array.textureHashes[arrayIndex])
                {
                    textureID = array.textures[arrayIndex];
                    return true;
                }
            }

            return false;
        }

        u8* TextureHandlerVK::ReadFile(const std::string& filename, i32& width, i32& height, i32& layers, i32& mipLevels, VkFormat& format, size_t& fileSize)
        {
            format = VK_FORMAT_R8G8B8A8_UNORM;
            int channels;
            stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            u8* textureMemory = nullptr;
            mipLevels = 1; // If we are not loading using gli we don't support mips, so don't bother with it
            layers = 1; // If we are not loading using gli we don't support layers, so don't bother with it

            if (!pixels)
            {
                gli::texture texture = gli::load(filename);
                if (texture.empty())
                {
                    DebugHandler::PrintFatal("Failed to load texture (%s)", filename.c_str());
                }

                gli::gl gl(gli::gl::PROFILE_GL33);
                gli::gl::format const gliFormat = gl.translate(texture.format(), texture.swizzles());

                width = texture.extent().x;
                height = texture.extent().y;
                layers = static_cast<i32>(texture.layers());
                mipLevels = static_cast<i32>(texture.levels());

                format = vkGetFormatFromOpenGLInternalFormat(gliFormat.Internal);
                fileSize = texture.size();
                
                textureMemory = new u8[fileSize];
                memcpy(textureMemory, texture.data(), fileSize);
            }
            else
            {
                fileSize = width * height * 4;

                textureMemory = new u8[fileSize];
                memcpy(textureMemory, pixels, fileSize);
            }

            return textureMemory;
        }

        void TextureHandlerVK::CreateTexture(Texture& texture, u8* pixels)
        {
            // Create staging buffer
            BufferDesc bufferDesc;
            bufferDesc.name = texture.debugName + "_StagingBuffer";
            bufferDesc.size = texture.fileSize;
            bufferDesc.usage = BufferUsage::TRANSFER_SOURCE;
            bufferDesc.cpuAccess = BufferCPUAccess::WriteOnly;
            BufferID stagingBuffer = _bufferHandler->CreateBuffer(bufferDesc);

            void* data;
            vmaMapMemory(_device->_allocator, _bufferHandler->GetBufferAllocation(stagingBuffer), &data);
            memcpy(data, pixels, texture.fileSize);
            vmaUnmapMemory(_device->_allocator, _bufferHandler->GetBufferAllocation(stagingBuffer));

            // Create image
            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = static_cast<u32>(texture.width);
            imageInfo.extent.height = static_cast<u32>(texture.height);
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = texture.mipLevels;
            imageInfo.arrayLayers = texture.layers;
            imageInfo.format = texture.format;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.flags = 0; // Optional

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            if (vmaCreateImage(_device->_allocator, &imageInfo, &allocInfo, &texture.image, &texture.allocation, nullptr) != VK_SUCCESS)
            {
                DebugHandler::PrintFatal("Failed to create image!");
            }

            DebugMarkerUtilVK::SetObjectName(_device->_device, (u64)texture.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, texture.debugName.c_str());

            // Copy data from stagingBuffer into image
            _device->TransitionImageLayout(texture.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture.layers, texture.mipLevels);
            _device->CopyBufferToImage(_bufferHandler->GetBuffer(stagingBuffer), texture.image, texture.format, static_cast<u32>(texture.width), static_cast<u32>(texture.height), texture.layers, texture.mipLevels);
            _device->TransitionImageLayout(texture.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture.layers, texture.mipLevels);

            _bufferHandler->DestroyBuffer(stagingBuffer);

            // Create color view
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = texture.image;
            viewInfo.viewType = (texture.layers > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = texture.format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = texture.mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = texture.layers;

            if (vkCreateImageView(_device->_device, &viewInfo, nullptr, &texture.imageView) != VK_SUCCESS)
            {
                DebugHandler::PrintFatal("Failed to create texture image view!");
            }

            DebugMarkerUtilVK::SetObjectName(_device->_device, (u64)texture.imageView, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, texture.debugName.c_str());
        }
    }
}