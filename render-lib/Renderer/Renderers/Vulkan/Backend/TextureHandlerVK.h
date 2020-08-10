#pragma once
#include <NovusTypes.h>
#include <vector>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

#include "../../../Descriptors/TextureDesc.h"
#include "../../../Descriptors/TextureArrayDesc.h"

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;
        class BufferHandlerVK;

        class TextureHandlerVK
        {
        public:
            void Init(RenderDeviceVK* device, BufferHandlerVK* bufferHandler);

            void LoadDebugTexture(const TextureDesc& desc);

            TextureID LoadTexture(const TextureDesc& desc);
            TextureID LoadTextureIntoArray(const TextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex);

            TextureArrayID CreateTextureArray(const TextureArrayDesc& desc);

            TextureID CreateDataTexture(const DataTextureDesc& desc);
            TextureID CreateDataTextureIntoArray(const DataTextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex);

            const std::vector<TextureID>& GetTextureIDsInArray(const TextureArrayID id);

            bool IsOnionTexture(const TextureID id);

            VkImageView GetImageView(const TextureID id);
            VkImageView GetDebugTextureImageView();
            VkImageView GetDebugOnionTextureImageView();

            u32 GetTextureArraySize(const TextureArrayID id);

        private:
            struct Texture
            {
                u64 hash;

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

        private:
            u64 CalculateDescHash(const TextureDesc& desc);
            bool TryFindExistingTexture(u64 descHash, size_t& id);
            bool TryFindExistingTextureInArray(TextureArrayID arrayID, u64 descHash, size_t& arrayIndex, TextureID& textureId);

            u8* ReadFile(const std::string& filename, i32& width, i32& height, i32& layers, i32& mipLevels, VkFormat& format, size_t& fileSize);
            void CreateTexture(Texture& texture, u8* pixels);

        private:
            RenderDeviceVK* _device;
            BufferHandlerVK* _bufferHandler;

            TextureID _debugTexture;
            TextureID _debugOnionTexture; // "TextureArrays" using texture layers rather than arrays of descriptors are now called Onion Textures to make it possible to differentiate between them...

            std::vector<Texture> _textures;
            std::vector<TextureArray> _textureArrays;
        };
    }
}