
struct InstanceData
{
    float4x4 instanceMatrix;

    uint modelId;
    uint activeSequenceId;
    float animProgress;
    uint boneDeformOffset;
};

struct AnimationModelInfo
{
    uint numBones; // Only 16 bit is used here, rest is padding
    uint boneInfoOffset;
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
};

struct AnimationTrackInfo
{
    uint sequenceIndex; // Only 16 bit is used here, rest is padding

    uint duration;

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