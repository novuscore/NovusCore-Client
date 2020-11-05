#include "globalData.inc.hlsl"
#include "cModel.inc.hlsl"

[[vk::binding(1, PER_PASS)]] ByteAddressBuffer _vertices;
[[vk::binding(2, PER_PASS)]] ByteAddressBuffer _instances;

struct InstanceData
{
    float4x4 instanceMatrix;
};

struct PackedVertex
{
    uint data0; // half positionX, half positionY
    uint data1; // half positionZ, u8 octNormal[2]
    uint data2; // half uv0X, half uv0Y
    uint data3; // half uv1X, half uv1Y
};

struct Vertex
{
    float3 position;
    float3 normal;
    float4 uv01;
};

struct VSInput
{
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct VSOutput
{
    float4 position : SV_Position;
    uint drawCallID : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float4 uv01 : TEXCOORD2;
};

InstanceData LoadInstanceData(uint instanceID)
{
    InstanceData instanceData;

    instanceData = _instances.Load<InstanceData>(instanceID * 64); // 64 = sizeof(InstanceData)

    return instanceData;
}

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

Vertex LoadVertex(uint vertexID)
{
    PackedVertex packedVertex = _vertices.Load<PackedVertex>(vertexID * 16); // 48 = sizeof(PackedVertex)
    
    Vertex vertex;
    vertex.position = UnpackPosition(packedVertex);
    vertex.normal = UnpackNormal(packedVertex);
    vertex.uv01 = UnpackUVs(packedVertex);

    return vertex;
}

VSOutput main(VSInput input)
{
    uint drawCallID = input.instanceID;
    
    DrawCallData drawCallData = LoadDrawCallData(drawCallID);
    
    InstanceData instanceData = LoadInstanceData(drawCallData.instanceID);
    Vertex vertex = LoadVertex(input.vertexID); 

    float4 position = float4(vertex.position, 1.0f);
    position = mul(position, instanceData.instanceMatrix);
    
    VSOutput output;
    output.drawCallID = drawCallID;
    output.position = mul(position, _viewData.viewProjectionMatrix);
    output.normal = mul(vertex.normal, (float3x3)instanceData.instanceMatrix);
    output.uv01 = vertex.uv01;

    return output;
}