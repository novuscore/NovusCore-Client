#include "ShaderHandlerVK.h"

#include <Utils/StringUtils.h>
#include <Utils/DebugHandler.h>
#include <ShaderCooker/ShaderCache.h>
#include <ShaderCooker/ShaderCompiler.h>
#include <vulkan/vulkan.h>
#include <filesystem>
#include <fstream>

#include "RenderDeviceVK.h"

namespace Renderer
{
    namespace Backend
    {
        const std::filesystem::path SHADER_CACHE_PATH = "Data/shaders/_shaders.cache";

        void ShaderHandlerVK::Init(RenderDeviceVK* device)
        {
            _device = device;

            _shaderCache = new ShaderCooker::ShaderCache();
            _shaderCompiler = new ShaderCooker::ShaderCompiler();

            const bool debugSkipCache = false;
            if (!debugSkipCache)
            {
                if (_shaderCache->Load(SHADER_CACHE_PATH))
                {
                    DebugHandler::PrintSuccess("Loaded shadercache from: %s", SHADER_CACHE_PATH.string().c_str());
                }
                else
                {
                    DebugHandler::Print("Creating shadercache at: %s", SHADER_CACHE_PATH.string().c_str());
                }
            }
            else
            {
                DebugHandler::Print("Skipped loading shadercache due to debugSkipCache being true");
            }

            _shaderCompiler->SetSourceDirPath(SHADER_SOURCE_DIR);
            _shaderCompiler->SetBinDirPath("Data/shaders");
            _shaderCompiler->SetShaderCache(_shaderCache);
            _shaderCompiler->SetShouldForceCompile(true); // ShaderHandler will only request compilation of files we want to compile, this might include force compiling something that is up to date.
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
            return LoadShader<VertexShaderID>(desc.path, desc.permutationFields, _vertexShaders);
        }

        PixelShaderID ShaderHandlerVK::LoadShader(const PixelShaderDesc& desc)
        {
            return LoadShader<PixelShaderID>(desc.path, desc.permutationFields, _pixelShaders);
        }

        ComputeShaderID ShaderHandlerVK::LoadShader(const ComputeShaderDesc& desc)
        {
            return LoadShader<ComputeShaderID>(desc.path, desc.permutationFields, _computeShaders);
        }

        void ShaderHandlerVK::ReadFile(const std::string& filename, ShaderBinary& binary)
        {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open())
            {
                DebugHandler::PrintFatal("Failed to open file!");
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
                DebugHandler::PrintFatal("Failed to create shader module!");
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
        
        std::string ShaderHandlerVK::GetPermutationPath(const std::string& shaderPathString, const std::vector<PermutationField>& permutationFields)
        {
            std::filesystem::path shaderPath = shaderPathString;
            std::string filename = shaderPath.filename().string();

            size_t firstExtensionOffset = filename.find_first_of('.');

            std::string permutationName = filename.substr(0, firstExtensionOffset);
            std::string extension = filename.substr(firstExtensionOffset);

            for (const PermutationField& permutationField : permutationFields)
            {
                permutationName += "-" + permutationField.key + permutationField.value;
            }

            shaderPath = shaderPath.parent_path() / (permutationName + extension);
            return shaderPath.string();
        }

        std::filesystem::path GetShaderBinPath(const std::string& shaderPath)
        {
            std::string binShaderPath = shaderPath + ".spv";
            std::filesystem::path binPath = std::filesystem::path("Data/shaders/") / binShaderPath;
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
                DebugHandler::PrintFatal("Tried to load a shader (%s) which does not exist at expected location (%s)", shaderPath, sourcePath.string());
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

            return _shaderCache->HasChanged(sourcePath);
        }

        bool ShaderHandlerVK::CompileShader(const std::string& shaderPath)
        {
            std::filesystem::path shaderAbsolutePath = std::filesystem::path(SHADER_SOURCE_DIR) / shaderPath;
            shaderAbsolutePath = std::filesystem::absolute(shaderAbsolutePath.make_preferred());

            _shaderCompiler->Start();
            _shaderCompiler->AddPath(shaderAbsolutePath);
            _shaderCompiler->Process();

            while (_shaderCompiler->GetStage() != ShaderCooker::ShaderCompiler::Stage::STOPPED)
            {
                std::this_thread::yield();
            }

            _shaderCache->Save(SHADER_CACHE_PATH);

            return _shaderCompiler->GetNumCompiledShaders() > 0;
        }
    }
}