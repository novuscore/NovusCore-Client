#pragma once
#include <Renderer/Descriptors/ModelDesc.h>

struct Model
{
    Renderer::ModelID modelId;
    Renderer::InstanceData instanceData;
    bool isVisible = false;
};