#include "DebugRenderer.h"

#include <Renderer/Renderer.h>
#include <Renderer/RenderGraph.h>
#include <Renderer/CommandList.h>

#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

DebugRenderer::DebugRenderer(Renderer::Renderer* renderer)
{
	_renderer = renderer;

	const size_t debugVertexCounts[DBG_VERTEX_BUFFER_COUNT] = {
		4 * 1024,  // DBG_VERTEX_BUFFER_LINES_2D,
		32 * 1024, // DBG_VERTEX_BUFFER_LINES_3D,
		4 * 1024,  // DBG_VERTEX_BUFFER_TRIS_2D,
		32 * 1024, // DBG_VERTEX_BUFFER_TRIS_3D,
	};

	size_t totalVertexCount = 0;
	for (size_t i = 0; i < DBG_VERTEX_BUFFER_COUNT; ++i)
	{
		_debugVertexRanges[i] = uvec2(totalVertexCount, debugVertexCounts[i]);
		totalVertexCount += debugVertexCounts[i];
	}

	{
		Renderer::BufferDesc bufferDesc;
		bufferDesc.name = "DebugVertexBuffer";
		bufferDesc.size = totalVertexCount * sizeof(DebugVertex);
		bufferDesc.usage = Renderer::BufferUsage::VERTEX_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION | Renderer::BufferUsage::STORAGE_BUFFER;
		_debugVertexBuffer = _renderer->CreateBuffer(bufferDesc);
		
	}

	{
		Renderer::BufferDesc bufferDesc;
		bufferDesc.name = "DebugVertexRangeBuffer";
		bufferDesc.size = sizeof(uvec2) * DBG_VERTEX_BUFFER_COUNT;
		bufferDesc.usage = Renderer::BufferUsage::TRANSFER_DESTINATION | Renderer::BufferUsage::STORAGE_BUFFER;
		_debugVertexRangeBuffer = _renderer->CreateBuffer(bufferDesc);

		Renderer::BufferDesc stagingBufferDesc;
		stagingBufferDesc.name = "DebugRangeUploadBuffer";
		stagingBufferDesc.size = bufferDesc.size;
		stagingBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
		stagingBufferDesc.usage = Renderer::BufferUsage::TRANSFER_SOURCE;

		Renderer::BufferID stagingBuffer = _renderer->CreateBuffer(stagingBufferDesc);
		_renderer->QueueDestroyBuffer(stagingBuffer);

		void* mappedMemory = _renderer->MapBuffer(stagingBuffer);
		memcpy(mappedMemory, _debugVertexRanges, sizeof(_debugVertexRanges));
		_renderer->UnmapBuffer(stagingBuffer);
	}

	{
		Renderer::BufferDesc bufferDesc;
		bufferDesc.name = "DebugVertexCounterBuffer";
		bufferDesc.size = sizeof(u32) * DBG_VERTEX_BUFFER_COUNT;
		bufferDesc.usage = Renderer::BufferUsage::TRANSFER_DESTINATION | Renderer::BufferUsage::STORAGE_BUFFER;
		_debugVertexCounterBuffer = _renderer->CreateBuffer(bufferDesc);
	}

	{
		Renderer::BufferDesc bufferDesc;
		bufferDesc.name = "DebugDrawArgumentBuffer";
		bufferDesc.size = sizeof(VkDrawIndirectCommand) * DBG_VERTEX_BUFFER_COUNT;
		bufferDesc.usage = Renderer::BufferUsage::INDIRECT_ARGUMENT_BUFFER | Renderer::BufferUsage::TRANSFER_DESTINATION | Renderer::BufferUsage::STORAGE_BUFFER;
		_drawArgumentBuffer = _renderer->CreateBuffer(bufferDesc);
	}
}

static u32 GetDrawBufferOffset(DebugRenderer::DebugVertexBufferType bufferType)
{
	return bufferType * sizeof(VkDrawIndirectCommand);
}

void DebugRenderer::AddUploadPass(Renderer::RenderGraph* renderGraph)
{
	struct PassData
	{
	};

	renderGraph->AddPass<PassData>("DebugUpload", 
		[=](PassData& data, Renderer::RenderGraphBuilder& builder) -> bool
		{
			return true;
		},
		[=](PassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) -> void
		{
			u32 sourceVertexOffset[DBG_VERTEX_BUFFER_COUNT];
			u32 sourceVertexCount[DBG_VERTEX_BUFFER_COUNT];

			size_t totalSourceVertexCount = 0;
			for (size_t i = 0; i < DBG_VERTEX_BUFFER_COUNT; ++i)
			{
				const auto& vertices = _debugVertices[i];
				sourceVertexOffset[i] = static_cast<u32>(totalSourceVertexCount);
				sourceVertexCount[i] = static_cast<u32>(vertices.size());
				totalSourceVertexCount += vertices.size();
			}

			const size_t totalBufferSize = totalSourceVertexCount * sizeof(DebugVertex);

			if (totalBufferSize == 0)
			{
				return;
			}

			{
				Renderer::BufferDesc stagingBufferDesc;
				stagingBufferDesc.name = "DebugVertexUploadBuffer";
				stagingBufferDesc.size = totalBufferSize;
				stagingBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
				stagingBufferDesc.usage = Renderer::BufferUsage::TRANSFER_SOURCE;

				Renderer::BufferID vertexStagingBuffer = _renderer->CreateBuffer(stagingBufferDesc);
				_renderer->QueueDestroyBuffer(vertexStagingBuffer);

				void* vertexBufferMemory = _renderer->MapBuffer(vertexStagingBuffer);

				for (size_t i = 0; i < DBG_VERTEX_BUFFER_COUNT; ++i)
				{
					const auto& vertices = _debugVertices[i];
					const u32 targetOffset = _debugVertexRanges[i].x;
					const u32 sourceOffset = sourceVertexOffset[i] * sizeof(DebugVertex);
					const u32 size = sourceVertexCount[i] * sizeof(DebugVertex);
					if (size > 0)
					{
						memcpy((char*)vertexBufferMemory + sourceOffset, vertices.data(), size);
						commandList.CopyBuffer(_debugVertexBuffer, targetOffset, vertexStagingBuffer, sourceOffset, size);
					}
				}

				_renderer->UnmapBuffer(vertexStagingBuffer);
			}


			{
				Renderer::BufferDesc stagingBufferDesc;
				stagingBufferDesc.name = "DebugCounterUploadBuffer";
				stagingBufferDesc.size = DBG_VERTEX_BUFFER_COUNT * sizeof(u32);
				stagingBufferDesc.cpuAccess = Renderer::BufferCPUAccess::WriteOnly;
				stagingBufferDesc.usage = Renderer::BufferUsage::TRANSFER_SOURCE;

				Renderer::BufferID counterStagingBuffer = _renderer->CreateBuffer(stagingBufferDesc);
				_renderer->QueueDestroyBuffer(counterStagingBuffer);

				void* counterBufferMemory = _renderer->MapBuffer(counterStagingBuffer);
				memcpy(counterBufferMemory, sourceVertexCount, stagingBufferDesc.size);
				_renderer->UnmapBuffer(counterStagingBuffer);

				commandList.CopyBuffer(_debugVertexCounterBuffer, 0, counterStagingBuffer, 0, stagingBufferDesc.size);
			}

			commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToComputeShaderRW, _debugVertexBuffer);
			commandList.PipelineBarrier(Renderer::PipelineBarrierType::TransferDestToComputeShaderRW, _debugVertexCounterBuffer);

			for (auto&& vertices : _debugVertices)
			{
				vertices.clear();
			}
		});
}

void DebugRenderer::AddDrawArgumentPass(Renderer::RenderGraph* renderGraph, u8 frameIndex)
{
	struct PassData
	{
	};

	renderGraph->AddPass<PassData>("DebugArguments", 
		[=](PassData& data, Renderer::RenderGraphBuilder& builder) -> bool
		{
			return true;
		}, 
		[=](PassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) -> void
		{
			Renderer::ComputeShaderDesc shaderDesc;
			shaderDesc.path = "debugDrawArguments.cs.hlsl";

			Renderer::ComputePipelineDesc pipelineDesc;
			pipelineDesc.computeShader = _renderer->LoadShader(shaderDesc);

			const Renderer::ComputePipelineID pipeline = _renderer->CreatePipeline(pipelineDesc);

			_argumentsDescriptorSet.Bind("_vertexRanges"_h, _debugVertexRangeBuffer);
			_argumentsDescriptorSet.Bind("_vertexCounters"_h, _debugVertexCounterBuffer);
			_argumentsDescriptorSet.Bind("_drawArguments"_h, _drawArgumentBuffer);

			commandList.BeginPipeline(pipeline);
			commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::PER_DRAW, &_argumentsDescriptorSet, frameIndex);
			commandList.Dispatch(1, 1, 1);
			commandList.EndPipeline(pipeline);

			commandList.PipelineBarrier(Renderer::PipelineBarrierType::ComputeWriteToIndirectArguments, _drawArgumentBuffer);
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
			data.mainColor = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD);
			data.mainDepth = builder.Write(depthTarget, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD);

			return true;// Return true from setup to enable this pass, return false to disable it
		},
		[=](Debug3DPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) // Execute
		{
			GPU_SCOPED_PROFILER_ZONE(commandList, DebugRender3D);

			Renderer::GraphicsPipelineDesc pipelineDesc;
			resources.InitializePipelineDesc(pipelineDesc);

			{
				
			}

			{
				// Shader
				Renderer::VertexShaderDesc vertexShaderDesc;
				vertexShaderDesc.path = "debug3D.vs.hlsl";

				Renderer::PixelShaderDesc pixelShaderDesc;
				pixelShaderDesc.path = "debug3D.ps.hlsl";

				pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);
				pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

				// Input layouts TODO: Improve on this, if I set state 0 and 3 it won't work etc... Maybe responsibility for this should be moved to ModelHandler and the cooker?
				pipelineDesc.states.inputLayouts[0].enabled = true;
				pipelineDesc.states.inputLayouts[0].SetName("Position");
				pipelineDesc.states.inputLayouts[0].format = Renderer::InputFormat::R32G32B32_FLOAT;
				pipelineDesc.states.inputLayouts[0].inputClassification = Renderer::InputClassification::PER_VERTEX;
				pipelineDesc.states.inputLayouts[0].alignedByteOffset = 0;

				pipelineDesc.states.inputLayouts[1].enabled = true;
				pipelineDesc.states.inputLayouts[1].SetName("Color");
				pipelineDesc.states.inputLayouts[1].format = Renderer::InputFormat::R8G8B8A8_UNORM;
				pipelineDesc.states.inputLayouts[1].inputClassification = Renderer::InputClassification::PER_VERTEX;
				pipelineDesc.states.inputLayouts[1].alignedByteOffset = 12;

				pipelineDesc.states.primitiveTopology = Renderer::PrimitiveTopology::Lines;

				// Depth state
				pipelineDesc.states.depthStencilState.depthEnable = true;
				pipelineDesc.states.depthStencilState.depthWriteEnable = false;
				pipelineDesc.states.depthStencilState.depthFunc = Renderer::ComparisonFunc::GREATER;

				// Rasterizer state
				pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::BACK;
				pipelineDesc.states.rasterizerState.frontFaceMode = Renderer::FrontFaceState::COUNTERCLOCKWISE;

				pipelineDesc.renderTargets[0] = data.mainColor;

				pipelineDesc.depthStencil = data.mainDepth;

				// Set pipeline
				Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
				commandList.BeginPipeline(pipeline);

				commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);
				commandList.SetVertexBuffer(0, _debugVertexBuffer);

				// Draw
				commandList.DrawIndirect(_drawArgumentBuffer, GetDrawBufferOffset(DBG_VERTEX_BUFFER_LINES_3D), 1);

				commandList.EndPipeline(pipeline);
			}
		});
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
			data.mainColor = builder.Write(renderTarget, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD);

			return true;// Return true from setup to enable this pass, return false to disable it
		},
		[=](Debug2DPassData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList) // Execute
		{
			GPU_SCOPED_PROFILER_ZONE(commandList, DebugRender2D);

			Renderer::GraphicsPipelineDesc pipelineDesc;
			resources.InitializePipelineDesc(pipelineDesc);

			// Rasterizer state
			pipelineDesc.states.rasterizerState.cullMode = Renderer::CullMode::BACK;

			// Render targets.
			pipelineDesc.renderTargets[0] = data.mainColor;

			// Shader
			Renderer::VertexShaderDesc vertexShaderDesc;
			vertexShaderDesc.path = "debug2D.vs.hlsl";

			Renderer::PixelShaderDesc pixelShaderDesc;
			pixelShaderDesc.path = "debug2D.ps.hlsl";

			pipelineDesc.states.vertexShader = _renderer->LoadShader(vertexShaderDesc);
			pipelineDesc.states.pixelShader = _renderer->LoadShader(pixelShaderDesc);

			// Input layouts TODO: Improve on this, if I set state 0 and 3 it won't work etc... Maybe responsibility for this should be moved to ModelHandler and the cooker?
			pipelineDesc.states.inputLayouts[0].enabled = true;
			pipelineDesc.states.inputLayouts[0].SetName("Position");
			pipelineDesc.states.inputLayouts[0].format = Renderer::InputFormat::R32G32B32_FLOAT;
			pipelineDesc.states.inputLayouts[0].inputClassification = Renderer::InputClassification::PER_VERTEX;
			pipelineDesc.states.inputLayouts[0].alignedByteOffset = 0;

			pipelineDesc.states.inputLayouts[1].enabled = true;
			pipelineDesc.states.inputLayouts[1].SetName("Color");
			pipelineDesc.states.inputLayouts[1].format = Renderer::InputFormat::R8G8B8A8_UNORM;
			pipelineDesc.states.inputLayouts[1].inputClassification = Renderer::InputClassification::PER_VERTEX;
			pipelineDesc.states.inputLayouts[1].alignedByteOffset = 12;

			pipelineDesc.states.primitiveTopology = Renderer::PrimitiveTopology::Lines;

			// Set pipeline
			Renderer::GraphicsPipelineID pipeline = _renderer->CreatePipeline(pipelineDesc); // This will compile the pipeline and return the ID, or just return ID of cached pipeline
			commandList.BeginPipeline(pipeline);

			//commandList.BindDescriptorSet(Renderer::DescriptorSetSlot::GLOBAL, globalDescriptorSet, frameIndex);
			commandList.SetVertexBuffer(0, _debugVertexBuffer);

			// Draw
			commandList.DrawIndirect(_drawArgumentBuffer, GetDrawBufferOffset(DBG_VERTEX_BUFFER_LINES_2D), 1);

			commandList.EndPipeline(pipeline);
		});
}

void DebugRenderer::DrawLine2D(const glm::vec2& from, const glm::vec2& to, uint32_t color)
{
	if (_debugVertices[DBG_VERTEX_BUFFER_LINES_2D].size() + 2 > _debugVertexRanges[DBG_VERTEX_BUFFER_LINES_2D].y)
	{
		DebugHandler::PrintError("Debug vertex buffer out of memory.");
		return;
	}

	_debugVertices[DBG_VERTEX_BUFFER_LINES_2D].push_back({ glm::vec3(from, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_2D].push_back({ glm::vec3(to, 0.0f), color });
}

void DebugRenderer::DrawLine3D(const glm::vec3& from, const glm::vec3& to, uint32_t color)
{
	if (_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].size() + 2 > _debugVertexRanges[DBG_VERTEX_BUFFER_LINES_3D].y)
	{
		DebugHandler::PrintError("Debug vertex buffer out of memory.");
		return;
	}

	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ from, color });
	_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].push_back({ to, color });
}

void DebugRenderer::DrawAABB3D(const vec3& v0, const vec3& v1, uint32_t color)
{
	if (_debugVertices[DBG_VERTEX_BUFFER_LINES_3D].size() + 24 > _debugVertexRanges[DBG_VERTEX_BUFFER_LINES_3D].y)
	{
		DebugHandler::PrintError("Debug vertex buffer out of memory.");
		return;
	}

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
	if (_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].size() + 3 > _debugVertexRanges[DBG_VERTEX_BUFFER_TRIS_2D].y)
	{
		DebugHandler::PrintError("Debug vertex buffer out of memory.");
		return;
	}

	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(v0, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(v1, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(v2, 0.0f), color });
}

void DebugRenderer::DrawTriangle3D(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, uint32_t color)
{
	if (_debugVertices[DBG_VERTEX_BUFFER_TRIS_3D].size() + 3 > _debugVertexRanges[DBG_VERTEX_BUFFER_TRIS_3D].y)
	{
		DebugHandler::PrintError("Debug vertex buffer out of memory.");
		return;
	}

	_debugVertices[DBG_VERTEX_BUFFER_TRIS_3D].push_back({ v0, color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_3D].push_back({ v1, color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_3D].push_back({ v2, color });
}

void DebugRenderer::DrawRectangle2D(const glm::vec2& min, const glm::vec2& max, uint32_t color)
{
	if (_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].size() + 6 > _debugVertexRanges[DBG_VERTEX_BUFFER_TRIS_2D].y)
	{
		DebugHandler::PrintError("Debug vertex buffer out of memory.");
		return;
	}

	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(min.x, min.y, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(max.x, min.y, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(max.x, max.y, 0.0f), color });

	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(min.x, min.y, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(max.x, max.y, 0.0f), color });
	_debugVertices[DBG_VERTEX_BUFFER_TRIS_2D].push_back({ glm::vec3(min.x, max.y, 0.0f), color });
}

vec3 DebugRenderer::UnProject(const vec3& point, const mat4x4& m)
{
	vec4 obj = m * vec4(point, 1.0f);
	obj /= obj.w;
	return vec3(obj);
}

void DebugRenderer::DrawFrustum(const mat4x4& viewProjectionMatrix, uint32_t color)
{
	const mat4x4 m = glm::inverse(viewProjectionMatrix);

	vec4 viewport(0.0f, 0.0f, 640.0f, 360.0f);

	const vec3 near0 = UnProject(vec3(-1.0f, -1.0f, 0.0f), m);
	const vec3 near1 = UnProject(vec3(+1.0f, -1.0f, 0.0f), m);
	const vec3 near2 = UnProject(vec3(+1.0f, +1.0f, 0.0f), m);
	const vec3 near3 = UnProject(vec3(-1.0f, +1.0f, 0.0f), m);

	const vec3 far0 = UnProject(vec3(-1.0f, -1.0f, 1.0f), m);
	const vec3 far1 = UnProject(vec3(+1.0f, -1.0f, 1.0f), m);
	const vec3 far2 = UnProject(vec3(+1.0f, +1.0f, 1.0f), m);
	const vec3 far3 = UnProject(vec3(-1.0f, +1.0f, 1.0f), m);

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
