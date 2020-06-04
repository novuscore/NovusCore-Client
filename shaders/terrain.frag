#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_separate_shader_objects : enable

// Textures
layout(set = 2, binding = 0) uniform sampler terrainSampler;
layout(set = 3, binding = 0) uniform texture2D terrainTextures[4096];

/*layout(set = 4, binding = 0) uniform ChunkData 
{
	int diffuseId;
} chunkData;*/

// From vertex shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main() 
{
	vec2 uv = fragTexCoord.xy / 8.0f;
	//outColor = texture(sampler2D(terrainTextures[chunkData.diffuseId], terrainSampler), uv);
	outColor = texture(sampler2D(terrainTextures[0], terrainSampler), uv);
}