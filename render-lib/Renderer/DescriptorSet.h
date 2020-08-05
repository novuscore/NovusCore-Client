#pragma once
#include <NovusTypes.h>
#include "Descriptors/SamplerDesc.h"
#include "Descriptors/TextureDesc.h"
#include "Descriptors/TextureArrayDesc.h"
#include <robin_hood.h>

namespace Renderer
{
    struct IConstantBuffer;
    struct IStorageBuffer;

    enum DescriptorType
    {
        DESCRIPTOR_TYPE_SAMPLER,
        DESCRIPTOR_TYPE_TEXTURE,
        DESCRIPTOR_TYPE_TEXTURE_ARRAY,
        DESCRIPTOR_TYPE_CONSTANT_BUFFER,
        DESCRIPTOR_TYPE_STORAGE_BUFFER
    };

    struct Descriptor
    {
        u32 nameHash;

        DescriptorType descriptorType;

        TextureID textureID;
        SamplerID samplerID;
        TextureArrayID textureArrayID;
        BufferID bufferID;
        //IConstantBuffer* constantBuffer;
        //IStorageBuffer* storageBuffer;
    };

    enum DescriptorSetSlot
    {
        GLOBAL,
        PER_PASS,
        PER_DRAW
    };

    struct DescriptorSetBackend
    {
        
    };

    class DescriptorSet
    {
    public:
        
    public:
        DescriptorSet()
            : _backend(nullptr)
        {}

        void Bind(const std::string& name, SamplerID samplerID);
        void Bind(u32 nameHash, SamplerID samplerID);

        void Bind(const std::string& name, TextureID textureID);
        void Bind(u32 nameHash, TextureID textureID);

        void Bind(const std::string& name, TextureArrayID textureArrayID);
        void Bind(u32 nameHash, TextureArrayID textureArrayID);

        void Bind(const std::string& name, IConstantBuffer* constantBuffer);
        void Bind(u32 nameHash, IConstantBuffer* constantBuffer);

        void Bind(const std::string& name, IStorageBuffer* storageBuffer);
        void Bind(u32 nameHash, IStorageBuffer* storageBuffer);

        const std::vector<Descriptor>& GetDescriptors() { return _boundDescriptors; }
        
        DescriptorSetBackend* GetBackend() { return _backend; }
        void SetBackend(DescriptorSetBackend* backend) { _backend = backend; }

    private:

    private:
        std::vector<Descriptor> _boundDescriptors;

        DescriptorSetBackend* _backend = nullptr;
    };
}