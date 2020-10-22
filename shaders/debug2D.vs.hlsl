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
	float4 pos = float4((input.position.x * 2.0f) - 1.f, (input.position.y * 2.0f) - 1.f, 0.f, 1.0f);
	output.pos = pos;
	output.color = input.color;
	return output;
}
