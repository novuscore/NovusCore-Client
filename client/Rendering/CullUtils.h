#pragma once
#include <NovusTypes.h>

#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/DescriptorSet.h>
#include "RenderResources.h"

namespace Renderer
{
	class Renderer;
	class RenderGraphResources;
	class CommandList;
}
class DepthPyramidUtils
{
public:
	static void BuildPyramid(Renderer::Renderer* renderer, Renderer::RenderGraphResources& graphResources, Renderer::CommandList& commandList, RenderResources& resources, u32 frameIndex);

	static Renderer::DescriptorSet _reduceDescriptorSet;
};