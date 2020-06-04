#pragma once
#include <NovusTypes.h>
#include <vector>
#include <vulkan/vulkan.h>

#include "../../../Descriptors/TextureDesc.h"
#include "../../../Descriptors/TextureArrayDesc.h"

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;

        class TextureHandlerVK
        {
        public:
            TextureHandlerVK();
            ~TextureHandlerVK();

            void LoadDebugTexture(RenderDeviceVK* device, const TextureDesc& desc);

            TextureID LoadTexture(RenderDeviceVK* device, const TextureDesc& desc);
            TextureID LoadTextureIntoArray(RenderDeviceVK* device, const TextureDesc& desc, TextureArrayID textureArray, u32& arrayIndex);

            TextureID CreateDataTexture(RenderDeviceVK* device, const DataTextureDesc& desc);
            TextureArrayID CreateTextureArray(RenderDeviceVK* device, const TextureArrayDesc& desc);

            VkImageView GetImageView(const TextureID id);
            VkDescriptorSet GetDescriptorSet(const TextureID id);
            VkDescriptorSet GetDescriptorSet(const TextureArrayID id);

        private:
            struct Texture
            {
                i32 width;
                i32 height;

                VkFormat format;

                VkDeviceMemory memory;
                VkImage image;
                VkImageView imageView;

                VkDescriptorSetLayout descriptorSetLayout;
                VkDescriptorPool descriptorPool;
                VkDescriptorSet descriptorSet;

                std::string debugName = "";
            };

            struct TextureArray
            {
                std::vector<TextureID> textures;

                VkDescriptorSetLayout descriptorSetLayout;
                VkDescriptorPool descriptorPool;
                VkDescriptorSet descriptorSet;
            };

        private:
            u8* ReadFile(const std::string& filename, i32& width, i32& height, VkFormat& format);
            void CreateTexture(RenderDeviceVK* device, Texture& texture, u8* pixels);

        private:
            Texture _debugTexture;
            std::vector<Texture> _textures;
            std::vector<TextureArray> _textureArrays;
        };
    }
}