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

struct SetupCB
{
	uint maxThreadGroups;
};

struct NumKeys
{
	uint numKeys;
};

[[vk::push_constant]] SetupCB _setupCB;												// Setup Indirect Constant buffer

[[vk::binding(0, PER_PASS)]] StructuredBuffer<NumKeys> _numKeys;
[[vk::binding(1, PER_PASS)]] RWStructuredBuffer<FFX_ParallelSortCB> _constants;		// UAV for constant buffer parameters for indirect execution
[[vk::binding(2, PER_PASS)]] RWStructuredBuffer<uint> _countScatterArgs;			// Count and Scatter Args for indirect execution
[[vk::binding(3, PER_PASS)]] RWStructuredBuffer<uint> _reduceScanArgs;				// Reduce and Scan Args for indirect execution

[numthreads(1, 1, 1)]
void main(uint localID : SV_GroupThreadID)
{
	FFX_ParallelSort_SetupIndirectParams(_numKeys[0].numKeys, _setupCB.maxThreadGroups, _constants, _countScatterArgs, _reduceScanArgs);
}