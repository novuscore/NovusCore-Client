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

    void DescriptorSet::Bind(const std::string& name, IConstantBuffer* constantBuffer)
    {
        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.size());
        Bind(nameHash, constantBuffer);
    }

    void DescriptorSet::Bind(u32 nameHash, IConstantBuffer* constantBuffer)
    {
        for (u32 i = 0; i < _boundDescriptors.size(); i++)
        {
            if (nameHash == _boundDescriptors[i].nameHash)
            {
                _boundDescriptors[i].descriptorType = DescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
                //_boundDescriptors[i].constantBuffer = constantBuffer;
                return;
            }
        }

        u32 newIndex = static_cast<u32>(_boundDescriptors.size());
        Descriptor& boundDescriptor = _boundDescriptors.emplace_back();
        boundDescriptor.nameHash = nameHash;
        boundDescriptor.descriptorType = DESCRIPTOR_TYPE_CONSTANT_BUFFER;
        //boundDescriptor.constantBuffer = constantBuffer;
    }

    void DescriptorSet::Bind(const std::string& name, IStorageBuffer* storageBuffer)
    {
        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.size());
        Bind(nameHash, storageBuffer);
    }

    void DescriptorSet::Bind(u32 nameHash, IStorageBuffer* storageBuffer)
    {
        for (u32 i = 0; i < _boundDescriptors.size(); i++)
        {
            if (nameHash == _boundDescriptors[i].nameHash)
            {
                _boundDescriptors[i].descriptorType = DescriptorType::DESCRIPTOR_TYPE_STORAGE_BUFFER;
                //_boundDescriptors[i].storageBuffer = storageBuffer;
                return;
            }
        }

        u32 newIndex = static_cast<u32>(_boundDescriptors.size());
        Descriptor& boundDescriptor = _boundDescriptors.emplace_back();
        boundDescriptor.nameHash = nameHash;
        boundDescriptor.descriptorType = DESCRIPTOR_TYPE_STORAGE_BUFFER;
        //boundDescriptor.storageBuffer = storageBuffer;
    }
}