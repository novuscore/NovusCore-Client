
[[vk::binding(2, 0)]] RWTexture2D<float> _target;
[[vk::binding(0, 0)]] SamplerState _sampler;
[[vk::binding(1, 0)]] Texture2D<float> _source;


struct Constants
{
    float2 imageSize;
    uint level;
    uint dummy;
};

[[vk::push_constant]] Constants _constants;


[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float2 uvOffset = float2(0.5, 0.5);
    //if (_constants.level == 0)
    //{
    //    uvOffset = float2(0, 0);
    //}
    float2 uv = (float2(DTid.xy) + uvOffset) / _constants.imageSize;

    float depth = _source.SampleLevel(_sampler, uv, 0).x;

    _target[DTid.xy] = depth;
}