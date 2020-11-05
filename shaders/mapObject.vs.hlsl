#include "globalData.inc.hlsl"

[[vk::binding(0, PER_PASS)]] ByteAddressBuffer _vertices;
[[vk::binding(1, PER_PASS)]] ByteAddressBuffer _instanceData;
[[vk::binding(2, PER_PASS)]] ByteAddressBuffer _instanceLookup;
[[vk::binding(6, PER_PASS)]] Texture2D<float4> _textures[4096]; // This binding needs to stay up to date with the one in mapObject.ps.hlsl or we're gonna have a baaaad time

struct InstanceLookupData
{
    uint16_t instanceID;
    uint16_t materialParamID;
    uint16_t cullingDataID;
    uint16_t vertexColorTextureID0;
    uint16_t vertexColorTextureID1;
    uint16_t padding1;
    uint vertexOffset;
};

struct InstanceData
{
    float4x4 instanceMatrix;
};

struct PackedVertex
{
    uint data0;
    uint data1;
    uint data2;
    uint data3;
};

struct Vertex
{
    float3 position;
    float3 normal;
    float4 color0;
    float4 color1;
    float4 uv;
};


struct VSInput
{
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float4 color0 : TEXCOORD1;
    float4 color1 : TEXCOORD2;
    float4 uv01 : TEXCOORD3;
    uint materialParamID : TEXCOORD4;
};

InstanceData LoadInstanceData(uint instanceID)
{
    InstanceData instanceData;

    instanceData = _instanceData.Load<InstanceData>(instanceID * 64); // 64 = sizeof(InstanceData)

    return instanceData;
}

// PackedVertex is packed like this:
// data0
//  half positionX
//  half positionY
// data1
//  half positionZ
//  u8 octNormalX
//  u8 octNormalY
// data2
//  half uvX
//  half uvY
// data3
//  half uvZ
//  half uvW


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
    vertex.normal = UnpackNormal(packedVertex);
    vertex.uv = UnpackUVs(packedVertex);
    
    return vertex;
}

Vertex LoadVertex(uint vertexID, uint vertexColorTextureID0, uint vertexColorTextureID1, uint vertexOffset)
{
    PackedVertex packedVertex = _vertices.Load<PackedVertex>(vertexID * 16);
    
    Vertex vertex = UnpackVertex(packedVertex);

    uint offsetVertexID = vertexID - vertexOffset;
    uint3 vertexColorUV = uint3(offsetVertexID % 1024, offsetVertexID / 1024, 0);

    /*if (vertexOffset > vertexID)
    {
        vertex.color0 = float4(1.0f, 0.0f, 0.0f, 1.0f);
        vertex.color1 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    else
    {
        vertex.color0 = float4(0.0f, 1.0f, 0.0f, 1.0f);
        vertex.color1 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }*/

    vertex.color0 = _textures[vertexColorTextureID0].Load(vertexColorUV);
    vertex.color1 = _textures[vertexColorTextureID1].Load(vertexColorUV);

    return vertex;
}

VSOutput main(VSInput input)
{
    VSOutput output;

    InstanceLookupData lookupData = _instanceLookup.Load<InstanceLookupData>(input.instanceID * 16); // 16 = sizeof(InstanceLookupData)
    
    uint instanceID = lookupData.instanceID;
    uint vertexColorTextureID0 = lookupData.vertexColorTextureID0;
    uint vertexColorTextureID1 = lookupData.vertexColorTextureID1;
    uint vertexOffset = lookupData.vertexOffset;
    uint materialParamID = lookupData.materialParamID;

    InstanceData instanceData = LoadInstanceData(instanceID);
    Vertex vertex = LoadVertex(input.vertexID, vertexColorTextureID0, vertexColorTextureID1, vertexOffset);

    float4 position = float4(vertex.position, 1.0f);
    position = mul(position, instanceData.instanceMatrix);

    output.position = mul(position, _viewData.viewProjectionMatrix);
    output.normal = mul(vertex.normal, (float3x3)instanceData.instanceMatrix);
    output.materialParamID = materialParamID;
    output.color0 = vertex.color0;
    output.color1 = vertex.color1;

    output.uv01 = vertex.uv;

    return output;
}