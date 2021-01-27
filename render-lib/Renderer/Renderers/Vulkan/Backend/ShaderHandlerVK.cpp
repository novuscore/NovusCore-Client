#include "ShaderHandlerVK.h"
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <ShaderCooker/ShaderCooker.h>
#include "RenderDeviceVK.h"
#include <fstream>
#include <filesystem>

namespace Renderer
{
    namespace Backend
    {
        void ShaderHandlerVK::Init(RenderDeviceVK* device)
        {
            _device = device;

            _shaderCooker = new ShaderCooker::ShaderCooker();

            std::filesystem::path includePath = SHADER_SOURCE_DIR;
            _shaderCooker->AddIncludeDir(includePath);
        }

        void ShaderHandlerVK::ReloadShaders(bool forceRecompileAll)
        {
            _forceRecompileAll = forceRecompileAll;
            
            _vertexShaders.clear();
            _pixelShaders.clear();
            _computeShaders.clear();
        }

        VertexShaderID ShaderHandlerVK::LoadShader(const VertexShaderDesc& desc)
        {
            return LoadShader<VertexShaderID>(desc.path, _vertexShaders);
        }

        PixelShaderID ShaderHandlerVK::LoadShader(const PixelShaderDesc& desc)
        {
            return LoadShader<PixelShaderID>(desc.path, _pixelShaders);
        }

        ComputeShaderID ShaderHandlerVK::LoadShader(const ComputeShaderDesc& desc)
        {
            return LoadShader<ComputeShaderID>(desc.path, _computeShaders);
        }

        void ShaderHandlerVK::ReadFile(const std::string& filename, ShaderBinary& binary)
        {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open())
            {
                NC_LOG_FATAL("Failed to open file!");
            }

            size_t fileSize = (size_t)file.tellg();
            binary.resize(fileSize);

            file.seekg(0);
            file.read(binary.data(), fileSize);

            file.close();
        }

        VkShaderModule ShaderHandlerVK::CreateShaderModule(const ShaderBinary& binary)
        {
            VkShaderModuleCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = binary.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(binary.data());

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(_device->_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            {
                NC_LOG_FATAL("Failed to create shader module!");
            }

            return shaderModule;
        }

        bool ShaderHandlerVK::TryFindExistingShader(const std::string& shaderPath, std::vector<Shader>& shaders, size_t& id)
        {
            u32 shaderPathHash = StringUtils::fnv1a_32(shaderPath.c_str(), shaderPath.length());

            id = 0;
            for (Shader& existingShader : shaders)
            {
                if (StringUtils::fnv1a_32(existingShader.path.c_str(), existingShader.path.length()) == shaderPathHash)
                {
                    return true;
                }
                id++;
            }

            return false;
        }
        
        std::filesystem::path GetShaderBinPath(const std::string& shaderPath)
        {
            std::string binShaderPath = shaderPath + ".spv";
            std::filesystem::path binPath = std::filesystem::path(SHADER_BIN_DIR) / binShaderPath;
            return std::filesystem::absolute(binPath.make_preferred());
        }

        std::string ShaderHandlerVK::GetShaderBinPathString(const std::string& shaderPath)
        {
            return GetShaderBinPath(shaderPath).string();
        }

        bool ShaderHandlerVK::NeedsCompile(const std::string& shaderPath)
        {
            std::filesystem::path sourcePath = std::filesystem::path(SHADER_SOURCE_DIR) / shaderPath;
            sourcePath = std::filesystem::absolute(sourcePath.make_preferred());

            if (!std::filesystem::exists(sourcePath))
            {
                NC_LOG_FATAL("Tried to load a shader (%s) which does not exist at expected location (%s)", shaderPath, sourcePath.string());
            }

            if (_forceRecompileAll)
            {
                return true; // If we should force recompile all shaders, we want to compile it
            }

            std::filesystem::path binPath = GetShaderBinPath(shaderPath);

            if (!std::filesystem::exists(binPath))
            {
                return true; // If the shader binary does not exist, we want to compile it
            }

            std::filesystem::file_time_type sourceModifiedTime = std::filesystem::last_write_time(sourcePath);
            std::filesystem::file_time_type binModifiedTime = std::filesystem::last_write_time(binPath);

            return sourceModifiedTime > binModifiedTime; // If sourceModifiedTime is newer we need to compile
        }

        bool ShaderHandlerVK::CompileShader(const std::string& shaderPath)
        {
            std::filesystem::path sourcePath = std::filesystem::path(SHADER_SOURCE_DIR) / shaderPath;
            sourcePath = std::filesystem::absolute(sourcePath.make_preferred());

            std::filesystem::path binPath = GetShaderBinPath(shaderPath);

            char* blob;
            size_t size;
            if (_shaderCooker->CompileFile(sourcePath, blob, size))
            {
                std::filesystem::create_directories(binPath.parent_path());

                std::ofstream ofstream(binPath, std::ios::trunc | std::ofstream::binary);
                ofstream.write(blob, size);
                ofstream.close();

                return true;
            }
            return false;
        }
    }
}