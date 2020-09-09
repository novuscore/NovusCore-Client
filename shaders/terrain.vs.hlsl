#include "globalData.inc.hlsl"
#include "terrain.inc.hlsl"

[[vk::binding(0, PER_PASS)]] ByteAddressBuffer _vertices;
[[vk::binding(1, PER_PASS)]] ByteAddressBuffer _cellDataVS;

struct VSInput
{
    uint vertexID : SV_VertexID;
    uint packedChunkCellID : TEXCOORD0;
    uint cellIndex : TEXCOORD1;
};

struct VSOutput
{
    float4 position : SV_Position;
    uint packedChunkCellID : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 color : TEXCOORD3;
    uint cellIndex : TEXCOORD4;
};

struct PackedVertex
{
    uint data0;
    uint data1;
};

struct Vertex
{
    float3 position;
    float3 normal;
    float3 color;
    float2 uv;
};

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
    
    // Unpack normal, color and height
    uint normal = packedVertex.data0;// & 0x00FFFFFFu;
    uint color = ((packedVertex.data1 & 0x0000FFFFu) << 8u) | (packedVertex.data0 >> 24u);
    uint height = packedVertex.data1 >> 16u;
    
    Vertex vertex;
    vertex.normal = UnpackNormal(normal);
    vertex.color = UnpackColor(color);
    vertex.position.y = UnpackHalf(height);
    
    return vertex;
}

Vertex LoadVertex(uint chunkID, uint cellID, uint vertexBaseOffset, uint vertexID)
{
    // Load height
    const uint vertexIndex = vertexBaseOffset + vertexID;
    const PackedVertex packedVertex = _vertices.Load<PackedVertex>(vertexIndex * 8); // 8 = sizeof(PackedVertex)

    Vertex vertex = UnpackVertex(packedVertex);
    
    // Set remaining vertex parameters which are based on vertexID
    float2 cellPos = GetCellPosition(chunkID, cellID);
    float2 vertexPos = GetCellSpaceVertexPosition(vertexID);

    const float CELL_PRECISION = CELL_SIDE_SIZE / 8.0f;

    vertex.position.x = -((-vertexPos.x) * CELL_PRECISION + cellPos.x);
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
    const PackedCellData rawCellData = _cellDataVS.Load<PackedCellData>(globalCellID * 12); // sizeof(PackedCellData) = 12

    CellData cellData;

    // Unpack diffuse IDs
    cellData.diffuseIDs.x = (rawCellData.packedDiffuseIDs1 >> 0) & 0xffff;
    cellData.diffuseIDs.y = (rawCellData.packedDiffuseIDs1 >> 16) & 0xffff;
    cellData.diffuseIDs.z = (rawCellData.packedDiffuseIDs2 >> 0) & 0xffff;
    cellData.diffuseIDs.w = (rawCellData.packedDiffuseIDs2 >> 16) & 0xffff;

    // Unpack holes
    cellData.holes = rawCellData.packedHoles & 0xffff;

    return cellData;
}

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
    output.uv = vertex.uv;
    output.packedChunkCellID = input.packedChunkCellID;
    output.normal = vertex.normal;
    output.color = vertex.color;
    output.cellIndex = input.cellIndex;

    return output;
}