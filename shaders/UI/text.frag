#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform SharedUniformBufferObject 
{
	vec4 textColor;
    vec4 outlineColor;
	float outlineWidth;
} textUbo;

layout(set = 1, binding = 0) uniform sampler _sampler;
layout(set = 2, binding = 0) uniform texture2D _texture;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
	float distance = texture(sampler2D(_texture, _sampler), fragTexCoord).r;
	float smoothWidth = fwidth(distance);
	float alpha = smoothstep(0.5 - smoothWidth, 0.5 + smoothWidth, distance);
	vec3 rgb = vec3(alpha) * textUbo.textColor.rgb;

	if (textUbo.outlineWidth > 0.0)
	{
		float w = 1.0 - textUbo.outlineWidth;
		alpha = smoothstep(w - smoothWidth, w + smoothWidth, distance);
		rgb += mix(vec3(alpha), textUbo.outlineColor.rgb, alpha);
	}

	outColor = vec4(rgb, alpha);
}