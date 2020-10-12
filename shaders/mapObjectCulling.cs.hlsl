#include "globalData.inc.hlsl"

struct Constants
{
	float4 frustumPlanes[6];
    float3 cameraPosition;   
    uint maxDrawCount;
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

struct InstanceLookupData
{
    uint16_t instanceID;
    uint16_t materialParamID;
    uint16_t cullingDataID;
    uint16_t vertexColorTextureID0;
    uint16_t vertexColorTextureID1;
    uint16_t padding1;
    uint vertexOffset;
};

struct InstanceData
{
    float4x4 instanceMatrix;
};

[[vk::binding(0, PER_PASS)]] ByteAddressBuffer _argumentBuffer;
[[vk::binding(1, PER_PASS)]] RWByteAddressBuffer _culledArgumentBuffer;
[[vk::binding(2, PER_PASS)]] RWByteAddressBuffer _drawCountBuffer;

[[vk::binding(3, PER_PASS)]] ByteAddressBuffer _cullingData;
[[vk::binding(4, PER_PASS)]] ByteAddressBuffer _instanceLookup;
[[vk::binding(5, PER_PASS)]] ByteAddressBuffer _instanceData;

[[vk::binding(6, PER_PASS)]] ConstantBuffer<Constants> _constants;

CullingData LoadCullingData(uint instanceIndex)
{
    const PackedCullingData packed = _cullingData.Load<PackedCullingData>(instanceIndex * 16); // 16 = sizeof(PackedCullingData)
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

InstanceData LoadInstanceData(uint instanceID)
{
    InstanceData instanceData;

    instanceData = _instanceData.Load<InstanceData>(instanceID * 64); // 64 = sizeof(InstanceData)

    return instanceData;
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
    
    DrawCommand command = _argumentBuffer.Load<DrawCommand>(drawCommandIndex * 20); // 20 = sizeof(DrawCommand)
    uint instanceID = command.firstInstance;
    
    const InstanceLookupData lookupData = _instanceLookup.Load<InstanceLookupData>(instanceID * 16); // 16 = sizeof(InstanceLookupData)
    
    const CullingData cullingData = LoadCullingData(lookupData.cullingDataID);
    const InstanceData instanceData = LoadInstanceData(lookupData.instanceID);
    
    // Get center and extents
    float3 center = (cullingData.boundingBox.min + cullingData.boundingBox.max) * 0.5f;
    float3 extents = cullingData.boundingBox.max - center;
    
    // Transform center
    const float4x4 m = instanceData.instanceMatrix;
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
    
    uint outIndex;
	_drawCountBuffer.InterlockedAdd(0, 1, outIndex);
    
	_culledArgumentBuffer.Store<DrawCommand>(outIndex * 20, command); // 20 = sizeof(DrawCommand)
}
