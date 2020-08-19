
struct Vertex
{
    float2 position;
    float2 uv;
};

[[vk::binding(0, PER_DRAW)]] ByteAddressBuffer _vertexData;

struct VertexInput
{
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    uint charIndex : TEXCOORD1;
};

Vertex LoadVertex(uint instanceID, uint vertexID)
{
    Vertex vertex;

    uint sizeOfVertex = 16; // sizeof(Vertex)

    uint vertexOffset = instanceID * 4 + vertexID; // 4 vertices per instance
    vertex.position = _vertexData.Load<float2>(vertexOffset * sizeOfVertex);
    vertex.uv = _vertexData.Load<float2>(vertexOffset * sizeOfVertex + 8);

    return vertex;
}

VertexOutput main(VertexInput input)
{
    VertexOutput output;

    Vertex vertex = LoadVertex(input.instanceID, input.vertexID);

    output.position = float4((vertex.position * 2.0f) - 1.0f, 0.0f, 1.0f);
    output.uv = vertex.uv;
    output.charIndex = input.instanceID;

    return output;
}