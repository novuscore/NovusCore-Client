
struct TextData
{
    float4 textColor;
    float4 outlineColor;
    float outlineWidth;
};

[[vk::binding(0, PER_PASS)]] SamplerState _sampler;

[[vk::binding(1, PER_DRAW)]] ConstantBuffer<TextData> _textData;
[[vk::binding(2, PER_DRAW)]] StructuredBuffer<uint> _textureIDs;
[[vk::binding(3, PER_DRAW)]] Texture2D<float4> _textures[128];

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    uint charIndex : TEXCOORD1;
};

float4 main(VertexOutput input) : SV_Target
{
    uint textureID = _textureIDs[input.charIndex];

    float distance = _textures[textureID].SampleLevel(_sampler, input.uv, 0).r;
    float smoothWidth = fwidth(distance);
    float alpha = smoothstep(0.5 - smoothWidth, 0.5 + smoothWidth, distance);
    float3 rgb = float3(alpha, alpha, alpha) * _textData.textColor.rgb;

    if (_textData.outlineWidth > 0.0)
    {
        float w = 1.0 - _textData.outlineWidth;
        alpha = smoothstep(w - smoothWidth, w + smoothWidth, distance);
        rgb += lerp(float3(alpha, alpha, alpha), _textData.outlineColor.rgb, alpha);
    }

    return float4(rgb, alpha);
}