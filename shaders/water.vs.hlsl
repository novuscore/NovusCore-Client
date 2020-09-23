#include "globalData.inc.hlsl"

[[vk::binding(0, PER_PASS)]] ByteAddressBuffer _instanceData;
[[vk::binding(1, PER_PASS)]] ByteAddressBuffer _vertexPositions;

struct InstanceData
{
    float4x4 instanceMatrix;

    // This is packed as
    //  - packedVertexData0
    //      half: Vertex 0 Height
    //      half: Vertex 1 Height

    //  - packedVertexData1
    //      half: Vertex 2 Height
    //      half: Vertex 3 Height

    uint packedVertexData0;
    uint packedVertexData1;
};

struct Vertex
{
    float3 position;
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

InstanceData LoadInstanceData(uint instanceID)
{
    InstanceData instanceData;

    instanceData = _instanceData.Load<InstanceData>(instanceID * 72); // 72 = sizeof(InstanceData)

    return instanceData;
}

Vertex LoadVertex(uint vertexID, InstanceData instanceData)
{
    Vertex vertex;
    float vertex0Height = f16tof32(instanceData.packedVertexData0);
    float vertex1Height = f16tof32(instanceData.packedVertexData0 >> 16);
    float vertex2Height = f16tof32(instanceData.packedVertexData1);
    float vertex3Height = f16tof32(instanceData.packedVertexData1 >> 16);
    float4 vertexHeights = float4(vertex0Height, vertex1Height, vertex2Height, vertex3Height);

    float2 vertexPos = _vertexPositions.Load<float2>(vertexID * 8); // 8 = sizeof(float2)
    vertex.position = float3(vertexPos.x, vertexHeights[vertexID], vertexPos.y);

    return vertex;
}

VSOutput main(VSInput input)
{
    VSOutput output;

    InstanceData instanceData = LoadInstanceData(input.instanceID);
    Vertex vertex = LoadVertex(input.vertexID, instanceData); 

    float4 position = float4(vertex.position, 1.0f);
    position = mul(position, instanceData.instanceMatrix);

    output.position = mul(position, _viewData.viewProjectionMatrix);
    output.uv = position.xz / 4.1666f;
    output.textureOffset = input.instanceID;

    return output;
}