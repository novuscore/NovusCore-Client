#pragma once
#include <NovusTypes.h>
#include <NovusTypeHeader.h>
#include <vector>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

#include "../../../Descriptors/ModelDesc.h"
#include "../../../Descriptors/BufferDesc.h"

namespace Renderer
{
    namespace Backend
    {
        class RenderDeviceVK;
        class BufferHandlerVK;

        class ModelHandlerVK
        {
            // Update the second value of this when the format exported from the converter gets changed
            const NovusTypeHeader EXPECTED_TYPE_HEADER = NovusTypeHeader(42, 2);
        public:
            void Init(RenderDeviceVK* device, BufferHandlerVK* bufferHandler);

            ModelID CreatePrimitiveModel(const PrimitiveModelDesc& desc);
            void UpdatePrimitiveModel(ModelID model, const PrimitiveModelDesc& desc);

            ModelID LoadModel(const ModelDesc& desc);

            VkBuffer GetVertexBuffer(ModelID modelID);

            u32 GetNumIndices(ModelID modelID);
            VkBuffer GetIndexBuffer(ModelID modelID);
            
        private:
            struct Model
            {
                ModelDesc desc;

                BufferID vertexBuffer = BufferID::Invalid();
                BufferID indexBuffer = BufferID::Invalid();

                u32 numVertices = 0;
                u32 numIndices = 0;

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
            void InitializeModel(Model& model, const TempModelData& data);
            void UpdateVertices(Model& model, const std::vector<Vertex>& vertices);
            void UpdateIndices(Model& model, const std::vector<u32>& indices);

        private:
            RenderDeviceVK* _device;
            BufferHandlerVK* _bufferHandler;

            std::vector<Model> _models;
        };
    }
}