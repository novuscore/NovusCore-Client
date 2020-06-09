#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform SharedUniformBufferObject 
{
    vec4 color;
} panelUbo;

layout(set = 1, binding = 0) uniform sampler _sampler;
layout(set = 2, binding = 0) uniform texture2D _texture;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
	outColor = texture(sampler2D(_texture, _sampler), fragTexCoord);
	outColor *= panelUbo.color;
}