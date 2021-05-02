permutation COLOR_PASS = [0, 1];

#include "globalData.inc.hlsl"
#include "mapObject.inc.hlsl"

struct InstanceData
{
    float4x4 instanceMatrix;
};

struct PackedVertex
{
    uint data0; // half positionX, half positionY
    uint data1; // half positionZ, uint8_t octNormalX, uint8_t octNormalY
    uint data2; // half uvX, half uvY
    uint data3; // half uvZ, half uvW
}; // 16 bytes

struct Vertex
{
    float3 position;
    float4 uv;

#if COLOR_PASS
    float3 normal;
    float4 color0;
    float4 color1;
#endif
};

[[vk::binding(1, PER_PASS)]] StructuredBuffer<PackedVertex> _packedVertices;
[[vk::binding(2, PER_PASS)]] StructuredBuffer<InstanceData> _instanceData;
#if COLOR_PASS
[[vk::binding(7, PER_PASS)]] Texture2D<float4> _textures[4096]; // This binding needs to stay up to date with the one in mapObject.ps.hlsl or we're gonna have a baaaad time
#endif

float3 UnpackPosition(PackedVertex packedVertex)
{
    float3 position;
    
    position.x = f16tof32(packedVertex.data0);
    position.y = f16tof32(packedVertex.data0 >> 16);
    position.z = f16tof32(packedVertex.data1);
    
    return position;
}

float3 OctNormalDecode(float2 f)
{
    f = f * 2.0 - 1.0;
 
    // https://twitter.com/Stubbesaurus/status/937994790553227264
    float3 n = float3( f.x, f.y, 1.0 - abs( f.x ) - abs( f.y ) );
    float t = saturate( -n.z );
    n.xy += n.xy >= 0.0 ? -t : t;
    return normalize( n );
}

float3 UnpackNormal(PackedVertex packedVertex)
{
    uint x = (packedVertex.data1 >> 16) & 0xFF;
    uint y = packedVertex.data1 >> 24;
    
    float2 octNormal = float2(x, y) / 255.0f;
    return OctNormalDecode(octNormal);
}

float4 UnpackUVs(PackedVertex packedVertex)
{
    float4 uvs;
    
    uvs.x = f16tof32(packedVertex.data2);
    uvs.y = f16tof32(packedVertex.data2 >> 16);
    uvs.z = f16tof32(packedVertex.data3);
    uvs.w = f16tof32(packedVertex.data3 >> 16);

    return uvs;
}

Vertex UnpackVertex(PackedVertex packedVertex)
{
    Vertex vertex;
    vertex.position = UnpackPosition(packedVertex);
    vertex.uv = UnpackUVs(packedVertex);

#if COLOR_PASS
    vertex.normal = UnpackNormal(packedVertex);
#endif
    
    return vertex;
}

Vertex LoadVertex(uint vertexID, uint vertexColor1Offset, uint vertexColor2Offset, uint vertexColorTextureID0, uint vertexColorTextureID1, uint vertexMeshOffset)
{
    PackedVertex packedVertex = _packedVertices[vertexID];
    
    Vertex vertex = UnpackVertex(packedVertex);

#if COLOR_PASS
    uint offsetVertex = vertexID - vertexMeshOffset;

    bool hasVertexColor1 = vertexColor1Offset != 0xffffffff;
    {
        uint offsetVertexID1 = (offsetVertex + vertexColor1Offset) * hasVertexColor1;
        uint3 vertexColorUV1 = uint3((float)offsetVertexID1 % 1024.0f, (float)offsetVertexID1 / 1024.0f, 0);

        vertex.color0 = _textures[NonUniformResourceIndex(vertexColorTextureID0)].Load(vertexColorUV1) * float4(hasVertexColor1, hasVertexColor1, hasVertexColor1, 1.0f);
    }

    bool hasVertexColor2 = vertexColor2Offset != 0xffffffff;
    {
        uint offsetVertexID2 = (offsetVertex + vertexColor2Offset) * hasVertexColor2;
        uint3 vertexColorUV2 = uint3((float)offsetVertexID2 % 1024.0f, (float)offsetVertexID2 / 1024.0f, 0);

        vertex.color1 = _textures[NonUniformResourceIndex(vertexColorTextureID1)].Load(vertexColorUV2) * float4(hasVertexColor2, hasVertexColor2, hasVertexColor2, 1.0f);
    }
#endif

    return vertex;
}

struct VSInput
{
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct VSOutput
{
    float4 position : SV_Position;
    uint materialParamID : TEXCOORD0;
    float4 uv01 : TEXCOORD1;
#if COLOR_PASS
    float3 normal : TEXCOORD2;
    float4 color0 : TEXCOORD3;
    float4 color1 : TEXCOORD4;
    uint instanceLookupID : TEXCOORD5;
#endif
};

VSOutput main(VSInput input)
{
    VSOutput output;

    InstanceLookupData lookupData = LoadInstanceLookupData(input.instanceID);
    
    uint instanceID = lookupData.instanceID;
    uint vertexColorTextureID0 = lookupData.vertexColorTextureID0;
    uint vertexColorTextureID1 = lookupData.vertexColorTextureID1;
    uint vertexOffset = lookupData.vertexOffset;
    uint materialParamID = lookupData.materialParamID;

    InstanceData instanceData = _instanceData[instanceID];
    Vertex vertex = LoadVertex(input.vertexID, lookupData.vertexColor1Offset, lookupData.vertexColor2Offset, vertexColorTextureID0, vertexColorTextureID1, vertexOffset);

    float4 position = float4(vertex.position, 1.0f);
    position = mul(position, instanceData.instanceMatrix);

    output.position = mul(position, _viewData.viewProjectionMatrix);
    output.materialParamID = materialParamID;
    output.uv01 = vertex.uv;

#if COLOR_PASS
    output.normal = mul(vertex.normal, (float3x3)instanceData.instanceMatrix);
    output.color0 = vertex.color0;
    output.color1 = vertex.color1;
    output.instanceLookupID = input.instanceID;
#endif

    return output;
}