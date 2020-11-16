
[[vk::binding(0, PER_PASS)]] ByteAddressBuffer _drawCallDatas;

struct DrawCallData
{
    uint instanceID;
    uint cullingDataID;
    uint16_t textureUnitOffset;
    uint16_t numTextureUnits;
    uint renderPriority;
};

DrawCallData LoadDrawCallData(uint drawCallID)
{
    DrawCallData drawCallData = _drawCallDatas.Load<DrawCallData>(drawCallID * 16); // 16 = sizeof(DrawCallData)
    
    return drawCallData;
}