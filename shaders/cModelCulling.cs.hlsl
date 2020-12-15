#include "globalData.inc.hlsl"
#include "cModel.inc.hlsl"

struct Constants
{
	float4 frustumPlanes[6];
    float3 cameraPosition;   
    uint maxDrawCount;
    bool shouldPrepareSort;
};

struct DrawCall
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

struct PackedCullingData
{
    uint data0; // half minBoundingBox.x, half minBoundingBox.y, 
    uint data1; // half minBoundingBox.z, half maxBoundingBox.x,  
    uint data2; // half maxBoundingBox.y, half maxBoundingBox.z, 
    float sphereRadius;
}; // 16 bytes

struct AABB
{
    float3 min;
    float3 max;
};

struct CullingData
{
    AABB boundingBox;
    float sphereRadius;
};

struct Instance
{
    float4x4 instanceMatrix;
};

// Inputs
[[vk::push_constant]] Constants _constants;
[[vk::binding(1, PER_PASS)]] StructuredBuffer<DrawCall> _drawCalls;
[[vk::binding(2, PER_PASS)]] StructuredBuffer<Instance> _instances;
[[vk::binding(3, PER_PASS)]] StructuredBuffer<PackedCullingData> _cullingDatas;

// Outputs
[[vk::binding(4, PER_PASS)]] RWByteAddressBuffer _drawCount;
[[vk::binding(5, PER_PASS)]] RWStructuredBuffer<DrawCall> _culledDrawCalls;
[[vk::binding(6, PER_PASS)]] RWStructuredBuffer<uint64_t> _sortKeys; // OPTIONAL, only needed if _constants.shouldPrepareSort
[[vk::binding(7, PER_PASS)]] RWStructuredBuffer<uint> _sortValues; // OPTIONAL, only needed if _constants.shouldPrepareSort

CullingData LoadCullingData(uint instanceIndex)
{
    PackedCullingData packed = _cullingDatas[instanceIndex];
    CullingData cullingData;

    cullingData.boundingBox.min.x = f16tof32(packed.data0);
    cullingData.boundingBox.min.y = f16tof32(packed.data0 >> 16);
    cullingData.boundingBox.min.z = f16tof32(packed.data1);
    
    cullingData.boundingBox.max.x = f16tof32(packed.data1 >> 16);
    cullingData.boundingBox.max.y = f16tof32(packed.data2);
    cullingData.boundingBox.max.z = f16tof32(packed.data2 >> 16);
    
    cullingData.sphereRadius = packed.sphereRadius;
    
    return cullingData;
}

bool IsAABBInsideFrustum(float4 frustum[6], AABB aabb)
{
    [unroll]
    for (int i = 0; i < 6; ++i)
    {
        const float4 plane = frustum[i];

        float3 p;

        // X axis 
        if (plane.x > 0) 
        {
            p.x = aabb.max.x;
        }
        else 
        {
            p.x = aabb.min.x;
        }

        // Y axis 
        if (plane.y > 0) 
        {
            p.y = aabb.max.y;
        }
        else 
        {
            p.y = aabb.min.y;
        }

        // Z axis 
        if (plane.z > 0)
        {
            p.z = aabb.max.z;
        }
        else
        {
            p.z = aabb.min.z;
        }
        
        if (dot(plane.xyz, p) + plane.w <= 0)
        {
            return false;
        }
    }

	return true;
}

#define UINT_MAX 0xFFFFu
uint64_t CalculateSortKey(DrawCall drawCall, DrawCallData drawCallData, Instance instance)
{
    // Get the position to sort against
    const float3 refPos = _constants.cameraPosition;
    const float3 position = float3(instance.instanceMatrix._41, instance.instanceMatrix._42, instance.instanceMatrix._43);
    const float distanceFromCamera = distance(refPos, position);
    const float distanceAccuracy = 0.01f;
    
    // We want to construct a 64 bit sorting key, it will look like this but we can't make a union:
    /*
        struct SortingKey
        {
            uint8_t renderPriority : 8;
            
            uint8_t padding : 8; // Use this if we need extra precision on something
    
            uint32_t invDistanceFromCamera : 32; // This is converted to a fixed decimal value based on distance from camera, since the bit format of floats would mess with our comparison
            uint16_t localInstanceID : 16; // This makes the sorting stable if the distance is the same (submeshes inside a mesh)
        };
    */
    
    uint64_t renderPriority = drawCallData.renderPriority;
    uint64_t invDistanceFromCameraUint = UINT_MAX - (uint)(distanceFromCamera / distanceAccuracy);
    uint64_t localInstanceID = drawCall.firstInstance % 65535;
    
    uint64_t sortKey = 0;
    
    sortKey |= renderPriority << 56;
    // Padding here
    sortKey |= invDistanceFromCameraUint << 16;
    sortKey |= localInstanceID;
    
    return sortKey;
}

[numthreads(32, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= _constants.maxDrawCount)
    {
        return;
    }
    
    // Load DrawCall
    const uint drawCallIndex = dispatchThreadId.x;
    
    DrawCall drawCall = _drawCalls[drawCallIndex];
    
    uint drawCallID = drawCall.firstInstance;
    DrawCallData drawCallData = LoadDrawCallData(drawCallID);
    
    const CullingData cullingData = LoadCullingData(drawCallData.cullingDataID);
    const Instance instance = _instances[drawCallData.instanceID];
    
    // Get center and extents
    float3 center = (cullingData.boundingBox.min + cullingData.boundingBox.max) * 0.5f;
    float3 extents = cullingData.boundingBox.max - center;
    
    // Transform center
    const float4x4 m = instance.instanceMatrix;
    float3 transformedCenter = mul(float4(center, 1.0f), m).xyz;
    
    // Transform extents (take maximum)
    const float3x3 absMatrix = float3x3(abs(m[0].xyz), abs(m[1].xyz), abs(m[2].xyz));
    float3 transformedExtents =  mul(extents, absMatrix);
    
    // Transform to min/max AABB representation
    AABB aabb;
    aabb.min = transformedCenter - transformedExtents;
    aabb.max = transformedCenter + transformedExtents;
    
    // Cull the AABB
    if (!IsAABBInsideFrustum(_constants.frustumPlanes, aabb))
    {
        return;
    }
    
    // Store DrawCall
    uint outIndex;
	_drawCount.InterlockedAdd(0, 1, outIndex);
    _culledDrawCalls[outIndex] = drawCall;

    // If we expect to sort afterwards, output the data needed for it
    if (_constants.shouldPrepareSort)
    {
        _sortKeys[outIndex] = CalculateSortKey(drawCall, drawCallData, instance);
        _sortValues[outIndex] = outIndex;
    }
}