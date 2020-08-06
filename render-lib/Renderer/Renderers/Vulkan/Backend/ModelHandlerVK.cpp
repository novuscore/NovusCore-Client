#include "ModelHandlerVK.h"
#include <cassert>
#include <filesystem>
#include <Utils/DebugHandler.h>
#include <Utils/FileReader.h>
#include "RenderDeviceVK.h"
#include "DebugMarkerUtilVK.h"

namespace Renderer
{
    namespace Backend
    {
        void ModelHandlerVK::Init(RenderDeviceVK* device)
        {
            _device = device;
        }

        ModelID ModelHandlerVK::CreatePrimitiveModel(const PrimitiveModelDesc& desc)
        {
            size_t nextHandle = _models.size();

            // Make sure we haven't exceeded the limit of the DepthImageID type, if this hits you need to change type of DepthImageID to something bigger
            assert(nextHandle < ModelID::MaxValue());
            using type = type_safe::underlying_type<ModelID>;

            Model model;
            model.debugName = desc.debugName;

            TempModelData tempData;

            tempData.indexType = 3; // Triangle list, nothing else is really used these days
            tempData.vertices = desc.vertices;
            tempData.indices = desc.indices;

            InitializeModel(model, tempData);

            _models.push_back(model);
            return ModelID(static_cast<type>(nextHandle));
        }

        void ModelHandlerVK::UpdatePrimitiveModel(ModelID modelID, const PrimitiveModelDesc& desc)
        {
            using type = type_safe::underlying_type<ModelID>;
            Model& model = _models[static_cast<type>(modelID)];
            
            UpdateVertices(model, desc.vertices);
        }

        ModelID ModelHandlerVK::LoadModel(const ModelDesc& desc)
        {
            size_t nextHandle = _models.size();

            // Make sure we haven't exceeded the limit of the DepthImageID type, if this hits you need to change type of DepthImageID to something bigger
            assert(nextHandle < ModelID::MaxValue());
            using type = type_safe::underlying_type<ModelID>;

            Model model;
            model.debugName = desc.path;

            TempModelData tempData;

            LoadFromFile(desc, tempData);
            InitializeModel(model, tempData);
                
            _models.push_back(model);
            return ModelID(static_cast<type>(nextHandle));
        }

        VkBuffer ModelHandlerVK::GetVertexBuffer(ModelID modelID)
        {
            using type = type_safe::underlying_type<ModelID>;

            // Lets make sure this id exists
            assert(_models.size() > static_cast<type>(modelID));

            Model& model = _models[static_cast<type>(modelID)];
            if (model.numVertices == 0)
            {
                NC_LOG_FATAL("Tried to get the vertex buffer of model (%s) which doesn't have vertices", model.debugName);
            }

            return model.vertexBuffer;
        }

        u32 ModelHandlerVK::GetNumIndices(ModelID modelID)
        {
            using type = type_safe::underlying_type<ModelID>;

            // Lets make sure this id exists
            assert(_models.size() > static_cast<type>(modelID));

            Model& model = _models[static_cast<type>(modelID)];

            return model.numIndices;
        }

        VkBuffer ModelHandlerVK::GetIndexBuffer(ModelID modelID)
        {
            using type = type_safe::underlying_type<ModelID>;

            // Lets make sure this id exists
            assert(_models.size() > static_cast<type>(modelID));

            Model& model = _models[static_cast<type>(modelID)];
            if (model.numIndices == 0)
            {
                NC_LOG_FATAL("Tried to get the index buffer of model (%s) which doesn't have indices", model.debugName);
            }

            return model.indexBuffer;
        }

        void ModelHandlerVK::LoadFromFile(const ModelDesc& desc, TempModelData& data)
        {
            // Open header
            std::filesystem::path path = std::filesystem::absolute(desc.path);
            FileReader file(path.string(), path.filename().string());
            if (!file.Open())
            {
                NC_LOG_FATAL("Could not open Model file %s", desc.path.c_str());
            }

            assert(file.Length() > sizeof(NovusTypeHeader));

            std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<32768>();
            file.Read(*buffer, file.Length());

            // Read header
            NovusTypeHeader header;
            if (!buffer->Get<NovusTypeHeader>(header))
            {
                NC_LOG_FATAL("Model file %s did not have a valid NovusTypeHeader", desc.path.c_str());
            }

            if (header != EXPECTED_TYPE_HEADER)
            {
                if (header.typeID != EXPECTED_TYPE_HEADER.typeID)
                {
                    NC_LOG_FATAL("Model file %s had an invalid TypeID in its NovusTypeHeader, %u != %u", header.typeID, EXPECTED_TYPE_HEADER.typeID);
                }
                if (header.typeVersion != EXPECTED_TYPE_HEADER.typeVersion)
                {
                    NC_LOG_FATAL("Model file %s had an invalid TypeVersion in its NovusTypeHeader, %u != %u", header.typeVersion, EXPECTED_TYPE_HEADER.typeVersion);
                }
            }

            // Read vertex count
            u32 vertexCount;
            if (!buffer->GetU32(vertexCount))
            {
                NC_LOG_FATAL("Model file %s did not have a valid vertexCount", desc.path.c_str());
            }
            
            // Read vertices
            data.vertices.resize(vertexCount);
            
            for (u32 i = 0; i < vertexCount; i++)
            {
                if (!buffer->Get<vec3>(data.vertices[i].pos))
                {
                    NC_LOG_FATAL("Model file %s failed to read vertex %u position", desc.path.c_str(), i);
                }

                if (!buffer->Get<vec3>(data.vertices[i].normal))
                {
                    NC_LOG_FATAL("Model file %s failed to read vertex %u normal", desc.path.c_str(), i);
                }

                if (!buffer->Get<vec2>(data.vertices[i].texCoord))
                {
                    NC_LOG_FATAL("Model file %s failed to read vertex %u texCoord", desc.path.c_str(), i);
                }
            }

            // Read index type
            if (!buffer->GetI32(data.indexType))
            {
                NC_LOG_FATAL("Model file %s did not have a valid indexType", desc.path.c_str());
            }

            // Read index count
            u32 indexCount;
            if (!buffer->GetU32(indexCount))
            {
                NC_LOG_FATAL("Model file %s did not have a valid indexCount", desc.path.c_str());
            }

            // Read indices
            data.indices.resize(indexCount);

            for (u32 i = 0; i < indexCount; i++)
            {
                if (!buffer->GetU32(data.indices[i]))
                {
                    NC_LOG_FATAL("Model file %s failed to read index %u", desc.path.c_str(), i);
                }
            }
        }

        void ModelHandlerVK::InitializeModel(Model& model, const TempModelData& data)
        {
            model.numVertices = static_cast<u32>(data.vertices.size());
            model.numIndices = static_cast<u32>(data.indices.size());
            

            // -- Create vertex buffer --
            if (model.numVertices > 0)
            {
                VkDeviceSize vertexBufferSize = sizeof(data.vertices[0]) * data.vertices.size();
                _device->CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, model.vertexBuffer, model.vertexBufferAllocation);

                DebugMarkerUtilVK::SetObjectName(_device->_device, (u64)model.vertexBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, model.debugName.c_str());

                UpdateVertices(model, data.vertices);
            }
            
            // -- Create index buffer --
            if (model.numIndices > 0)
            {
                VkDeviceSize indexBufferSize = sizeof(data.indices[0]) * data.indices.size();
                _device->CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, model.indexBuffer, model.indexBufferAllocation);

                DebugMarkerUtilVK::SetObjectName(_device->_device, (u64)model.vertexBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, model.debugName.c_str());

                UpdateIndices(model, data.indices);
            }

            // -- Create attribute descriptor --
            model.attributeDescriptions.resize(3);

            // Position
            model.attributeDescriptions[0].binding = 0;
            model.attributeDescriptions[0].location = 0;
            model.attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            model.attributeDescriptions[0].offset = offsetof(Vertex, pos);

            // Normal
            model.attributeDescriptions[1].binding = 0;
            model.attributeDescriptions[1].location = 1;
            model.attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            model.attributeDescriptions[1].offset = offsetof(Vertex, normal);

            // Texcoord
            model.attributeDescriptions[2].binding = 0;
            model.attributeDescriptions[2].location = 2;
            model.attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            model.attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
        }

        void ModelHandlerVK::UpdateVertices(Model& model, const std::vector<Vertex>& vertices)
        {
            VkBuffer stagingBuffer;
            VmaAllocation stagingBufferAllocation;

            // Create a staging buffer
            VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
            _device->CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingBufferAllocation);

            // Copy our vertex data into the staging buffer
            void* vertexData;
            vmaMapMemory(_device->_allocator, stagingBufferAllocation, &vertexData);
            memcpy(vertexData, vertices.data(), (size_t)vertexBufferSize);
            vmaUnmapMemory(_device->_allocator, stagingBufferAllocation);

            // Copy the vertex data from our staging buffer to our vertex buffer
            _device->CopyBuffer(model.vertexBuffer, 0, stagingBuffer, 0, vertexBufferSize);

            // Destroy and free our staging buffer
            vmaDestroyBuffer(_device->_allocator, stagingBuffer, stagingBufferAllocation);
        }

        void ModelHandlerVK::UpdateIndices(Model& model, const std::vector<u32>& indices)
        {
            VkBuffer stagingBuffer;
            VmaAllocation stagingBufferAllocation;

            // Create a staging buffer
            VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
            _device->CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, stagingBuffer, stagingBufferAllocation);

            // Copy our index data into the staging buffer
            void* indexData;
            vmaMapMemory(_device->_allocator, stagingBufferAllocation, &indexData);
            memcpy(indexData, indices.data(), (size_t)indexBufferSize);
            vmaUnmapMemory(_device->_allocator, stagingBufferAllocation);

            // Copy the index data from our staging buffer to our vertex buffer
            _device->CopyBuffer(model.indexBuffer, 0, stagingBuffer, 0, indexBufferSize);

            // Destroy and free our staging buffer
            vmaDestroyBuffer(_device->_allocator, stagingBuffer, stagingBufferAllocation);
        }
    }
}