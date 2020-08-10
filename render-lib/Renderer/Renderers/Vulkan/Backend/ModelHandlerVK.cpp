#include "ModelHandlerVK.h"
#include <cassert>
#include <filesystem>
#include <Utils/DebugHandler.h>
#include <Utils/FileReader.h>
#include "RenderDeviceVK.h"
#include "DebugMarkerUtilVK.h"
#include "BufferHandlerVK.h"

namespace Renderer
{
    namespace Backend
    {
        void ModelHandlerVK::Init(RenderDeviceVK* device, BufferHandlerVK* bufferHandler)
        {
            _device = device;
            _bufferHandler = bufferHandler;
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

            return _bufferHandler->GetBuffer(model.vertexBuffer);
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

            return _bufferHandler->GetBuffer(model.indexBuffer);
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
                BufferDesc bufferDesc;
                bufferDesc.name = model.debugName + "_VertexBuffer";
                bufferDesc.size = sizeof(data.vertices[0]) * data.vertices.size();
                bufferDesc.usage = BUFFER_USAGE_TRANSFER_DESTINATION | BUFFER_USAGE_VERTEX_BUFFER;
                model.vertexBuffer = _bufferHandler->CreateBuffer(bufferDesc);

                DebugMarkerUtilVK::SetObjectName(_device->_device, (u64)_bufferHandler->GetBuffer(model.vertexBuffer), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, model.debugName.c_str());

                UpdateVertices(model, data.vertices);
            }
            
            // -- Create index buffer --
            if (model.numIndices > 0)
            {
                BufferDesc bufferDesc;
                bufferDesc.name = model.debugName + "_IndexBuffer";
                bufferDesc.size = sizeof(data.indices[0]) * data.indices.size();
                bufferDesc.usage = BUFFER_USAGE_TRANSFER_DESTINATION | BUFFER_USAGE_INDEX_BUFFER;
                model.indexBuffer = _bufferHandler->CreateBuffer(bufferDesc);

                DebugMarkerUtilVK::SetObjectName(_device->_device, (u64)_bufferHandler->GetBuffer(model.vertexBuffer), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, model.debugName.c_str());

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
            // Create a staging buffer
            BufferDesc bufferDesc;
            bufferDesc.name = model.debugName + "_VertexBuffer";
            bufferDesc.size = sizeof(vertices[0]) * vertices.size();
            bufferDesc.usage = BUFFER_USAGE_TRANSFER_SOURCE;
            bufferDesc.cpuAccess = BufferCPUAccess::WriteOnly;
            BufferID stagingBuffer = _bufferHandler->CreateBuffer(bufferDesc);

            VmaAllocation allocation = _bufferHandler->GetBufferAllocation(stagingBuffer);

            // Copy our vertex data into the staging buffer
            void* vertexData;
            vmaMapMemory(_device->_allocator, allocation, &vertexData);
            memcpy(vertexData, vertices.data(), (size_t)bufferDesc.size);
            vmaUnmapMemory(_device->_allocator, allocation);

            // Copy the vertex data from our staging buffer to our vertex buffer
            _device->CopyBuffer(_bufferHandler->GetBuffer(model.vertexBuffer), 0, _bufferHandler->GetBuffer(stagingBuffer), 0, bufferDesc.size);

            // Destroy and free our staging buffer
            _bufferHandler->DestroyBuffer(stagingBuffer);
        }

        void ModelHandlerVK::UpdateIndices(Model& model, const std::vector<u32>& indices)
        {
            // Create a staging buffer
            BufferDesc bufferDesc;
            bufferDesc.name = model.debugName + "_VertexBuffer";
            bufferDesc.size = sizeof(indices[0]) * indices.size();
            bufferDesc.usage = BUFFER_USAGE_TRANSFER_SOURCE;
            bufferDesc.cpuAccess = BufferCPUAccess::WriteOnly;
            BufferID stagingBuffer = _bufferHandler->CreateBuffer(bufferDesc);

            // Copy our index data into the staging buffer
            void* indexData;
            vmaMapMemory(_device->_allocator, _bufferHandler->GetBufferAllocation(stagingBuffer), &indexData);
            memcpy(indexData, indices.data(), (size_t)bufferDesc.size);
            vmaUnmapMemory(_device->_allocator, _bufferHandler->GetBufferAllocation(stagingBuffer));

            // Copy the index data from our staging buffer to our vertex buffer
            _device->CopyBuffer(_bufferHandler->GetBuffer(model.indexBuffer), 0, _bufferHandler->GetBuffer(stagingBuffer), 0, bufferDesc.size);

            // Destroy and free our staging buffer
            _bufferHandler->DestroyBuffer(stagingBuffer);
        }
    }
}