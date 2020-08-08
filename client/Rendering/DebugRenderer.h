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

	void Flush(Renderer::CommandList* commandList);

	void Add2DPass(Renderer::RenderGraph* renderGraph, Renderer::BufferID viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);
	void Add3DPass(Renderer::RenderGraph* renderGraph, Renderer::BufferID viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex);

	void DrawLine2D(const vec2& from, const vec2& to, uint32_t color);
	void DrawLine3D(const vec3& from, const vec3& to, uint32_t color);
	void DrawTriangle2D(const vec2& v0, const vec2& v1, const vec2& v2, uint32_t color);
	void DrawTriangle3D(const vec3& v0, const vec3& v1, const vec3& v2, uint32_t color);
	void DrawRectangle2D(const vec2& min, const vec2& max, uint32_t color);

private:
	struct DebugVertex
	{
		glm::vec3 pos;
		uint32_t color;
	};

	enum DebugVertexBufferType
	{
		DBG_VERTEX_BUFFER_LINES_2D,
		DBG_VERTEX_BUFFER_LINES_3D,
		DBG_VERTEX_BUFFER_TRIS_2D,
		DBG_VERTEX_BUFFER_TRIS_3D,
		DBG_VERTEX_BUFFER_COUNT,
	};

	Renderer::Renderer* _renderer = nullptr;

	std::vector<DebugVertex> _debugVertices[DBG_VERTEX_BUFFER_COUNT];
	uint32_t _debugVertexOffset[DBG_VERTEX_BUFFER_COUNT];
	uint32_t _debugVertexCount[DBG_VERTEX_BUFFER_COUNT];

	Renderer::BufferID _debugVertexBuffer;

	Renderer::DescriptorSet _passDescriptorSet;
};