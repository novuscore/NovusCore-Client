#include "globalData.inc.hlsl"
#include "terrain.inc.hlsl"
#include "debug.inc.hlsl"

#include "pyramidCulling.inc.hlsl"

#define USE_PACKED_HEIGHT_RANGE 1

struct Constants
{
    float4 frustumPlanes[6];
    float4x4 viewmat;
    uint occlusionCull;
};

[[vk::push_constant]] Constants _constants;
[[vk::binding(0, PER_PASS)]] StructuredBuffer<CellInstance> _instances;
[[vk::binding(1, PER_PASS)]] StructuredBuffer<uint> _heightRanges;
[[vk::binding(2, PER_PASS)]] RWStructuredBuffer<CellInstance> _culledInstances;
[[vk::binding(3, PER_PASS)]] RWByteAddressBuffer _drawCount;

[[vk::binding(4, PER_PASS)]] SamplerState _depthSampler; 
[[vk::binding(5, PER_PASS)]] Texture2D<float> _depthPyramid;

float2 ReadHeightRange(uint instanceIndex)
{
#if USE_PACKED_HEIGHT_RANGE
	const uint packed = _heightRanges[instanceIndex];
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

        float3 vmin;

        // X axis 
        if (plane.x > 0)
        {
            vmin.x = aabb.min.x;
        }
        else 
        {
            vmin.x = aabb.max.x;
        }

        // Y axis 
        if (plane.y > 0)
        {
            vmin.y = aabb.min.y;
        }
        else 
        {
            vmin.y = aabb.max.y;
        }

        // Z axis 
        if (plane.z > 0)
        {
            vmin.z = aabb.min.z;
        }
        else
        {
            vmin.z = aabb.max.z;
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
	const uint instanceIndex = dispatchThreadId.x;
	CellInstance instance = _instances[instanceIndex];

    const uint cellID = instance.packedChunkCellID & 0xffff;
    const uint chunkID = instance.packedChunkCellID >> 16;

    const float2 heightRange = ReadHeightRange(instanceIndex);
    AABB aabb = GetCellAABB(chunkID, cellID, heightRange);
    
    if (!IsAABBInsideFrustum(_constants.frustumPlanes, aabb))
    {
        return; 
    }
    if (_constants.occlusionCull) 
    {
        if (!IsVisible(aabb.min, aabb.max,_viewData.eye, _depthPyramid, _depthSampler, _viewData.lastViewProjectionMatrix))
        {
            return;
        }
    }

    //if (distance(_viewData.eye, (aabb.min + aabb.max) * 0.5) < 500)
    //{
    //    debugDrawAABB3D(aabb.min, aabb.max, 0xff0000ff);
    //}

    uint outInstanceIndex;
    _drawCount.InterlockedAdd(4, 1, outInstanceIndex);
      
    _culledInstances[outInstanceIndex] = instance;
}
