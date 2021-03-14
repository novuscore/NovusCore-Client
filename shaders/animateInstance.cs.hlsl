#include "cModel.inc.hlsl"
#include "debug.inc.hlsl"
#include "cModelAnimation.inc.hlsl"

//struct Constants
//{
//    uint numInstances;
//    float deltaTime;
//};

// Inputs
//[[vk::push_constant]] Constants _constants;
[[vk::binding(1, PER_PASS)]] ByteAddressBuffer _visibleInstanceCount;
[[vk::binding(2, PER_PASS)]] StructuredBuffer<uint> _visibleInstanceIndices;
[[vk::binding(3, PER_PASS)]] RWStructuredBuffer<InstanceData> _instances;
[[vk::binding(4, PER_PASS)]] StructuredBuffer<AnimationSequence> _animationSequence;
[[vk::binding(5, PER_PASS)]] StructuredBuffer<AnimationModelInfo> _animationModelInfo;
[[vk::binding(6, PER_PASS)]] StructuredBuffer<AnimationBoneInfo> _animationBoneInfo;
[[vk::binding(7, PER_PASS)]] RWStructuredBuffer<float4x4> _animationBoneDeformMatrix;
[[vk::binding(8, PER_PASS)]] RWStructuredBuffer<AnimationBoneInstanceData> _animationBoneInstances;
[[vk::binding(9, PER_PASS)]] StructuredBuffer<AnimationTrackInfo> _animationTrackInfo;
[[vk::binding(10, PER_PASS)]] StructuredBuffer<uint> _animationTrackTimestamp;
[[vk::binding(11, PER_PASS)]] StructuredBuffer<float4> _animationTrackValue;

AnimationState GetAnimationState(AnimationBoneInstanceData boneInstanceData)
{
    AnimationState state;
    state.animationProgress = boneInstanceData.animationProgress;

    return state;
}

[numthreads(32, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    const uint visibleInstanceCount = _visibleInstanceCount.Load(0);
    if (dispatchThreadId.x >= visibleInstanceCount)
    {
        return;
    }

    const uint instanceID = _visibleInstanceIndices[dispatchThreadId.x];

    const InstanceData instanceData = _instances[instanceID];
    const AnimationModelInfo modelInfo = _animationModelInfo[instanceData.modelId];

    int numSequences = modelInfo.packedData0& 0xFFFF;
    int numBones = (modelInfo.packedData0 >> 16) & 0xFFFF;

    if (numSequences == 0)
    {
        return;
    }
    
    for (int i = 0; i < numBones; i++)
    {
        const AnimationBoneInfo boneInfo = _animationBoneInfo[modelInfo.boneInfoOffset + i];
        const uint parentBoneId = (boneInfo.packedData1 >> 16) & 0xFFFF;

        const AnimationBoneInstanceData boneInstance = _animationBoneInstances[instanceData.boneInstanceDataOffset + i];
        const uint sequenceIndex = boneInstance.packedData0 & 0xFFFF;

        float4x4 parentBoneMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        //float3 parentPivotPoint = float3(0.f, 0.f, 0.f);
        if (parentBoneId != 65535)
        {
            parentBoneMatrix = _animationBoneDeformMatrix[instanceData.boneDeformOffset + parentBoneId];

            //const AnimationBoneInfo parentBoneInfo = _animationBoneInfo[modelInfo.boneInfoOffset + parentBoneId];
            //parentPivotPoint = float3(parentBoneInfo.pivotPointX, parentBoneInfo.pivotPointY, parentBoneInfo.pivotPointZ);
        }

        // If we aren't animating, assume parentBoneMatrix or Identity
        if (boneInstance.animateState == 0)
        {
            _animationBoneDeformMatrix[instanceData.boneDeformOffset + i] = parentBoneMatrix;
            continue;
        }

        if ((boneInfo.flags & 1) != 0)
        {
            AnimationContext ctx;
            ctx.activeSequenceId = sequenceIndex;
            ctx.animationTrackInfos = _animationTrackInfo;
            ctx.trackTimestamps = _animationTrackTimestamp;
            ctx.trackValues = _animationTrackValue;
            ctx.boneInfo = boneInfo;
            ctx.state = GetAnimationState(boneInstance);

            float4x4 currBoneMatrix = GetBoneMatrix(ctx);
            currBoneMatrix = mul(currBoneMatrix, parentBoneMatrix);

            //DebugRenderBone(instanceData, modelBoneInfo.offset + i, float3(boneInfo.pivotPointX, boneInfo.pivotPointY, boneInfo.pivotPointZ), currBoneMatrix, modelBoneInfo.offset + parentBoneId, parentPivotPoint, parentBoneMatrix, true);
            _animationBoneDeformMatrix[instanceData.boneDeformOffset + i] = currBoneMatrix;
        }
        else
        {
            _animationBoneDeformMatrix[instanceData.boneDeformOffset + i] = parentBoneMatrix;
        }
    }
}