#include "DebugRenderer.h"

#include <Renderer/Renderer.h>
#include <Renderer/CommandList.h>

#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

DebugRenderer::DebugRenderer(Renderer::Renderer* renderer)
{
	_renderer = renderer;

	Renderer::BufferDesc bufferDesc;
	bufferDesc.name = "DebugVertexBuffer";
	bufferDesc.size = 128 * 1024 * 1024;
	bufferDesc.usage = Renderer::BUFFER_USAGE_VERTEX_BUFFER | Renderer::BUFFER_USAGE_TRANSFER_DESTINATION;
	_debugVertexBuffer = _renderer->CreateBuffer(bufferDesc);
}

void DebugRenderer::Flush(Renderer::CommandList* commandList)
{
	size_t totalVertexCount = 0;
	for (size_t i = 0; i < DBG_VERTEX_BUFFER_COUNT; ++i)
	{
		const auto& vertices = _debugVertices[i];
		_debugVertexOffset[i] = static_cast<uint32_t>(totalVertexCount);
		_debugVertexCount[i] = static_cast<uint32_t>(vertices.size());
		totalVertexCount += vertices.size();
	}

	const size_t totalBufferSize = totalVertexCount * sizeof(DebugVertex);

	if (totalBufferSize == 0)
	{
		return;
	}

	Renderer::BufferDesc stagingBufferDesc;
	stagingBufferDesc.name = "DebugVertexUploadBuffer";
	stagingBufferDesc.size = totalBufferSize;
	stagingBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
	stagingBufferDesc.usage = Renderer::BUFFER_USAGE_TRANSFER_SOURCE;

	Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(stagingBufferDesc);
	_renderer->QueueDestroyBuffer(stagingBuffer);

	void* mappedMemory = _renderer->MapBuffer(stagingBuffer);

	for (size_t i = 0; i < DBG_VERTEX_BUFFER_COUNT; ++i)
	{
		const auto& vertices = _debugVertices[i];
		const uint32_t offset = _debugVertexOffset[i] * sizeof(DebugVertex);
		const uint32_t size = _debugVertexCount[i] * sizeof(DebugVertex);
		if (size > 0)
		{
			memcpy((char*)mappedMemory + offset, vertices.data(), size);
		}
	}

	_renderer->UnmapBuffer(stagingBuffer);
	commandList->CopyBuffer(_debugVertexBuffer, 0, stagingBuffer, 0, totalBufferSize);

	commandList->PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToVertexBuffer, _debugVertexBuffer);

	for (auto&& vertices : _debugVertices)
	{
		vertices.clear();
	}
}

void DebugRenderer::Add2DPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex)
{
	struct Debug2DPassData
	{
		Renderer::RenderPassMutableResource mainColor;
	};
	renderGraph->AddPass<Debug2DPassData>("DebugRender2D",
		[=](Debug2DPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
		{
			data.mainColor = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_LOAD);

			return true;// Return true from setup to enable this pass, return false to disable it
		},
		[=](Debug2DPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) // Execute
		{
			GPU_SCOPED_PROFILER_ZONE(commandList, DebugRender2D);

			Renderer::GraphicsPipelineDesc pipelineDesc;
			resources.InitializePipelineDesc(pipelineDesc);

			// Rasterizer state
			pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK;

			// Render targets.
			pipelineDesc.renderTargets[0] = data.mainColor;

			// Shader
			Renderer::VertexShaderDesc vertexShaderDesc;
			vertexShaderDesc.path = "Data/shaders/debug2D.vs.hlsl.spv";

			Renderer::PixelShaderDesc pixelShaderDesc;
			pixelShaderDesc.path = "Data/shaders/debug2D.ps.hlsl.spv";

			pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);
			pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

			// Input layouts TODO: Improve on this, if I set state 0 and 3 it won't work etc... Maybe responsibility for this should be moved to ModelHandler and the cooker?
			pipelineDesc.states.inputLayouts[0].enabled = true;
			pipelineDesc.states.inputLayouts[0].SetName("Position");
			pipelineDesc.states.inputLayouts[0].format = Renderer::InputFormat::INPUT_FORMAT_R32G32B32_FLOAT;
			pipelineDesc.states.inputLayouts[0].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_VERTEX;
			pipelineDesc.states.inputLayouts[0].alignedByteOffset = 0;

			pipelineDesc.states.inputLayouts[1].enabled = true;
			pipelineDesc.states.inputLayouts[1].SetName("Color");
			pipelineDesc.states.inputLayouts[1].format = Renderer::InputFormat::INPUT_FORMAT_R8G8B8A8_UNORM;
			pipelineDesc.states.inputLayouts[1].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_VERTEX;
			pipelineDesc.states.inputLayouts[1].alignedByteOffset = 12;

			pipelineDesc.states.primitiveTopology = Renderer::PrimitiveTopology::Lines;

			// Set pipeline
			Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
			commandList.BeginPipeline(pipeline);

			commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);
			commandList.SetVertexBuffer(0, _debugVertexBuffer);

			// Draw
			commandList.Draw(_debugVertexCount[DBG_VERTEX_BUFFER_LINES_2D], 1, _debugVertexOffset[DBG_VERTEX_BUFFER_LINES_2D], 0);

			commandList.EndPipeline(pipeline);
		});
}

void DebugRenderer::Add3DPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex)
{
	struct Debug3DPassData
	{
		Renderer::RenderPassMutableResource mainColor;
		Renderer::RenderPassMutableResource mainDepth;
	};
	renderGraph->AddPass<Debug3DPassData>("DebugRender3D",
		[=](Debug3DPassData& data, Renderer::RenderGraphBuilder& builder) // Setup
		{
			data.mainColor = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_LOAD);
			data.mainDepth = builder.Write(depthTarget, Renderer::RenderGraphBuilder::WriteMode::WRITE_MODE_RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD_MODE_LOAD);

			return true;// Return true from setup to enable this pass, return false to disable it
		},
		[=](Debug3DPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) // Execute
		{
			GPU_SCOPED_PROFILER_ZONE(commandList, DebugRender3D);

			Renderer::GraphicsPipelineDesc pipelineDesc;
			resources.InitializePipelineDesc(pipelineDesc);

			Flush(&commandList);

			// Shader
			Renderer::VertexShaderDesc vertexShaderDesc;
			vertexShaderDesc.path = "Data/shaders/debug3D.vs.hlsl.spv";

			Renderer::PixelShaderDesc pixelShaderDesc;
			pixelShaderDesc.path = "Data/shaders/debug3D.ps.hlsl.spv";

			pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);
			pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

			// Input layouts TODO: Improve on this, if I set state 0 and 3 it won't work etc... Maybe responsibility for this should be moved to ModelHandler and the cooker?
			pipelineDesc.states.inputLayouts[0].enabled = true;
			pipelineDesc.states.inputLayouts[0].SetName("Position");
			pipelineDesc.states.inputLayouts[0].format = Renderer::InputFormat::INPUT_FORMAT_R32G32B32_FLOAT;
			pipelineDesc.states.inputLayouts[0].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_VERTEX;
			pipelineDesc.states.inputLayouts[0].alignedByteOffset = 0;

			pipelineDesc.states.inputLayouts[1].enabled = true;
			pipelineDesc.states.inputLayouts[1].SetName("Color");
			pipelineDesc.states.inputLayouts[1].format = Renderer::InputFormat::INPUT_FORMAT_R8G8B8A8_UNORM;
			pipelineDesc.states.inputLayouts[1].inputClassification = Renderer::InputClassification::INPUT_CLASSIFICATION_PER_VERTEX;
			pipelineDesc.states.inputLayouts[1].alignedByteOffset = 12;

			pipelineDesc.states.primitiveTopology = Renderer::PrimitiveTopology::Lines;

			// Depth state
			pipelineDesc.states.depthStencilState.depthEnable = true;
			pipelineDesc.states.depthStencilState.depthWriteEnable = false;
			pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::COMPARISON_FUNC_LESS;

			// Rasterizer state
			pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::CULL_MODE_BACK;
			pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::FRONT_FACE_STATE_COUNTERCLOCKWISE;

			pipelineDesc.renderTargets[0] = data.mainColor;

			pipelineDesc.depthStencil = data.mainDepth;

			// Set pipeline
			Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
			commandList.BeginPipeline(pipeline);

			commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);
			commandList.SetVertexBuffer(0, _debugVertexBuffer);

			// Draw
			commandList.Draw(_debugVertexCount[DBG_VERTEX_BUFFER_LINES_3D], 1, _debugVertexOffset[DBG_VERTEX_BUFFER_LINES_3D], 0);

			commandList.EndPipeline(pipeline);
		});
}

void DebugRenderer::DrawLine2D(const glm::vec2& from, const glm::vec2& to, uint32_t color)
{
	_debugVertices[DBG_VERTEX_BUFFER_LINES_2D].push_back({ glm::vec3(from, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_2D].push_back({ glm::vec3(to, 0.0f), color });
}

void DebugRenderer::DrawLine3D(const glm::vec3& from, const glm::vec3& to, uint32_t color)
{
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ from, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ to, color });
}

void DebugRenderer::DrawAABB3D(const vec3& v0, const vec3& v1, uint32_t color)
{
	// Bottom
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v0.x, v0.y, v0.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v1.x, v0.y, v0.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v1.x, v0.y, v0.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v1.x, v0.y, v1.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v1.x, v0.y, v1.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v0.x, v0.y, v1.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v0.x, v0.y, v1.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v0.x, v0.y, v0.z }, color });

	// Top
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v0.x, v1.y, v0.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v1.x, v1.y, v0.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v1.x, v1.y, v0.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v1.x, v1.y, v1.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v1.x, v1.y, v1.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v0.x, v1.y, v1.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v0.x, v1.y, v1.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v0.x, v1.y, v0.z }, color });

	// Vertical edges
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v0.x, v0.y, v0.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v0.x, v1.y, v0.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v1.x, v0.y, v0.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v1.x, v1.y, v0.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v0.x, v0.y, v1.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v0.x, v1.y, v1.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v1.x, v0.y, v1.z }, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ { v1.x, v1.y, v1.z }, color });
}

void DebugRenderer::DrawTriangle2D(const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2, uint32_t color)
{
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(v0, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(v1, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(v2, 0.0f), color });
}

void DebugRenderer::DrawTriangle3D(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, uint32_t color)
{
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_3D].push_back({ v0, color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_3D].push_back({ v1, color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_3D].push_back({ v2, color });
}

void DebugRenderer::DrawRectangle2D(const glm::vec2& min, const glm::vec2& max, uint32_t color)
{
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(min.x, min.y, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(max.x, min.y, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(max.x, max.y, 0.0f), color });

	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(min.x, min.y, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(max.x, max.y, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(min.x, max.y, 0.0f), color });
}

__forceinline static vec3 unproject(const vec3& point, const mat4x4& m)
{
	vec4 obj = m * vec4(point, 1.0f);
	obj /= obj.w;
	return vec3(obj);
}

void DebugRenderer::DrawFrustum(const mat4x4& viewProjectionMatrix, uint32_t color)
{
	const mat4x4 m = glm::inverse(viewProjectionMatrix);

	vec4 viewport(0.0f, 0.0f, 640.0f, 360.0f);

	const vec3 near0 = unproject(vec3(-1.0f, -1.0f, 0.0f), m);
	const vec3 near1 = unproject(vec3(+1.0f, -1.0f, 0.0f), m);
	const vec3 near2 = unproject(vec3(+1.0f, +1.0f, 0.0f), m);
	const vec3 near3 = unproject(vec3(-1.0f, +1.0f, 0.0f), m);

	const vec3 far0 = unproject(vec3(-1.0f, -1.0f, 1.0f), m);
	const vec3 far1 = unproject(vec3(+1.0f, -1.0f, 1.0f), m);
	const vec3 far2 = unproject(vec3(+1.0f, +1.0f, 1.0f), m);
	const vec3 far3 = unproject(vec3(-1.0f, +1.0f, 1.0f), m);

	// Near plane
	DrawLine3D(near0, near1, color);
	DrawLine3D(near1, near2, color);
	DrawLine3D(near2, near3, color);
	DrawLine3D(near3, near0, color);

	// Far plane
	DrawLine3D(far0, far1, color);
	DrawLine3D(far1, far2, color);
	DrawLine3D(far2, far3, color);
	DrawLine3D(far3, far0, color);

	// Edges
	DrawLine3D(near0, far0, color);
	DrawLine3D(near1, far1, color);
	DrawLine3D(near2, far2, color);
	DrawLine3D(near3, far3, color);
}
