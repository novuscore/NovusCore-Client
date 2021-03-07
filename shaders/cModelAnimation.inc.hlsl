struct AnimationState
{
    float animProgress;
};

struct AnimationContext
{
    AnimationState state;
    AnimationBoneInfo boneInfo;

    StructuredBuffer<AnimationTrackInfo> animationTrackInfos;
    StructuredBuffer<uint> sequenceTimestamps;
    StructuredBuffer<float4> sequenceValues;
};

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

float4x4 GetBoneMatrix(AnimationContext ctx)
{
    const AnimationState state = ctx.state;
    const AnimationBoneInfo boneInfo = ctx.boneInfo;

    const uint numTranslationSequences = boneInfo.packedData0 & 0xFFFF;
    const uint numRotationSequences = (boneInfo.packedData0 >> 16) & 0xFFFF;
    const uint numScaleSequences = boneInfo.packedData1 & 0xFFFF;

    const float3 pivotPoint = float3(boneInfo.pivotPointX, boneInfo.pivotPointY, boneInfo.pivotPointZ);

    float4x4 boneMatrix = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    float4 translationValue = float4(0.f, 0.f, 0.f, 0.f);
    float4 rotationValue = float4(0.f, 0.f, 0.f, 1.f);
    float4 scaleValue = float4(1.f, 1.f, 1.f, 0.f);

    const uint isTranslationTrackGlobalSequence = (boneInfo.flags & 0x2) != 0;
    const uint isRotationTrackGlobalSequence = (boneInfo.flags & 0x4) != 0;
    const uint isScaleTrackGlobalSequence = (boneInfo.flags & 0x8) != 0;

    if (isScaleTrackGlobalSequence == false)
    {
        for (int i = 0; i < numScaleSequences; i++)
        {
            AnimationTrackInfo trackInfo = ctx.animationTrackInfos[boneInfo.scaleSequenceOffset + i];
            //if (trackInfo.sequenceIndex != 0 /*instanceData.activeSequenceId*/) // Snake / Chest
            if (trackInfo.sequenceIndex != 2 /*instanceData.activeSequenceId*/)  // Dudu Cat // Murloc
            //if (trackInfo.sequenceIndex != 36 /*instanceData.activeSequenceId*/) // LK Murloc
                continue;

            uint numTimestamps = trackInfo.packedData0 & 0xFFFF;
            uint numValues = (trackInfo.packedData0 >> 16) & 0xFFFF;

            for (int j = 0; j < numTimestamps; j++)
            {
                float sequenceTimestamp = ((float)ctx.sequenceTimestamps[trackInfo.timestampOffset + j] / 1000.f);
                if (state.animProgress < sequenceTimestamp)
                {
                    float defaultTimestamp = 0.f;
                    float4 defaultValue = float4(1.f, 1.f, 1.f, 0.f);

                    if (j > 0)
                    {
                        float defaultTimestamp = ((float)ctx.sequenceTimestamps[trackInfo.timestampOffset + (j - 1)] / 1000.f);
                        float4 defaultValue = ctx.sequenceValues[trackInfo.valueOffset + (j - 1)];
                    }

                    float nextValueTimestamp = ((float)ctx.sequenceTimestamps[trackInfo.timestampOffset + j] / 1000.f);
                    float4 nextValue = ctx.sequenceValues[trackInfo.valueOffset + j];

                    float time = (state.animProgress - defaultTimestamp) / (nextValueTimestamp - defaultTimestamp);
                    scaleValue = lerp(defaultValue, nextValue, time);

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
            AnimationTrackInfo trackInfo = ctx.animationTrackInfos[boneInfo.rotationSequenceOffset + o];
            //if (trackInfo.sequenceIndex != 0 /*instanceData.activeSequenceId*/) // Snake / Chest
            if (trackInfo.sequenceIndex != 2 /*instanceData.activeSequenceId*/)  // Dudu Cat // Murloc
            //if (trackInfo.sequenceIndex != 36 /*instanceData.activeSequenceId*/) // LK Murloc
                continue;

            uint numTimestamps = trackInfo.packedData0 & 0xFFFF;
            uint numValues = (trackInfo.packedData0 >> 16) & 0xFFFF;

            for (int j = 0; j < numTimestamps; j++)
            {
                float sequenceTimestamp = ((float)ctx.sequenceTimestamps[trackInfo.timestampOffset + j] / 1000.f);
                if (state.animProgress < sequenceTimestamp)
                {
                    float defaultTimestamp = 0.f;
                    float4 defaultValue = float4(0.f, 0.f, 0.f, 1.f);

                    if (j > 0)
                    {
                        defaultTimestamp = ((float)ctx.sequenceTimestamps[trackInfo.timestampOffset + (j - 1)] / 1000.f);
                        defaultValue = ctx.sequenceValues[trackInfo.valueOffset + (j - 1)];
                    }

                    float nextValueTimestamp = ((float)ctx.sequenceTimestamps[trackInfo.timestampOffset + j] / 1000.f);
                    float4 nextValue = ctx.sequenceValues[trackInfo.valueOffset + j];

                    float time = (state.animProgress - defaultTimestamp) / (nextValueTimestamp - defaultTimestamp);
                    rotationValue = slerp(defaultValue, nextValue, time);
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
            AnimationTrackInfo trackInfo = ctx.animationTrackInfos[boneInfo.translationSequenceOffset + p];

            //if (trackInfo.sequenceIndex != 0 /*instanceData.activeSequenceId*/) // Snake / Chest
            if (trackInfo.sequenceIndex != 2 /*instanceData.activeSequenceId*/) // Dudu Cat // Murloc
            //if (trackInfo.sequenceIndex != 36 /*instanceData.activeSequenceId*/) // LK Murloc
                continue;

            uint numTimestamps = trackInfo.packedData0 & 0xFFFF;
            uint numValues = (trackInfo.packedData0 >> 16) & 0xFFFF;

            for (int j = 0; j < numTimestamps; j++)
            {
                float sequenceTimestamp = ((float)ctx.sequenceTimestamps[trackInfo.timestampOffset + j] / 1000.f);
                if (state.animProgress < sequenceTimestamp)
                {
                    float defaultTimestamp = 0.f;
                    float4 defaultValue = float4(0.f, 0.f, 0.f, 0.f);

                    if (j > 0)
                    {
                        defaultTimestamp = ((float)ctx.sequenceTimestamps[trackInfo.timestampOffset + (j - 1)] / 1000.f);
                        defaultValue = ctx.sequenceValues[trackInfo.valueOffset + (j - 1)];
                    }

                    float nextValueTimestamp = ((float)ctx.sequenceTimestamps[trackInfo.timestampOffset + j] / 1000.f);
                    float4 nextValue = ctx.sequenceValues[trackInfo.valueOffset + j];

                    float time = (state.animProgress - defaultTimestamp) / (nextValueTimestamp - defaultTimestamp);
                    translationValue = lerp(defaultValue, nextValue, time);

                    break;
                }
            }

            break;
        }
    }

    boneMatrix = mul(MatrixTranslate(pivotPoint.xyz), boneMatrix);

    boneMatrix = mul(MatrixTranslate(translationValue.xyz), boneMatrix);
    boneMatrix = mul(RotationToMatrix(rotationValue), boneMatrix);
    boneMatrix = mul(MatrixScale(scaleValue.xyz), boneMatrix);

    boneMatrix = mul(MatrixTranslate(-pivotPoint.xyz), boneMatrix);

    return boneMatrix;
}