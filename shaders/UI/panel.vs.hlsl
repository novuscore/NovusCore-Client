
struct Vertex
{
    float2 position;
    float2 uv;
};

[[vk::binding(0, PER_DRAW)]] cbuffer _vertices
{
    Vertex vertices[6];
};

struct VertexInput
{
    uint vertexID : SV_VertexID;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;

    Vertex vertex = vertices[input.vertexID];

    output.position = float4((vertex.position * 2.0f) - 1.0f, 0.0f, 1.0f);
    output.uv = vertex.uv;

    return output;
}