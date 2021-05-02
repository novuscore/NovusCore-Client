permutation COLOR_PASS = [0, 1];

#include "globalData.inc.hlsl"
#include "terrain.inc.hlsl"

struct PackedVertex
{
    uint packed0;
    uint packed1;
}; // 8 bytes

struct Vertex
{
    float3 position;
#if COLOR_PASS
    float3 normal;
    float3 color;
    float2 uv;
#endif
};

[[vk::binding(1, PER_PASS)]] StructuredBuffer<PackedVertex> _packedVertices;

float3 UnpackNormal(uint encoded)
{
    uint x = encoded & 0x000000FFu;
    uint y = (encoded >> 8u) & 0x000000FFu;
    uint z = (encoded >> 16u) & 0x000000FFu;
    
    float3 normal = (float3(x, y, z) / 127.0f) - 1.0f;
    return normalize(normal);
}

float3 UnpackColor(uint encoded)
{
    float r = (float)((encoded) & 0xFF);
    float g = (float)((encoded >> 8) & 0xFF);
    float b = (float)((encoded >> 16) & 0xFF);
    
    return float3(r, g, b) / 127.0f;
}

float UnpackHalf(uint encoded)
{
    return f16tof32(encoded);
}

Vertex UnpackVertex(const PackedVertex packedVertex)
{
    // The vertex consists of 8 bytes of data, we split this into two uints called data0 and data1
    // data0 contains, in order:
    // u8 normal.x
    // u8 normal.y
    // u8 normal.z
    // u8 color.r
    //
    // data1 contains, in order:
    // u8 color.g
    // u8 color.b
    // half height, 2 bytes
    
    Vertex vertex;

#if COLOR_PASS
    // Unpack normal and color
    uint normal = packedVertex.packed0;// & 0x00FFFFFFu;
    uint color = ((packedVertex.packed1 & 0x0000FFFFu) << 8u) | (packedVertex.packed0 >> 24u);

    vertex.normal = UnpackNormal(normal);
    vertex.color = UnpackColor(color);
#endif
    
    // Unpack height
    uint height = packedVertex.packed1 >> 16u;
    vertex.position.z = UnpackHalf(height);
    
    return vertex;
}

Vertex LoadVertex(uint chunkID, uint cellID, uint vertexBaseOffset, uint vertexID)
{
    // Load height
    const uint vertexIndex = vertexBaseOffset + vertexID;
    const PackedVertex packedVertex = _packedVertices[vertexIndex];
    float2 vertexPos = GetCellSpaceVertexPosition(vertexID);

    Vertex vertex = UnpackVertex(packedVertex);

    vertex.position.xy = GetGlobalVertexPosition(chunkID, cellID, vertexID);
#if COLOR_PASS
    vertex.uv = float2(-vertexPos.y, -vertexPos.x); // Negated to go from 3D coordinates to 2D
#endif

    return vertex;
}

struct VSInput
{
    uint vertexID : SV_VertexID;
    uint packedChunkCellID : TEXCOORD0;
    uint cellIndex : TEXCOORD1;
};

struct VSOutput
{
    float4 position : SV_Position;

#if COLOR_PASS
    uint packedChunkCellID : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 color : TEXCOORD3;
    uint cellIndex : TEXCOORD4;
#endif
};

VSOutput main(VSInput input)
{
    VSOutput output;

    CellData cellData = LoadCellData(input.cellIndex);
    if (IsHoleVertex(input.vertexID, cellData.holes))
    {
        const float NaN = asfloat(0b01111111100000000000000000000000);
        output.position = float4(NaN, NaN, NaN, NaN);
        return output;
    }

    const uint cellID = input.packedChunkCellID & 0xffff;
    const uint chunkID = input.packedChunkCellID >> 16;

    uint vertexBaseOffset = input.cellIndex * NUM_VERTICES_PER_CELL;
    Vertex vertex = LoadVertex(chunkID, cellID, vertexBaseOffset, input.vertexID);

    output.position = mul(float4(vertex.position, 1.0f), _viewData.viewProjectionMatrix);

#if COLOR_PASS
    output.uv = vertex.uv;
    output.packedChunkCellID = input.packedChunkCellID;
    output.normal = vertex.normal;
    output.color = vertex.color;
    output.cellIndex = input.cellIndex;
#endif

    return output;
}