#include "TextureHandlerVK.h"

#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <Utils/XXHash64.h>
#include <Utils/FileReader.h>
#include <Utils/SafeVector.h>
#include <vulkan/vulkan.h>
#include <gli/gli.hpp>
#include <vector>
#include <queue>
#include <filesystem>

#include "vk_mem_alloc.h"
#include "RenderDeviceVK.h"
#include "FormatConverterVK.h"
#include "DebugMarkerUtilVK.h"
#include "BufferHandlerVK.h"
#include "UploadBufferHandlerVK.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "vkformat/vk_format.h"
#include "vk_format_utils.h"
#include <tracy/Tracy.hpp>

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

            bool layoutUndefined = true;
        };

        struct TextureArray
        {
            u32 size;
            SafeVector<TextureID>* textures = nullptr;
            SafeVector<u64>* textureHashes = nullptr;
        };

        struct TextureHandlerVKData : ITextureHandlerVKData
        {
            SafeVector<Texture*> textures;
            std::queue<Texture*> freeTextureQueue;

            SafeVector<TextureArray> textureArrays;
        };

        void TextureHandlerVK::Init(RenderDeviceVK* device, BufferHandlerVK* bufferHandler, UploadBufferHandlerVK* uploadBufferHandler)
        {
            _data = new TextureHandlerVKData();
            _device = device;
            _bufferHandler = bufferHandler;
            _uploadBufferHandler = uploadBufferHandler;
        }

        void TextureHandlerVK::InitDebugTexture()
        {
            // Create 2d debug texture
            DataTextureDesc desc;
            desc.debugName = "DebugTexture";
            desc.format = ImageFormat::R8G8B8A8_UNORM;
            desc.width = 8;
            desc.height = 8;
            desc.layers = 1;

            // 8x8 checkerboard pattern between black and magenta
            desc.data = new u8[4*8*8]
            {
                0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,
                255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,
                0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,
                255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,
                0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,
                255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,
                0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,
                255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255,       255, 0, 255, 255,   0, 0, 0, 255
            };
            
            _debugTexture = CreateDataTexture(desc);
            delete[] desc.data;

            // Create 3d debug texture (onion)
            desc.width = 1;
            desc.height = 1;
            desc.layers = 256;
            desc.format = ImageFormat::R8G8B8A8_UNORM;
            desc.data = new u8[1 * 1 * 256 * 4]{ 1 };

            _debugOnionTexture = CreateDataTexture(desc);
            delete[] desc.data;
        }

        TextureID TextureHandlerVK::LoadTexture(const TextureDesc& desc)
        {
            ZoneScoped;

            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);

            // Check the cache, we only want to do this for LOADED textures though, never CREATED data textures
            size_t nextID;
            u64 cacheDescHash = CalculateDescHash(desc);
            if (TryFindExistingTexture(cacheDescHash, nextID))
            {
                TextureID::type id = static_cast<TextureID::type>(nextID);

                const Texture* texture = nullptr;

                data.textures.ReadLock(
                    [&](const std::vector<Texture*>& textures)
                    {
                        texture = textures[id];
                    });

                if (texture->loaded)
                {
                    return TextureID(id); // We already loaded this texture
                }
            }

            // TODO: Check the clearlist before allocating a new one

            TextureID textureID;
            Texture* texture = new Texture();

            data.textures.WriteLock(
                [&](std::vector<Texture*>& textures)
                {
                    size_t nextHandle = textures.size();

                    // Make sure we haven't exceeded the limit of the ImageID type, if this hits you need to change type of ImageID to something bigger
                    if (nextHandle >= TextureID::MaxValue())
                    {
                        DebugHandler::PrintFatal("We exceeded the limit of the TextureID type!");
                    }

                    textures.push_back(texture);
                    textureID = TextureID(static_cast<TextureID::type>(nextHandle));
                });

            texture->hash = cacheDescHash;
            texture->debugName = desc.path;

            texture->textureIndex = static_cast<TextureID::type>(textureID);
            LoadFile(desc.path, *texture, textureID);

            return textureID;
        }

        TextureID TextureHandlerVK::LoadTextureIntoArray(const TextureDesc& desc, TextureArrayID textureArrayID, u32& arrayIndex)
        {
            ZoneScoped;

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
            if (id >= data.textureArrays.Size())
            {
                DebugHandler::PrintFatal("Tried to load into a TextureArrayID which doesn't exist! (%u)", id);
            }

            textureID = LoadTexture(desc);

            {
                ZoneScopedN("TextureArrays WriteLock");
                data.textureArrays.WriteLock(
                    [&](std::vector<TextureArray>& textureArrays)
                    {
                        TextureArray& textureArray = textureArrays[static_cast<TextureArrayID::type>(textureArrayID)];
                        arrayIndex = static_cast<u32>(textureArray.textures->Size());
                        textureArray.textures->PushBack(textureID);
                        textureArray.textureHashes->PushBack(descHash);
                    });
            }
            
            return textureID;
        }

        void TextureHandlerVK::UnloadTexture(const TextureID textureID)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);

            data.textures.WriteLock(
                [&](std::vector<Texture*>& textures)
                {
                    Texture* texture = textures[static_cast<TextureID::type>(textureID)];

                    if (!texture->loaded)
                    {
                        return;
                    }

                    texture->loaded = false;
                    texture->hash = 0;

                    vmaFreeMemory(_device->_allocator, texture->allocation);
                    vkDestroyImage(_device->_device, texture->image, nullptr);
                    vkDestroyImageView(_device->_device, texture->imageView, nullptr);

                    data.freeTextureQueue.push(texture);
                });
        }

        void TextureHandlerVK::UnloadTexturesInArray(const TextureArrayID textureArrayID, u32 unloadStartIndex)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);

            data.textureArrays.WriteLock(
                [&](std::vector<TextureArray>& textureArrays)
                {
                    TextureArray& textureArray = textureArrays[static_cast<TextureArrayID::type>(textureArrayID)];

                    textureArray.textures->ReadLock(
                        [&](const std::vector<TextureID>& textures) 
                        {
                            for (u32 i = unloadStartIndex; i < textures.size(); i++)
                            {
                                UnloadTexture(textures[i]);
                            }
                        });

                    textureArray.textureHashes->Resize(unloadStartIndex);
                    textureArray.textures->Resize(unloadStartIndex);
                });
        }

        TextureArrayID TextureHandlerVK::CreateTextureArray(const TextureArrayDesc& desc)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            if (desc.size == 0)
            {
                DebugHandler::PrintFatal("Tried to create a texture array with a size of zero!");
            }

            size_t nextHandle;
            data.textureArrays.WriteLock(
                [&](std::vector<TextureArray>& textureArrays)
                {
                    nextHandle = textureArrays.size();
                    TextureArray& textureArray = textureArrays.emplace_back();
                    textureArray.textures = new SafeVector<TextureID>();
                    textureArray.textures->Reserve(desc.size);
                    textureArray.textureHashes = new SafeVector<u64>();
                    textureArray.textureHashes->Reserve(desc.size);
                    textureArray.size = desc.size;
                });

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

            size_t nextHandle;
            Texture* texture = new Texture();

            data.textures.WriteLock([&](std::vector<Texture*>& textures)
                {
                    nextHandle = textures.size();

                    if (nextHandle >= TextureID::MaxValue())
                    {
                        DebugHandler::PrintFatal("We exceeded the limit of the TextureID type!");
                    }

                    textures.push_back(texture);
                });

            texture->debugName = desc.debugName;

            texture->width = desc.width;
            texture->height = desc.height;
            texture->layers = desc.layers;
            texture->mipLevels = 1;
            texture->format = FormatConverterVK::ToVkFormat(desc.format);
            texture->fileSize = Math::RoofToInt(static_cast<f64>(texture->width) * static_cast<f64>(texture->height) * static_cast<f64>(texture->layers) * FormatTexelSize(texture->format));

            TextureID textureID = TextureID(static_cast<TextureID::type>(nextHandle));

            // Create texture
            CreateTexture(*texture);

            // Create upload buffer
            auto uploadBuffer = _uploadBufferHandler->CreateUploadBuffer(textureID, 0, texture->fileSize);

            // Copy data to upload buffer
            memcpy(uploadBuffer->mappedMemory, desc.data, texture->fileSize);

            return textureID;
        }

        TextureID TextureHandlerVK::CreateDataTextureIntoArray(const DataTextureDesc& desc, TextureArrayID textureArrayID, u32& arrayIndex)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);

            if (static_cast<TextureArrayID::type>(textureArrayID) >= data.textureArrays.Size())
            {
                DebugHandler::PrintFatal("Tried to create DataTexture (%s) into invalid array", desc.debugName.c_str());
            }

            TextureID textureID = CreateDataTexture(desc);

            data.textureArrays.WriteLock(
                [&](std::vector<TextureArray>& textureArrays)
                {
                    TextureArray& textureArray = textureArrays[static_cast<TextureArrayID::type>(textureArrayID)];
                    arrayIndex = static_cast<u32>(textureArray.textures->Size());
                    textureArray.textures->PushBack(textureID);
                    u64 zero = 0;
                    textureArray.textureHashes->PushBack(zero);
                });

            return textureID;
        }

        void TextureHandlerVK::CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, size_t srcOffset, TextureID dstTextureID)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            TextureID::type id = static_cast<TextureID::type>(dstTextureID);

            data.textures.WriteLock(
                [&](std::vector<Texture*>& textures)
                {
                    // Lets make sure this id exists
                    if (textures.size() <= id)
                    {
                        DebugHandler::PrintFatal("Tried to access invalid TextureID: %u", id);
                    }

                    Texture& texture = *textures[id];

                    // If the texture has been unloaded, just return
                    if (!texture.loaded)
                        return;

                    // Transition to TRANSFER_DST_OPTIMAL
                    VkImageLayout beforeLayout = (texture.layoutUndefined) ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    _device->TransitionImageLayout(commandBuffer, texture.image, VK_IMAGE_ASPECT_COLOR_BIT, beforeLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture.layers, texture.mipLevels);

                    // Do the copy
                    _device->CopyBufferToImage(commandBuffer, srcBuffer, srcOffset, texture.image, texture.format, static_cast<u32>(texture.width), static_cast<u32>(texture.height), texture.layers, texture.mipLevels);

                    // Transition back to SHADER_READ_ONLY_OPTIMAL
                    _device->TransitionImageLayout(commandBuffer, texture.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture.layers, texture.mipLevels);
                    texture.layoutUndefined = false;
                });
        }

        void TextureHandlerVK::TransitionImageLayout(VkCommandBuffer commandBuffer, TextureID textureID, VkImageAspectFlags aspects, VkImageLayout oldLayout, VkImageLayout newLayout)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            TextureID::type id = static_cast<TextureID::type>(textureID);

        }

        const SafeVector<TextureID>& TextureHandlerVK::GetTextureIDsInArray(const TextureArrayID textureArrayID)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            TextureArrayID::type id = static_cast<TextureArrayID::type>(textureArrayID);

            // Lets make sure this id exists
            if (data.textureArrays.Size() <= id)
            {
                DebugHandler::PrintFatal("Tried to access invalid TextureArrayID: %u", id);
            }

            const SafeVector<TextureID>* textures = nullptr;
            data.textureArrays.ReadLock(
                [&](const std::vector<TextureArray>& textureArrays)
                {
                    textures = textureArrays[id].textures;
                });

            return *textures;
        }

        bool TextureHandlerVK::IsOnionTexture(const TextureID textureID)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            TextureID::type id = static_cast<TextureID::type>(textureID);

            // Lets make sure this id exists
            if (data.textures.Size() <= id)
            {
                DebugHandler::PrintFatal("Tried to access invalid TextureID: %u", id);
            }

            return data.textures.ReadGet(id)->layers != 1;
        }

        VkImageView TextureHandlerVK::GetImageView(const TextureID textureID)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            TextureID::type id = static_cast<TextureID::type>(textureID);

            // Lets make sure this id exists
            if (data.textures.Size() <= id)
            {
                DebugHandler::PrintFatal("Tried to access invalid TextureID: %u", id);
            }

            return data.textures.ReadGet(id)->imageView;
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
            if (data.textureArrays.Size() <= id)
            {
                DebugHandler::PrintFatal("Tried to access invalid TextureArrayID: %u", id);
            }

            return data.textureArrays.ReadGet(id).size;
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

            bool foundTexture = false;
            data.textures.ReadLock(
                [&](const std::vector<Texture*>& textures)
                {
                    for (auto* texture : textures)
                    {
                        if (descHash == texture->hash)
                        {
                            foundTexture = true;
                            return;
                        }
                        id++;
                    }
                });

            return foundTexture;
        }

        bool TextureHandlerVK::TryFindExistingTextureInArray(TextureArrayID textureArrayID, u64 descHash, size_t& arrayIndex, TextureID& textureID)
        {
            TextureHandlerVKData& data = static_cast<TextureHandlerVKData&>(*_data);
            TextureArrayID::type id = static_cast<TextureArrayID::type>(textureArrayID);
            if (data.textureArrays.Size() <= id)
            {
                DebugHandler::PrintFatal("Tried to access invalid TextureArrayID: %u", id);
            }

            bool foundTexture = false;
            data.textureArrays.WriteLock(
                [&](std::vector<TextureArray>& textureArrays)
                {
                    TextureArray& array = textureArrays[id];

                    for (arrayIndex = 0; arrayIndex < array.textureHashes->Size(); arrayIndex++)
                    {
                        if (descHash == array.textureHashes->ReadGet(arrayIndex))
                        {
                            textureID = array.textures->ReadGet(arrayIndex);
                            foundTexture = true;
                            return;
                        }
                    }
                });

            return foundTexture;
        }

        void TextureHandlerVK::LoadFile(const std::string& filename, Texture& texture, TextureID textureID)
        {
            ZoneScoped;

            int channels;

            void* pixels = stbi_load(filename.c_str(), &texture.width, &texture.height, &channels, STBI_rgb_alpha);

            gli::texture gliTexture;

            // If stbi could open this file
            if (pixels != nullptr)
            {
                texture.fileSize = texture.width * texture.height * channels;
                texture.format = VK_FORMAT_R8G8B8A8_UNORM;
                texture.mipLevels = 1; // If we are not loading using gli we don't support mips, so don't bother with it
                texture.layers = 1; // If we are not loading using gli we don't support layers, so don't bother with it
            }
            else
            {
                // Try to open it with gli
                gliTexture = gli::load(filename);
                if (gliTexture.empty())
                {
                    DebugHandler::PrintFatal("Failed to load texture (%s)", filename.c_str());
                }

                gli::gl gl(gli::gl::PROFILE_GL33);
                gli::gl::format const gliFormat = gl.translate(gliTexture.format(), gliTexture.swizzles());

                texture.width = gliTexture.extent().x;
                texture.height = gliTexture.extent().y;
                texture.layers = static_cast<i32>(gliTexture.layers());
                texture.mipLevels = static_cast<i32>(gliTexture.levels());

                texture.format = vkGetFormatFromOpenGLInternalFormat(gliFormat.Internal);
                texture.fileSize = gliTexture.size();

                pixels = gliTexture.data();
            }

            {
                ZoneScopedN("CreateTexture");
                // Create texture
                CreateTexture(texture);

                // Create upload buffer
                auto uploadBuffer = _uploadBufferHandler->CreateUploadBuffer(textureID, 0, texture.fileSize);

                // Copy data to upload buffer
                memcpy(uploadBuffer->mappedMemory, pixels, texture.fileSize);
            }
        }

        void TextureHandlerVK::CreateTexture(Texture& texture)
        {
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

            //_device->TransitionImageLayout(texture.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture.layers, texture.mipLevels);

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