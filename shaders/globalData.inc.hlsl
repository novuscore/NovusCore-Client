
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

float3 Lighting(float3 color, float3 vertexColor, float3 normal, bool isLit)
{
    // For Indoor WMO Groups
    /*
        float nDotL = saturate(dot(normal, -normalize(_lightData.lightDir.xyz)));

        float3 lightColor = saturate((_lightData.lightColor.rgb * nDotL) + float3(0.003303f, 0.002263f, 0.001687f));// _lightData.ambientColor.rgb);
        lightColor.rgb *= 0.5f;
        lightColor.rgb += vertexColor.rgb;
        lightColor.rgb = saturate(lightColor.rgb);
        return color * (2.0f * lightColor);
    */

    float3 currColor;
    float3 lDiffuse = float3(0.0f, 0.0f, 0.0f);
    float3 accumlatedLight = float3(1.0f, 1.0f, 1.0f);

    if (isLit)
    {
        float nDotL = saturate(dot(normal, -normalize(_lightData.lightDir.xyz)));

        float3 ambientColor = _lightData.ambientColor.rgb + vertexColor;

        float3 skyColor = (ambientColor * 1.10000002);
        float3 groundColor = (ambientColor * 0.699999988);

        currColor = lerp(groundColor, skyColor, 0.5f + (0.5f * nDotL));
        lDiffuse = _lightData.lightColor.rgb * nDotL;
    }
    else
    {
        currColor = _lightData.ambientColor.rgb + vertexColor.rgb;
        accumlatedLight = float3(0.0f, 0.0f, 0.0f);
    }

    // Experimental Gamma Correction
    /*
        float3 gammaDiffTerm = color * (currColor + lDiffuse);
        float3 linearDiffTerm = (color * color) * accumlatedLight;
        return sqrt(gammaDiffTerm * gammaDiffTerm + linearDiffTerm); 
    */
    
    float3 gammaDiffTerm = color * (currColor + lDiffuse);
    return gammaDiffTerm;
}