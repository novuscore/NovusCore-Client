#include "terrain.inc.hlsl"

[[vk::binding(0, PER_PASS)]] cbuffer ViewData
{
    float4x4 viewProjectionMatrix : packoffset(c0);
};
[[vk::binding(1, PER_PASS)]] ByteAddressBuffer _vertexHeights;

struct VSInput
{
    uint packedChunkCellID : TEXCOORD0;
    uint vertexID : SV_VertexID;
};

struct VSOutput
{
    uint packedChunkCellID : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 position : SV_Position;
};

struct Vertex
{
    float3 position;
    float2 uv;
};

Vertex LoadVertex(uint chunkID, uint cellID, uint vertexID)
{
    // Load height
    const uint globalCellID = GetGlobalCellID(chunkID, cellID);
    const uint heightIndex = (globalCellID * NUM_VERTICES_PER_CELL) + vertexID;
    const float height = _vertexHeights.Load<float>(heightIndex * 4); // 4 = sizeof(float) 

    float2 cellPos = GetCellPosition(chunkID, cellID);
    float2 vertexPos = GetCellSpaceVertexPosition(vertexID);

    const float CELL_PRECISION = CELL_SIDE_SIZE / 8.0f;

    Vertex vertex;
    vertex.position.x = ((-vertexPos.x) * CELL_PRECISION + cellPos.x);
    vertex.position.y = -height;
    vertex.position.z = (-vertexPos.y) * CELL_PRECISION + cellPos.y;
    vertex.uv = vertexPos;

    vertex.position = mul(float3x3(
         0, 0, 1,
         0, 1, 0,
        -1, 0, 0
    ), vertex.position);

    return vertex;
}

VSOutput main(VSInput input)
{
    VSOutput output;

    const uint cellID = input.packedChunkCellID & 0xffff;
    const uint chunkID = input.packedChunkCellID >> 16;

    Vertex vertex = LoadVertex(chunkID, cellID, input.vertexID);

    output.position = mul(float4(vertex.position, 1.0f), viewProjectionMatrix);
    output.uv = vertex.uv;
    output.packedChunkCellID = input.packedChunkCellID;

    return output;
}