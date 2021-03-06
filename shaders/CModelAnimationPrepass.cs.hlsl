#include "cModel.inc.hlsl"
#include "debug.inc.hlsl"

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
[[vk::binding(7, PER_PASS)]] StructuredBuffer<float3> _animationSequenceValueVec3;
[[vk::binding(8, PER_PASS)]] StructuredBuffer<float4> _animationSequenceValueVec4;

float4x4 MatrixTranslate(float3 v)
{
    float4x4 result = { 1, 0, 0, 0, 
                        0, 1, 0, 0, 
                        0, 0, 1, 0, 
                        v[0], v[1], v[2], 1 };
    return result;
}

float4x4 MatrixScale(float3 v)
{
    float4x4 result = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    result[0] = result[0] * v[0];
    result[1] = result[1] * v[1];
    result[2] = result[2] * v[2];

    return result;
}

/*
inline Matrix<T, 4> ToMatrix4() const {
    const T x2 = v_[0] * v_[0], y2 = v_[1] * v_[1], z2 = v_[2] * v_[2];
    const T sx = s_ * v_[0], sy = s_ * v_[1], sz = s_ * v_[2];
    const T xz = v_[0] * v_[2], yz = v_[1] * v_[2], xy = v_[0] * v_[1];
    return Matrix<T, 4>(1 - 2 * (y2 + z2), 2 * (xy + sz), 2 * (xz - sy), 0.0f,
                        2 * (xy - sz), 1 - 2 * (x2 + z2), 2 * (sx + yz), 0.0f,
                        2 * (sy + xz), 2 * (yz - sx), 1 - 2 * (x2 + y2), 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);
  }
*/

float4x4 RotationToMatrix(float4 quat)
{
    //quat.x = -quat.x;
    //quat.y = -quat.y;
    //quat.z = -quat.z;

    float x2 = quat.x * quat.x;
    float y2 = quat.y * quat.y;
    float z2 = quat.z * quat.z;
    float sx = quat.w * quat.x;
    float sy = quat.w * quat.y;
    float sz = quat.w * quat.z;
    float xz = quat.x * quat.z;
    float yz = quat.y * quat.z;
    float xy = quat.x * quat.y;

    return float4x4(1 - 2 * (y2 + z2), 2 * (xy + sz), 2 * (xz - sy), 0.0f,
        2 * (xy - sz), 1 - 2 * (x2 + z2), 2 * (sx + yz), 0.0f,
        2 * (sy + xz), 2 * (yz - sx), 1 - 2 * (x2 + y2), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

float4 slerp(float4 a, float4 b, float t)
{
    const float l2 = dot(a, b);
    if (l2 < 0.0f)
    {
        b = float4(-b.x, -b.y, -b.z, -b.w); // Might just be -b
    }

    float4 c;
    c.x = a.x - t * (a.x - b.x);
    c.y = a.y - t * (a.y - b.y);
    c.z = a.z - t * (a.z - b.z);
    c.w = a.w - t * (a.w - b.w);

    return normalize(c);
}

float4x4 GetBoneMatrix(InstanceData instanceData, int boneIndex)
{
    AnimationBoneInfo boneInfo = _animationBoneInfo[boneIndex];

    uint numTranslationSequences = boneInfo.packedData0 & 0xFFFF;
    uint numRotationSequences = (boneInfo.packedData0 >> 16) & 0xFFFF;
    uint numScaleSequences = boneInfo.packedData1 & 0xFFFF;

    float3 pivotPoint = float3(boneInfo.pivotPointX, boneInfo.pivotPointY, boneInfo.pivotPointZ);

    float4x4 boneMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    float3 translationValue = float3(0.f, 0.f, 0.f);
    float4 rotationValue = float4(0.f, 0.f, 0.f, 1.f);
    float3 scaleValue = float3(1.f, 1.f, 1.f);

    uint isTranslationTrackGlobalSequence = (boneInfo.flags & 0x2) != 0;
    uint isRotationTrackGlobalSequence = (boneInfo.flags & 0x4) != 0;
    uint isScaleTrackGlobalSequence = (boneInfo.flags & 0x8) != 0;

    if (isScaleTrackGlobalSequence == false)
    {
        for (int i = 0; i < numScaleSequences; i++)
        {
            AnimationTrackInfo trackInfo = _animationTrackInfo[boneInfo.scaleSequenceOffset + i];
            //if (trackInfo.sequenceIndex != 0 /*instanceData.activeSequenceId*/) // Snake / Chest
            if (trackInfo.sequenceIndex != 2 /*instanceData.activeSequenceId*/) // Murloc
                continue;

            uint numTimestamps = trackInfo.packedData0 & 0xFFFF;
            uint numValues = (trackInfo.packedData0 >> 16) & 0xFFFF;

            for (int j = 0; j < numTimestamps; j++)
            {
                float sequenceTimestamp = ((float)_animationSequenceTimestamp[trackInfo.timestampOffset + j] / 1000.f);
                if (instanceData.animProgress < sequenceTimestamp)
                {
                    float defaultTimestamp = 0.f;
                    float3 defaultValue = float3(1.f, 1.f, 1.f);

                    if (j > 0)
                    {
                        float defaultTimestamp = ((float)_animationSequenceTimestamp[trackInfo.timestampOffset + (j - 1)] / 1000.f);
                        float3 defaultValue = _animationSequenceValueVec3[trackInfo.valueOffset + (j - 1)];
                    }

                    float nextValueTimestamp = ((float)_animationSequenceTimestamp[trackInfo.timestampOffset + j] / 1000.f);
                    float3 nextValue = _animationSequenceValueVec3[trackInfo.valueOffset + j];

                    float time = (instanceData.animProgress - defaultTimestamp) / (nextValueTimestamp - defaultTimestamp);
                    //scaleValue = lerp(defaultValue, nextValue, time);

                    break;
                }
            }

            break;
        }
    }

    if (isRotationTrackGlobalSequence == false)
    {
        for (int o = 0; o < numRotationSequences; o++)
        {
            AnimationTrackInfo trackInfo = _animationTrackInfo[boneInfo.rotationSequenceOffset + o];
            //if (trackInfo.sequenceIndex != 0 /*instanceData.activeSequenceId*/) // Snake / Chest
            if (trackInfo.sequenceIndex != 2 /*instanceData.activeSequenceId*/) // Murloc
                continue;

            uint numTimestamps = trackInfo.packedData0 & 0xFFFF;
            uint numValues = (trackInfo.packedData0 >> 16) & 0xFFFF;

            for (int j = 0; j < numTimestamps; j++)
            {
                float sequenceTimestamp = ((float)_animationSequenceTimestamp[trackInfo.timestampOffset + j] / 1000.f);
                if (instanceData.animProgress < sequenceTimestamp)
                {
                    float defaultTimestamp = 0.f;
                    float4 defaultValue = float4(0.f, 0.f, 0.f, 1.f);

                    if (j > 0)
                    {
                        defaultTimestamp = ((float)_animationSequenceTimestamp[trackInfo.timestampOffset + (j - 1)] / 1000.f);
                        defaultValue = _animationSequenceValueVec4[trackInfo.valueOffset + (j - 1)];
                    }

                    float nextValueTimestamp = ((float)_animationSequenceTimestamp[trackInfo.timestampOffset + j] / 1000.f);
                    float4 nextValue = _animationSequenceValueVec4[trackInfo.valueOffset + j];

                    float time = (instanceData.animProgress - defaultTimestamp) / (nextValueTimestamp - defaultTimestamp);
                    //rotationValue = slerp(defaultValue, nextValue, time);
                    break;
                }
            }

            break;
        }
    }

    if (isTranslationTrackGlobalSequence == false)
    {
        for (int p = 0; p < numTranslationSequences; p++)
        {
            AnimationTrackInfo trackInfo = _animationTrackInfo[boneInfo.translationSequenceOffset + p];

            //if (trackInfo.sequenceIndex != 0 /*instanceData.activeSequenceId*/) // Snake / Chest
            if (trackInfo.sequenceIndex != 2 /*instanceData.activeSequenceId*/) // Murloc
                continue;

            uint numTimestamps = trackInfo.packedData0 & 0xFFFF;
            uint numValues = (trackInfo.packedData0 >> 16) & 0xFFFF;

            for (int j = 0; j < numTimestamps; j++)
            {
                float sequenceTimestamp = ((float)_animationSequenceTimestamp[trackInfo.timestampOffset + j] / 1000.f);
                if (instanceData.animProgress < sequenceTimestamp)
                {
                    float defaultTimestamp = 0.f;
                    float3 defaultValue = float3(0.f, 0.f, 0.f);

                    if (j > 0)
                    {
                        defaultTimestamp = ((float)_animationSequenceTimestamp[trackInfo.timestampOffset + (j - 1)] / 1000.f);
                        defaultValue = _animationSequenceValueVec3[trackInfo.valueOffset + (j - 1)];
                    }

                    float nextValueTimestamp = ((float)_animationSequenceTimestamp[trackInfo.timestampOffset + j] / 1000.f);
                    float3 nextValue = _animationSequenceValueVec3[trackInfo.valueOffset + j];

                    float time = (instanceData.animProgress - defaultTimestamp) / (nextValueTimestamp - defaultTimestamp);
                    translationValue = lerp(defaultValue, nextValue, time);

                    break;
                }
            }

            break;
        }
    }

    boneMatrix = mul(MatrixTranslate(pivotPoint), boneMatrix);
    
    boneMatrix = mul(MatrixTranslate(translationValue), boneMatrix);
    boneMatrix = mul(RotationToMatrix(rotationValue), boneMatrix);
    boneMatrix = mul(MatrixScale(scaleValue), boneMatrix);

    boneMatrix = mul(MatrixTranslate(-pivotPoint), boneMatrix);

    return boneMatrix;
}

void DebugRenderBone(InstanceData instanceData, int boneIndex, float4x4 boneMatrix, int parentBoneIndex, float4x4 parentBoneMatrix, bool drawLineToParent)
{
    AnimationBoneInfo boneInfo = _animationBoneInfo[boneIndex];
    AnimationBoneInfo parentBoneInfo = _animationBoneInfo[parentBoneIndex];

    float3 pivotPoint = float3(boneInfo.pivotPointX, boneInfo.pivotPointY, boneInfo.pivotPointZ);
    float4 position = mul(float4(pivotPoint, 1.0f), boneMatrix);
    position.xy = -position.xy;

    float4 currPos = mul(position, instanceData.instanceMatrix);

    float4 minPos = currPos;
    minPos.xyz -= 0.025f;

    float4 posMax = currPos;
    posMax.xyz += 0.025f;

    debugDrawAABB3D((float3)minPos, (float3)posMax, 0xffff00ff);
    //debugDrawMatrix(pivotBoneMatrix, float3(0.05f, 0.05f, 0.05f));

    if (drawLineToParent == true && parentBoneIndex != 65535)
    {
        float3 parentPivotPoint = float3(parentBoneInfo.pivotPointX, parentBoneInfo.pivotPointY, parentBoneInfo.pivotPointZ);
        float4 parentPosition = mul(float4(parentPivotPoint, 1.0f), parentBoneMatrix);
        parentPosition.xy = -parentPosition.xy;

        float4 parentPos = mul(parentPosition, instanceData.instanceMatrix);

        debugDrawLine3D((float3)currPos, (float3)parentPos, 0xffffff00);
    }

    /*float3 position = boneMatrix[3].xyz;
    position.xy = -position.xy;

    float3 minPos = position;
    minPos.xyz -= 0.025f;

    float3 posMax = position;
    posMax.xyz += 0.025f;

    //debugDrawAABB3D(minPos, posMax, 0xffff00ff);
    debugDrawMatrix(boneMatrix, float3(0.05f, 0.05f, 0.05f));*/
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
    AnimationTrackInfo trackInfo = _animationTrackInfo[6]; // Murloc
    //AnimationTrackInfo trackInfo = _animationTrackInfo[3]; // Chest

    if (instanceData.animProgress > (float(trackInfo.duration) / 1000))
    {
        instanceData.animProgress = 0;
    }
    else
    {
        instanceData.animProgress += 0.1f * _constants.deltaTime;
    }

    _instances[dispatchThreadId.x] = instanceData;

    AnimationModelBoneInfo modelBoneInfo = _animationModelBoneInfo[instanceData.modelId];

    float4x4 animationBoneDeformInfo[256];

    for (int i = 0; i < modelBoneInfo.numBones; i++)
    {
        AnimationBoneInfo boneInfo = _animationBoneInfo[modelBoneInfo.offset + i];

        float4x4 currBoneMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        float4x4 parentBoneMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
        uint parentBoneId = (boneInfo.packedData1 >> 16) & 0xFFFF;

        if (parentBoneId != 65535)
        {
            parentBoneMatrix = animationBoneDeformInfo[parentBoneId];
        }

        if ((boneInfo.flags & 1) != 0)
        {
            currBoneMatrix = GetBoneMatrix(instanceData, modelBoneInfo.offset + i);
            currBoneMatrix = mul(currBoneMatrix, parentBoneMatrix);

            DebugRenderBone(instanceData, modelBoneInfo.offset + i, currBoneMatrix, modelBoneInfo.offset + parentBoneId, parentBoneMatrix, true);
            animationBoneDeformInfo[i] = currBoneMatrix;
        }
        else
        {
            animationBoneDeformInfo[i] = parentBoneMatrix;
        }
    }

    for (int j = 0; j < modelBoneInfo.numBones; j++)
    {
        _animationBoneDeformInfo[modelBoneInfo.offset + j].boneMatrix = animationBoneDeformInfo[j];
    }
}