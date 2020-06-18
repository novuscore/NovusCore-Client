#pragma once
#include <NovusTypes.h>
#include <Utils/StrongTypedef.h>
#include <vector>

namespace Renderer
{
    struct Vertex
    {
        vec3 pos;
        vec3 normal;
        vec2 texCoord;
    };

    struct ModelDesc
    {
        std::string path;
    };

    struct PrimitiveModelDesc
    {
        std::vector<Vertex> vertices;
        std::vector<u32> indices;

        std::string debugName;
    };

    // Lets strong-typedef an ID type with the underlying type of u16
    STRONG_TYPEDEF(ModelID, u16);

}