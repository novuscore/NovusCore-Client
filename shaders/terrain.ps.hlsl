#include "common.inc.hlsl"
#include "globalData.inc.hlsl"
#include "terrain.inc.hlsl"

[[vk::binding(2, PER_PASS)]] StructuredBuffer<ChunkData> _chunkData;

[[vk::binding(3, PER_PASS)]] SamplerState _alphaSampler;
[[vk::binding(4, PER_PASS)]] SamplerState _colorSampler;
[[vk::binding(5, PER_PASS)]] Texture2D<float4> _ambientOcclusion;

[[vk::binding(6, PER_PASS)]] Texture2D<float4> _terrainColorTextures[4096];
[[vk::binding(7, PER_PASS)]] Texture2DArray<float4> _terrainAlphaTextures[NUM_CHUNKS_PER_MAP_SIDE * NUM_CHUNKS_PER_MAP_SIDE];


struct PSInput
{
    float4 position : SV_Position;
    uint packedChunkCellID : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 color : TEXCOORD3;
    uint cellIndex : TEXCOORD4;
};

struct PSOutput
{
    float4 color : SV_Target0;
    uint objectID : SV_Target1;
};

PSOutput main(PSInput input)
{
    PSOutput output;

    const uint cellID = input.packedChunkCellID & 0xffff;
    const uint chunkID = input.packedChunkCellID >> 16;
    
    // Our UVs currently go between 0 and 8, with wrapping. This is correct for terrain color textures
    float2 uv = input.uv; // [0.0 .. 8.0]

    // However the alpha needs to be between 0 and 1, so lets convert it
    float3 alphaUV = float3(saturate(uv / 8.0f), float(cellID));

    const CellData cellData = LoadCellData(input.cellIndex);

    const uint chunkIndex = input.cellIndex / NUM_CELLS_PER_CHUNK;
    const ChunkData chunkData = _chunkData[chunkIndex];

    // We have 4 uints per chunk for our diffuseIDs, this gives us a size and alignment of 16 bytes which is exactly what GPUs want
    // However, we need a fifth uint for alphaID, so we decided to pack it into the LAST diffuseID, which gets split into two uint16s
    // This is what it looks like
    // [1111] diffuseIDs.x
    // [2222] diffuseIDs.y
    // [3333] diffuseIDs.z
    // [AA44] diffuseIDs.w Alpha is read from the most significant bits, the fourth diffuseID read from the least 
    uint diffuse0ID = cellData.diffuseIDs.x;
    uint diffuse1ID = cellData.diffuseIDs.y;
    uint diffuse2ID = cellData.diffuseIDs.z;
    uint diffuse3ID = cellData.diffuseIDs.w;
    uint alphaID = chunkData.alphaID;

    float3 alpha = _terrainAlphaTextures[NonUniformResourceIndex(alphaID)].Sample(_alphaSampler, alphaUV).rgb;
    float minusAlphaBlendSum = (1.0 - clamp(alpha.x + alpha.y + alpha.z, 0.0, 1.0));
    float4 weightsVector = float4(minusAlphaBlendSum, alpha);

    float4 color = float4(0, 0, 0, 0);

    float4 diffuse0 = _terrainColorTextures[NonUniformResourceIndex(diffuse0ID)].Sample(_colorSampler, uv) * weightsVector.x;
    color += diffuse0;

    float4 diffuse1 = _terrainColorTextures[NonUniformResourceIndex(diffuse1ID)].Sample(_colorSampler, uv) * weightsVector.y;
    color += diffuse1;

    float4 diffuse2 = _terrainColorTextures[NonUniformResourceIndex(diffuse2ID)].Sample(_colorSampler, uv) * weightsVector.z;
    color += diffuse2;

    float4 diffuse3 = _terrainColorTextures[NonUniformResourceIndex(diffuse3ID)].Sample(_colorSampler, uv) * weightsVector.w;
    color += diffuse3;

    // Apply Vertex Lighting
    color.rgb *= input.color;

    // Apply lighting
    float3 normal = normalize(input.normal);
    float ambientOcclusion = _ambientOcclusion.Load(float3(input.position.xy, 0)).x;
    color.rgb = Lighting(color.rgb, float3(0.0f, 0.0f, 0.0f), normal, ambientOcclusion, true);

    output.color = saturate(color);

    //float2 octNormal = OctNormalEncode(normal);
    //output.normal = float4(octNormal, 0, 0);
    //float3 viewspaceNormal = normalize(mul(normal, (float3x3)_viewData.viewMatrix));
    //viewspaceNormal.y = -viewspaceNormal.y;

    //float2 octNormal = OctNormalEncode(viewspaceNormal);
    //output.normal = float4(octNormal, 0, 0);

    // 4 most significant bits are used as a type identifier, remaining bits are packedChunkCellID
    output.objectID = uint(ObjectType::Terrain) << 28;
    output.objectID += input.packedChunkCellID;
    return output;
}