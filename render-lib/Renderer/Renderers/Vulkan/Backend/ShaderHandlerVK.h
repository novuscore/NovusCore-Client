#pragma once
#include <NovusTypes.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include <cassert>
#include "../../../Descriptors/VertexShaderDesc.h"
#include "../../../Descriptors/PixelShaderDesc.h"
#include "../../../Descriptors/ComputeShaderDesc.h"
#include "SpirvReflect.h"
#include <Utils/DebugHandler.h>

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;

        typedef std::vector<char> ShaderBinary;

        struct BindInfo
        {
            std::string name;
            u32 nameHash;

            VkDescriptorType descriptorType;
            u32 set;
            u32 binding;
            u32 count;
            VkShaderStageFlagBits stageFlags;
        };

        struct BindInfoPushConstants
        {
            int size;
        };

        struct BindReflection
        {
            std::vector<BindInfo> dataBindings;
            std::vector<BindInfoPushConstants> pushConstants;
        };

        class ShaderHandlerVK
        {
            using vsIDType = type_safe::underlying_type<VertexShaderID>;
            using psIDType = type_safe::underlying_type<PixelShaderID>;
            using csIDType = type_safe::underlying_type<ComputeShaderID>;

        public:
            void Init(RenderDeviceVK* device);

            VertexShaderID LoadShader(const VertexShaderDesc& desc);
            PixelShaderID LoadShader(const PixelShaderDesc& desc);
            ComputeShaderID LoadShader(const ComputeShaderDesc& desc);

            VkShaderModule GetShaderModule(const VertexShaderID id) { return _vertexShaders[static_cast<vsIDType>(id)].module; }
            VkShaderModule GetShaderModule(const PixelShaderID id) { return _pixelShaders[static_cast<psIDType>(id)].module; }
            VkShaderModule GetShaderModule(const ComputeShaderID id) { return _computeShaders[static_cast<csIDType>(id)].module; }

            const BindReflection& GetBindReflection(const VertexShaderID id)
            {
                return  _vertexShaders[static_cast<vsIDType>(id)].bindReflection;
            }
            const BindReflection& GetBindReflection(const PixelShaderID id)
            { 
                return _pixelShaders[static_cast<psIDType>(id)].bindReflection;
            }

        private:
            struct Shader
            {
                std::string path;
                VkShaderModule module;
                ShaderBinary spirv;

                BindReflection bindReflection;
            };

        private:
            template <typename T>
            T LoadShader(const std::string& shaderPath, std::vector<Shader>& shaders)
            {
                size_t id;
                using idType = type_safe::underlying_type<T>;

                // If shader is already loaded, return ID of already loaded version
                if (TryFindExistingShader(shaderPath, shaders, id))
                {
                    return T(static_cast<idType>(id));
                }

                id = shaders.size();
                assert(id < T::MaxValue());

                shaders.emplace_back();
                Shader& shader = shaders.back();
                ReadFile(shaderPath, shader.spirv);
                shader.path = shaderPath;
                shader.module = CreateShaderModule(shader.spirv);

                // Reflect descriptor sets
                SpvReflectShaderModule reflectModule;
                SpvReflectResult result = spvReflectCreateShaderModule(shader.spirv.size(), shader.spirv.data(), &reflectModule);

                if (result != SPV_REFLECT_RESULT_SUCCESS)
                {
                    NC_LOG_FATAL("We failed to reflect the spirv of %s", shaderPath.c_str());
                }

                uint32_t count = 0;
                result = spvReflectEnumerateDescriptorSets(&reflectModule, &count, NULL);

                if (result != SPV_REFLECT_RESULT_SUCCESS)
                {
                    NC_LOG_FATAL("We failed to reflect the spirv descriptor set count of %s", shaderPath.c_str());
                }

                
                if (count > 0)
                {
                    std::vector<SpvReflectDescriptorSet*> descriptorSets(count);
                    
                    result = spvReflectEnumerateDescriptorSets(&reflectModule, &count, descriptorSets.data());

                    if (result != SPV_REFLECT_RESULT_SUCCESS)
                    {
                        NC_LOG_FATAL("We failed to reflect the spirv descriptor sets of %s", shaderPath.c_str());
                    }

                    for (auto* descriptorSet : descriptorSets)
                    {
                        for (uint32_t binding = 0; binding < descriptorSet->binding_count; binding++)
                        {
                            const SpvReflectDescriptorBinding* reflectionBinding = descriptorSet->bindings[binding];
                            BindInfo bindInfo;
                            bindInfo.descriptorType = static_cast<VkDescriptorType>(reflectionBinding->descriptor_type);
                            bindInfo.set = descriptorSet->set;
                            bindInfo.binding = reflectionBinding->binding;
                            bindInfo.count = reflectionBinding->count;
                            bindInfo.stageFlags = static_cast<VkShaderStageFlagBits>(reflectModule.shader_stage);

                            bindInfo.name = reflectionBinding->name;
                            bindInfo.nameHash = StringUtils::fnv1a_32(bindInfo.name.c_str(), bindInfo.name.length());

                            shader.bindReflection.dataBindings.push_back(bindInfo);
                        }
                    }
                }
                
                return T(static_cast<idType>(id));
            }
            
            void ReadFile(const std::string& filename, ShaderBinary& binary);
            VkShaderModule CreateShaderModule(const ShaderBinary& binary);
            bool TryFindExistingShader(const std::string& shaderPath, std::vector<Shader>& shaders, size_t& id);

        private:
            RenderDeviceVK* _device;

            std::vector<Shader> _vertexShaders;
            std::vector<Shader> _pixelShaders;
            std::vector<Shader> _computeShaders;
        };
    }
}
