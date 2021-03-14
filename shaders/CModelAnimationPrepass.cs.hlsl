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
[[vk::binding(3, PER_PASS)]] RWStructuredBuffer<InstanceData> _instances;
[[vk::binding(4, PER_PASS)]] StructuredBuffer<AnimationSequence> _animationSequence;
[[vk::binding(5, PER_PASS)]] StructuredBuffer<AnimationModelInfo> _animationModelInfo;
[[vk::binding(6, PER_PASS)]] StructuredBuffer<AnimationBoneInfo> _animationBoneInfo;
[[vk::binding(7, PER_PASS)]] RWStructuredBuffer<float4x4> _animationBoneDeformMatrix;
[[vk::binding(8, PER_PASS)]] RWStructuredBuffer<AnimationBoneInstanceData> _animationBoneInstances;
[[vk::binding(9, PER_PASS)]] StructuredBuffer<AnimationTrackInfo> _animationTrackInfo;
[[vk::binding(10, PER_PASS)]] StructuredBuffer<uint> _animationTrackTimestamp;
[[vk::binding(11, PER_PASS)]] StructuredBuffer<float4> _animationTrackValue;

void UpdateBone(inout AnimationBoneInstanceData bone, in AnimationSequence sequence)
{
    if (bone.animationProgress > sequence.duration)
    {
        uint isLooping = bone.animateState == 2;
        bone.animateState = 2 * isLooping;
        bone.animationProgress = 0;
    }
    else
    {
        bone.animationProgress += 1.f * _constants.deltaTime;
    }
}

[numthreads(32, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    const uint instanceID = dispatchThreadId.x;
    if (instanceID >= _constants.numInstances)
    {
        return;
    }

    const InstanceData instanceData = _instances[instanceID];
    const AnimationModelInfo modelInfo = _animationModelInfo[instanceData.modelId];

    const int numSequences = modelInfo.packedData0 & 0xFFFF;
    const int numBones = (modelInfo.packedData0 >> 16) & 0xFFFF;

    if (numSequences == 0)
    {
        return;
    }

    for (int i = 0; i < numBones; i++)
    {
        AnimationBoneInstanceData boneInstance = _animationBoneInstances[instanceData.boneInstanceDataOffset + i];

        const uint sequenceIndex = boneInstance.packedData0 & 0xFFFF;
        const AnimationSequence sequence = _animationSequence[modelInfo.sequenceOffset + sequenceIndex];

        UpdateBone(boneInstance, sequence);

        _animationBoneInstances[instanceData.boneInstanceDataOffset + i] = boneInstance;
    }
}