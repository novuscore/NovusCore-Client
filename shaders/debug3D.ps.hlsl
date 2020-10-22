struct PSInput
{
	float4 color : Color;
};

float4 main(PSInput input) : SV_Target0
{
	return input.color;
}