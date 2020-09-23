#include "globalData.inc.hlsl"

[[vk::binding(0, PER_PASS)]] ByteAddressBuffer _instanceData;

[[vk::binding(0, PER_DRAW)]] ByteAddressBuffer _vertexPositions;
[[vk::binding(1, PER_DRAW)]] ByteAddressBuffer _vertexNormals;
[[vk::binding(2, PER_DRAW)]] ByteAddressBuffer _vertexUVs0;
[[vk::binding(3, PER_DRAW)]] ByteAddressBuffer _vertexUVs1;

struct InstanceData
{
    float4x4 instanceMatrix;
};

struct Vertex
{
    float3 position;
    float3 normal;
    float2 uv0;
    float2 uv1;
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
    float2 uv0 : TEXCOORD1;
    float2 uv1 : TEXCOORD2;
};

InstanceData LoadInstanceData(uint instanceID)
{
    InstanceData instanceData;

    instanceData = _instanceData.Load<InstanceData>(instanceID * 64); // 64 = sizeof(InstanceData)

    return instanceData;
}

Vertex LoadVertex(uint vertexID)
{
    Vertex vertex;

    vertex.position = _vertexPositions.Load<float3>(vertexID * 12); // 12 = sizeof(float3)
    vertex.normal = _vertexNormals.Load<float3>(vertexID * 12); // 12 = sizeof(float3)
    vertex.uv0 = _vertexUVs0.Load<float2>(vertexID * 8); // 8 = sizeof(float2)
    vertex.uv1 = _vertexUVs1.Load<float2>(vertexID * 8); // 8 = sizeof(float2)

    // TODO: Remove this from the shader, we want to do this in the dataextractor instead
    vertex.position = float3(-vertex.position.x, vertex.position.z, -vertex.position.y);
    vertex.normal = vertex.normal.rbg;
    vertex.normal.z = -vertex.normal.z;

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
    output.uv0 = vertex.uv0;
    output.uv1 = vertex.uv1;

    return output;
}