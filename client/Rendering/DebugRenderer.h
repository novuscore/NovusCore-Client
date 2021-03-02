#pragma once

#include <NovusTypes.h>

#include <Renderer/DescriptorSet.h>

#include <Renderer/Descriptors/BufferDesc.h>
#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>

#include <vector>

namespace Renderer
{
	class Renderer;
	class RenderGraph;
	class CommandList;
};

class DebugRenderer
{
public:
	DebugRenderer(Renderer::Renderer* renderer);

	void AddUploadPass(Renderer::RenderGraph* renderGraph);
	void AddDrawArgumentPass(Renderer::RenderGraph* renderGraph, u8 frameIndex);
	void Add2DPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);
	void Add3DPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);

	void DrawLine2D(const vec2& from, const vec2& to, uint32_t color);
	void DrawLine3D(const vec3& from, const vec3& to, uint32_t color);
	void DrawAABB3D(const vec3& v0, const vec3& v1, uint32_t color);
	void DrawTriangle2D(const vec2& v0, const vec2& v1, const vec2& v2, uint32_t color);
	void DrawTriangle3D(const vec3& v0, const vec3& v1, const vec3& v2, uint32_t color);
	void DrawRectangle2D(const vec2& min, const vec2& max, uint32_t color);
	void DrawFrustum(const mat4x4& viewProjectionMatrix, uint32_t color);

	inline const Renderer::DescriptorSet* GetDescriptorSet() const
	{
		return &_descriptorSet;
	}

	static vec3 UnProject(const vec3& point, const mat4x4& m);

	enum DebugVertexBufferType
	{
		DBG_VERTEX_BUFFER_LINES_2D,
		DBG_VERTEX_BUFFER_LINES_3D,
		DBG_VERTEX_BUFFER_TRIS_2D,
		DBG_VERTEX_BUFFER_TRIS_3D,
		DBG_VERTEX_BUFFER_COUNT,
	};

private:
	struct DebugVertex
	{
		glm::vec3 pos;
		uint32_t color;
	};

	Renderer::Renderer* _renderer = nullptr;

	std::vector<DebugVertex> _debugVertices[DBG_VERTEX_BUFFER_COUNT];
	uvec2 _debugVertexRanges[DBG_VERTEX_BUFFER_COUNT]; // offset, count
	
	Renderer::DescriptorSet _descriptorSet;

	Renderer::DescriptorSet _argumentsDescriptorSet;
	
	Renderer::BufferID _debugVertexBuffer;
	Renderer::BufferID _debugVertexRangeBuffer;
	Renderer::BufferID _debugVertexCounterBuffer;
	Renderer::BufferID _drawArgumentBuffer;
};