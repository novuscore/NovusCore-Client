#pragma once
#include <NovusTypes.h>
#include <vulkan/vulkan_core.h>
#include <cassert>
#include <vector>

namespace Renderer
{
    namespace Backend
    {
        class DebugMarkerUtilVK
        {
        public:
            
            static bool GetDebugMarkersEnabled() 
            {
                return _debugMarkersEnabled; 
            }

            static void SetObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name)
            {
                if (_debugMarkersAvailable && _debugMarkersEnabled)
                {
                    VkDebugMarkerObjectNameInfoEXT nameInfo = {};
                    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
                    nameInfo.objectType = objectType;
                    nameInfo.object = object;
                    nameInfo.pObjectName = name;
                    fnDebugMarkerSetObjectName(device, &nameInfo);
                }
            }

            static void PushMarker(VkCommandBuffer commandBuffer, Color color, std::string name)
            {
                if (_debugMarkersAvailable && _debugMarkersEnabled)
                {
                    VkDebugMarkerMarkerInfoEXT markerInfo = {};
                    markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;

                    markerInfo.color[0] = color.r;
                    markerInfo.color[1] = color.g;
                    markerInfo.color[2] = color.b;
                    markerInfo.color[3] = color.a;

                    markerInfo.pMarkerName = name.c_str();
                    fnCmdDebugMarkerBegin(commandBuffer, &markerInfo);
                }
            }

            static void PopMarker(VkCommandBuffer commandBuffer)
            {
                if (_debugMarkersAvailable && _debugMarkersEnabled)
                {
                    fnCmdDebugMarkerEnd(commandBuffer);
                }
            }

        private:
            static void InitializeFunctions(VkDevice device)
            {
                assert(!_initialized);
                _initialized = true;

                if (_debugMarkersAvailable && _debugMarkersEnabled)
                {
                    fnDebugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
                    fnDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
                    fnCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
                    fnCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
                    fnCmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
                }
            }

            static void CheckExtension(const VkExtensionProperties& extension)
            {
                if (_debugMarkersEnabled)
                {
                    if (!strcmp(extension.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
                    {
                        _debugMarkersAvailable = true;
                    }
                }
            }

            static void AddValidationLayer(std::vector<const char*>& layers) 
            {
                if (_debugMarkersAvailable && _debugMarkersEnabled)
                    layers.push_back("VK_EXT_debug_maker"); 
            }

            static void AddEnabledExtension(std::vector<const char*>& extensions)
            {
                if (_debugMarkersAvailable && _debugMarkersEnabled)
                    extensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
            }

            static void SetDebugMarkersEnabled(bool available) { _debugMarkersEnabled = available; }

        private:
            static bool _initialized;
            static bool _debugMarkersEnabled;
            static bool _debugMarkersAvailable;

            static PFN_vkDebugMarkerSetObjectTagEXT fnDebugMarkerSetObjectTag;
            static PFN_vkDebugMarkerSetObjectNameEXT fnDebugMarkerSetObjectName;
            static PFN_vkCmdDebugMarkerBeginEXT fnCmdDebugMarkerBegin;
            static PFN_vkCmdDebugMarkerEndEXT fnCmdDebugMarkerEnd;
            static PFN_vkCmdDebugMarkerInsertEXT fnCmdDebugMarkerInsert;

            friend class RenderDeviceVK;
        };
    }
}