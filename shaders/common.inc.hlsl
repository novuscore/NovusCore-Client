
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