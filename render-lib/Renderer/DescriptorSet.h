#pragma once
#include <NovusTypes.h>
#include <Utils/StringUtils.h>
#include "Descriptors/BufferDesc.h"
#include "Descriptors/SamplerDesc.h"
#include "Descriptors/TextureDesc.h"
#include "Descriptors/ImageDesc.h"
#include "Descriptors/DepthImageDesc.h"
#include "Descriptors/TextureArrayDesc.h"

namespace Renderer
{
    enum DescriptorType
    {
        DESCRIPTOR_TYPE_SAMPLER,
        DESCRIPTOR_TYPE_TEXTURE,
        DESCRIPTOR_TYPE_TEXTURE_ARRAY,
        DESCRIPTOR_TYPE_IMAGE,
        DESCRIPTOR_TYPE_DEPTH_IMAGE,
        DESCRIPTOR_TYPE_STORAGE_IMAGE,
        DESCRIPTOR_TYPE_BUFFER,
    };

    struct Descriptor
    {
        u32 nameHash;
        u32 imageMipLevel;
        DescriptorType descriptorType;

        TextureID textureID;
        ImageID imageID;
        DepthImageID depthImageID;
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

        void Bind(const std::string& name, ImageID imageID, u32 mipLevel = 0 );
        void Bind(u32 nameHash, ImageID imageID, u32 mipLevel = 0);

        void Bind(StringUtils::StringHash nameHash, DepthImageID imageID);

        void BindStorage(StringUtils::StringHash nameHash, ImageID imageID, u32 mipLevel = 0);

        void Bind(const std::string& name, BufferID buffer);
        void Bind(u32 nameHash, BufferID buffer);

        const std::vector<Descriptor>& GetDescriptors() { return _boundDescriptors; }

    private:

    private:
        std::vector<Descriptor> _boundDescriptors;
    };
}