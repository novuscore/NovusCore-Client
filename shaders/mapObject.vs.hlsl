#include "globalData.inc.hlsl"

[[vk::binding(0, PER_PASS)]] ByteAddressBuffer _instanceData;

[[vk::binding(0, PER_DRAW)]] ByteAddressBuffer _vertexPositions;
[[vk::binding(1, PER_DRAW)]] ByteAddressBuffer _vertexNormals;
[[vk::binding(2, PER_DRAW)]] Texture2D _vertexColorsTexture;
[[vk::binding(3, PER_DRAW)]] ByteAddressBuffer _vertexUVs;

struct InstanceData
{
    float4x4 instanceMatrix;
};

struct Vertex
{
    float3 position;
    float3 normal;
    float4 color0;
    float4 color1;
    float4 uv01;
};

struct VSInput
{
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float4 color0 : TEXCOORD1;
    float4 color1 : TEXCOORD2;
    float4 uv01 : TEXCOORD3;
};

InstanceData LoadInstanceData(uint instanceID)
{
    InstanceData instanceData;

    instanceData = _instanceData.Load<InstanceData>(instanceID * 24); // 24 = sizeof(InstanceData)

    return instanceData;
}

Vertex LoadVertex(uint vertexID)
{
    Vertex vertex;

    vertex.position = _vertexPositions.Load<float3>(vertexID * 12); // 12 = sizeof(float3)
    vertex.normal = _vertexNormals.Load<float3>(vertexID * 12); // 12 = sizeof(float3)

    vertex.color0 = _vertexColorsTexture.Load(int3(vertexID*2, 0, 0)); // 2 because we have 2 sets of vertexColors
    vertex.color1 = _vertexColorsTexture.Load(int3(vertexID*2 + 1, 0, 0)); // 2 because we have 2 sets of vertexColors, 1 because this loads the second set

    vertex.uv01 = _vertexUVs.Load<float4>(vertexID * 16); // 16 = sizeof(float4)

    //vertex.normal = float3(-vertex.normal.x, vertex.normal.y, -vertex.normal.z);

    return vertex;
}

VSOutput main(VSInput input)
{
    VSOutput output;

    InstanceData instanceData = LoadInstanceData(input.instanceID);
    Vertex vertex = LoadVertex(input.vertexID); 

    float4 position = float4(vertex.position, 1.0f);
    position = mul(position, instanceData.instanceMatrix);

    output.position = mul(position, _viewData.viewProjectionMatrix);
    output.normal = mul(vertex.normal, (float3x3)instanceData.instanceMatrix);

    output.color0 = vertex.color0;
    output.color1 = vertex.color1;

    output.uv01 = vertex.uv01;

    return output;
}