
struct Constants
{
	//float nearPlane;
	float farPlane;
	float nearPlane;
};

[[vk::push_constant]] Constants _constants;

[[vk::binding(0, PER_PASS)]] SamplerState _sampler;
[[vk::binding(1, PER_PASS)]] Texture2D<float> _depth;
[[vk::binding(2, PER_PASS)]] RWTexture2D<float> _linearDepth;

float LinearDepth(float depth)
{
	float near = _constants.nearPlane;
	float far = _constants.farPlane;

	return near * far / (far + depth * (near - far));
}

[numthreads(32, 32, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	float2 pixelPos = dispatchThreadID.xy;

	float2 dimensions;
	_linearDepth.GetDimensions(dimensions.x, dimensions.y);

	if (any(pixelPos >= dimensions))
	{
		return;
	}
	
	float2 uv = pixelPos / dimensions;
	float depth = _depth.SampleLevel(_sampler, uv, 0);

	// Not needed? o_O
	//float linearDepth = LinearDepth(depth);

	_linearDepth[pixelPos] = depth;
}