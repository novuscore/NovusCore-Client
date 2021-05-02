permutation COLOR_PASS = [0, 1];

#include "common.inc.hlsl"
#include "globalData.inc.hlsl"
#include "cModel.inc.hlsl"

struct PackedVertex
{
    uint packed0; // half positionX, half positionY
    uint packed1; // half positionZ, u8 octNormal[2]
    uint packed2; // half uv0X, half uv0Y
    uint packed3; // half uv1X, half uv1Y
    uint packed4; // bone indices (0..4)
    uint packed5; // bone weights (0..4)
}; // 24 bytes

struct Vertex
{
    float3 position;
    float4 uv01;
#if COLOR_PASS
    float3 normal;
#endif

    uint4 boneIndices;
    float4 boneWeights;
};

[[vk::binding(1, PER_PASS)]] StructuredBuffer<PackedVertex> _packedVertices;
[[vk::binding(2, PER_PASS)]] StructuredBuffer<InstanceData> _instances;
[[vk::binding(3, PER_PASS)]] StructuredBuffer<float4x4> _animationBoneDeformMatrix;

InstanceData LoadInstanceData(uint instanceID)
{
    InstanceData instanceData;

    instanceData = _instances[instanceID];

    return instanceData;
}

float3 UnpackPosition(PackedVertex packedVertex)
{
    float3 position;
    
    position.x = f16tof32(packedVertex.packed0);
    position.y = f16tof32(packedVertex.packed0 >> 16);
    position.z = f16tof32(packedVertex.packed1);
    
    return position;
}

float3 UnpackNormal(PackedVertex packedVertex)
{
    uint x = (packedVertex.packed1 >> 16) & 0xFF;
    uint y = packedVertex.packed1 >> 24;
    
    float2 octNormal = float2(x, y) / 255.0f;
    return OctNormalDecode(octNormal);
}

float4 UnpackUVs(PackedVertex packedVertex)
{
    float4 uvs;
    
    uvs.x = f16tof32(packedVertex.packed2);
    uvs.y = f16tof32(packedVertex.packed2 >> 16);
    uvs.z = f16tof32(packedVertex.packed3);
    uvs.w = f16tof32(packedVertex.packed3 >> 16);

    return uvs;
}

uint4 UnpackBoneIndices(PackedVertex packedVertex)
{
    uint4 boneIndices;

    boneIndices.x = packedVertex.packed4 & 0xFF;
    boneIndices.y = (packedVertex.packed4 >> 8) & 0xFF;
    boneIndices.z = (packedVertex.packed4 >> 16) & 0xFF;
    boneIndices.w = (packedVertex.packed4 >> 24) & 0xFF;

    return boneIndices;
}

float4 UnpackBoneWeights(PackedVertex packedVertex)
{
    float4 boneWeights;

    boneWeights.x = (float)(packedVertex.packed5 & 0xFF) / 255.f;
    boneWeights.y = (float)((packedVertex.packed5 >> 8) & 0xFF) / 255.f;
    boneWeights.z = (float)((packedVertex.packed5 >> 16) & 0xFF) / 255.f;
    boneWeights.w = (float)((packedVertex.packed5 >> 24) & 0xFF) / 255.f;

    return boneWeights;
}

Vertex LoadVertex(uint vertexID)
{
    PackedVertex packedVertex = _packedVertices[vertexID];
    
    Vertex vertex;
    vertex.position = UnpackPosition(packedVertex);
    vertex.uv01 = UnpackUVs(packedVertex);
#if COLOR_PASS
    vertex.normal = UnpackNormal(packedVertex);
#endif

    vertex.boneIndices = UnpackBoneIndices(packedVertex);
    vertex.boneWeights = UnpackBoneWeights(packedVertex);

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
    uint drawCallID : TEXCOORD0;
    float4 uv01 : TEXCOORD1;

#if COLOR_PASS
    float3 normal : TEXCOORD2;
#endif
};

VSOutput main(VSInput input)
{
    uint drawCallID = input.instanceID;
    Vertex vertex = LoadVertex(input.vertexID);

    DrawCallData drawCallData = LoadDrawCallData(drawCallID);
    InstanceData instanceData = LoadInstanceData(drawCallData.instanceID);

    float4x4 boneTransformMatrix = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

    if (instanceData.boneDeformOffset != 4294967295)
    {
        boneTransformMatrix = float4x4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

        for (int i = 0; i < 4; i++)
        {
            boneTransformMatrix += mul(vertex.boneWeights[i], _animationBoneDeformMatrix[instanceData.boneDeformOffset + vertex.boneIndices[i]]);
        }
    }

    float4 position = mul(float4(vertex.position, 1.0f), boneTransformMatrix);
    position = mul(float4(-position.x, -position.y, position.z, 1.0f), instanceData.instanceMatrix);
    
    VSOutput output;
    output.position = mul(position, _viewData.viewProjectionMatrix);
    output.drawCallID = drawCallID;
    output.uv01 = vertex.uv01;

#if COLOR_PASS
    output.normal = mul(vertex.normal, (float3x3)instanceData.instanceMatrix);
#endif

    return output;
}