#pragma once
#include <NovusTypes.h>
#include <NovusTypeHeader.h>
#include <vector>
#include <vulkan/vulkan.h>

#include "../../../Descriptors/ModelDesc.h"

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;

        class ModelHandlerVK
        {
            // Update the second value of this when the format exported from the converter gets changed
            const NovusTypeHeader EXPECTED_TYPE_HEADER = NovusTypeHeader(42, 2);
        public:
            ModelHandlerVK();
            ~ModelHandlerVK();

            ModelID CreatePrimitiveModel(RenderDeviceVK* device, const PrimitiveModelDesc& desc);
            void UpdatePrimitiveModel(RenderDeviceVK* device, ModelID model, const PrimitiveModelDesc& desc);

            ModelID LoadModel(RenderDeviceVK* device, const ModelDesc& desc);

            VkBuffer GetVertexBuffer(ModelID modelID);

            u32 GetNumIndices(ModelID modelID);
            VkBuffer GetIndexBuffer(ModelID modelID);
            
        private:
            struct Model
            {
                ModelDesc desc;
                VkBuffer vertexBuffer;
                VkDeviceMemory vertexBufferMemory;
                VkBuffer indexBuffer;
                VkDeviceMemory indexBufferMemory;
                u32 numIndices;

                std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
                std::string debugName;
            };

            struct TempModelData
            {
                i32 indexType;
                std::vector<Vertex> vertices;
                std::vector<u32> indices;
            };

        private:
            void LoadFromFile(const ModelDesc& desc, TempModelData& data);
            void InitializeModel(RenderDeviceVK* device, Model& model, const TempModelData& data);
            void UpdateVertices(RenderDeviceVK* device, Model& model, const std::vector<Vertex>& vertices);
            void UpdateIndices(RenderDeviceVK* device, Model& model, const std::vector<u32>& indices);

        private:
            std::vector<Model> _models;
        };
    }
}