#include "terrain.inc.hlsl"

[[vk::binding(0, PER_PASS)]] cbuffer ViewData
{
    float4x4 viewProjectionMatrix : packoffset(c0);
};
[[vk::binding(1, PER_PASS)]] ByteAddressBuffer _vertexHeights;
[[vk::binding(2, PER_PASS)]] ByteAddressBuffer _cellDataVS;

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
    vertex.position.x = -((-vertexPos.x) * CELL_PRECISION + cellPos.x);
    vertex.position.y = height;
    vertex.position.z = (-vertexPos.y) * CELL_PRECISION + cellPos.y;
    vertex.uv = vertexPos;

    vertex.position = mul(float3x3(
         0, 0, 1,
         0, 1, 0,
        -1, 0, 0
    ), vertex.position);

    return vertex;
}

CellData LoadCellData(uint globalCellID)
{
    const PackedCellData rawCellData = _cellDataVS.Load<PackedCellData>(globalCellID * 8); // sizeof(PackedCellData) = 8

    CellData cellData;

    // Unpack diffuse IDs
    cellData.diffuseIDs.x = (rawCellData.packedDiffuseIDs >> 0) & 0xff;
    cellData.diffuseIDs.y = (rawCellData.packedDiffuseIDs >> 8) & 0xff;
    cellData.diffuseIDs.z = (rawCellData.packedDiffuseIDs >> 16) & 0xff;
    cellData.diffuseIDs.w = (rawCellData.packedDiffuseIDs >> 24) & 0xff;

    // Unpack holes
    cellData.holes = rawCellData.packedHoles & 0xffff;

    return cellData;
}

VSOutput main(VSInput input)
{
    VSOutput output;

    const uint cellID = input.packedChunkCellID & 0xffff;
    const uint chunkID = input.packedChunkCellID >> 16;

    const uint globalCellID = GetGlobalCellID(chunkID, cellID);
    CellData cellData = LoadCellData(globalCellID);
    if (IsHoleVertex(input.vertexID, cellData.holes))
    {
        const float NaN = asfloat(0b01111111100000000000000000000000);
        output.position = float4(NaN, NaN, NaN, NaN);
        return output;
    }

    Vertex vertex = LoadVertex(chunkID, cellID, input.vertexID);

    output.position = mul(float4(vertex.position, 1.0f), viewProjectionMatrix);
    output.uv = vertex.uv;
    output.packedChunkCellID = input.packedChunkCellID;

    return output;
}