/** 
  \file SAO_blur.pix
  \author Morgan McGuire and Michael Mara, NVIDIA Research

  \brief 7-tap 1D cross-bilateral blur using a packed depth key

  DX11 HLSL port by Leonardo Zide, Treyarch
  VK HLSL Compute Shader port by Pursche, NovusCore
  
  Open Source under the "BSD" license: http://www.opensource.org/licenses/bsd-license.php

  Copyright (c) 2011-2012, NVIDIA
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//////////////////////////////////////////////////////////////////////////////////////////////
// Tunable Parameters:

/** Increase to make edges crisper. Decrease to reduce temporal flicker. */
#define EDGE_SHARPNESS     (1.0)

/** Step in 2-pixel intervals since we already blurred against neighbors in the
    first AO pass.  This constant can be increased while R decreases to improve
    performance at the expense of some dithering artifacts. 
    
    Morgan found that a scale of 3 left a 1-pixel checkerboard grid that was
    unobjectionable after shading was applied but eliminated most temporal incoherence
    from using small numbers of sample taps.
    */
#define SCALE               (2)

/** Filter radius in pixels. This will be multiplied by SCALE. */
#define R                   (4)

/** Type of data to read from source.  This macro allows
    the same blur shader to be used on different kinds of input data. */
#define VALUE_TYPE        float

/** Swizzle to use to extract the channels of source. This macro allows
    the same blur shader to be used on different kinds of input data. */
#define VALUE_COMPONENTS   r

#define VALUE_IS_KEY       0

/** Channel encoding the bilateral key value (which must not be the same as VALUE_COMPONENTS) */
#define KEY_COMPONENTS     gb

// Gaussian coefficients
static const float gaussian[] = 
//	{ 0.356642, 0.239400, 0.072410, 0.009869 };
//	{ 0.398943, 0.241971, 0.053991, 0.004432, 0.000134 };  // stddev = 1.0
	{ 0.153170, 0.144893, 0.122649, 0.092902, 0.062970 };  // stddev = 2.0
//	{ 0.111220, 0.107798, 0.098151, 0.083953, 0.067458, 0.050920, 0.036108 }; // stddev = 3.0

struct Constants
{
	float2 axis; // (1, 0) or (0, 1)
};

[[vk::push_constant]] Constants _constants;

[[vk::binding(0, PER_PASS)]] Texture2D<float4> _source;
[[vk::binding(1, PER_PASS)]] RWTexture2D<float4> _destination;

/** Returns a number on (0, 1) */
float unpackKey(float2 p)
{
	return p.x * (256.0 / 257.0) + p.y * (1.0 / 257.0);
}

[numthreads(32, 32, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	float2 pixelPos = dispatchThreadID.xy;

	float2 dimensions;
	_destination.GetDimensions(dimensions.x, dimensions.y);

	if (any(pixelPos > dimensions))
	{
		return;
	}

	float4 output = float4(0, 0, 0, 0);

	int2 ssC = pixelPos;
	float4 temp = _source.Load(int3(ssC, 0));

	output.KEY_COMPONENTS = temp.KEY_COMPONENTS;
	float key = unpackKey(output.KEY_COMPONENTS);

	float sum = temp.VALUE_COMPONENTS;

	if (key == 1.0) 
	{
		// Sky pixel (if you aren't using depth keying, disable this test)
		output.VALUE_COMPONENTS = sum;
		_destination[pixelPos] = output;
		return;
	}
	
	// Base weight for depth falloff.  Increase this for more blurriness,
	// decrease it for better edge discrimination
	float BASE = gaussian[0];
	float totalWeight = BASE;
	sum *= totalWeight;

	[unroll]
	for (int r = -R; r <= R; ++r) 
	{
		// We already handled the zero case above.  This loop should be unrolled and the branch discarded
		if (r != 0) 
		{
			temp = _source.Load(int3(ssC + _constants.axis * (r * SCALE), 0));
			float tapKey = unpackKey(temp.KEY_COMPONENTS);
			float value = temp.VALUE_COMPONENTS;

			// spatial domain: offset gaussian tap
			float weight = 0.3 + gaussian[abs(r)];

			// range domain (the "bilateral" weight). As depth difference increases, decrease weight.
			weight *= max(0.0, 1.0 - (2000.0 * EDGE_SHARPNESS) * abs(tapKey - key));

			sum += value * weight;
			totalWeight += weight;
		}
	}

	const float epsilon = 0.0001;
	output.VALUE_COMPONENTS = sum / (totalWeight + epsilon);

	_destination[pixelPos] = output;
}