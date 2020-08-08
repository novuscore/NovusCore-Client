#include "terrain.inc.hlsl"

struct CellData
{
    uint2 diffuseIDs;
};

struct ChunkData
{
    uint alphaID;
};

[[vk::binding(2, PER_PASS)]] ByteAddressBuffer _cellData;
[[vk::binding(3, PER_PASS)]] ByteAddressBuffer _chunkData;

[[vk::binding(4, PER_PASS)]] SamplerState _alphaSampler;
[[vk::binding(5, PER_PASS)]] SamplerState _colorSampler;

[[vk::binding(6, PER_PASS)]] Texture2D<float4> _terrainColorTextures[4096];
[[vk::binding(7, PER_PASS)]] Texture2DArray<float4> _terrainAlphaTextures[196];

struct PSInput
{
    uint packedChunkCellID : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

uint4 LoadCellTextureIDs(uint globalCellID)
{
    const CellData cellData = _cellData.Load<CellData>(globalCellID * 8); // sizeof(CellData) = 8

    uint4 ids;
    ids.y = cellData.diffuseIDs.x >> 16;
    ids.x = cellData.diffuseIDs.x & 0xffff;
    ids.w = cellData.diffuseIDs.y >> 16;
    ids.z = cellData.diffuseIDs.y & 0xffff;
    return ids;
}

PSOutput main(PSInput input)
{
    PSOutput output;

    const uint cellID = input.packedChunkCellID & 0xffff;
    const uint chunkID = input.packedChunkCellID >> 16;
    
    // Our UVs currently go between 0 and 8, with wrapping. This is correct for terrain color textures
    float2 uv = input.uv; // [0.0 .. 8.0]

    // However the alpha needs to be between 0 and 1, so lets convert it
    float3 alphaUV = float3(uv / 8.0f, float(cellID));

    const uint globalCellID = (chunkID * NUM_CELLS_PER_CHUNK) + cellID;
    const uint4 diffuseIDs = LoadCellTextureIDs(globalCellID);

    const ChunkData chunkData = _chunkData.Load<ChunkData>(chunkID * 4); // sizeof(ChunkData) = 4

    // We have 4 uints per chunk for our diffuseIDs, this gives us a size and alignment of 16 bytes which is exactly what GPUs want
    // However, we need a fifth uint for alphaID, so we decided to pack it into the LAST diffuseID, which gets split into two uint16s
    // This is what it looks like
    // [1111] diffuseIDs.x
    // [2222] diffuseIDs.y
    // [3333] diffuseIDs.z
    // [AA44] diffuseIDs.w Alpha is read from the most significant bits, the fourth diffuseID read from the least 
    uint diffuse0ID = diffuseIDs.x;
    uint diffuse1ID = diffuseIDs.y;
    uint diffuse2ID = diffuseIDs.z;
    uint diffuse3ID = diffuseIDs.w;
    uint alphaID = chunkData.alphaID;

    float3 alpha = _terrainAlphaTextures[alphaID].Sample(_alphaSampler, alphaUV).rgb;
    float4 diffuse0 = _terrainColorTextures[diffuse0ID].Sample(_colorSampler, uv);
    float4 diffuse1 = _terrainColorTextures[diffuse1ID].Sample(_colorSampler, uv);
    float4 diffuse2 = _terrainColorTextures[diffuse2ID].Sample(_colorSampler, uv);
    float4 diffuse3 = _terrainColorTextures[diffuse3ID].Sample(_colorSampler, uv);
    float4 color = diffuse0;
    color = (diffuse1 * alpha.x) + (color * (1.0f - alpha.x));
    color = (diffuse2 * alpha.y) + (color * (1.0f - alpha.y));
    color = (diffuse3 * alpha.z) + (color * (1.0f - alpha.z));
    output.color = color;

    //output.color = float4(1, 0, 0, 1);

    return output;
}