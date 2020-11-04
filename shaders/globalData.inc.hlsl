
struct ViewData
{
    float4x4 viewProjectionMatrix;
};
[[vk::binding(0, GLOBAL)]] ConstantBuffer<ViewData> _viewData;

struct LightData
{
    float4 ambientColor;
    float4 lightColor;
    float4 lightDir;
};
[[vk::binding(1, GLOBAL)]] ConstantBuffer<LightData> _lightData;

float3 Lighting(float3 color, float3 normal, bool isLit)
{
    float3 currColor;
    float3 lDiffuse = float3(0, 0, 0);
    float3 accumlatedLight = float3(1.0f, 1.0f, 1.0f);

    if (isLit)
    {
        float nDotL = saturate(dot(normal, -normalize(_lightData.lightDir.xyz)));

        float3 ambientColor = _lightData.ambientColor.rgb;

        float3 skyColor = (ambientColor * 1.10000002);
        float3 groundColor = (ambientColor * 0.699999988);

        currColor = lerp(groundColor, skyColor, 0.5 + (0.5 * nDotL));
        lDiffuse = _lightData.lightColor.rgb * nDotL;
    }
    else
    {
        currColor = float3(1.0, 1.0, 1.0);
    }

    float3 gammaDiffTerm = color * (currColor + lDiffuse);
    float3 linearDiffTerm = (color * color) * accumlatedLight;
    return sqrt(gammaDiffTerm * gammaDiffTerm + linearDiffTerm);
}