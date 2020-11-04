
[[vk::binding(0, PER_PASS)]] ByteAddressBuffer _drawCallDatas;

struct DrawCallData
{
    uint instanceID;
    uint16_t textureUnitOffset;
    uint16_t numTextureUnits;
};

DrawCallData LoadDrawCallData(uint drawCallID)
{
    DrawCallData drawCallData = _drawCallDatas.Load<DrawCallData>(drawCallID * 8); // 8 = sizeof(DrawCallData)
    
    return drawCallData;
}