#include "globalData.inc.hlsl"

struct VSInput
{
	float3 position : Position;
	float4 color : Color;
};

struct VSOutput
{
	float4 pos : SV_Position;
	float4 color : Color;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.pos = mul(float4(input.position, 1.0f), _viewData.viewProjectionMatrix);
	output.color = input.color;
	return output;
}
