
struct PanelData
{
    float4 color;
};

[[vk::binding(0, PER_PASS)]] SamplerState _sampler;

[[vk::binding(1, PER_DRAW)]] ConstantBuffer<PanelData> _panelData;
[[vk::binding(2, PER_DRAW)]] Texture2D<float4> _texture;

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(VertexOutput input) : SV_Target
{
    return _texture.SampleLevel(_sampler, input.uv, 0) * _panelData.color;
}