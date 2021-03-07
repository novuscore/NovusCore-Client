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
[[vk::binding(2, PER_PASS)]] StructuredBuffer<AnimationModelBoneInfo> _animationModelBoneInfo;
[[vk::binding(3, PER_PASS)]] StructuredBuffer<AnimationBoneInfo> _animationBoneInfo;
[[vk::binding(4, PER_PASS)]] RWStructuredBuffer<AnimationBoneDeformInfo> _animationBoneDeformInfo;
[[vk::binding(5, PER_PASS)]] StructuredBuffer<AnimationTrackInfo> _animationTrackInfo;
[[vk::binding(6, PER_PASS)]] StructuredBuffer<uint> _animationSequenceTimestamp;
[[vk::binding(7, PER_PASS)]] StructuredBuffer<float4> _animationSequenceValueVec;

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

    //AnimationTrackInfo trackInfo = _animationTrackInfo[11]; // Snake
    //AnimationTrackInfo trackInfo = _animationTrackInfo[6]; // Murloc
    //AnimationTrackInfo trackInfo = _animationTrackInfo[46]; // LK Murloc
    AnimationTrackInfo trackInfo = _animationTrackInfo[2]; // Dudu Cat
    //AnimationTrackInfo trackInfo = _animationTrackInfo[3]; // Chest

    if (instanceData.animProgress > (float(trackInfo.duration) / 1000))
    {
        instanceData.animProgress = 0;
    }
    else
    {
        instanceData.animProgress += 1.f * _constants.deltaTime;
    }

    _instances[dispatchThreadId.x] = instanceData;

    const AnimationState state = GetAnimationState(instanceData);
    const AnimationModelBoneInfo modelBoneInfo = _animationModelBoneInfo[instanceData.modelId];

    for (int i = 0; i < modelBoneInfo.numBones; i++)
    {
        AnimationBoneInfo boneInfo = _animationBoneInfo[modelBoneInfo.offset + i];

        float4x4 currBoneMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        float4x4 parentBoneMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        uint parentBoneId = (boneInfo.packedData1 >> 16) & 0xFFFF;
        float3 parentPivotPoint = float3(0.f, 0.f, 0.f);

        if (parentBoneId != 65535)
        {
            parentBoneMatrix = _animationBoneDeformInfo[parentBoneId].boneMatrix;

            AnimationBoneInfo parentBoneInfo = _animationBoneInfo[modelBoneInfo.offset + parentBoneId];
            parentPivotPoint = float3(parentBoneInfo.pivotPointX, parentBoneInfo.pivotPointY, parentBoneInfo.pivotPointZ);
        }

        if ((boneInfo.flags & 1) != 0)
        {
            AnimationContext ctx;
            ctx.animationTrackInfos = _animationTrackInfo;
            ctx.sequenceTimestamps = _animationSequenceTimestamp;
            ctx.sequenceValues = _animationSequenceValueVec;
            ctx.boneInfo = boneInfo;
            ctx.state = state;

            currBoneMatrix = GetBoneMatrix(ctx);
            currBoneMatrix = mul(currBoneMatrix, parentBoneMatrix);

            //DebugRenderBone(instanceData, modelBoneInfo.offset + i, float3(boneInfo.pivotPointX, boneInfo.pivotPointY, boneInfo.pivotPointZ), currBoneMatrix, modelBoneInfo.offset + parentBoneId, parentPivotPoint, parentBoneMatrix, true);
            _animationBoneDeformInfo[i].boneMatrix = currBoneMatrix;
        }
        else
        {
            _animationBoneDeformInfo[i].boneMatrix = parentBoneMatrix;
        }
    }
}