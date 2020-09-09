#include "globalData.inc.hlsl"

// This file is a dummy shader, we don't use it for rendering but we want to load it to reflect descriptorsets from globalData.inc.hlsl

struct VSInput
{

};

struct VSOutput
{
	float4 pos : SV_Position;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.pos = float4(1, 1, 1, 1);
	return output;
}
