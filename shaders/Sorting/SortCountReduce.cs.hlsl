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
[[vk::binding(1, PER_PASS)]] RWStructuredBuffer<uint64_t> _sumTable;			// The sum table we will write sums to
[[vk::binding(2, PER_PASS)]] RWStructuredBuffer<uint64_t> _reducedSumTable;		// The reduced sum table we will write sums to

[numthreads(FFX_PARALLELSORT_THREADGROUP_SIZE, 1, 1)]
void main(uint localID : SV_GroupThreadID, uint groupID : SV_GroupID)
{
	// Call the reduce part of the algorithm
	FFX_ParallelSort_ReduceCount(localID, groupID, _constants,  _sumTable, _reducedSumTable);
}
