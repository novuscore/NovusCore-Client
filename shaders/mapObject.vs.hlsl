
[[vk::binding(0, PER_PASS)]] cbuffer ViewData
{
    float4x4 viewProjectionMatrix : packoffset(c0);
};

[[vk::binding(1, PER_PASS)]] ByteAddressBuffer _instanceData;

[[vk::binding(0, PER_DRAW)]] ByteAddressBuffer _vertexPositions;
[[vk::binding(1, PER_DRAW)]] ByteAddressBuffer _vertexNormals;
[[vk::binding(2, PER_DRAW)]] ByteAddressBuffer _vertexUVs;

struct InstanceData
{
    float4x4 instanceMatrix;
};

struct Vertex
{
    float3 position;
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
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
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
    vertex.uv0 = _vertexUVs.Load<float2>(vertexID * 16); // 16 = sizeof(float2) * 2 because we have 2 sets of UVs
    vertex.uv1 = _vertexUVs.Load<float2>(vertexID * 16 + 8); // 16 = sizeof(float2) * 2 because we have 2 sets of UVs

    // TODO: Remove this from the shader, we want to do this in the dataextractor instead
    vertex.position = float3(-vertex.position.x, vertex.position.z, -vertex.position.y);

    return vertex;
}

VSOutput main(VSInput input)
{
    VSOutput output;

    InstanceData instanceData = LoadInstanceData(input.instanceID);
    Vertex vertex = LoadVertex(input.vertexID); 

    float4 position = float4(vertex.position, 1.0f);
    position = mul(position, instanceData.instanceMatrix);

    output.position = mul(position, viewProjectionMatrix);
    output.uv0 = vertex.uv0;
    output.uv1 = vertex.uv1;

    return output;
}