
struct PackedInstanceLookupData
{
    uint packed0; // uint16_t instanceID, uint16_t materialParamID
    uint packed1; // uint16_t cullingDataID, uint16_t vertexColorTextureID0
    uint packed2; // uint16_t vertexColorTextureID1, uint16_t padding
    uint vertexOffset;
    uint vertexColor0Offset;
    uint vertexColor1Offset;
    uint loadedObjectID;
}; // 28 bytes

struct InstanceLookupData
{
    uint instanceID;
    uint materialParamID;
    uint cullingDataID;
    uint vertexColorTextureID0;
    uint vertexColorTextureID1;
    uint vertexOffset;
    uint vertexColor0Offset;
    uint vertexColor1Offset;
    uint loadedObjectID;
};

[[vk::binding(0, PER_PASS)]] StructuredBuffer<PackedInstanceLookupData> _packedInstanceLookup;

InstanceLookupData LoadInstanceLookupData(uint instanceLookupDataID)
{
    PackedInstanceLookupData packedInstanceLookupData = _packedInstanceLookup[instanceLookupDataID];
    
    InstanceLookupData instanceLookupData;
    
    instanceLookupData.instanceID = packedInstanceLookupData.packed0 & 0xFFFF;
    instanceLookupData.materialParamID = (packedInstanceLookupData.packed0 >> 16) & 0xFFFF;
    instanceLookupData.cullingDataID = packedInstanceLookupData.packed1 & 0xFFFF;
    instanceLookupData.vertexColorTextureID0 = (packedInstanceLookupData.packed1 >> 16) & 0xFFFF;
    instanceLookupData.vertexColorTextureID1 = packedInstanceLookupData.packed2 & 0xFFFF;
    instanceLookupData.vertexOffset = packedInstanceLookupData.vertexOffset;
    instanceLookupData.vertexColor0Offset = packedInstanceLookupData.vertexColor0Offset;
    instanceLookupData.vertexColor1Offset = packedInstanceLookupData.vertexColor1Offset;
    instanceLookupData.loadedObjectID = packedInstanceLookupData.loadedObjectID;
    
    return instanceLookupData;
}