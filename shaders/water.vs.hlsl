#include "globalData.inc.hlsl"

struct DrawCallData
{
    uint textureStartIndex;
    uint textureCount;
};

struct Vertex
{
    float3 position;
    uint pad;
};

struct VSInput
{
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    uint textureOffset : TEXCOORD1;
};

[[vk::binding(0, PER_PASS)]] StructuredBuffer<DrawCallData> _drawCallDatas;
[[vk::binding(1, PER_PASS)]] StructuredBuffer<Vertex> _vertices;

VSOutput main(VSInput input)
{
    VSOutput output;

    DrawCallData drawCallData = _drawCallDatas[input.instanceID];
    Vertex vertex = _vertices[input.vertexID];
    float4 position = float4(vertex.position, 1.0f);

    output.position = mul(position, _viewData.viewProjectionMatrix);
    output.uv = float2(0.5f, 0.5f); // TODO: Pass UV
    output.textureOffset = drawCallData.textureStartIndex;

    return output;
}