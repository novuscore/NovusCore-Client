/**
 \file SSAO_minify.pix
 \author Morgan McGuire and Michael Mara, NVIDIA Research
  DX11 HLSL port by Leonardo Zide, Tryearch
  VK HLSL Compute Shader port by Pursche, NovusCore

  Open Source under the "BSD" license: http://www.opensource.org/licenses/bsd-license.php

  Copyright (c) 2011-2012, NVIDIA
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  */

[[vk::binding(0, PER_PASS)]] Texture2D<float> _source;
[[vk::binding(1, PER_PASS)]] RWTexture2D<float> _destination;

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

	int2 ssP = pixelPos;

	// Rotated grid subsampling to avoid XY directional bias or Z precision bias while downsampling
	_destination[pixelPos] = _source.Load(int3(ssP * 2 + int2((ssP.y & 1) ^ 1, (ssP.x & 1) ^ 1), 0));
}