#pragma once
#include <NovusTypes.h>
#include <Utils/StrongTypedef.h>

#include "PermutationField.h"

namespace Renderer
{
    struct ComputeShaderDesc
    {
        void AddPermutationField(const std::string& key, const std::string& value)
        {
            PermutationField& permutationField = permutationFields.emplace_back();
            permutationField.key = key;
            permutationField.value = value;
        }

        std::string path;
        std::vector<PermutationField> permutationFields;
    };

    // Lets strong-typedef an ID type with the underlying type of u16
    STRONG_TYPEDEF(ComputeShaderID, u16);
}