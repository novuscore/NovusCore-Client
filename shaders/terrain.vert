#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 0, binding = 0) uniform SharedUniformBufferObject 
{
    mat4 view;
    mat4 proj;
} sharedUbo;

layout(set = 1, binding = 0) uniform ModelUniformBufferObject 
{
	vec4 colorMultiplier;
    mat4 model;
} modelUbo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main()
{
    gl_Position = sharedUbo.proj * sharedUbo.view * modelUbo.model * vec4(inPosition, 1.0);
	fragTexCoord = inTexCoord;
}