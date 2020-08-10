#include "RenderDeviceVK.h"
#include <Utils/DebugHandler.h>
#include <vector>
#include "vk_format_utils.h"
#include <tracy/TracyVulkan.hpp>

#include "../../../../Window/Window.h"
#include "DebugMarkerUtilVK.h"
#include "SwapChainVK.h"
#include "ShaderHandlerVK.h"
#include "../../../Descriptors/VertexShaderDesc.h"
#include "../../../Descriptors/PixelShaderDesc.h"
#include "DescriptorSetBuilderVK.h"

#pragma warning (push)
#pragma warning(disable : 4005)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma warning(pop)
#include <map>
#include <set>

#define NOVUSCORE_RENDERER_DEBUG_OVERRIDE 0
#define NOVUSCORE_RENDERER_GPU_VALIDATION 0

namespace Renderer
{
    namespace Backend
    {
        bool RenderDeviceVK::_initialized = false;

        const std::vector<const char*> validationLayers =
        {
            "VK_LAYER_KHRONOS_validation"
        };

        const std::vector<const char*> deviceExtensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            //"VK_KHR_get_physical_device_properties2",
            "VK_KHR_maintenance3",
            "VK_EXT_descriptor_indexing"
        };

        RenderDeviceVK::~RenderDeviceVK()
        {
            // TODO: All cleanup

            delete _descriptorMegaPool;
        }

        void RenderDeviceVK::Init()
        {
            if (!_initialized)
                InitOnce();
        }

        void RenderDeviceVK::InitWindow(ShaderHandlerVK* shaderHandler, Window* window)
        {
            if (!_initialized)
            {
                NC_LOG_FATAL("You need to intialize the render device before intializing a window");
            }

            GLFWwindow* glfwWindow = window->GetWindow();
            
            ivec2 size;
            glfwGetWindowSize(glfwWindow, &size.x, &size.y);

            _mainWindowSize = size;

            // -- Create our swapchain abstraction and give it to the window --
            SwapChainVK* swapChain = new SwapChainVK(this);
            swapChain->window = glfwWindow;
            window->SetSwapChain(swapChain);
            _swapChains.push_back(swapChain);

            // -- Create the swapchain --
            CreateSurface(glfwWindow, swapChain);
            CreateSwapChain(size, swapChain);
            CreateImageViews(swapChain);
            CreateFrameBuffers(swapChain);
            CreateBlitPipeline(shaderHandler, swapChain, "blitFloat", IMAGE_COMPONENT_TYPE_FLOAT);
            CreateBlitPipeline(shaderHandler, swapChain, "blitUint", IMAGE_COMPONENT_TYPE_UINT);
            CreateBlitPipeline(shaderHandler, swapChain, "blitInt", IMAGE_COMPONENT_TYPE_SINT);
        }

        void RenderDeviceVK::FlushGPU()
        {

        }

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData)
        {
            if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                NC_LOG_ERROR("Validation layer: %s", pCallbackData->pMessage)
            }
            else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            {
                NC_LOG_WARNING("Validation layer: %s", pCallbackData->pMessage)
            }

            return VK_FALSE;
        }

        void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
        {
            createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = DebugCallback;
        }

        void RenderDeviceVK::InitOnce()
        {
#if _DEBUG || NOVUSCORE_RENDERER_DEBUG_OVERRIDE
            DebugMarkerUtilVK::SetDebugMarkersEnabled(true);
#endif
            InitVulkan();
            SetupDebugMessenger();
            PickPhysicalDevice();
            CreateLogicalDevice();
            CreateAllocator();
            CreateCommandPool();
            CreateTracyContext();

            _descriptorMegaPool = new DescriptorMegaPoolVK();
            _descriptorMegaPool->Init(FRAME_INDEX_COUNT, this);

            _initialized = true;
        }

        void RenderDeviceVK::InitVulkan()
        {
#if _DEBUG || NOVUSCORE_RENDERER_DEBUG_OVERRIDE
            // Check validation layer support
            CheckValidationLayerSupport();
#endif

            // -- Initialize Vulkan --
            VkApplicationInfo appInfo = {};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "NovusCore Client";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "NovusCore";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_0;

#if (_DEBUG || NOVUSCORE_RENDERER_DEBUG_OVERRIDE) && NOVUSCORE_RENDERER_GPU_VALIDATION
            VkValidationFeatureEnableEXT gpuValidationFeature = VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT;

            VkValidationFeaturesEXT validationFeatures = {};
            validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            validationFeatures.enabledValidationFeatureCount = 1;
            validationFeatures.pEnabledValidationFeatures = &gpuValidationFeature;
#endif

            VkInstanceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            // Check extensions
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
            std::vector<VkExtensionProperties> extensions(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

            NC_LOG_MESSAGE("[Renderer]: Supported extensions:");
            for (VkExtensionProperties& extension : extensions)
            {
                NC_LOG_MESSAGE("[Renderer]: %s", extension.extensionName);
            }

            auto requiredExtensions = GetRequiredExtensions();

            createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
            createInfo.ppEnabledExtensionNames = requiredExtensions.data();

#if _DEBUG || NOVUSCORE_RENDERER_DEBUG_OVERRIDE
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            PopulateDebugMessengerCreateInfo(debugCreateInfo);

#if NOVUSCORE_RENDERER_GPU_VALIDATION
            createInfo.pNext = &validationFeatures;
            validationFeatures.pNext = &debugCreateInfo;
#else
            createInfo.pNext = &debugCreateInfo;
#endif
            
#else
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
#endif
            if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create Vulkan instance!");
            }
        }

        VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
        {
            auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr)
            {
                return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
            }
            else
            {
                return VK_ERROR_EXTENSION_NOT_PRESENT;
            }
        }

        void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr)
            {
                func(instance, debugMessenger, pAllocator);
            }
        }

        void RenderDeviceVK::SetupDebugMessenger()
        {
#if _DEBUG || NOVUSCORE_RENDERER_DEBUG_OVERRIDE
            VkDebugUtilsMessengerCreateInfoEXT createInfo;
            PopulateDebugMessengerCreateInfo(createInfo);

            if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to set up Vulkan debug messenger!");
            }
#endif
        }

        void RenderDeviceVK::PickPhysicalDevice()
        {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

            if (deviceCount == 0)
            {
                NC_LOG_FATAL("Failed to find GPUs with Vulkan support!");
            }

            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

            // Use an ordered map to automatically sort candidates by increasing score
            std::multimap<int, VkPhysicalDevice> candidates;

            for (const auto& device : devices)
            {
                int score = RateDeviceSuitability(device);
                candidates.insert(std::make_pair(score, device));
            }

            // Check if the best candidate is suitable at all
            if (candidates.rbegin()->first > 0)
            {
                _physicalDevice = candidates.rbegin()->second;
            }
            else
            {
                NC_LOG_FATAL("Failed to find a suitable Vulkan GPU!");
            }
        }

        void RenderDeviceVK::CreateLogicalDevice()
        {
            QueueFamilyIndices indices = FindQueueFamilies(_physicalDevice);

            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

            float queuePriority = 1.0f;
            for (uint32_t queueFamily : uniqueQueueFamilies) 
            {
                VkDeviceQueueCreateInfo queueCreateInfo = {};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures = {};
            descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
            descriptorIndexingFeatures.runtimeDescriptorArray = true;

            VkPhysicalDeviceFeatures2 deviceFeatures = {};
            deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            deviceFeatures.features.samplerAnisotropy = VK_TRUE;
            deviceFeatures.features.fragmentStoresAndAtomics = VK_TRUE;
            deviceFeatures.features.vertexPipelineStoresAndAtomics = VK_TRUE;
            deviceFeatures.pNext = &descriptorIndexingFeatures;


            VkDeviceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

            createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            createInfo.pQueueCreateInfos = queueCreateInfos.data();
            createInfo.pEnabledFeatures = NULL; 
            createInfo.pNext = &deviceFeatures;

            std::vector<const char*> enabledExtensions;
            for (const char* extension : deviceExtensions)
            {
                enabledExtensions.push_back(extension);
            }
            DebugMarkerUtilVK::AddEnabledExtension(enabledExtensions);

            createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
            createInfo.ppEnabledExtensionNames = enabledExtensions.data();

#if _DEBUG || NOVUSCORE_RENDERER_DEBUG_OVERRIDE
            std::vector<const char*> enabledLayers;
            for (const char* layer : validationLayers)
            {
                enabledLayers.push_back(layer);
            }
            DebugMarkerUtilVK::AddValidationLayer(enabledLayers);

            createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
            createInfo.ppEnabledLayerNames = enabledLayers.data();
#else
            createInfo.enabledLayerCount = 0;
#endif

            if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create logical device!");
            }

            DebugMarkerUtilVK::InitializeFunctions(_device);

            vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
            vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);
        }

        void RenderDeviceVK::CreateAllocator()
        {
            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice = _physicalDevice;
            allocatorInfo.device = _device;
            allocatorInfo.instance = _instance;

            vmaCreateAllocator(&allocatorInfo, &_allocator);
        }

        void RenderDeviceVK::CreateCommandPool()
        {
            QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(_physicalDevice);

            VkCommandPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

            if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create command pool!");
            }
        }

        void RenderDeviceVK::CreateTracyContext()
        {
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = _commandPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer tracyBuffer;
            vkAllocateCommandBuffers(_device, &allocInfo, &tracyBuffer);

            _tracyContext = TracyVkContext(_physicalDevice, _device, _graphicsQueue, tracyBuffer);

            vkQueueWaitIdle(_graphicsQueue);
            vkFreeCommandBuffers(_device, _commandPool, 1, &tracyBuffer);
        }

        void RenderDeviceVK::CreateSurface(GLFWwindow* window, SwapChainVK* swapChain)
        {
            if (glfwCreateWindowSurface(_instance, window, nullptr, &swapChain->surface) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create window surface!");
            }
        }

        void RenderDeviceVK::CreateSwapChain(const ivec2& windowSize, SwapChainVK* swapChain)
        {
            SwapChainSupportDetails swapChainSupport = swapChain->QuerySwapChainSupport(_physicalDevice);

            if (SwapChainVK::FRAME_BUFFER_COUNT < swapChainSupport.capabilities.minImageCount || SwapChainVK::FRAME_BUFFER_COUNT > swapChainSupport.capabilities.maxImageCount)
            {
                NC_LOG_FATAL("Physical device does not support the requested number of swapchain images, you requested %u, and it supports between %u and %u", SwapChainVK::FRAME_BUFFER_COUNT, swapChainSupport.capabilities.minImageCount, swapChainSupport.capabilities.maxImageCount);
            }

            VkSurfaceFormatKHR surfaceFormat = swapChain->ChooseSwapSurfaceFormat(swapChainSupport.formats);
            VkPresentModeKHR presentMode = swapChain->ChooseSwapPresentMode(swapChainSupport.presentModes);
            VkExtent2D extent = swapChain->ChooseSwapExtent(windowSize, swapChainSupport.capabilities);

            VkSwapchainCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = swapChain->surface;

            createInfo.minImageCount = SwapChainVK::FRAME_BUFFER_COUNT;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = FindQueueFamilies(_physicalDevice);
            uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, queueFamilyIndices[1], swapChain->surface, &presentSupport);

            if (!presentSupport)
            {
                NC_LOG_FATAL("Selected physical device does not support presenting!");
            }

            if (indices.graphicsFamily != indices.presentFamily)
            {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            else
            {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount = 0; // Optional
                createInfo.pQueueFamilyIndices = nullptr; // Optional
            }

            createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;

            createInfo.oldSwapchain = VK_NULL_HANDLE;

            if (vkCreateSwapchainKHR(_device, &createInfo, nullptr, &swapChain->swapChain) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create swap chain!");
            }
            
            u32 imageCount = SwapChainVK::FRAME_BUFFER_COUNT;
            vkGetSwapchainImagesKHR(_device, swapChain->swapChain, &imageCount, nullptr);
            vkGetSwapchainImagesKHR(_device, swapChain->swapChain, &imageCount, swapChain->images.items.data());

            swapChain->imageFormat = surfaceFormat.format;
            swapChain->extent = extent;
        }

        void RenderDeviceVK::CreateImageViews(SwapChainVK* swapChain)
        {
            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            for (size_t i = 0; i < SwapChainVK::FRAME_BUFFER_COUNT; i++)
            {
                VkImageViewCreateInfo viewInfo = {};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = swapChain->images.Get(i);
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = swapChain->imageFormat;
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.baseMipLevel = 0;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseArrayLayer = 0;
                viewInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(_device, &viewInfo, nullptr, &swapChain->imageViews.Get(i)) != VK_SUCCESS)
                {
                    NC_LOG_FATAL("Failed to create texture image view!");
                }

                if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &swapChain->imageAvailableSemaphores.Get(i)) != VK_SUCCESS)
                {
                    NC_LOG_FATAL("Failed to create image available semaphore!");
                }

                if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &swapChain->blitFinishedSemaphores.Get(i)) != VK_SUCCESS)
                {
                    NC_LOG_FATAL("Failed to create blit finished semaphore!");
                }
            }
        }

        void RenderDeviceVK::CreateFrameBuffers(SwapChainVK* swapChain)
        {
            // Create a dummy renderpass
            VkAttachmentDescription colorAttachment = {};
            colorAttachment.format = swapChain->imageFormat;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference colorAttachmentRef = {};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;

            VkSubpassDependency dependency = {};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;// | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;// | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = 1;
            renderPassInfo.pAttachments = &colorAttachment;
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &swapChain->renderPass) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create render pass!");
            }

            // Create framebuffers
            for (size_t i = 0; i < SwapChainVK::FRAME_BUFFER_COUNT; i++)
            {
                VkFramebufferCreateInfo framebufferInfo = {};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = swapChain->renderPass;
                framebufferInfo.attachmentCount = 1;
                framebufferInfo.pAttachments = &swapChain->imageViews.Get(i);
                framebufferInfo.width = swapChain->extent.width;
                framebufferInfo.height = swapChain->extent.height;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &swapChain->framebuffers.Get(i)) != VK_SUCCESS)
                {
                    NC_LOG_FATAL("Failed to create framebuffer!");
                }
            }

            // Create sampler
            VkSamplerCreateInfo samplerInfo = {};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_NEAREST;
            samplerInfo.minFilter = VK_FILTER_NEAREST;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = 16;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 0.0f;

            if (vkCreateSampler(_device, &samplerInfo, nullptr, &swapChain->sampler) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create texture sampler!");
            }
        }

        void RenderDeviceVK::CreateBlitPipeline(ShaderHandlerVK* shaderHandler, SwapChainVK* swapChain, std::string fragShaderName, ImageComponentType componentType)
        {
            BlitPipeline& pipeline = swapChain->blitPipelines[componentType];

            // Load shaders
            VertexShaderDesc vertexShaderDesc;
            vertexShaderDesc.path = "Data/shaders/blit.vs.hlsl.spv";
            VertexShaderID vertexShader = shaderHandler->LoadShader(vertexShaderDesc);

            PixelShaderDesc pixelShaderDesc;
            pixelShaderDesc.path = "Data/shaders/" + fragShaderName + ".ps.hlsl.spv";
            PixelShaderID pixelShader = shaderHandler->LoadShader(pixelShaderDesc);

            // Create shader stage infos
            VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
            vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

            vertShaderStageInfo.module = shaderHandler->GetShaderModule(vertexShader);
            vertShaderStageInfo.pName = "main";

            VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
            fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

            fragShaderStageInfo.module = shaderHandler->GetShaderModule(pixelShader);
            fragShaderStageInfo.pName = "main";

            VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

            // Create Descriptor Set Layout from reflected SPIR-V
            std::vector<BindInfo> bindInfos;
            {
                const BindReflection& bindReflection = shaderHandler->GetBindReflection(vertexShader);
                bindInfos.insert(bindInfos.end(), bindReflection.dataBindings.begin(), bindReflection.dataBindings.end());
            }
            {
                const BindReflection& bindReflection = shaderHandler->GetBindReflection(pixelShader);
                bindInfos.insert(bindInfos.end(), bindReflection.dataBindings.begin(), bindReflection.dataBindings.end());
            }

            std::vector<VkDescriptorSetLayoutBinding> bindings;

            for (BindInfo& bindInfo : bindInfos)
            {
                VkDescriptorSetLayoutBinding layoutBinding = {};

                layoutBinding.binding = bindInfo.binding;
                layoutBinding.descriptorType = bindInfo.descriptorType;
                layoutBinding.descriptorCount = bindInfo.count;
                layoutBinding.stageFlags = bindInfo.stageFlags;

                bindings.push_back(layoutBinding);
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<u32>(bindings.size());
            layoutInfo.pBindings = bindings.data();

            if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &pipeline.descriptorSetLayout) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor set layout!");
            }

            // Create descriptor pool
            VkDescriptorPoolSize descriptorPoolSizes[2] = {};
            descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLER;
            descriptorPoolSizes[0].descriptorCount = 3;
            descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorPoolSizes[1].descriptorCount = 3;

            VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
            descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptorPoolInfo.poolSizeCount = 2;
            descriptorPoolInfo.pPoolSizes = descriptorPoolSizes;
            descriptorPoolInfo.maxSets = 2;

            if (vkCreateDescriptorPool(_device, &descriptorPoolInfo, nullptr, &pipeline.descriptorPool) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create descriptor pool!");
            }

            // Create descriptor set
            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = pipeline.descriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &pipeline.descriptorSetLayout;

            for (u32 i = 0; i < pipeline.descriptorSets.Num; i++)
            {
                VkResult result = vkAllocateDescriptorSets(_device, &allocInfo, &pipeline.descriptorSets.Get(i));
                if (result != VK_SUCCESS)
                {
                    NC_LOG_FATAL("Failed to allocate descriptor sets!");
                }
            }

            // No vertex info
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 0;
            vertexInputInfo.pVertexBindingDescriptions = nullptr;
            vertexInputInfo.vertexAttributeDescriptionCount = 0;
            vertexInputInfo.pVertexAttributeDescriptions = nullptr;

            VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            // Set viewport and scissor rect
            VkViewport viewport = {};
            viewport.x = 0;
            viewport.y = 0;
            viewport.width = static_cast<f32>(swapChain->extent.width);
            viewport.height = static_cast<f32>(swapChain->extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor = {};
            scissor.offset = { 0, 0 };
            scissor.extent = swapChain->extent;

            VkPipelineViewportStateCreateInfo viewportState = {};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.pViewports = &viewport;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &scissor;

            // Rasterizer
            VkPipelineRasterizationStateCreateInfo rasterizer = {};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
            rasterizer.depthBiasEnable = false;
            rasterizer.depthBiasConstantFactor = 0.0f;
            rasterizer.depthBiasClamp = 0.0f;
            rasterizer.depthBiasSlopeFactor = 1.0f;

            // Multisampling
            VkPipelineMultisampleStateCreateInfo multisampling = {};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisampling.minSampleShading = 1.0f;
            multisampling.pSampleMask = nullptr;
            multisampling.alphaToCoverageEnable = VK_FALSE;
            multisampling.alphaToOneEnable = VK_FALSE;

            // Blender
            VkPipelineColorBlendAttachmentState colorBlendAttachment;
            colorBlendAttachment.blendEnable = false;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

            VkPipelineColorBlendStateCreateInfo colorBlending = {};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = false;
            colorBlending.logicOp = VK_LOGIC_OP_NO_OP;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;
            colorBlending.blendConstants[0] = 0.0f;
            colorBlending.blendConstants[1] = 0.0f;
            colorBlending.blendConstants[2] = 0.0f;
            colorBlending.blendConstants[3] = 0.0f;

            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &pipeline.descriptorSetLayout;
            pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
            pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

            if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &pipeline.pipelineLayout) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create pipeline layout!");
            }

            VkGraphicsPipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount = 2;
            pipelineInfo.pStages = shaderStages;
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = nullptr; // Optional
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.pDynamicState = nullptr; // Optional
            pipelineInfo.layout = pipeline.pipelineLayout;
            pipelineInfo.renderPass = swapChain->renderPass;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
            pipelineInfo.basePipelineIndex = -1; // Optional

            if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline.pipeline) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create graphics pipeline!");
            }
        }

        void RenderDeviceVK::CleanupSwapChain(SwapChainVK* swapChain)
        {
            // Destroy frame buffers
            for (u32 i = 0; i < swapChain->framebuffers.Num; i++)
            {
                vkDestroyFramebuffer(_device, swapChain->framebuffers.Get(i), nullptr);
            }

            // Free command buffers *NOT SURE IF NEEDED?*

            // Destroy blit pipelines
            for (u32 i = 0; i < 3; i++)
            {
                BlitPipeline& pipeline = swapChain->blitPipelines[i];

                // Destroy pipeline
                vkDestroyPipeline(_device, pipeline.pipeline, nullptr);

                // Destroy pipeline layout
                vkDestroyPipelineLayout(_device, pipeline.pipelineLayout, nullptr);

                // Destroy descriptor set layout
                vkDestroyDescriptorSetLayout(_device, pipeline.descriptorSetLayout, nullptr);

                // Destroy descriptor pool
                vkDestroyDescriptorPool(_device, pipeline.descriptorPool, nullptr);
            }

            // Destroy renderpass
            vkDestroyRenderPass(_device, swapChain->renderPass, nullptr);

            // Destroy imageviews
            for (u32 i = 0; i < swapChain->imageViews.Num; i++)
            {
                vkDestroyImageView(_device, swapChain->imageViews.Get(i), nullptr);
                
            }

            // Destroy semaphores
            for (u32 i = 0; i < swapChain->imageAvailableSemaphores.Num; i++)
            {
                vkDestroySemaphore(_device, swapChain->imageAvailableSemaphores.Get(i), nullptr);
            }
            for (u32 i = 0; i < swapChain->blitFinishedSemaphores.Num; i++)
            {
                vkDestroySemaphore(_device, swapChain->blitFinishedSemaphores.Get(i), nullptr);
            }
            
            // Destroy swap chain
            vkDestroySwapchainKHR(_device, swapChain->swapChain, nullptr);

            // Destroy glfw surface
            vkDestroySurfaceKHR(_instance, swapChain->surface, nullptr);
        }

        void RenderDeviceVK::RecreateSwapChain(ShaderHandlerVK* shaderHandler, SwapChainVK* swapChain)
        {
            // Get the new size
            ivec2 size;
            glfwGetWindowSize(swapChain->window, &size.x, &size.y);

            if (size.x <= 0 || size.y <= 0)
                return;

            _mainWindowSize = size;

            vkDeviceWaitIdle(_device); // Wait for any in progress rendering to finish

            // Destroy the parts of the swapchain we need to destroy
            CleanupSwapChain(swapChain);

            // Recreate the swapchain
            CreateSurface(swapChain->window, swapChain);
            CreateSwapChain(size, swapChain);
            CreateImageViews(swapChain);
            CreateFrameBuffers(swapChain);

            // Recreate blit pipelines
            CreateBlitPipeline(shaderHandler, swapChain, "blitFloat", IMAGE_COMPONENT_TYPE_FLOAT);
            CreateBlitPipeline(shaderHandler, swapChain, "blitUint", IMAGE_COMPONENT_TYPE_UINT);
            CreateBlitPipeline(shaderHandler, swapChain, "blitInt", IMAGE_COMPONENT_TYPE_SINT);
        }

        int RenderDeviceVK::RateDeviceSuitability(VkPhysicalDevice device)
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            int score = 0;

            // Discrete GPUs have a significant performance advantage
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                score += 1000;
            }

            // Maximum possible size of textures affects graphics quality
            score += deviceProperties.limits.maxImageDimension2D;

            if (!deviceFeatures.samplerAnisotropy)
            {
                NC_LOG_MESSAGE("[Renderer]: GPU Detected %s with score %i because it doesn't support sampler anisotropy", deviceProperties.deviceName, 0);
                return 0;
            }

            // Application can't function without geometry shaders
            if (!deviceFeatures.geometryShader)
            {
                NC_LOG_MESSAGE("[Renderer]: GPU Detected %s with score %i because it doesn't support geometry shaders", deviceProperties.deviceName, 0);
                return 0;
            }

            // Application requires VK_QUEUE_GRAPHICS_BIT
            QueueFamilyIndices indices = FindQueueFamilies(device);
            if (!indices.IsComplete())
            {
                NC_LOG_MESSAGE("[Renderer]: GPU Detected %s with score %i because it doesn't support VK_QUEUE_GRAPHICS_BIT", deviceProperties.deviceName, 0);
                return 0;
            }

            // Make sure it supports the extensions we need
            bool extensionsSupported = CheckDeviceExtensionSupport(device);
            if (!extensionsSupported)
            {
                NC_LOG_MESSAGE("[Renderer]: GPU Detected %s with score %i because it doesn't support the required extensions", deviceProperties.deviceName, 0);
                return 0;
            }

            NC_LOG_MESSAGE("[Renderer]: GPU Detected %s with score %i", deviceProperties.deviceName, score);
            return score;
        }

        QueueFamilyIndices RenderDeviceVK::FindQueueFamilies(VkPhysicalDevice device)
        {
            QueueFamilyIndices indices;

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            int i = 0;
            for (const auto& queueFamily : queueFamilies)
            {
                if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    indices.graphicsFamily = i;
                }
                
                VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
                VkSurfaceKHR surface;
                vkCreateWin32SurfaceKHR(_instance, &surfaceCreateInfo, nullptr, &surface);

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                if (queueFamily.queueCount > 0 && presentSupport) {
                    indices.presentFamily = i;
                }

                vkDestroySurfaceKHR(_instance, surface, nullptr);

                if (indices.IsComplete())
                {
                    break;
                }

                i++;
            }

            return indices;
        }

        bool RenderDeviceVK::CheckDeviceExtensionSupport(VkPhysicalDevice device)
        {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

            for (const auto& extension : availableExtensions)
            {
                requiredExtensions.erase(extension.extensionName);

                DebugMarkerUtilVK::CheckExtension(extension);
            }

            return requiredExtensions.empty();
        }

        void RenderDeviceVK::CheckValidationLayerSupport()
        {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            for (const char* layerName : validationLayers)
            {
                bool layerFound = false;

                for (const auto& layerProperties : availableLayers)
                {
                    if (strcmp(layerName, layerProperties.layerName) == 0)
                    {
                        layerFound = true;
                        break;
                    }
                }

                if (!layerFound)
                {
                    NC_LOG_FATAL("We do not support a validation layer that we need to support: %s", layerName);
                }
            }
        }

        std::vector<const char*> RenderDeviceVK::GetRequiredExtensions()
        {
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if _DEBUG || NOVUSCORE_RENDERER_DEBUG_OVERRIDE
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
            extensions.push_back("VK_KHR_get_physical_device_properties2");

            return extensions;
        }

        VkCommandBuffer RenderDeviceVK::BeginSingleTimeCommands()
        {
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = _commandPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            return commandBuffer;
        }

        void RenderDeviceVK::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
        {
            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(_graphicsQueue);

            vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
        }

        void RenderDeviceVK::CopyBuffer(VkBuffer dstBuffer, u64 dstOffset, VkBuffer srcBuffer, u64 srcOffset, u64 range)
        {
            VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

            VkBufferCopy copyRegion = {};
            copyRegion.srcOffset = srcOffset;
            copyRegion.dstOffset = dstOffset;
            copyRegion.size = range;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

            EndSingleTimeCommands(commandBuffer);
        }

        void RenderDeviceVK::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkFormat format, u32 width, u32 height, u32 numLayers, u32 numMipLevels)
        {
            VkDeviceSize bufferOffset = 0;

            VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

            std::vector<VkBufferImageCopy> regions;
            regions.reserve(numMipLevels);

            u32 curWidth = width;
            u32 curHeight = height;

            for (u32 i = 0; i < numMipLevels; i++)
            {
                VkBufferImageCopy& region = regions.emplace_back();

                region.bufferOffset = bufferOffset;
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;

                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = i;
                region.imageSubresource.baseArrayLayer = 0;
                region.imageSubresource.layerCount = numLayers;

                region.imageOffset = { 0, 0, 0 };
                region.imageExtent = {
                    curWidth,
                    curHeight,
                    1
                };

                if (!FormatIsCompressed(format)) // Uncompressed
                {
                    bufferOffset += static_cast<VkDeviceSize>(glm::ceil(curWidth * curHeight * FormatTexelSize(format)));
                }
                else if (FormatIsCompressed_BC(format)) // BC compression
                {
                    VkExtent3D texelExtent = FormatTexelBlockExtent(format);

                    vec2 blocks = vec2(curWidth, curHeight) / vec2(texelExtent.width, texelExtent.height); // Calculate how many blocks we have 
                    blocks = glm::ceil(blocks); // If we have fractional blocks, ceil it
                    u32 blockSize = FormatElementSize(format, VK_IMAGE_ASPECT_COLOR_BIT); // Get size in bytes per block

                    bufferOffset += static_cast<VkDeviceSize>(blocks.x * blocks.y * blockSize);
                }
                else
                {
                    NC_LOG_FATAL("Tried to use a format that wasn't uncompressed or used BC compression, what is this? id: %u", format)
                }

                curWidth /= 2;
                curHeight /= 2;
            }
            
            vkCmdCopyBufferToImage(
                commandBuffer,
                srcBuffer,
                dstImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                numMipLevels,
                regions.data()
            );

            EndSingleTimeCommands(commandBuffer);
        }

        void RenderDeviceVK::TransitionImageLayout(VkImage image, VkImageAspectFlags aspects, VkImageLayout oldLayout, VkImageLayout newLayout, u32 numLayers, u32 numMipLevels)
        {
            VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

            TransitionImageLayout(commandBuffer, image, aspects, oldLayout, newLayout, numLayers, numMipLevels);

            EndSingleTimeCommands(commandBuffer);
        }

        void RenderDeviceVK::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspects, VkImageLayout oldLayout, VkImageLayout newLayout, u32 numLayers, u32 numMipLevels)
        {
            VkImageMemoryBarrier imageBarrier = {};
            imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarrier.pNext = NULL;
            imageBarrier.oldLayout = oldLayout;
            imageBarrier.newLayout = newLayout;
            imageBarrier.image = image;
            imageBarrier.subresourceRange.aspectMask = aspects;
            imageBarrier.subresourceRange.baseMipLevel = 0;
            imageBarrier.subresourceRange.levelCount = numMipLevels;
            imageBarrier.subresourceRange.layerCount = numLayers;

            VkPipelineStageFlagBits srcFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlagBits dstFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            switch (oldLayout) 
            {
                case VK_IMAGE_LAYOUT_PREINITIALIZED:
                    imageBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    srcFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                    break;
                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                    imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    srcFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    break;
            }

            switch (newLayout) 
            {
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    dstFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    break;
                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                    imageBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
                    imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    break;
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    imageBarrier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    dstFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    imageBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    srcFlags = VkPipelineStageFlagBits(VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT);
                    dstFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    break;
            }
            
            vkCmdPipelineBarrier(commandBuffer, srcFlags, dstFlags, 0, 0, NULL, 0, NULL, 1, &imageBarrier);
        }
    }
}