#include "globalData.inc.hlsl"
#include "pyramidCulling.inc.hlsl"
#include "mapObject.inc.hlsl"

struct Constants
{
	float4 frustumPlanes[6];
    float3 cameraPosition;
    uint maxDrawCount;
    uint occlusionCull;
};

struct DrawCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

struct PackedCullingData
{
    uint packed0; // half minBoundingBox.x, half minBoundingBox.y, 
    uint packed1; // half minBoundingBox.z, half maxBoundingBox.x,  
    uint packed2; // half maxBoundingBox.y, half maxBoundingBox.z, 
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

struct InstanceData
{
    float4x4 instanceMatrix;
};

[[vk::binding(1, PER_PASS)]] StructuredBuffer<DrawCommand> _drawCommands;
[[vk::binding(2, PER_PASS)]] RWStructuredBuffer<DrawCommand> _culledDrawCommands;
[[vk::binding(3, PER_PASS)]] RWByteAddressBuffer _drawCount;
[[vk::binding(4, PER_PASS)]] RWByteAddressBuffer _triangleCount;

[[vk::binding(5, PER_PASS)]] StructuredBuffer<PackedCullingData> _packedCullingData;
[[vk::binding(6, PER_PASS)]] StructuredBuffer<InstanceData> _instanceData;

[[vk::binding(7, PER_PASS)]] ConstantBuffer<Constants> _constants;


[[vk::binding(8, PER_PASS)]] SamplerState _depthSampler;
[[vk::binding(9, PER_PASS)]] Texture2D<float> _depthPyramid;

CullingData LoadCullingData(uint instanceIndex)
{
    const PackedCullingData packed = _packedCullingData[instanceIndex];
    CullingData cullingData;

    cullingData.boundingBox.min.x = f16tof32(packed.packed0 & 0xffff);
    cullingData.boundingBox.min.y = f16tof32((packed.packed0 >> 16) & 0xffff);
    cullingData.boundingBox.min.z = f16tof32(packed.packed1 & 0xffff);
    
    cullingData.boundingBox.max.x = f16tof32((packed.packed1 >> 16) & 0xffff);
    cullingData.boundingBox.max.y = f16tof32(packed.packed2 & 0xffff);
    cullingData.boundingBox.max.z = f16tof32((packed.packed2 >> 16) & 0xffff);
    
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

[numthreads(32, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= _constants.maxDrawCount)
    {
        return;   
    }
    
    const uint drawCommandIndex = dispatchThreadId.x;
    
    DrawCommand command = _drawCommands[drawCommandIndex];
    uint instanceID = command.firstInstance;
    
    const InstanceLookupData lookupData = LoadInstanceLookupData(instanceID);
    
    const CullingData cullingData = LoadCullingData(lookupData.cullingDataID);
    const InstanceData instanceData = _instanceData[lookupData.instanceID];
    
    // Get center and extents
    float3 center = (cullingData.boundingBox.min + cullingData.boundingBox.max) * 0.5f;
    float3 extents = cullingData.boundingBox.max - center;
    
    // Transform center
    const float4x4 m = instanceData.instanceMatrix;
    float3 transformedCenter = mul(float4(center, 1.0f), m).xyz;
    
    // Transform extents (take maximum)
    const float3x3 absMatrix = float3x3(abs(m[0].xyz), abs(m[1].xyz), abs(m[2].xyz));
    float3 transformedExtents = mul(extents, absMatrix);
    
    // Transform to min/max AABB representation
    AABB aabb;
    aabb.min = transformedCenter - transformedExtents;
    aabb.max = transformedCenter + transformedExtents;
    
    // Cull the AABB
    if (!IsAABBInsideFrustum(_constants.frustumPlanes, aabb))
    {
        return;
    } 
    if (_constants.occlusionCull)
    { 
        if (!IsVisible(aabb.min, aabb.max, _viewData.eye, _depthPyramid, _depthSampler, _viewData.lastViewProjectionMatrix))
        { 
            return;
        }
    }

    uint outTriangles;
    _triangleCount.InterlockedAdd(0, command.indexCount/3, outTriangles);
    
    uint outIndex;
	_drawCount.InterlockedAdd(0, 1, outIndex);
    
	_culledDrawCommands[outIndex] = command;
}
