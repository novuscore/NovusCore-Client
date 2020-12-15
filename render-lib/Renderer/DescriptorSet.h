#pragma once
#include <NovusTypes.h>
#include "Descriptors/BufferDesc.h"
#include "Descriptors/SamplerDesc.h"
#include "Descriptors/TextureDesc.h"
#include "Descriptors/ImageDesc.h"
#include "Descriptors/TextureArrayDesc.h"
#include <robin_hood.h>

namespace Renderer
{
    enum DescriptorType
    {
        DESCRIPTOR_TYPE_SAMPLER,
        DESCRIPTOR_TYPE_TEXTURE,
        DESCRIPTOR_TYPE_TEXTURE_ARRAY,
        DESCRIPTOR_TYPE_IMAGE,
        DESCRIPTOR_TYPE_BUFFER,
    };

    struct Descriptor
    {
        u32 nameHash;

        DescriptorType descriptorType;

        TextureID textureID;
        ImageID imageID;
        SamplerID samplerID;
        TextureArrayID textureArrayID;
        BufferID bufferID;
    };

    enum DescriptorSetSlot
    {
        GLOBAL,
        PER_PASS,
        PER_DRAW
    };

    class DescriptorSet
    {
    public:
        
    public:
        DescriptorSet()
        {}

        void Bind(const std::string& name, SamplerID samplerID);
        void Bind(u32 nameHash, SamplerID samplerID);

        void Bind(const std::string& name, TextureID textureID);
        void Bind(u32 nameHash, TextureID textureID);

        void Bind(const std::string& name, TextureArrayID textureArrayID);
        void Bind(u32 nameHash, TextureArrayID textureArrayID);

        void Bind(const std::string& name, ImageID imageID);
        void Bind(u32 nameHash, ImageID imageID);

        void Bind(const std::string& name, BufferID buffer);
        void Bind(u32 nameHash, BufferID buffer);

        const std::vector<Descriptor>& GetDescriptors() { return _boundDescriptors; }

    private:

    private:
        std::vector<Descriptor> _boundDescriptors;
    };
}