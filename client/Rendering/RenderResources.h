#pragma once
#include "ViewConstantBuffer.h"
#include "LightConstantBuffer.h"

#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/DepthImageDesc.h>
#include <Renderer/DescriptorSet.h>
#include <Renderer/Buffer.h>

struct RenderResources
{
    Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer;
    Renderer::Buffer<LightConstantBuffer>* lightConstantBuffer;

    Renderer::DescriptorSet globalDescriptorSet;
    Renderer::DescriptorSet debugDescriptorSet;

    // Permanent resources
    Renderer::ImageID color;
    Renderer::ImageID objectIDs;
    Renderer::ImageID depthPyramid;
    Renderer::ImageID ambientObscurance;

    Renderer::DepthImageID depth;
};