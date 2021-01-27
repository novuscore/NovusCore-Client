
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