
struct InstanceData
{
    float4x4 instanceMatrix;

    uint modelId;
    uint boneDeformOffset;
    uint boneInstanceDataOffset;
    uint padding0;

    /*uint modelId;
    uint activeSequenceId;
    float animProgress;
    uint boneDeformOffset;*/
};

struct AnimationBoneInstanceData
{
    float animationProgress;
    uint packedData0; // sequenceIndex (16 bit), sequenceOverrideIndex (16 bit)
    uint animationframeIndex;
    uint animateState; // 0 == STOPPED, 1 == PLAY_ONCE, 2 == PLAY_LOOP
};

struct AnimationSequence
{
    uint packedData0; // animationId (16 bit), animationSubId (16 bit)
    uint packedData1; // nextSubAnimationId (16 bit), nextAliasId (16 bit)

    uint flags; // 0x1(IsAlwaysPlaying), 0x2(IsAlias), 0x4(BlendTransition)
    float duration;

    uint packedRepeatRange; // Min (16 bit), Max (16 bit)
    uint packedBlendTimes; // Start (16 bit), End (16 bit)

    uint padding0;
    uint padding1;
};

struct AnimationModelInfo
{
    uint packedData0; // numSequences (16 bit), numBones (16 bit)
    uint sequenceOffset;
    uint boneInfoOffset;
    uint padding0;
};

struct AnimationBoneInfo
{
    uint packedData0; // numTranslationTracks, numRotationTracks
    uint packedData1; // numScaleTracks, parentBoneId

    uint translationTrackOffset;
    uint rotationTrackOffset;
    uint scaleTrackOffset;

    uint flags;

    float pivotPointX;
    float pivotPointY;
    float pivotPointZ;
    uint padding0;
    uint padding1;
    uint padding2;
};

struct AnimationTrackInfo
{
    uint sequenceIndex; // Only 16 bit is used here, rest is padding

    uint packedData0; // numTimestamps, numValues
    uint timestampOffset;
    uint valueOffset;
};

struct PackedDrawCallData
{
    uint instanceID;
    uint cullingDataID;
    uint packed; // uint16_t textureUnitOffset, uint16_t numTextureUnits
    uint renderPriority;
}; // 16 bytes

struct DrawCallData
{
    uint instanceID;
    uint cullingDataID;
    uint textureUnitOffset;
    uint numTextureUnits;
    uint renderPriority;
};

[[vk::binding(0, PER_PASS)]] StructuredBuffer<PackedDrawCallData> _packedDrawCallDatas;

DrawCallData LoadDrawCallData(uint drawCallID)
{
    PackedDrawCallData packedDrawCallData = _packedDrawCallDatas[drawCallID];
    
    DrawCallData drawCallData;
    
    drawCallData.instanceID = packedDrawCallData.instanceID;
    drawCallData.cullingDataID = packedDrawCallData.cullingDataID;
    
    drawCallData.textureUnitOffset = packedDrawCallData.packed & 0xFFFF;
    drawCallData.numTextureUnits = (packedDrawCallData.packed >> 16) && 0xFFFF;
    
    drawCallData.renderPriority = packedDrawCallData.renderPriority;
    
    return drawCallData;
}