#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

layout(set = 2, binding = 0) uniform sampler _sampler;
layout(set = 3, binding = 0) uniform texture2D _texture;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    //outColor = vec4(fragTexCoord, 0.0, 1.0); // Debug Texcoords
	outColor = texture(sampler2D(_texture, _sampler), fragTexCoord);
}