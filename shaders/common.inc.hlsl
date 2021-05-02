#define SIZEOF_UINT  4
#define SIZEOF_UINT2 8
#define SIZEOF_UINT3 12
#define SIZEOF_UINT4 16
#define SIZEOF_DRAW_INDIRECT_ARGUMENTS 16

float2 OctWrap(float2 v)
{
    return (1.0 - abs(v.yx)) * (v.xy >= 0.0 ? 1.0 : -1.0);
}

float2 OctNormalEncode(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0 ? n.xy : OctWrap(n.xy);
    n.xy = n.xy * 0.5 + 0.5;
    return n.xy;
}

float3 OctNormalDecode(float2 f)
{
    f = f * 2.0 - 1.0;

    // https://twitter.com/Stubbesaurus/status/937994790553227264
    float3 n = float3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = saturate(-n.z);
    n.xy += n.xy >= 0.0 ? -t : t;
    return normalize(n);
}

float4 ToFloat4(int input, float defaultAlphaVal)
{
    return float4(input, 0, 0, defaultAlphaVal);
}

float4 ToFloat4(int2 input, float defaultAlphaVal)
{
    return float4(input, 0, defaultAlphaVal);
}

float4 ToFloat4(int3 input, float defaultAlphaVal)
{
    return float4(input, defaultAlphaVal);
}

float4 ToFloat4(int4 input, float defaultAlphaVal)
{
    return float4(input);
}

float4 ToFloat4(uint input, float defaultAlphaVal)
{
    return float4(input, 0, 0, defaultAlphaVal);
}

float4 ToFloat4(uint2 input, float defaultAlphaVal)
{
    return float4(input, 0, defaultAlphaVal);
}

float4 ToFloat4(uint3 input, float defaultAlphaVal)
{
    return float4(input, defaultAlphaVal);
}

float4 ToFloat4(uint4 input, float defaultAlphaVal)
{
    return float4(input);
}

float4 ToFloat4(float input, float defaultAlphaVal)
{
    return float4(input, 0, 0, defaultAlphaVal);
}

float4 ToFloat4(float2 input, float defaultAlphaVal)
{
    return float4(input, 0, defaultAlphaVal);
}

float4 ToFloat4(float3 input, float defaultAlphaVal)
{
    return float4(input, defaultAlphaVal);
}

float4 ToFloat4(float4 input, float defaultAlphaVal)
{
    return input;
}