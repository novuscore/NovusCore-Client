// Parallel Bitonic sorting on the GPU, inspired by: http://www.valentinkraft.de/parallel-bitonic-sorting-on-the-gpu-compute-shader-implementation-in-unreal/
#include "cModel.inc.hlsl"

#define BITONIC_BLOCK_SIZE 1024

#define FLT_MAX 3.402823466e+38
#define FLT_EPSILON 0.00001

struct Constants
{
    float3 referencePosition;
    uint level;
    uint levelMask;
};

struct DrawCall
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

struct InstanceData
{
    float4x4 instanceMatrix;
};

[[vk::binding(1, PER_PASS)]] RWByteAddressBuffer _drawCalls;
[[vk::binding(2, PER_PASS)]] ByteAddressBuffer _instances;
[[vk::binding(3, PER_PASS)]] RWByteAddressBuffer _input;

[[vk::push_constant]] const Constants _constants;

DrawCall LoadDrawCall(uint drawCallID)
{
    DrawCall drawCall = _drawCalls.Load<DrawCall>(drawCallID * 20); // 20 = sizeof(DrawCall)
    
    return drawCall;
}

InstanceData LoadInstanceData(uint instanceID)
{
    InstanceData instanceData = _instances.Load<InstanceData>(instanceID * 64); // 64 = sizeof(InstanceData)

    return instanceData;
}

uint64_t CalculateSortKey(DrawCall drawCall)
{
    DrawCallData drawCallData = LoadDrawCallData(drawCall.firstInstance);
    InstanceData instance = LoadInstanceData(drawCallData.instanceID);
    
    // Get the position to sort against
    const float3 refPos = _constants.referencePosition;
    const float3 position = float3(instance.instanceMatrix._41, instance.instanceMatrix._42, instance.instanceMatrix._43);
    const float distanceFromCamera = distance(refPos, position);
    const float distanceAccuracy = 0.01f;
    
    // We want to construct a 64 bit sorting key, it will look like this but we can't make a union:
    /*
        struct SortingKey
        {
            uint8_t wasCulled : 1;
            uint8_t renderPriority : 8;
            
            uint8_t padding : 7; // Use this if we need extra precision on something
    
            uint32_t distanceFromCamera : 32; // This is converted to a fixed decimal value based on distance from camera, since the bit format of floats would mess with our comparison
            uint16_t localInstanceID : 16; // This makes the sorting stable if the distance is the same (submeshes inside a mesh)
        };
    */
    
    uint64_t wasNotCulled = drawCall.instanceCount != 0;
    uint64_t renderPriority = drawCallData.renderPriority;
    uint64_t distanceFromCameraUint = (uint)(distanceFromCamera / distanceAccuracy);
    uint64_t localInstanceID = drawCall.firstInstance % 65535;

    uint64_t key = 0;
    
    key |= wasNotCulled << 63;
    key |= renderPriority << 55;
    // Padding here
    key |= distanceFromCameraUint << 16;
    key |= localInstanceID;
    
    return key;
}

// Thread group shared memory limit (DX11): 32KB --> 2048 float4 values --> 32 thread groups optimum --> 1024 Threads optimum (?)
// Only shared within a thread group!
groupshared DrawCall _sharedDrawCalls[BITONIC_BLOCK_SIZE];
groupshared uint64_t _sharedSortKeys[BITONIC_BLOCK_SIZE];

struct CSInput
{
    uint3 groupID : SV_GroupID; // Unused?
    uint3 dispatchThreadID : SV_DispatchThreadID;
    uint3 groupThreadID : SV_GroupThreadID; // Unused?
    uint groupIndex : SV_GroupIndex;
};

[numthreads(BITONIC_BLOCK_SIZE, 1, 1)]
void main(CSInput input)
{
    const uint GI = input.groupIndex;
    const uint3 DTid = input.dispatchThreadID;
    
    // Load initial data
    const uint drawCallID = DTid.y * BITONIC_BLOCK_SIZE + DTid.x;
    
    DrawCall drawCall = LoadDrawCall(drawCallID);
    uint64_t sortKey = CalculateSortKey(drawCall);

    _sharedDrawCalls[GI] = drawCall;
    _sharedSortKeys[GI] = sortKey;
    GroupMemoryBarrierWithGroupSync();
  
    // Now each thread must pick the min or max of the two elements it is comparing. 
    // The thread cannot compare and swap both elements because that would require random access writes.
    for(uint j = _constants.level >> 1; j > 0; j >>= 1)
    {
        DrawCall drawCall1 = _sharedDrawCalls[GI & ~j];
        DrawCall drawCall2 = _sharedDrawCalls[GI | j];
        
        uint64_t key1 = _sharedSortKeys[GI & ~j];
        uint64_t key2 = _sharedSortKeys[GI | j];
        
        // Atomic compare operation
        uint64_t resultSortKey;
        DrawCall resultDrawCall;
        
        bool shouldXOR = ((key1 >= key2) == (bool) (_constants.levelMask & DTid.x));
        if (shouldXOR)
        {
            resultSortKey = _sharedSortKeys[GI ^ j];
            resultDrawCall = _sharedDrawCalls[GI ^ j];
        }
        else
        {
            resultSortKey = _sharedSortKeys[GI];
            resultDrawCall = _sharedDrawCalls[GI];
        }
        GroupMemoryBarrierWithGroupSync();
        
        // Store result
        _sharedSortKeys[GI] = resultSortKey;
        _sharedDrawCalls[GI] = resultDrawCall;
        GroupMemoryBarrierWithGroupSync();
    }  
    // Update buffers with sorted values
    uint id = DTid.y * BITONIC_BLOCK_SIZE + DTid.x;
    uint offset = id * 20;
    
    drawCall = _sharedDrawCalls[GI];
    
    _drawCalls.Store4(offset, uint4(drawCall.indexCount, drawCall.instanceCount, drawCall.firstIndex, drawCall.vertexOffset));
    _drawCalls.Store(offset+16, drawCall.firstInstance);
    
    _input.Store4(offset, uint4(drawCall.indexCount, drawCall.instanceCount, drawCall.firstIndex, drawCall.vertexOffset));
    _input.Store(offset+16, drawCall.firstInstance);
}