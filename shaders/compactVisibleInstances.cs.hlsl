#include "common.inc.hlsl"

[[vk::binding(0, PER_DRAW)]] ByteAddressBuffer _visibleInstanceMask;
[[vk::binding(1, PER_DRAW)]] RWByteAddressBuffer _visibleInstanceCount;
[[vk::binding(2, PER_DRAW)]] RWByteAddressBuffer _visibleInstanceIDs;

[numthreads(32, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint groupThreadIndex : SV_GroupIndex)
{
	const uint groupIndex = groupID.x;

	const uint mask = _visibleInstanceMask.Load(groupIndex * SIZEOF_UINT);
	const bool visible = mask & (1 << groupThreadIndex);

	if (visible)
	{
		uint appendOffset;
		_visibleInstanceCount.InterlockedAdd(0, 1, appendOffset);

		const uint instanceID = (groupIndex * 32) + groupThreadIndex;
		_visibleInstanceIDs.Store(appendOffset * SIZEOF_UINT, instanceID);
	}

	/*const uint mask = _visibleInstanceMask.Load(groupIndex * SIZEOF_UINT);
	const bool visible = true;// mask & (1 << WaveGetLaneIndex());

	const uint laneOffset = WavePrefixCountBits(visible);
	const uint appendCount = WaveActiveCountBits(visible);

	uint appendOffset;
	if (WaveIsFirstLane())
	{
		_visibleInstanceCount.InterlockedAdd(0, appendCount, appendOffset);
	}

	appendOffset = WaveReadLaneFirst(appendOffset);

	if (visible)
	{
		const uint instanceID = (groupIndex * 32) + WaveGetLaneIndex();
		_visibleInstanceIDs.Store((appendOffset + laneOffset) * SIZEOF_UINT, instanceID);
	}*/
}