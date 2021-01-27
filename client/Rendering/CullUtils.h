#pragma once
#include <NovusTypes.h>

#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/DescriptorSet.h>

namespace Renderer
{
	class Renderer;
	class RenderGraphResources;
	class CommandList;
}
class DepthPyramidUtils
{
public:
	static void BuildPyramid(Renderer::Renderer* renderer, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList, u32 frameIndex,Renderer::DepthImageID depthImage, Renderer::ImageID pyramidImage);

	static Renderer::DescriptorSet _reduceDescriptorSet;
};