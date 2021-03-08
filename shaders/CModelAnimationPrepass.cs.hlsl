#include "cModel.inc.hlsl"
#include "debug.inc.hlsl"
#include "cModelAnimation.inc.hlsl"

struct Constants
{
    uint numInstances;
    float deltaTime;
};

// Inputs
[[vk::push_constant]] Constants _constants;
[[vk::binding(1, PER_PASS)]] RWStructuredBuffer<InstanceData> _instances;
[[vk::binding(2, PER_PASS)]] StructuredBuffer<AnimationSequence> _animationSequence;
[[vk::binding(3, PER_PASS)]] StructuredBuffer<AnimationModelInfo> _animationModelInfo;
[[vk::binding(4, PER_PASS)]] StructuredBuffer<AnimationBoneInfo> _animationBoneInfo;
[[vk::binding(5, PER_PASS)]] RWStructuredBuffer<float4x4> _animationBoneDeformMatrix;
[[vk::binding(6, PER_PASS)]] StructuredBuffer<AnimationTrackInfo> _animationTrackInfo;
[[vk::binding(7, PER_PASS)]] StructuredBuffer<uint> _animationTrackTimestamp;
[[vk::binding(8, PER_PASS)]] StructuredBuffer<float4> _animationTrackValue;

AnimationState GetAnimationState(InstanceData instanceData)
{
    AnimationState state;
    state.animProgress = instanceData.animProgress;

    return state;
}

[numthreads(32, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= _constants.numInstances)
    {
        return;
    }

    InstanceData instanceData = _instances[dispatchThreadId.x];
    const AnimationModelInfo modelInfo = _animationModelInfo[instanceData.modelId];

    AnimationSequence sequence = _animationSequence[modelInfo.sequenceOffset + instanceData.activeSequenceId];
    if (instanceData.animProgress > sequence.duration)
    {
        instanceData.animProgress = 0;
    }
    else
    {
        instanceData.animProgress += 1.f * _constants.deltaTime;
    }

    _instances[dispatchThreadId.x] = instanceData;

    const AnimationState state = GetAnimationState(instanceData);

    int numSequences = modelInfo.packedData0& 0xFFFF;
    int numBones = (modelInfo.packedData0 >> 16) & 0xFFFF;

    if (numSequences == 0 || instanceData.activeSequenceId == 65535)
        return;

    for (int i = 0; i < numBones; i++)
    {
        float4x4 currBoneMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        float4x4 parentBoneMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

        AnimationBoneInfo boneInfo = _animationBoneInfo[modelInfo.boneInfoOffset + i];

        uint parentBoneId = (boneInfo.packedData1 >> 16) & 0xFFFF;
        float3 parentPivotPoint = float3(0.f, 0.f, 0.f);

        if (parentBoneId != 65535)
        {
            parentBoneMatrix = _animationBoneDeformMatrix[instanceData.boneDeformOffset + parentBoneId];

            AnimationBoneInfo parentBoneInfo = _animationBoneInfo[modelInfo.boneInfoOffset + parentBoneId];
            parentPivotPoint = float3(parentBoneInfo.pivotPointX, parentBoneInfo.pivotPointY, parentBoneInfo.pivotPointZ);
        }

        if ((boneInfo.flags & 1) != 0)
        {
            AnimationContext ctx;
            ctx.activeSequenceId = instanceData.activeSequenceId;
            ctx.animationTrackInfos = _animationTrackInfo;
            ctx.trackTimestamps = _animationTrackTimestamp;
            ctx.trackValues = _animationTrackValue;
            ctx.boneInfo = boneInfo;
            ctx.state = state;

            currBoneMatrix = GetBoneMatrix(ctx);
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