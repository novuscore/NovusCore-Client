#include "globalData.inc.hlsl"
#include "terrain.inc.hlsl"

#define USE_PACKED_HEIGHT_RANGE 1

struct Constants
{
	float4 frustumPlanes[6];
};

[[vk::binding(0, PER_PASS)]] ByteAddressBuffer _instances;
[[vk::binding(1, PER_PASS)]] ByteAddressBuffer _heightRanges;
[[vk::binding(2, PER_PASS)]] RWByteAddressBuffer _culledInstances;
[[vk::binding(3, PER_PASS)]] RWByteAddressBuffer _argumentBuffer;
[[vk::binding(4, PER_PASS)]] ConstantBuffer<Constants> _constants;

float2 ReadHeightRange(uint instanceIndex)
{
#if USE_PACKED_HEIGHT_RANGE
	const uint packed = _heightRanges.Load(instanceIndex * 4);
	const float min = f16tof32(packed >> 16);
	const float max = f16tof32(packed);
	return float2(min, max);
#else
	const float2 minmax = asfloat(_heightRanges.Load2(instanceIndex * 8));
	return minmax;
#endif
}

bool IsAABBInsideFrustum(float4 frustum[6], AABB aabb)
{
    [unroll]
    for (int i = 0; i < 6; ++i)
    {
        const float4 plane = frustum[i];

        float3 vmin, vmax;

        // X axis 
        if (plane.x > 0) 
        {
            vmin.x = aabb.min.x;
            vmax.x = aabb.max.x;
        }
        else 
        {
            vmin.x = aabb.max.x;
            vmax.x = aabb.min.x;
        }

        // Y axis 
        if (plane.y > 0) 
        {
            vmin.y = aabb.min.y;
            vmax.y = aabb.max.y;
        }
        else 
        {
            vmin.y = aabb.max.y;
            vmax.y = aabb.min.y;
        }

        // Z axis 
        if (plane.z > 0)
        {
            vmin.z = aabb.min.z;
            vmax.z = aabb.max.z;
        }
        else
        {
            vmin.z = aabb.max.z;
            vmax.z = aabb.min.z;
        }

        if (dot(plane.xyz, vmin) + plane.w <= 0)
        {
            return false;
        }
    }

	return true;
}

[numthreads(32, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	// todo: initialize the draw arguments before dispatch
	if (dispatchThreadId.x == 0)
	{
		// indexCount, instanceCount, indexOffset, vertexOffset
		_argumentBuffer.Store4(0, uint4(NUM_INDICES_PER_CELL, 0, 0, 0));
		// instanceOffset
		_argumentBuffer.Store(16, 0);
	}

	const uint instanceIndex = dispatchThreadId.x;
	CellInstance instance = _instances.Load<CellInstance>(instanceIndex * 8);

	const uint cellID = instance.packedChunkCellID & 0xffff;
	const uint chunkID = instance.packedChunkCellID >> 16;

	const float2 heightRange = ReadHeightRange(instanceIndex);
	const AABB aabb = GetCellAABB(chunkID, cellID, heightRange);

    if (!IsAABBInsideFrustum(_constants.frustumPlanes, aabb))
    {
        return;
    }

	uint outInstanceIndex;
	_argumentBuffer.InterlockedAdd(4, 1, outInstanceIndex);

	_culledInstances.Store<CellInstance>(outInstanceIndex * 8, instance);
}
