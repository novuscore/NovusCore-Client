/**
 \file SAO_AO.pix
 \author Morgan McGuire and Michael Mara, NVIDIA Research
   
 Reference implementation of the Scalable Ambient Obscurance (SAO) screen-space ambient obscurance algorithm.

 The optimized algorithmic structure of SAO was published in McGuire, Mara, and Luebke, Scalable Ambient Obscurance,
 <i>HPG</i> 2012, and was developed at NVIDIA with support from Louis Bavoil.

 The mathematical ideas of AlchemyAO were first described in McGuire, Osman, Bukowski, and Hennessy, The
 Alchemy Screen-Space Ambient Obscurance Algorithm, <i>HPG</i> 2011 and were developed at
 Vicarious Visions.

 DX11 HLSL port by Leonardo Zide of Treyarch
 VK HLSL Compute Shader port by Pursche, NovusCore

 <hr>

  Open Source under the "BSD" license: http://www.opensource.org/licenses/bsd-license.php

  Copyright (c) 2011-2012, NVIDIA
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  */
#include "Common.inc.hlsl"

// Total number of direct samples to take at each pixel
#define NUM_SAMPLES (11)

// If using depth mip levels, the log of the maximum pixel offset before we need to switch to a lower 
// miplevel to maintain reasonable spatial locality in the cache
// If this number is too small (< 3), too many taps will land in the same pixel, and we'll get bad variance that manifests as flashing.
// If it is too high (> 5), we'll get bad performance because we're not using the MIP levels effectively
#define LOG_MAX_OFFSET 3

// This must be less than or equal to the MAX_MIP_LEVEL defined in SSAO.cpp
#define MAX_MIP_LEVEL 5

/** Used for preventing AO computation on the sky (at infinite depth) and defining the CS Z to bilateral depth key scaling.
	This need not match the real far plane*/
#define FAR_PLANE_Z (100000.0)
#define NEAR_PLANE_Z (1.0)

	// This is the number of turns around the circle that the spiral pattern makes.  This should be prime to prevent
	// taps from lining up.  This particular choice was tuned for NUM_SAMPLES == 9
#define NUM_SPIRAL_TURNS (7)

struct Constants
{
	/** The height in pixels of a 1m object if viewed from 1m away.
	You can compute it from your projection matrix.  The actual value is just
	a scale factor on radius; you can simply hardcode this to a constant (~500)
	and make your radius value unitless (...but resolution dependent.)  */
	float projScale;
	float radius; // World-space AO radius in scene units (r).  e.g., 1.0m
	float bias; // Bias to avoid AO in smooth corners, e.g., 0.01m
	float intensity; // Darkending factor, e.g., 1.0

	float4x4 viewMatrix;
	float4x4 invProjectionMatrix;
};

[[vk::push_constant]] Constants _constants;

[[vk::binding(0, PER_PASS)]] Texture2D<float> _depth;
[[vk::binding(1, PER_PASS)]] RWTexture2D<float4> _destination;

float LinearDepth(float depth)
{
	float near = NEAR_PLANE_Z;
	float far = FAR_PLANE_Z;

	return near * far / (far + depth * (near - far));
}

float3 ndcReconstructCSPos(float2 pos, float z)
{
	float4 p = float4(pos, z, 1.0f);

	float4 tmp = mul(p, _constants.invProjectionMatrix);
	return tmp.xyz / tmp.w;
}

// Returns a unit vector and a screen-space radius for the tap on a unit disk (the caller should scale by the actual disk radius)
float2 tapLocation(int sampleNumber, float spinAngle, out float ssR) 
{
	// Radius relative to ssR
	float alpha = float(sampleNumber + 0.5) * (1.0 / NUM_SAMPLES);
	float angle = alpha * (NUM_SPIRAL_TURNS * 6.28) + spinAngle;

	ssR = alpha;
	return float2(cos(angle), sin(angle));
}

// Used for packing Z into the GB channels
float CSZToKey(float z) 
{
	return clamp(z * (1.0 / -FAR_PLANE_Z), 0.0, 1.0);
}

// Used for packing Z into the GB channels
void packKey(float key, out float2 p) 
{
	// Round to the nearest 1/256.0
	float temp = floor(key * 256.0);

	// Integer part
	p.x = temp * (1.0 / 256.0);

	// Fractional part
	p.y = key * 256.0 - temp;
}

// Read the camera-space position of the point at screen-space pixel ssP
float3 getPosition(int2 ssP, float2 resolution) 
{
	float depth = _depth.Load(int3(ssP, 0)).r;

	float2 uv = (ssP + float2(0.5, 0.5)) / resolution;
	float2 ndcPos = (uv - 0.5f) * 2.0f;

	float3 P;
	P = ndcReconstructCSPos(ndcPos, depth);

	return P;
}

// Read the camera-space position of the point at screen-space pixel ssP + unitOffset * ssR.  Assumes length(unitOffset) == 1
float3 getOffsetPosition(int2 ssC, float2 unitOffset, float ssR, float2 resolution) 
{
	// Derivation:
	int mipLevel = clamp((int)floor(log2(ssR)) - LOG_MAX_OFFSET, 0, MAX_MIP_LEVEL);

	int2 ssP = int2(ssR * unitOffset) + ssC;

	// Divide coordinate by 2^mipLevel
	float depth = _depth.Load(int3(ssP >> mipLevel, mipLevel)).r;

	float2 uv = (ssP + float2(0.5, 0.5)) / resolution;
	float2 ndcPos = (uv - 0.5f) * 2.0f;

	float3 P;
	P = ndcReconstructCSPos(ndcPos, depth);

	return P;
}

// Compute the occlusion due to sample with index \a i about the pixel at \a ssC that corresponds to camera-space point \a C with unit normal \a n_C, using maximum screen-space sampling radius \a ssDiskRadius
float sampleAO(in int2 ssC, in float3 C, in float3 n_C, in float ssDiskRadius, in int tapIndex, in float randomPatternRotationAngle, float2 resolution)
{
	// Offset on the unit disk, spun for this pixel
	float ssR;
	float2 unitOffset = tapLocation(tapIndex, randomPatternRotationAngle, ssR);
	ssR *= ssDiskRadius;

	// The occluding point in camera space
	float3 Q = getOffsetPosition(ssC, unitOffset, ssR, resolution);

	float3 v = Q - C;

	float vv = dot(v, v);
	float vn = dot(v, n_C);

	const float epsilon = 0.01;
	const float radius2 = _constants.radius * _constants.radius;

	float f = max(radius2 - vv, 0.0); 
	return f * f * f * max((vn - _constants.bias) / (epsilon + vv), 0.0);
}

float nrand(uint2 uv)
{
	float2 K1 = float2(
		23.14069263277926, // e^pi (Gelfond's constant)
		2.665144142690225 // 2^sqrt(2) (Gelfond–Schneider constant)
	);
	return frac(cos(dot(uv, K1)) * 12345.6789);
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

	int2 ssC = pixelPos;
	int2 ssX = ssC + int2(1, 0);
	int2 ssY = ssC + int2(0, 1);

	// World space point being shaded
	float3 C = getPosition(ssC, dimensions);
	float3 CX = getPosition(ssX, dimensions);
	float3 CY = getPosition(ssY, dimensions);

	float2 bilateralKey;
	packKey(CSZToKey(C.z), bilateralKey);

	// Hash function used in the HPG12 AlchemyAO paper
	float randomPatternRotationAngle = nrand(ssC);//(3 * ssC.x ^ ssC.y + ssC.x * ssC.y) * 10;

	// Load the normals of the pixel
	//float2 octNormal = _normals[ssC].xy;
	//float3 n_C = OctNormalDecode(octNormal);
	float3 n_C = normalize(cross(CX - C, CY - C));

	// Choose the screen-space sample radius
	// proportional to the projected area of the sphere
	float ssDiskRadius = -_constants.projScale * _constants.radius / C.z;

	float sum = 0.0;
	for (int i = 0; i < NUM_SAMPLES; ++i) 
	{
		sum += sampleAO(ssC, C, n_C, ssDiskRadius, i, randomPatternRotationAngle, dimensions);
	}

	const float radius2 = _constants.radius * _constants.radius;

	float temp = radius2 * _constants.radius;
	sum /= temp * temp;
	float A = max(0.0, 1.0 - sum * _constants.intensity * (5.0 / NUM_SAMPLES));

	// Bilateral box-filter over a quad for free, respecting depth edges
	// (the difference that this makes is subtle)
	// Disabled since we don't have ddx/ddy support in compute shaders
	/*if (abs(ddx(C.z)) < 0.02) 
	{
		A -= ddx(A) * ((ssC.x & 1) - 0.5);
	}
	if (abs(ddy(C.z)) < 0.02) 
	{
		A -= ddy(A) * ((ssC.y & 1) - 0.5);
	}*/

	float4 color = float4(0, 0, 0, 0);
	color.r = A;
	color.gb = bilateralKey;

	_destination[pixelPos] = color;
}