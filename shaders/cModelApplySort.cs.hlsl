
struct Constants
{
    uint maxDrawCount;
};

struct DrawCall
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

[[vk::push_constant]] Constants _constants;
[[vk::binding(0, PER_PASS)]] StructuredBuffer<uint> _sortValues;

[[vk::binding(1, PER_PASS)]] StructuredBuffer<uint> _culledDrawCount;
[[vk::binding(2, PER_PASS)]] StructuredBuffer<DrawCall> _culledDrawCalls;
[[vk::binding(3, PER_PASS)]] RWStructuredBuffer<DrawCall> _sortedCulledDrawCalls;

[numthreads(32, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint sortIndex = dispatchThreadId.x;
    uint drawCount = _culledDrawCount[0];
    
    if (sortIndex >= drawCount)
    {
        return;
    }
    
    uint drawCallSortedIndex = _sortValues[sortIndex];
    _sortedCulledDrawCalls[sortIndex] = _culledDrawCalls[drawCallSortedIndex];
}