#pragma once
#include <NovusTypes.h>
#include <vector>
#include <optional>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

#include "BufferBackendVK.h"

#include "../../../Descriptors/ImageDesc.h"
#include "../../../Descriptors/DepthImageDesc.h"

class Window;
struct GLFWwindow;

namespace tracy
{
    class VkCtx;
}

namespace Renderer
{
    namespace Backend
    {
        struct BufferBackend;
        struct BufferBackendVK;
        struct SwapChainVK;
        class ShaderHandlerVK;
        struct DescriptorMegaPoolVK;

        struct QueueFamilyIndices
        {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;

            bool IsComplete()
            {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
        };

        class RenderDeviceVK
        {
        public:
            ~RenderDeviceVK();

            void Init();
            void InitWindow(ShaderHandlerVK* shaderHandler, Window* window);

            BufferBackend* CreateBufferBackend(size_t size, Backend::BufferBackend::Type type, Backend::BufferBackend::Usage usage);

            u32 GetFrameIndex() { return _frameIndex; }
            void EndFrame() { _frameIndex = (_frameIndex + 1) % FRAME_INDEX_COUNT; }

            void FlushGPU();

        private:
            void InitOnce();

            // InitOnce helper functions
            void InitVulkan();
            void SetupDebugMessenger();
            void PickPhysicalDevice();
            void CreateLogicalDevice();
            void CreateAllocator();
            void CreateCommandPool();
            void CreateTracyContext();

            // InitWindow helper functions
            void CreateSurface(GLFWwindow* window, SwapChainVK* swapChain);
            void CreateSwapChain(const ivec2& windowSize, SwapChainVK* swapChain);
            void CreateImageViews(SwapChainVK* swapChain);
            void CreateFrameBuffers(SwapChainVK* swapChain);
            void CreateBlitPipeline(ShaderHandlerVK* shaderHandler, SwapChainVK* swapChain, std::string fragShaderName, ImageComponentType componentType);

            void CleanupSwapChain(SwapChainVK* swapChain);
            void RecreateSwapChain(ShaderHandlerVK* shaderHandler, SwapChainVK* swapChain);

            int RateDeviceSuitability(VkPhysicalDevice device);
            QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
            bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

            void CheckValidationLayerSupport();
            std::vector<const char*> GetRequiredExtensions();

            VkCommandBuffer BeginSingleTimeCommands();
            void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

            void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation);
            void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
            void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkFormat format, u32 width, u32 height, u32 numLayers, u32 numMipLevels);
            void TransitionImageLayout(VkImage image, VkImageAspectFlags aspects, VkImageLayout oldLayout, VkImageLayout newLayout, u32 numLayers, u32 numMipLevels);
            void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspects, VkImageLayout oldLayout, VkImageLayout newLayout, u32 numLayers, u32 numMipLevels);

            uvec2 GetMainWindowSize() { return _mainWindowSize; }
        private:
            uvec2 _mainWindowSize;

            static const u32 FRAME_INDEX_COUNT = 2;
            static bool _initialized;
            u32 _frameIndex;

            VkInstance _instance;
            VkDebugUtilsMessengerEXT _debugMessenger;

            VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
            VkDevice _device = VK_NULL_HANDLE;
            VkCommandPool _commandPool = VK_NULL_HANDLE;

            VkQueue _graphicsQueue = VK_NULL_HANDLE;
            VkQueue _presentQueue = VK_NULL_HANDLE;

            std::vector<BufferBackendVK*> _bufferBackends;
            std::vector<SwapChainVK*> _swapChains;

            VmaAllocator _allocator;

            DescriptorMegaPoolVK* _descriptorMegaPool;

            tracy::VkCtx* _tracyContext = nullptr;

            friend class RendererVK;
            friend struct BufferBackendVK;
            friend class ImageHandlerVK;
            friend class TextureHandlerVK;
            friend class ModelHandlerVK;
            friend class ShaderHandlerVK;
            friend class PipelineHandlerVK;
            friend class CommandListHandlerVK;
            friend class SamplerHandlerVK;
            friend class SemaphoreHandlerVK;
            friend struct DescriptorAllocatorHandleVK;
            friend class DescriptorAllocatorPoolVKImpl;
            friend class DescriptorSetBuilderVK;
        };
    }
}