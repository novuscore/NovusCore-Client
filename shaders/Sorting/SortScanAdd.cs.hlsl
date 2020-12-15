// Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

//--------------------------------------------------------------------------------------
// ParallelSort Shaders/Includes
//--------------------------------------------------------------------------------------
#include "Sorting/FFX_ParallelSort.inc.hlsl"

[[vk::binding(0, PER_PASS)]] ConstantBuffer<FFX_ParallelSortCB> _constants;		// Constant Buffer
[[vk::binding(1, PER_PASS)]] RWStructuredBuffer<uint64_t> _scanSrc;				// Source for Scan Data
[[vk::binding(2, PER_PASS)]] RWStructuredBuffer<uint64_t> _scanDst;				// Destination for Scan Data
[[vk::binding(3, PER_PASS)]] RWStructuredBuffer<uint64_t> _scanScratch;			// Scratch data for Scan

[numthreads(FFX_PARALLELSORT_THREADGROUP_SIZE, 1, 1)]
void main(uint localID : SV_GroupThreadID, uint groupID : SV_GroupID)
{
	// When doing adds, we need to access data differently because reduce 
	// has a more specialized access pattern to match optimized count
	// Access needs to be done similarly to reduce
	// Figure out what bin data we are reducing
	uint binID = groupID / _constants.NumReduceThreadgroupPerBin;
	uint binOffset = binID * _constants.NumThreadGroups;

	// Get the base index for this thread group
	//uint BaseIndex = FFX_PARALLELSORT_ELEMENTS_PER_THREAD * FFX_PARALLELSORT_THREADGROUP_SIZE * (groupID / FFX_PARALLELSORT_SORT_BIN_COUNT);
	uint baseIndex = (groupID % _constants.NumReduceThreadgroupPerBin) * FFX_PARALLELSORT_ELEMENTS_PER_THREAD * FFX_PARALLELSORT_THREADGROUP_SIZE;

	FFX_ParallelSort_ScanPrefix(_constants.NumThreadGroups, localID, groupID, binOffset, baseIndex, true,
								_constants, _scanSrc, _scanDst, _scanScratch);
}