#include "RenderDeviceVK.h"
#include "vk_format_utils.h"
#include "DebugMarkerUtilVK.h"
#include "SwapChainVK.h"
#include "ImageHandlerVK.h"
#include "SemaphoreHandlerVK.h"
#include "DescriptorSetBuilderVK.h"
#include "../../../../Window/Window.h"
#include "../../../Descriptors/VertexShaderDesc.h"
#include "../../../Descriptors/PixelShaderDesc.h"
#include "imgui/imgui_impl_vulkan.h"

#include <map>
#include <set>
#include <vector>
#include <vulkan/vulkan.h>
#include <Utils/DebugHandler.h>
#include <tracy/TracyVulkan.hpp>

#pragma warning (push)
#pragma warning(disable : 4005)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma warning(pop)

#define ENABLE_VALIDATION_IN_DEBUG 0

namespace Renderer
{
    namespace Backend
    {
        bool RenderDeviceVK::_initialized = false;
        PFN_vkCmdDrawIndexedIndirectCountKHR RenderDeviceVK::fnVkCmdDrawIndexedIndirectCountKHR = nullptr;

        const std::vector<const char*> validationLayers =
        {
            "VK_LAYER_KHRONOS_validation"
        };

        const std::vector<const char*> deviceExtensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            //"VK_KHR_get_physical_device_properties2",
            "VK_KHR_maintenance1",
            "VK_KHR_maintenance3",
            "VK_EXT_descriptor_indexing",
            "VK_KHR_draw_indirect_count",
            "VK_KHR_shader_subgroup_extended_types"
        };

        struct ImguiContext
        {
            VkRenderPass imguiPass;
            VkDescriptorPool imguiPool;
        };

        RenderDeviceVK::~RenderDeviceVK()
        {
            // TODO: All cleanup

            delete _descriptorMegaPool;
            delete _imguiContext;
        }

        void RenderDeviceVK::Init()
        {
            if (!_initialized)
                InitOnce();
        }

        void RenderDeviceVK::InitWindow(ImageHandlerVK* imageHandler, SemaphoreHandlerVK* semaphoreHandler, Window* window)
        {
            if (!_initialized)
            {
                DebugHandler::PrintFatal("You need to intialize the render device before intializing a window");
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

            VkFormat format;
            CreateSwapChain(size, swapChain, format);

            CreateImages(swapChain, imageHandler, format);
            CreateSemaphores(swapChain, semaphoreHandler);
        }

        void RenderDeviceVK::FlushGPU()
        {
            vkDeviceWaitIdle(_device); // Wait for any in progress rendering to finish
        }

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData)
        {
            if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            {
                DebugHandler::PrintError("Validation layer: %s", pCallbackData->pMessage);
            }
            else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            {
                DebugHandler::PrintWarning("Validation layer: %s", pCallbackData->pMessage);
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
//#if _DEBUG
            DebugMarkerUtilVK::SetDebugMarkersEnabled(true);
//#endif
            InitVulkan();
            SetupDebugMessenger();
            PickPhysicalDevice();
            CreateLogicalDevice();
            CreateAllocator();
            CreateCommandPool();
            CreateTracyContext();

            _descriptorMegaPool = new DescriptorMegaPoolVK();
            _descriptorMegaPool->Init(FRAME_INDEX_COUNT, this);

            fnVkCmdDrawIndexedIndirectCountKHR = (PFN_vkCmdDrawIndexedIndirectCountKHR)vkGetDeviceProcAddr(_device, "vkCmdDrawIndexedIndirectCountKHR");

            _initialized = true;
        }

        void RenderDeviceVK::InitializeImguiVulkan()
        {
            if (_imguiContext == nullptr)
            {
                _imguiContext = new ImguiContext();
            }

            // Create the Render Pass
            {
                VkAttachmentDescription attachment = {};
                attachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
                attachment.samples = VK_SAMPLE_COUNT_1_BIT;
                attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                VkAttachmentReference color_attachment = {};
                color_attachment.attachment = 0;
                color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                VkSubpassDescription subpass = {};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &color_attachment;
                VkSubpassDependency dependency = {};
                dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                dependency.dstSubpass = 0;
                dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask = 0;
                dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                VkRenderPassCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                info.attachmentCount = 1;
                info.pAttachments = &attachment;
                info.subpassCount = 1;
                info.pSubpasses = &subpass;
                info.dependencyCount = 1;
                info.pDependencies = &dependency;
                vkCreateRenderPass(_device, &info, nullptr, &_imguiContext->imguiPass);
            }


            VkDescriptorPoolSize pool_sizes[] =
            {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1000 * ARRAYSIZE(pool_sizes);
            pool_info.poolSizeCount = (uint32_t)ARRAYSIZE(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;
            vkCreateDescriptorPool(_device, &pool_info, nullptr, &_imguiContext->imguiPool);

            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = _instance;
            init_info.PhysicalDevice = _physicalDevice;
            init_info.Device = _device;
            init_info.Queue = _graphicsQueue;
            init_info.DescriptorPool = _imguiContext->imguiPool;
            init_info.MinImageCount = 3;
            init_info.ImageCount = 3;

            ImGui_ImplVulkan_Init(&init_info, _imguiContext->imguiPass);

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = _commandPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer command_buffer;
            vkAllocateCommandBuffers(_device, &allocInfo, &command_buffer);

            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(command_buffer, &begin_info);

            ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

            VkSubmitInfo end_info = {};
            end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            end_info.commandBufferCount = 1;
            end_info.pCommandBuffers = &command_buffer;
            vkEndCommandBuffer(command_buffer);
            
            vkQueueSubmit(_graphicsQueue, 1, &end_info, VK_NULL_HANDLE);
            vkQueueWaitIdle(_graphicsQueue);

            ImGui_ImplVulkan_DestroyFontUploadObjects();
            vkFreeCommandBuffers(_device, _commandPool, 1, &command_buffer);
        }

        void RenderDeviceVK::InitVulkan()
        {
#if _DEBUG && ENABLE_VALIDATION_IN_DEBUG
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
            appInfo.apiVersion = VK_API_VERSION_1_1;

            VkInstanceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            // Check extensions
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
            std::vector<VkExtensionProperties> extensions(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

            /*DebugHandler::Print("[Renderer]: Supported extensions:");
            for (VkExtensionProperties& extension : extensions)
            {
                DebugHandler::Print("[Renderer]: %s", extension.extensionName);
            }*/

            auto requiredExtensions = GetRequiredExtensions();

            createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
            createInfo.ppEnabledExtensionNames = requiredExtensions.data();

#if _DEBUG && ENABLE_VALIDATION_IN_DEBUG
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            PopulateDebugMessengerCreateInfo(debugCreateInfo);


            createInfo.pNext = &debugCreateInfo;
            
#else
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
#endif
            if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS)
            {
                DebugHandler::PrintFatal("Failed to create Vulkan instance!");
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
#if _DEBUG && ENABLE_VALIDATION_IN_DEBUG
            VkDebugUtilsMessengerCreateInfoEXT createInfo;
            PopulateDebugMessengerCreateInfo(createInfo);

            if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS)
            {
                DebugHandler::PrintFatal("Failed to set up Vulkan debug messenger!");
            }
#endif
        }

        void RenderDeviceVK::PickPhysicalDevice()
        {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

            if (deviceCount == 0)
            {
                DebugHandler::PrintFatal("Failed to find GPUs with Vulkan support!");
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

                // Get the name of the GPU
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(_physicalDevice, &deviceProperties);

                _gpuName = deviceProperties.deviceName;
            }
            else
            {
                DebugHandler::PrintFatal("Failed to find a suitable Vulkan GPU!");
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
            
            VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR shaderSubgroupFeatures = {};
            shaderSubgroupFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES_KHR;
            shaderSubgroupFeatures.shaderSubgroupExtendedTypes = true;

            VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures = {};
            descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
            descriptorIndexingFeatures.runtimeDescriptorArray = true;
            descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = true;
            descriptorIndexingFeatures.pNext = &shaderSubgroupFeatures;

            VkPhysicalDeviceFeatures2 deviceFeatures = {};
            deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            deviceFeatures.features.samplerAnisotropy = VK_TRUE;
            deviceFeatures.features.fragmentStoresAndAtomics = VK_TRUE;
            deviceFeatures.features.vertexPipelineStoresAndAtomics = VK_TRUE;
            deviceFeatures.features.shaderInt64 = VK_TRUE;
            deviceFeatures.features.multiDrawIndirect = VK_TRUE;
            deviceFeatures.features.drawIndirectFirstInstance = VK_TRUE;
            deviceFeatures.features.independentBlend = VK_TRUE;
            deviceFeatures.pNext = &descriptorIndexingFeatures;

            CheckDeviceFeatureSupport(_physicalDevice, deviceFeatures);

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

#if _DEBUG && ENABLE_VALIDATION_IN_DEBUG
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
                DebugHandler::PrintFatal("Failed to create logical device!");
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
                DebugHandler::PrintFatal("Failed to create command pool!");
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
                DebugHandler::PrintFatal("Failed to create window surface!");
            }
        }

        void RenderDeviceVK::CreateSwapChain(const ivec2& windowSize, SwapChainVK* swapChain, VkFormat& format)
        {
            SwapChainSupportDetails swapChainSupport = swapChain->QuerySwapChainSupport(_physicalDevice);

            if (SwapChainVK::FRAME_BUFFER_COUNT < swapChainSupport.capabilities.minImageCount || SwapChainVK::FRAME_BUFFER_COUNT > swapChainSupport.capabilities.maxImageCount)
            {
                DebugHandler::PrintFatal("Physical device does not support the requested number of swapchain images, you requested %u, and it supports between %u and %u", SwapChainVK::FRAME_BUFFER_COUNT, swapChainSupport.capabilities.minImageCount, swapChainSupport.capabilities.maxImageCount);
            }

            VkSurfaceFormatKHR surfaceFormat = swapChain->ChooseSwapSurfaceFormat(swapChainSupport.formats);
            VkPresentModeKHR presentMode = swapChain->ChooseSwapPresentMode(swapChainSupport.presentModes);
            VkExtent2D extent = swapChain->ChooseSwapExtent(windowSize, swapChainSupport.capabilities);

            format = surfaceFormat.format;

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
                DebugHandler::PrintFatal("Selected physical device does not support presenting!");
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
                DebugHandler::PrintFatal("Failed to create swap chain!");
            }
        }

        void RenderDeviceVK::CreateSemaphores(SwapChainVK* swapChain, SemaphoreHandlerVK* semaphoreHandler)
        {
            for (u32 i = 0; i < SwapChainVK::FRAME_BUFFER_COUNT; i++)
            {
                swapChain->imageAvailableSemaphores.Get(i) = semaphoreHandler->CreateGPUSemaphore();
                swapChain->blitFinishedSemaphores.Get(i) = semaphoreHandler->CreateGPUSemaphore();
            }
        }

        void RenderDeviceVK::CreateImages(SwapChainVK* swapChain, ImageHandlerVK* imageHandler, VkFormat format)
        {
            u32 imageCount = SwapChainVK::FRAME_BUFFER_COUNT;
            vkGetSwapchainImagesKHR(_device, swapChain->swapChain, &imageCount, nullptr);

            for (u32 i = 0; i < imageCount; i++)
            {
                ImageDesc imageDesc;
                imageDesc.format = ImageFormat::B8G8R8A8_UNORM;
                imageDesc.debugName = "SwapChain" + std::to_string(i);
                imageDesc.dimensions = vec2(1, 1);
                imageDesc.dimensionType = ImageDimensionType::DIMENSION_SCALE;
                imageDesc.mipLevels = 1;
                imageDesc.sampleCount = SampleCount::SAMPLE_COUNT_1;

                swapChain->imageIDs.Get(i) = imageHandler->CreateImageFromSwapchain(imageDesc, format, swapChain->swapChain, imageCount, i);
            }
        }

        void RenderDeviceVK::CleanupSwapChain(SwapChainVK* swapChain)
        {
            // Destroy swap chain
            vkDestroySwapchainKHR(_device, swapChain->swapChain, nullptr);

            // Destroy glfw surface
            vkDestroySurfaceKHR(_instance, swapChain->surface, nullptr);
        }

        void RenderDeviceVK::RecreateSwapChain(ImageHandlerVK* imageHandler, SemaphoreHandlerVK* semaphoreHandler, SwapChainVK* swapChain)
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

            VkFormat format;
            CreateSwapChain(size, swapChain, format);

            CreateImages(swapChain, imageHandler, format);
            CreateSemaphores(swapChain, semaphoreHandler);
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
                DebugHandler::Print("[Renderer]: GPU Detected %s with score %i because it doesn't support sampler anisotropy", deviceProperties.deviceName, 0);
                return 0;
            }

            // Application can't function without geometry shaders
            if (!deviceFeatures.geometryShader)
            {
                DebugHandler::Print("[Renderer]: GPU Detected %s with score %i because it doesn't support geometry shaders", deviceProperties.deviceName, 0);
                return 0;
            }

            // Application requires VK_QUEUE_GRAPHICS_BIT
            QueueFamilyIndices indices = FindQueueFamilies(device);
            if (!indices.IsComplete())
            {
                DebugHandler::Print("[Renderer]: GPU Detected %s with score %i because it doesn't support VK_QUEUE_GRAPHICS_BIT", deviceProperties.deviceName, 0);
                return 0;
            }

            // Make sure it supports the extensions we need
            bool extensionsSupported = CheckDeviceExtensionSupport(device);
            if (!extensionsSupported)
            {
                DebugHandler::Print("[Renderer]: GPU Detected %s with score %i because it doesn't support the required extensions", deviceProperties.deviceName, 0);
                return 0;
            }

            DebugHandler::Print("[Renderer]: GPU Detected %s with score %i", deviceProperties.deviceName, score);
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

        bool RenderDeviceVK::CheckDeviceFeatureSupport(VkPhysicalDevice device, VkPhysicalDeviceFeatures2& requested)
        {
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT& requestedDescriptorIndexingFeatures = *static_cast<VkPhysicalDeviceDescriptorIndexingFeaturesEXT*>(requested.pNext);
            VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR& requestedShaderSubgroupFeatures = *static_cast<VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR*>(requestedDescriptorIndexingFeatures.pNext);

            VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR supportedShaderSubgroupFeatures = {};
            supportedShaderSubgroupFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES_KHR;

            VkPhysicalDeviceDescriptorIndexingFeaturesEXT supportedDescriptorIndexingFeatures = {};
            supportedDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
            supportedDescriptorIndexingFeatures.pNext = &supportedShaderSubgroupFeatures;

            VkPhysicalDeviceFeatures2 supported = {};
            supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            supported.pNext = &supportedDescriptorIndexingFeatures;
            vkGetPhysicalDeviceFeatures2(device, &supported);

            std::vector<std::string> errorMessages;
            bool didError = false;

            if (requested.features.robustBufferAccess && !supported.features.robustBufferAccess)
            {
                errorMessages.push_back("We requested device feature robustBufferAccess which was not supported!");
                didError = true;
            }
            if (requested.features.fullDrawIndexUint32 && !supported.features.fullDrawIndexUint32)
            {
                errorMessages.push_back("We requested device feature fullDrawIndexUint32 which was not supported!");
                didError = true;
            }
            if (requested.features.imageCubeArray && !supported.features.imageCubeArray)
            {
                errorMessages.push_back("We requested device feature imageCubeArray which was not supported!");
                didError = true;
            }
            if (requested.features.independentBlend && !supported.features.independentBlend)
            {
                errorMessages.push_back("We requested device feature independentBlend which was not supported!");
                didError = true;
            }
            if (requested.features.geometryShader && !supported.features.geometryShader)
            {
                errorMessages.push_back("We requested device feature geometryShader which was not supported!");
                didError = true;
            }
            if (requested.features.tessellationShader && !supported.features.tessellationShader)
            {
                errorMessages.push_back("We requested device feature tessellationShader which was not supported!");
                didError = true;
            }
            if (requested.features.sampleRateShading && !supported.features.sampleRateShading)
            {
                errorMessages.push_back("We requested device feature sampleRateShading which was not supported!");
                didError = true;
            }
            if (requested.features.dualSrcBlend && !supported.features.dualSrcBlend)
            {
                errorMessages.push_back("We requested device feature dualSrcBlend which was not supported!");
                didError = true;
            }
            if (requested.features.logicOp && !supported.features.logicOp)
            {
                errorMessages.push_back("We requested device feature logicOp which was not supported!");
                didError = true;
            }
            if (requested.features.multiDrawIndirect && !supported.features.multiDrawIndirect)
            {
                errorMessages.push_back("We requested device feature multiDrawIndirect which was not supported!");
                didError = true;
            }
            if (requested.features.drawIndirectFirstInstance && !supported.features.drawIndirectFirstInstance)
            {
                errorMessages.push_back("We requested device feature drawIndirectFirstInstance which was not supported!");
                didError = true;
            }
            if (requested.features.depthClamp && !supported.features.depthClamp)
            {
                errorMessages.push_back("We requested device feature depthClamp which was not supported!");
                didError = true;
            }
            if (requested.features.depthBiasClamp && !supported.features.depthBiasClamp)
            {
                errorMessages.push_back("We requested device feature depthBiasClamp which was not supported!");
                didError = true;
            }
            if (requested.features.fillModeNonSolid && !supported.features.fillModeNonSolid)
            {
                errorMessages.push_back("We requested device feature fillModeNonSolid which was not supported!");
                didError = true;
            }
            if (requested.features.depthBounds && !supported.features.depthBounds)
            {
                errorMessages.push_back("We requested device feature depthBounds which was not supported!");
                didError = true;
            }
            if (requested.features.wideLines && !supported.features.wideLines)
            {
                errorMessages.push_back("We requested device feature wideLines which was not supported!");
                didError = true;
            }
            if (requested.features.largePoints && !supported.features.largePoints)
            {
                errorMessages.push_back("We requested device feature largePoints which was not supported!");
                didError = true;
            }
            if (requested.features.alphaToOne && !supported.features.alphaToOne)
            {
                errorMessages.push_back("We requested device feature alphaToOne which was not supported!");
                didError = true;
            }
            if (requested.features.multiViewport && !supported.features.multiViewport)
            {
                errorMessages.push_back("We requested device feature multiViewport which was not supported!");
                didError = true;
            }
            if (requested.features.samplerAnisotropy && !supported.features.samplerAnisotropy)
            {
                errorMessages.push_back("We requested device feature samplerAnisotropy which was not supported!");
                didError = true;
            }
            if (requested.features.textureCompressionETC2 && !supported.features.textureCompressionETC2)
            {
                errorMessages.push_back("We requested device feature textureCompressionETC2 which was not supported!");
                didError = true;
            }
            if (requested.features.textureCompressionASTC_LDR && !supported.features.textureCompressionASTC_LDR)
            {
                errorMessages.push_back("We requested device feature textureCompressionASTC_LDR which was not supported!");
                didError = true;
            }
            if (requested.features.textureCompressionBC && !supported.features.textureCompressionBC)
            {
                errorMessages.push_back("We requested device feature textureCompressionBC which was not supported!");
                didError = true;
            }
            if (requested.features.occlusionQueryPrecise && !supported.features.occlusionQueryPrecise)
            {
                errorMessages.push_back("We requested device feature occlusionQueryPrecise which was not supported!");
                didError = true;
            }
            if (requested.features.pipelineStatisticsQuery && !supported.features.pipelineStatisticsQuery)
            {
                errorMessages.push_back("We requested device feature pipelineStatisticsQuery which was not supported!");
                didError = true;
            }
            if (requested.features.vertexPipelineStoresAndAtomics && !supported.features.vertexPipelineStoresAndAtomics)
            {
                errorMessages.push_back("We requested device feature vertexPipelineStoresAndAtomics which was not supported!");
                didError = true;
            }
            if (requested.features.fragmentStoresAndAtomics && !supported.features.fragmentStoresAndAtomics)
            {
                errorMessages.push_back("We requested device feature fragmentStoresAndAtomics which was not supported!");
                didError = true;
            }
            if (requested.features.shaderTessellationAndGeometryPointSize && !supported.features.shaderTessellationAndGeometryPointSize)
            {
                errorMessages.push_back("We requested device feature shaderTessellationAndGeometryPointSize which was not supported!");
                didError = true;
            }
            if (requested.features.shaderImageGatherExtended && !supported.features.shaderImageGatherExtended)
            {
                errorMessages.push_back("We requested device feature shaderImageGatherExtended which was not supported!");
                didError = true;
            }
            if (requested.features.shaderStorageImageExtendedFormats && !supported.features.shaderStorageImageExtendedFormats)
            {
                errorMessages.push_back("We requested device feature shaderStorageImageExtendedFormats which was not supported!");
                didError = true;
            }
            if (requested.features.shaderStorageImageMultisample && !supported.features.shaderStorageImageMultisample)
            {
                errorMessages.push_back("We requested device feature shaderStorageImageMultisample which was not supported!");
                didError = true;
            }
            if (requested.features.shaderStorageImageReadWithoutFormat && !supported.features.shaderStorageImageReadWithoutFormat)
            {
                errorMessages.push_back("We requested device feature shaderStorageImageReadWithoutFormat which was not supported!");
                didError = true;
            }
            if (requested.features.shaderStorageImageWriteWithoutFormat && !supported.features.shaderStorageImageWriteWithoutFormat)
            {
                errorMessages.push_back("We requested device feature shaderStorageImageWriteWithoutFormat which was not supported!");
                didError = true;
            }
            if (requested.features.shaderUniformBufferArrayDynamicIndexing && !supported.features.shaderUniformBufferArrayDynamicIndexing)
            {
                errorMessages.push_back("We requested device feature shaderUniformBufferArrayDynamicIndexing which was not supported!");
                didError = true;
            }
            if (requested.features.shaderSampledImageArrayDynamicIndexing && !supported.features.shaderSampledImageArrayDynamicIndexing)
            {
                errorMessages.push_back("We requested device feature shaderSampledImageArrayDynamicIndexing which was not supported!");
                didError = true;
            }
            if (requested.features.shaderStorageBufferArrayDynamicIndexing && !supported.features.shaderStorageBufferArrayDynamicIndexing)
            {
                errorMessages.push_back("We requested device feature shaderStorageBufferArrayDynamicIndexing which was not supported!");
                didError = true;
            }
            if (requested.features.shaderStorageImageArrayDynamicIndexing && !supported.features.shaderStorageImageArrayDynamicIndexing)
            {
                errorMessages.push_back("We requested device feature shaderStorageImageArrayDynamicIndexing which was not supported!");
                didError = true;
            }
            if (requested.features.shaderClipDistance && !supported.features.shaderClipDistance)
            {
                errorMessages.push_back("We requested device feature shaderClipDistance which was not supported!");
                didError = true;
            }
            if (requested.features.shaderCullDistance && !supported.features.shaderCullDistance)
            {
                errorMessages.push_back("We requested device feature shaderCullDistance which was not supported!");
                didError = true;
            }
            if (requested.features.shaderFloat64 && !supported.features.shaderFloat64)
            {
                errorMessages.push_back("We requested device feature shaderFloat64 which was not supported!");
                didError = true;
            }
            if (requested.features.shaderInt64 && !supported.features.shaderInt64)
            {
                errorMessages.push_back("We requested device feature shaderInt64 which was not supported!");
                didError = true;
            }
            if (requested.features.shaderInt16 && !supported.features.shaderInt16)
            {
                errorMessages.push_back("We requested device feature shaderInt16 which was not supported!");
                didError = true;
            }
            if (requested.features.shaderResourceResidency && !supported.features.shaderResourceResidency)
            {
                errorMessages.push_back("We requested device feature shaderResourceResidency which was not supported!");
                didError = true;
            }
            if (requested.features.shaderResourceMinLod && !supported.features.shaderResourceMinLod)
            {
                errorMessages.push_back("We requested device feature shaderResourceMinLod which was not supported!");
                didError = true;
            }
            if (requested.features.sparseBinding && !supported.features.sparseBinding)
            {
                errorMessages.push_back("We requested device feature sparseBinding which was not supported!");
                didError = true;
            }
            if (requested.features.sparseResidencyBuffer && !supported.features.sparseResidencyBuffer)
            {
                errorMessages.push_back("We requested device feature sparseResidencyBuffer which was not supported!");
                didError = true;
            }
            if (requested.features.sparseResidencyImage2D && !supported.features.sparseResidencyImage2D)
            {
                errorMessages.push_back("We requested device feature sparseResidencyImage2D which was not supported!");
                didError = true;
            }
            if (requested.features.sparseResidencyImage3D && !supported.features.sparseResidencyImage3D)
            {
                errorMessages.push_back("We requested device feature sparseResidencyImage3D which was not supported!");
                didError = true;
            }
            if (requested.features.sparseResidency2Samples && !supported.features.sparseResidency2Samples)
            {
                errorMessages.push_back("We requested device feature sparseResidency2Samples which was not supported!");
                didError = true;
            }
            if (requested.features.sparseResidency4Samples && !supported.features.sparseResidency4Samples)
            {
                errorMessages.push_back("We requested device feature sparseResidency4Samples which was not supported!");
                didError = true;
            }
            if (requested.features.sparseResidency8Samples && !supported.features.sparseResidency8Samples)
            {
                errorMessages.push_back("We requested device feature sparseResidency8Samples which was not supported!");
                didError = true;
            }
            if (requested.features.sparseResidency16Samples && !supported.features.sparseResidency16Samples)
            {
                errorMessages.push_back("We requested device feature sparseResidency16Samples which was not supported!");
                didError = true;
            }
            if (requested.features.sparseResidencyAliased && !supported.features.sparseResidencyAliased)
            {
                errorMessages.push_back("We requested device feature sparseResidencyAliased which was not supported!");
                didError = true;
            }
            if (requested.features.variableMultisampleRate && !supported.features.variableMultisampleRate)
            {
                errorMessages.push_back("We requested device feature variableMultisampleRate which was not supported!");
                didError = true;
            }
            if (requested.features.inheritedQueries && !supported.features.inheritedQueries)
            {
                errorMessages.push_back("We requested device feature inheritedQueries which was not supported!");
                didError = true;
            }

            // VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR
            if (requestedDescriptorIndexingFeatures.shaderInputAttachmentArrayDynamicIndexing && !supportedDescriptorIndexingFeatures.shaderInputAttachmentArrayDynamicIndexing)
            {
                errorMessages.push_back("We requested device feature shaderInputAttachmentArrayDynamicIndexing which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.shaderUniformTexelBufferArrayDynamicIndexing && !supportedDescriptorIndexingFeatures.shaderUniformTexelBufferArrayDynamicIndexing)
            {
                errorMessages.push_back("We requested device feature shaderUniformTexelBufferArrayDynamicIndexing which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.shaderStorageTexelBufferArrayDynamicIndexing && !supportedDescriptorIndexingFeatures.shaderStorageTexelBufferArrayDynamicIndexing)
            {
                errorMessages.push_back("We requested device feature shaderStorageTexelBufferArrayDynamicIndexing which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing && !supportedDescriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing)
            {
                errorMessages.push_back("We requested device feature shaderUniformBufferArrayNonUniformIndexing which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing && !supportedDescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing)
            {
                errorMessages.push_back("We requested device feature shaderSampledImageArrayNonUniformIndexing which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing && !supportedDescriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing)
            {
                errorMessages.push_back("We requested device feature shaderStorageBufferArrayNonUniformIndexing which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing && !supportedDescriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing)
            {
                errorMessages.push_back("We requested device feature shaderStorageImageArrayNonUniformIndexing which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.shaderInputAttachmentArrayNonUniformIndexing && !supportedDescriptorIndexingFeatures.shaderInputAttachmentArrayNonUniformIndexing)
            {
                errorMessages.push_back("We requested device feature shaderInputAttachmentArrayNonUniformIndexing which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.shaderUniformTexelBufferArrayNonUniformIndexing && !supportedDescriptorIndexingFeatures.shaderUniformTexelBufferArrayNonUniformIndexing)
            {
                errorMessages.push_back("We requested device feature shaderUniformTexelBufferArrayNonUniformIndexing which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.shaderStorageTexelBufferArrayNonUniformIndexing && !supportedDescriptorIndexingFeatures.shaderStorageTexelBufferArrayNonUniformIndexing)
            {
                errorMessages.push_back("We requested device feature shaderStorageTexelBufferArrayNonUniformIndexing which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind && !supportedDescriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind)
            {
                errorMessages.push_back("We requested device feature descriptorBindingUniformBufferUpdateAfterBind which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind && !supportedDescriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind)
            {
                errorMessages.push_back("We requested device feature descriptorBindingSampledImageUpdateAfterBind which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind && !supportedDescriptorIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind)
            {
                errorMessages.push_back("We requested device feature descriptorBindingStorageImageUpdateAfterBind which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind && !supportedDescriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind)
            {
                errorMessages.push_back("We requested device feature descriptorBindingStorageBufferUpdateAfterBind which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind && !supportedDescriptorIndexingFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind)
            {
                errorMessages.push_back("We requested device feature descriptorBindingUniformTexelBufferUpdateAfterBind which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind && !supportedDescriptorIndexingFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind)
            {
                errorMessages.push_back("We requested device feature descriptorBindingStorageTexelBufferUpdateAfterBind which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending && !supportedDescriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending)
            {
                errorMessages.push_back("We requested device feature descriptorBindingUpdateUnusedWhilePending which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.descriptorBindingPartiallyBound && !supportedDescriptorIndexingFeatures.descriptorBindingPartiallyBound)
            {
                errorMessages.push_back("We requested device feature descriptorBindingPartiallyBound which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount && !supportedDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount)
            {
                errorMessages.push_back("We requested device feature descriptorBindingVariableDescriptorCount which was not supported!");
                didError = true;
            }
            if (requestedDescriptorIndexingFeatures.runtimeDescriptorArray && !supportedDescriptorIndexingFeatures.runtimeDescriptorArray)
            {
                errorMessages.push_back("We requested device feature runtimeDescriptorArray which was not supported!");
                didError = true;
            }

            // VkPhysicalDeviceShaderSubgroupExtendedTypesFeaturesKHR
            if (requestedShaderSubgroupFeatures.shaderSubgroupExtendedTypes && !supportedShaderSubgroupFeatures.shaderSubgroupExtendedTypes)
            {
                errorMessages.push_back("We requested device feature shaderSubgroupExtendedTypes which was not supported!");
                didError = true;
            }

            if (didError)
            {
                for (const std::string& errorMessage : errorMessages)
                {
                    DebugHandler::PrintError(errorMessage);
                }

                DebugHandler::PrintFatal("The graphics card did not support all requested features!");
                return false;
            }

            return true;
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
                    DebugHandler::PrintFatal("We do not support a validation layer that we need to support: %s", layerName);
                }
            }
        }

        std::vector<const char*> RenderDeviceVK::GetRequiredExtensions()
        {
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if _DEBUG && ENABLE_VALIDATION_IN_DEBUG
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
            extensions.push_back("VK_KHR_get_physical_device_properties2");
            extensions.push_back("VK_EXT_debug_report");

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
                    DebugHandler::PrintFatal("Tried to use a format that wasn't uncompressed or used BC compression, what is this? id: %u", format);
                }

                curWidth = Math::Max(1, curWidth / 2);
                curHeight = Math::Max(1, curHeight / 2);
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

            VkPipelineStageFlagBits srcFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            VkPipelineStageFlagBits dstFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

            // TODO: Fix this
            if (oldLayout == VkImageLayout::VK_IMAGE_LAYOUT_GENERAL && newLayout == VkImageLayout::VK_IMAGE_LAYOUT_GENERAL)
            {
                imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            }

            switch (oldLayout) 
            {
                case VK_IMAGE_LAYOUT_PREINITIALIZED:
                    imageBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    srcFlags = VkPipelineStageFlagBits(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
                    break;
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    srcFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    break;
                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                    imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    srcFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    break;

                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                    imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
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
                    imageBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    dstFlags = VkPipelineStageFlagBits(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    imageBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                    imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    srcFlags = VkPipelineStageFlagBits(VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT);
                    dstFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    break;

                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                    imageBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                    break;
            }
            
            vkCmdPipelineBarrier(commandBuffer, srcFlags, dstFlags, 0, 0, NULL, 0, NULL, 1, &imageBarrier);
        }
    }
}