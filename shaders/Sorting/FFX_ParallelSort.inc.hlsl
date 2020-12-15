// FFX_ParallelSort.h
//
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

#define FFX_PARALLELSORT_SORT_BITS_PER_PASS		4
#define	FFX_PARALLELSORT_SORT_BIN_COUNT			(1 << FFX_PARALLELSORT_SORT_BITS_PER_PASS)
#define FFX_PARALLELSORT_ELEMENTS_PER_THREAD	4
#define FFX_PARALLELSORT_THREADGROUP_SIZE		128

//////////////////////////////////////////////////////////////////////////
// ParallelSort constant buffer parameters:
//
//	NumKeys								The number of keys to sort
//	Shift								How many bits to shift for this sort pass (we sort 4 bits at a time)
//	NumBlocksPerThreadGroup				How many blocks of keys each thread group needs to process
//	NumThreadGroups						How many thread groups are being run concurrently for sort
//	NumThreadGroupsWithAdditionalBlocks	How many thread groups need to process additional block data
//	NumReduceThreadgroupPerBin			How many thread groups are summed together for each reduced bin entry
//	NumScanValues						How many values to perform scan prefix (+ add) on
//////////////////////////////////////////////////////////////////////////

struct FFX_ParallelSortCB
{
	uint NumKeys;
	int  NumBlocksPerThreadGroup;
	uint NumThreadGroups;
	uint NumThreadGroupsWithAdditionalBlocks;
	uint NumReduceThreadgroupPerBin;
	uint NumScanValues;
};

groupshared uint gs_Histogram[FFX_PARALLELSORT_THREADGROUP_SIZE * FFX_PARALLELSORT_SORT_BIN_COUNT];
void FFX_ParallelSort_Count_uint(uint localID, uint groupID, FFX_ParallelSortCB CBuffer, uint ShiftBit, RWStructuredBuffer<uint64_t> SrcBuffer, RWStructuredBuffer<uint64_t> SumTable)
{
	// Start by clearing our local counts in LDS
	for (int i = 0; i < FFX_PARALLELSORT_SORT_BIN_COUNT; i++)
		gs_Histogram[(i * FFX_PARALLELSORT_THREADGROUP_SIZE) + localID] = 0;

	// Wait for everyone to catch up
	GroupMemoryBarrierWithGroupSync();

	// Data is processed in blocks, and how many we process can changed based on how much data we are processing
	// versus how many thread groups we are processing with
	int BlockSize = FFX_PARALLELSORT_ELEMENTS_PER_THREAD * FFX_PARALLELSORT_THREADGROUP_SIZE;

	// Figure out this thread group's index into the block data (taking into account thread groups that need to do extra reads)
	uint ThreadgroupBlockStart = (BlockSize * CBuffer.NumBlocksPerThreadGroup * groupID);
	uint NumBlocksToProcess = CBuffer.NumBlocksPerThreadGroup;

	if (groupID >= CBuffer.NumThreadGroups - CBuffer.NumThreadGroupsWithAdditionalBlocks)
	{
		ThreadgroupBlockStart += (groupID - (CBuffer.NumThreadGroups - CBuffer.NumThreadGroupsWithAdditionalBlocks)) * BlockSize;
		NumBlocksToProcess++;
	}

	// Get the block start index for this thread
	uint BlockIndex = ThreadgroupBlockStart + localID;

	// Count value occurrence
	for (uint BlockCount = 0; BlockCount < NumBlocksToProcess; BlockCount++, BlockIndex += BlockSize)
	{
		uint DataIndex = BlockIndex;

		// Pre-load the key values in order to hide some of the read latency
		uint64_t srcKeys[FFX_PARALLELSORT_ELEMENTS_PER_THREAD];
		srcKeys[0] = SrcBuffer[DataIndex];
		srcKeys[1] = SrcBuffer[DataIndex + FFX_PARALLELSORT_THREADGROUP_SIZE];
		srcKeys[2] = SrcBuffer[DataIndex + (FFX_PARALLELSORT_THREADGROUP_SIZE * 2)];
		srcKeys[3] = SrcBuffer[DataIndex + (FFX_PARALLELSORT_THREADGROUP_SIZE * 3)];

		for (uint i = 0; i < FFX_PARALLELSORT_ELEMENTS_PER_THREAD; i++)
		{
			if (DataIndex < CBuffer.NumKeys)
			{
				uint64_t localKey = (srcKeys[i] >> ShiftBit) & 0xf;
				InterlockedAdd(gs_Histogram[(localKey * FFX_PARALLELSORT_THREADGROUP_SIZE) + localID], 1l);
				DataIndex += FFX_PARALLELSORT_THREADGROUP_SIZE;
			}
		}
	}

	// Even though our LDS layout guarantees no collisions, our thread group size is greater than a wave
	// so we need to make sure all thread groups are done counting before we start tallying up the results
	GroupMemoryBarrierWithGroupSync();

	if (localID < FFX_PARALLELSORT_SORT_BIN_COUNT)
	{
		uint64_t sum = 0;
		for (int i = 0; i < FFX_PARALLELSORT_THREADGROUP_SIZE; i++)
		{
			sum += gs_Histogram[localID * FFX_PARALLELSORT_THREADGROUP_SIZE + i];
		}
		SumTable[localID * CBuffer.NumThreadGroups + groupID] = sum;
	}
}

groupshared uint64_t gs_LDSSums[FFX_PARALLELSORT_THREADGROUP_SIZE];
uint64_t FFX_ParallelSort_ThreadgroupReduce(uint64_t localSum, uint localID)
{
	// Do wave local reduce
	uint64_t waveReduced = WaveActiveSum(localSum);

	// First lane in a wave writes out wave reduction to LDS (this accounts for num waves per group greater than HW wave size)
	// Note that some hardware with very small HW wave sizes (i.e. <= 8) may exhibit issues with this algorithm, and have not been tested.
	uint waveID = localID / WaveGetLaneCount();
	if (WaveIsFirstLane())
		gs_LDSSums[waveID] = waveReduced;

	// Wait for everyone to catch up
	GroupMemoryBarrierWithGroupSync();

	// First wave worth of threads sum up wave reductions
	if (!waveID)
		waveReduced = WaveActiveSum( (localID < FFX_PARALLELSORT_THREADGROUP_SIZE / WaveGetLaneCount()) ? gs_LDSSums[localID] : 0);

	// Returned the reduced sum
	return waveReduced;
}

uint64_t FFX_ParallelSort_BlockScanPrefix(uint64_t localSum, uint localID)
{
	// Do wave local scan-prefix
	uint64_t wavePrefixed = WavePrefixSum(localSum);

	// Since we are dealing with thread group sizes greater than HW wave size, we need to account for what wave we are in.
	uint waveID = localID / WaveGetLaneCount();
	uint laneID = WaveGetLaneIndex();

	// Last element in a wave writes out partial sum to LDS
	if (laneID == WaveGetLaneCount() - 1)
		gs_LDSSums[waveID] = wavePrefixed + localSum;

	// Wait for everyone to catch up
	GroupMemoryBarrierWithGroupSync();

	// First wave prefixes partial sums
	if (!waveID)
		gs_LDSSums[localID] = WavePrefixSum(gs_LDSSums[localID]);

	// Wait for everyone to catch up
	GroupMemoryBarrierWithGroupSync();

	// Add the partial sums back to each wave prefix
	wavePrefixed += gs_LDSSums[waveID];

	return wavePrefixed;
}

void FFX_ParallelSort_ReduceCount(uint localID, uint groupID, FFX_ParallelSortCB CBuffer, RWStructuredBuffer<uint64_t> SumTable, RWStructuredBuffer<uint64_t> ReduceTable)
{
	// Figure out what bin data we are reducing
	uint BinID = groupID / CBuffer.NumReduceThreadgroupPerBin;
	uint BinOffset = BinID * CBuffer.NumThreadGroups;

	// Get the base index for this thread group
	uint BaseIndex = (groupID % CBuffer.NumReduceThreadgroupPerBin) * FFX_PARALLELSORT_ELEMENTS_PER_THREAD * FFX_PARALLELSORT_THREADGROUP_SIZE;

	// Calculate partial sums for entries this thread reads in
	uint64_t threadgroupSum = 0;
	for (uint i = 0; i < FFX_PARALLELSORT_ELEMENTS_PER_THREAD; ++i)
	{
		uint DataIndex = BaseIndex + (i * FFX_PARALLELSORT_THREADGROUP_SIZE) + localID;
		threadgroupSum += (DataIndex < CBuffer.NumThreadGroups) ? SumTable[BinOffset + DataIndex] : 0;
	}

	// Reduce across the entirety of the thread group
	threadgroupSum = FFX_ParallelSort_ThreadgroupReduce(threadgroupSum, localID);

	// First thread of the group writes out the reduced sum for the bin
	if (!localID)
		ReduceTable[groupID] = threadgroupSum;

	// What this will look like in the reduced table is:
	//	[ [bin0 ... bin0] [bin1 ... bin1] ... ]
}

// This is to transform uncoalesced loads into coalesced loads and 
// then scattered loads from LDS
groupshared uint64_t gs_LDS[FFX_PARALLELSORT_ELEMENTS_PER_THREAD][FFX_PARALLELSORT_THREADGROUP_SIZE];
void FFX_ParallelSort_ScanPrefix(uint numValuesToScan, uint localID, uint groupID, uint BinOffset, uint BaseIndex, bool AddPartialSums,
									FFX_ParallelSortCB CBuffer, RWStructuredBuffer<uint64_t> ScanSrc, RWStructuredBuffer<uint64_t> ScanDst, RWStructuredBuffer<uint64_t> ScanScratch)
{
	// Perform coalesced loads into LDS
	{
        for(uint i = 0; i < FFX_PARALLELSORT_ELEMENTS_PER_THREAD; i++)
        {
            uint DataIndex = BaseIndex + (i * FFX_PARALLELSORT_THREADGROUP_SIZE) + localID;

            uint col = ((i * FFX_PARALLELSORT_THREADGROUP_SIZE) + localID) / FFX_PARALLELSORT_ELEMENTS_PER_THREAD;
            uint row = ((i * FFX_PARALLELSORT_THREADGROUP_SIZE) + localID) % FFX_PARALLELSORT_ELEMENTS_PER_THREAD;
            gs_LDS[row][col] = (DataIndex < numValuesToScan) ? ScanSrc[BinOffset + DataIndex] : 0;
        }
    }

	// Wait for everyone to catch up
	GroupMemoryBarrierWithGroupSync();

	uint64_t threadgroupSum = 0;
	// Calculate the local scan-prefix for current thread
	{
        for(uint i = 0; i < FFX_PARALLELSORT_ELEMENTS_PER_THREAD; i++)
        {
            uint64_t tmp = gs_LDS[i][localID];
            gs_LDS[i][localID] = threadgroupSum;
            threadgroupSum += tmp;
        }
    }

	// Scan prefix partial sums
	threadgroupSum = FFX_ParallelSort_BlockScanPrefix(threadgroupSum, localID);

	// Add reduced partial sums if requested
	uint64_t partialSum = 0;
	if (AddPartialSums)
	{
		// Partial sum additions are a little special as they are tailored to the optimal number of 
		// thread groups we ran in the beginning, so need to take that into account
		partialSum = ScanScratch[groupID];
	}

	// Add the block scanned-prefixes back in
	{
        for(uint i = 0; i < FFX_PARALLELSORT_ELEMENTS_PER_THREAD; i++)
            gs_LDS[i][localID] += threadgroupSum;
    }

	// Wait for everyone to catch up
	GroupMemoryBarrierWithGroupSync();

	// Perform coalesced writes to scan dst
	{
        for(uint i = 0; i < FFX_PARALLELSORT_ELEMENTS_PER_THREAD; i++)
        {
            uint DataIndex = BaseIndex + (i * FFX_PARALLELSORT_THREADGROUP_SIZE) + localID;

            uint col = ((i * FFX_PARALLELSORT_THREADGROUP_SIZE) + localID) / FFX_PARALLELSORT_ELEMENTS_PER_THREAD;
            uint row = ((i * FFX_PARALLELSORT_THREADGROUP_SIZE) + localID) % FFX_PARALLELSORT_ELEMENTS_PER_THREAD;

            if(DataIndex < numValuesToScan)
                ScanDst[BinOffset + DataIndex] = gs_LDS[row][col] + partialSum;
        }
    }
}

// Offset cache to avoid loading the offsets all the time
groupshared uint64_t gs_BinOffsetCache[FFX_PARALLELSORT_THREADGROUP_SIZE];
// Local histogram for offset calculations
groupshared uint gs_LocalHistogram[FFX_PARALLELSORT_SORT_BIN_COUNT];
// Scratch area for algorithm
groupshared uint64_t gs_LDSScratch[FFX_PARALLELSORT_THREADGROUP_SIZE];
// LDS for uint sums
groupshared uint gs_LDSSums_uint[FFX_PARALLELSORT_THREADGROUP_SIZE];
void FFX_ParallelSort_Scatter_uint(uint localID, uint groupID, FFX_ParallelSortCB CBuffer, uint ShiftBit, RWStructuredBuffer<uint64_t> SrcBuffer, RWStructuredBuffer<uint64_t> DstBuffer, 
									RWStructuredBuffer<uint64_t> SumTable, RWStructuredBuffer<uint> SrcPayload, RWStructuredBuffer<uint> DstPayload)
{
	// Load the sort bin threadgroup offsets into LDS for faster referencing
	if (localID < FFX_PARALLELSORT_SORT_BIN_COUNT)
		gs_BinOffsetCache[localID] = SumTable[localID * CBuffer.NumThreadGroups + groupID];

	// Wait for everyone to catch up
	GroupMemoryBarrierWithGroupSync();

	// Data is processed in blocks, and how many we process can changed based on how much data we are processing
	// versus how many thread groups we are processing with
	int BlockSize = FFX_PARALLELSORT_ELEMENTS_PER_THREAD * FFX_PARALLELSORT_THREADGROUP_SIZE;

	// Figure out this thread group's index into the block data (taking into account thread groups that need to do extra reads)
	uint ThreadgroupBlockStart = (BlockSize * CBuffer.NumBlocksPerThreadGroup * groupID);
	uint NumBlocksToProcess = CBuffer.NumBlocksPerThreadGroup;

	if (groupID >= CBuffer.NumThreadGroups - CBuffer.NumThreadGroupsWithAdditionalBlocks)
	{
		ThreadgroupBlockStart += (groupID - (CBuffer.NumThreadGroups - CBuffer.NumThreadGroupsWithAdditionalBlocks)) * BlockSize;
		NumBlocksToProcess++;
	}

	// Get the block start index for this thread
	uint BlockIndex = ThreadgroupBlockStart + localID;

	// Count value occurences
	uint newCount;
	for (int BlockCount = 0; BlockCount < NumBlocksToProcess; BlockCount++, BlockIndex += BlockSize)
	{
		uint DataIndex = BlockIndex;
			
		// Pre-load the key values in order to hide some of the read latency
		uint64_t srcKeys[FFX_PARALLELSORT_ELEMENTS_PER_THREAD];
		srcKeys[0] = SrcBuffer[DataIndex];
		srcKeys[1] = SrcBuffer[DataIndex + FFX_PARALLELSORT_THREADGROUP_SIZE];
		srcKeys[2] = SrcBuffer[DataIndex + (FFX_PARALLELSORT_THREADGROUP_SIZE * 2)];
		srcKeys[3] = SrcBuffer[DataIndex + (FFX_PARALLELSORT_THREADGROUP_SIZE * 3)];

		uint srcValues[FFX_PARALLELSORT_ELEMENTS_PER_THREAD];
		srcValues[0] = SrcPayload[DataIndex];
		srcValues[1] = SrcPayload[DataIndex + FFX_PARALLELSORT_THREADGROUP_SIZE];
		srcValues[2] = SrcPayload[DataIndex + (FFX_PARALLELSORT_THREADGROUP_SIZE * 2)];
		srcValues[3] = SrcPayload[DataIndex + (FFX_PARALLELSORT_THREADGROUP_SIZE * 3)];

		for (int i = 0; i < FFX_PARALLELSORT_ELEMENTS_PER_THREAD; i++)
		{
			// Clear the local histogram
			if (localID < FFX_PARALLELSORT_SORT_BIN_COUNT)
				gs_LocalHistogram[localID] = 0;

			uint64_t localKey = (DataIndex < CBuffer.NumKeys ? srcKeys[i] : 0xffffffffffffffff);
			uint localValue = (DataIndex < CBuffer.NumKeys ? srcValues[i] : 0);

			// Sort the keys locally in LDS
			for (uint bitShift = 0; bitShift < FFX_PARALLELSORT_SORT_BITS_PER_PASS; bitShift += 2)
			{
				// Figure out the keyIndex
				uint64_t keyIndex = (localKey >> ShiftBit) & 0xf;
				uint64_t bitKey = (keyIndex >> bitShift) & 0x3;

				// Create a packed histogram 
				uint64_t packedHistogram = 1 << (bitKey * 8);

				// Sum up all the packed keys (generates counted offsets up to current thread group)
				uint64_t localSum = FFX_ParallelSort_BlockScanPrefix(packedHistogram, localID);

				// Last thread stores the updated histogram counts for the thread group
				// Scratch = 0xsum3|sum2|sum1|sum0 for thread group
				if (localID == (FFX_PARALLELSORT_THREADGROUP_SIZE - 1))
					gs_LDSScratch[0] = localSum + packedHistogram;

				// Wait for everyone to catch up
				GroupMemoryBarrierWithGroupSync();

				// Load the sums value for the thread group
				packedHistogram = gs_LDSScratch[0];

				// Add prefix offsets for all 4 bit "keys" (packedHistogram = 0xsum2_1_0|sum1_0|sum0|0)
				packedHistogram = (packedHistogram << 8) + (packedHistogram << 16) + (packedHistogram << 24);

				// Calculate the proper offset for this thread's value
				localSum += packedHistogram;

				// Calculate target offset
				uint64_t keyOffset = (localSum >> (bitKey * 8)) & 0xff;

				// Re-arrange the keys (store, sync, load)
				gs_LDSSums[keyOffset] = localKey;
				GroupMemoryBarrierWithGroupSync();
				localKey = gs_LDSSums[localID];

				// Wait for everyone to catch up
				GroupMemoryBarrierWithGroupSync();

				// Re-arrange the values if we have them (store, sync, load)
				gs_LDSSums_uint[keyOffset] = localValue;
				GroupMemoryBarrierWithGroupSync();
				localValue = gs_LDSSums_uint[localID];

				// Wait for everyone to catch up
				GroupMemoryBarrierWithGroupSync();
			}

			// Need to recalculate the keyIndex on this thread now that values have been copied around the thread group
			uint64_t keyIndex = (localKey >> ShiftBit) & 0xf;

			// Reconstruct histogram
			InterlockedAdd(gs_LocalHistogram[keyIndex], 1);

			// Wait for everyone to catch up
			GroupMemoryBarrierWithGroupSync();

			// Prefix histogram
			uint histogramPrefixSum = WavePrefixSum(localID < FFX_PARALLELSORT_SORT_BIN_COUNT ? gs_LocalHistogram[localID] : 0);

			// Broadcast prefix-sum via LDS
			if (localID < FFX_PARALLELSORT_SORT_BIN_COUNT)
				gs_LDSScratch[localID] = histogramPrefixSum;

			// Get the global offset for this key out of the cache
			uint64_t globalOffset = gs_BinOffsetCache[keyIndex];

			// Wait for everyone to catch up
			GroupMemoryBarrierWithGroupSync();

			// Get the local offset (at this point the keys are all in increasing order from 0 -> num bins in localID 0 -> thread group size)
			uint64_t localOffset = localID - gs_LDSScratch[keyIndex];

			// Write to destination
			uint totalOffset = uint(globalOffset + localOffset);

			if (totalOffset < CBuffer.NumKeys)
			{
				DstBuffer[totalOffset] = localKey;
				DstPayload[totalOffset] = localValue;
			}

			// Wait for everyone to catch up
			GroupMemoryBarrierWithGroupSync();

			// Update the cached histogram for the next set of entries
			if (localID < FFX_PARALLELSORT_SORT_BIN_COUNT)
				gs_BinOffsetCache[localID] += gs_LocalHistogram[localID];

			DataIndex += FFX_PARALLELSORT_THREADGROUP_SIZE;	// Increase the data offset by thread group size
		}
	}
}

void FFX_ParallelSort_SetupIndirectParams(uint NumKeys, uint MaxThreadGroups, RWStructuredBuffer<FFX_ParallelSortCB> CBuffer, RWStructuredBuffer<uint> CountScatterArgs, RWStructuredBuffer<uint> ReduceScanArgs)
{
	CBuffer[0].NumKeys = NumKeys;

	uint BlockSize = FFX_PARALLELSORT_ELEMENTS_PER_THREAD * FFX_PARALLELSORT_THREADGROUP_SIZE;
	uint NumBlocks = (NumKeys + BlockSize - 1) / BlockSize;

	// Figure out data distribution
	uint NumThreadGroupsToRun = MaxThreadGroups;
	uint BlocksPerThreadGroup = (NumBlocks / NumThreadGroupsToRun);
	CBuffer[0].NumThreadGroupsWithAdditionalBlocks = NumBlocks % NumThreadGroupsToRun;

	if (NumBlocks < NumThreadGroupsToRun)
	{
		BlocksPerThreadGroup = 1;
		NumThreadGroupsToRun = NumBlocks;
		CBuffer[0].NumThreadGroupsWithAdditionalBlocks = 0;
	}

	CBuffer[0].NumThreadGroups = NumThreadGroupsToRun;
	CBuffer[0].NumBlocksPerThreadGroup = BlocksPerThreadGroup;

	// Calculate the number of thread groups to run for reduction (each thread group can process BlockSize number of entries)
	uint NumReducedThreadGroupsToRun = FFX_PARALLELSORT_SORT_BIN_COUNT * ((BlockSize > NumThreadGroupsToRun) ? 1 : (NumThreadGroupsToRun + BlockSize - 1) / BlockSize);
	CBuffer[0].NumReduceThreadgroupPerBin = NumReducedThreadGroupsToRun / FFX_PARALLELSORT_SORT_BIN_COUNT;
	CBuffer[0].NumScanValues = NumReducedThreadGroupsToRun;	// The number of reduce thread groups becomes our scan count (as each thread group writes out 1 value that needs scan prefix)

	// Setup dispatch arguments
	CountScatterArgs[0] = NumThreadGroupsToRun;
	CountScatterArgs[1] = 1;
	CountScatterArgs[2] = 1;

	ReduceScanArgs[0] = NumReducedThreadGroupsToRun;
	ReduceScanArgs[1] = 1;
	ReduceScanArgs[2] = 1;
}
