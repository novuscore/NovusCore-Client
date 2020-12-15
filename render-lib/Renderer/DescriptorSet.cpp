#include "DescriptorSet.h"
#include <Utils/StringUtils.h>

namespace Renderer
{
    void DescriptorSet::Bind(const std::string& name, SamplerID samplerID)
    {
        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.size());
        Bind(nameHash, samplerID);
    }

    void DescriptorSet::Bind(u32 nameHash, SamplerID samplerID)
    {
        for (u32 i = 0; i < _boundDescriptors.size(); i++)
        {
            if (nameHash == _boundDescriptors[i].nameHash)
            {
                _boundDescriptors[i].descriptorType = DescriptorType::DESCRIPTOR_TYPE_SAMPLER;
                _boundDescriptors[i].samplerID = samplerID;
                return;
            }
        }

        u32 newIndex = static_cast<u32>(_boundDescriptors.size());
        Descriptor& boundDescriptor = _boundDescriptors.emplace_back();
        boundDescriptor.nameHash = nameHash;
        boundDescriptor.descriptorType = DESCRIPTOR_TYPE_SAMPLER;
        boundDescriptor.samplerID = samplerID;
    }

    void DescriptorSet::Bind(const std::string& name, TextureID textureID)
    {
        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.size());
        Bind(nameHash, textureID);
    }

    void DescriptorSet::Bind(u32 nameHash, TextureID textureID)
    {
        for (u32 i = 0; i < _boundDescriptors.size(); i++)
        {
            if (nameHash == _boundDescriptors[i].nameHash)
            {
                _boundDescriptors[i].descriptorType = DescriptorType::DESCRIPTOR_TYPE_TEXTURE;
                _boundDescriptors[i].textureID = textureID;
                return;
            }
        }

        u32 newIndex = static_cast<u32>(_boundDescriptors.size());
        Descriptor& boundDescriptor = _boundDescriptors.emplace_back();
        boundDescriptor.nameHash = nameHash;
        boundDescriptor.descriptorType = DESCRIPTOR_TYPE_TEXTURE;
        boundDescriptor.textureID = textureID;
    }

    void DescriptorSet::Bind(const std::string& name, TextureArrayID textureArrayID)
    {
        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.size());
        Bind(nameHash, textureArrayID);
    }

    void DescriptorSet::Bind(u32 nameHash, TextureArrayID textureArrayID)
    {
        for (u32 i = 0; i < _boundDescriptors.size(); i++)
        {
            if (nameHash == _boundDescriptors[i].nameHash)
            {
                _boundDescriptors[i].descriptorType = DescriptorType::DESCRIPTOR_TYPE_TEXTURE_ARRAY;
                _boundDescriptors[i].textureArrayID = textureArrayID;

                return;
            }
        }

        u32 newIndex = static_cast<u32>(_boundDescriptors.size());
        Descriptor& boundDescriptor = _boundDescriptors.emplace_back();
        boundDescriptor.nameHash = nameHash;
        boundDescriptor.descriptorType = DESCRIPTOR_TYPE_TEXTURE_ARRAY;
        boundDescriptor.textureArrayID = textureArrayID;
    }

    void DescriptorSet::Bind(const std::string& name, ImageID imageID)
    {
        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.size());
        Bind(nameHash, imageID);
    }

    void DescriptorSet::Bind(u32 nameHash, ImageID imageID)
    {
        for (u32 i = 0; i < _boundDescriptors.size(); i++)
        {
            if (nameHash == _boundDescriptors[i].nameHash)
            {
                _boundDescriptors[i].descriptorType = DescriptorType::DESCRIPTOR_TYPE_IMAGE;
                _boundDescriptors[i].imageID = imageID;
                return;
            }
        }

        u32 newIndex = static_cast<u32>(_boundDescriptors.size());
        Descriptor& boundDescriptor = _boundDescriptors.emplace_back();
        boundDescriptor.nameHash = nameHash;
        boundDescriptor.descriptorType = DESCRIPTOR_TYPE_IMAGE;
        boundDescriptor.imageID = imageID;
    }

    void DescriptorSet::Bind(const std::string& name, BufferID buffer)
    {
        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.size());
        Bind(nameHash, buffer);
    }

    void DescriptorSet::Bind(u32 nameHash, BufferID buffer)
    {
        for (u32 i = 0; i < _boundDescriptors.size(); i++)
        {
            if (nameHash == _boundDescriptors[i].nameHash)
            {
                _boundDescriptors[i].descriptorType = DescriptorType::DESCRIPTOR_TYPE_BUFFER;
                _boundDescriptors[i].bufferID = buffer;
                return;
            }
        }

        u32 newIndex = static_cast<u32>(_boundDescriptors.size());
        Descriptor& boundDescriptor = _boundDescriptors.emplace_back();
        boundDescriptor.nameHash = nameHash;
        boundDescriptor.descriptorType = DESCRIPTOR_TYPE_BUFFER;
        boundDescriptor.bufferID = buffer;
    }
}