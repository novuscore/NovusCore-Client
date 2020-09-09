
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