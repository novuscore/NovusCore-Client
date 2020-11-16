// Parallel Bitonic sorting on the GPU, inspired by: http://www.valentinkraft.de/parallel-bitonic-sorting-on-the-gpu-compute-shader-implementation-in-unreal/

#define TRANSPOSE_BLOCK_SIZE 16

struct Constants
{
    float3 referencePosition;
    uint level;
    uint levelMask;
    uint width;
    uint height;
};

struct DrawCall
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

[[vk::binding(0, PER_PASS)]] RWByteAddressBuffer _drawCalls;
[[vk::binding(1, PER_PASS)]] RWByteAddressBuffer _input;

[[vk::push_constant]] const Constants _constants;

groupshared DrawCall _transposeSharedData[TRANSPOSE_BLOCK_SIZE * TRANSPOSE_BLOCK_SIZE];

struct CSInput
{
    uint3 groupID : SV_GroupID; // Unused?
    uint3 dispatchThreadID : SV_DispatchThreadID;
    uint3 groupThreadID : SV_GroupThreadID; // Unused?
    uint groupIndex : SV_GroupIndex;
};

[numthreads(TRANSPOSE_BLOCK_SIZE, TRANSPOSE_BLOCK_SIZE, 1)]
void main(CSInput input)
{
    uint GI = input.groupIndex;
    uint3 DTid = input.dispatchThreadID;
    uint3 GTid = input.groupThreadID;
    
    uint loadIndex = (DTid.y * _constants.width + DTid.x); 
    _transposeSharedData[GI] = _input.Load<DrawCall>(loadIndex * 20); // 20 = sizeof(DrawCall)
    GroupMemoryBarrierWithGroupSync();
    
    uint2 xy = DTid.yx - GTid.yx + GTid.xy;
    uint storeIndex = xy.y * _constants.height + xy.x;
    uint drawCallIndex = GTid.x * TRANSPOSE_BLOCK_SIZE + GTid.y;
    
    DrawCall drawCall = _transposeSharedData[drawCallIndex];
    
    uint offset = storeIndex * 20;
    _drawCalls.Store4(offset, uint4(drawCall.indexCount, drawCall.instanceCount, drawCall.firstIndex, drawCall.vertexOffset));
    _drawCalls.Store(offset+16, drawCall.firstInstance);
}