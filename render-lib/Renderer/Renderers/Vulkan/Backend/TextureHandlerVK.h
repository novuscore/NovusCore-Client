#pragma once
#include <NovusTypes.h>
#include <vulkan/vulkan_core.h>

#include "../../../Descriptors/TextureDesc.h"
#include "../../../Descriptors/TextureArrayDesc.h"

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;
        class BufferHandlerVK;
        struct Texture;

        struct ITextureHandlerVKData {};

        class TextureHandlerVK
        {
        public:
            void Init(RenderDeviceVK* device, BufferHandlerVK* bufferHandler);

            void LoadDebugTexture(const TextureDesc& desc);

            TextureID LoadTexture(const TextureDesc& desc);
            TextureID LoadTextureIntoArray(const TextureDesc& desc, TextureArrayID textureArrayID, u32& arrayIndex);

            void UnloadTexture(const TextureID textureID);
            void UnloadTexturesInArray(const TextureArrayID textureArrayID, u32 unloadStartIndex);

            TextureArrayID CreateTextureArray(const TextureArrayDesc& desc);

            TextureID CreateDataTexture(const DataTextureDesc& desc);
            TextureID CreateDataTextureIntoArray(const DataTextureDesc& desc, TextureArrayID textureArrayID, u32& arrayIndex);

            const std::vector<TextureID>& GetTextureIDsInArray(const TextureArrayID textureID);

            bool IsOnionTexture(const TextureID textureID);

            VkImageView GetImageView(const TextureID textureID);
            VkImageView GetDebugTextureImageView();
            VkImageView GetDebugOnionTextureImageView();

            u32 GetTextureArraySize(const TextureArrayID textureArrayID);

        private:
            u64 CalculateDescHash(const TextureDesc& desc);
            bool TryFindExistingTexture(u64 descHash, size_t& id);
            bool TryFindExistingTextureInArray(TextureArrayID textureArrayID, u64 descHash, size_t& arrayIndex, TextureID& textureID);

            u8* ReadFile(const std::string& filename, i32& width, i32& height, i32& layers, i32& mipLevels, VkFormat& format, size_t& fileSize);
            void CreateTexture(Texture& texture, u8* pixels);

        private:
            ITextureHandlerVKData* _data;

            RenderDeviceVK* _device;
            BufferHandlerVK* _bufferHandler;

            TextureID _debugTexture;
            TextureID _debugOnionTexture; // "TextureArrays" using texture layers rather than arrays of descriptors are now called Onion Textures to make it possible to differentiate between them...
        };
    }
}