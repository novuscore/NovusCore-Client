#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension VK_EXT_descriptor_indexing : enable
//#extension GL_EXT_debug_printf : enable

layout(set = 0, binding = 0) uniform SharedUBO 
{
    mat4 view;
    mat4 proj;
} sharedUbo;

layout(set = 1, binding = 0) uniform ModelUBO 
{
	vec4 colorMultiplier;
    mat4 model;
} modelUbo;

struct Vertex
{
    vec4 position;
    vec4 texCoord;
};
layout(set = 2, binding = 0, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

layout(location = 0) in uint inInstanceID;

layout(location = 0) out uint fragInstanceID;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
    uint vertexID = gl_VertexIndex + (inInstanceID * 145); // 145 vertices per cell

    /*if (inInstanceID == 0)
    {
        debugPrintfEXT("VertexID: %u",vertexID);
    }*/

    vec3 position = vertices[vertexID].position.xyz;
    gl_Position = sharedUbo.proj * sharedUbo.view * modelUbo.model * vec4(position, 1.0);

	fragTexCoord = vertices[vertexID].texCoord.xy;
    fragInstanceID = inInstanceID;
}