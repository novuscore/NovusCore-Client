
struct Vertex
{
    float3 position;
    float2 uv;
};

[[vk::binding(0, PER_PASS)]] cbuffer ViewData
{
    float4x4 viewMatrix : packoffset(c0);
    float4x4 projMatrix : packoffset(c4);
};
[[vk::binding(0, PER_DRAW)]] cbuffer ModelData
{
    float4 colorMultiplier : packoffset(c0);
    float4x4 modelMatrix : packoffset(c1);
};
[[vk::binding(1, PER_DRAW)]] ByteAddressBuffer _vertexHeights;

Vertex LoadVertex(uint vertexID, uint instanceID)
{
    // Load height
    uint heightIndex = vertexID + (instanceID * 145u); // 145 vertices per cell, each new instance is a cell
    float height = asfloat(_vertexHeights.Load(heightIndex * 4)); // 4 is sizeof(float) in bytes

    // Calculate position.xz and uv from vertexID
    const uint MAP_CELLS_PER_CHUNK_SIDE = 16u;

    uint cellX = instanceID % MAP_CELLS_PER_CHUNK_SIDE;
    uint cellY = instanceID / MAP_CELLS_PER_CHUNK_SIDE;

    const float CELL_SIZE = 33.3333f; // yards
    const float CELL_PRECISION = CELL_SIZE / 8.0f;

    float cellPosX = -float(cellX) * CELL_SIZE;
    float cellPosZ = float(cellY) * CELL_SIZE;

    float vertexX = vertexID % 17.0f;
    float vertexY = floor(vertexID / 17.0f);

    bool isOddRow = vertexX > 8.01f;
    vertexX = vertexX - (8.5f * isOddRow);
    vertexY = vertexY + (0.5f * isOddRow);

    float vertexPosX = -vertexX * CELL_PRECISION;
    float vertexPosZ = vertexY * CELL_PRECISION;

    Vertex vertex;
    vertex.position = float3(vertexPosX + cellPosX, height, vertexPosZ + cellPosZ);
    vertex.uv = float2(vertexX, vertexY);

    return vertex;
}

struct VSInput
{
    uint instanceID : TEXCOORD0;
    uint vertexID : SV_VertexID;
};

struct VSOutput
{
    uint instanceID : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 position : SV_Position;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    Vertex vertex = LoadVertex(input.vertexID, input.instanceID);

    output.position = mul(float4(vertex.position, 1.0f), mul(modelMatrix, mul(viewMatrix, projMatrix)));
    output.uv = vertex.uv;
    output.instanceID = input.instanceID;

    return output;
}