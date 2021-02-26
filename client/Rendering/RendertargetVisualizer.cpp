#include "RendertargetVisualizer.h"
#include "RenderUtils.h"

#include <Renderer/Renderer.h>
#include <Renderer/RenderGraph.h>

#include <imgui/imgui.h>

RendertargetVisualizer::RendertargetVisualizer(Renderer::Renderer* renderer)
    : _renderer(renderer)
{
    CreatePermanentResources();
}

RendertargetVisualizer::~RendertargetVisualizer()
{

}

void RendertargetVisualizer::Update(f32 deltaTime)
{

}

void RendertargetVisualizer::DrawImgui()
{
    if (!_isVisible)
        return;

    if (ImGui::Begin("Rendertarget Visualizer", &_isVisible))
    {
        static std::string overridePreview = "NONE";
        static std::string overlayPreview = "NONE";

        if (ImGui::BeginCombo("Current override", overridePreview.c_str())) // The second parameter is the label previewed before opening the combo.
        {
            // NONE entry
            {
                bool isSelected = _overridingImageID == Renderer::ImageID::Invalid() && _overridingDepthImageID == Renderer::DepthImageID::Invalid();
                if (ImGui::Selectable("NONE", &isSelected))
                {
                    _overridingImageID = Renderer::ImageID::Invalid();
                    _overridingDepthImageID = Renderer::DepthImageID::Invalid();

                    overridePreview = "NONE";
                }
            }

            // Color images
            u32 numImages = _renderer->GetNumImages();
            for (u32 i = 0; i < numImages; i++)
            {
                Renderer::ImageID imageID = Renderer::ImageID(i);
                Renderer::ImageDesc desc = _renderer->GetImageDesc(imageID);

                // Don't display swapchain rendertargets
                if (StringUtils::BeginsWith(desc.debugName, "SwapChain"))
                {
                    continue;
                }

                // Don't display MainColor since that is what we copy into
                if (desc.debugName == "MainColor")
                {
                    continue;
                }

                bool isSelected = _overridingImageID == imageID;

                if (ImGui::Selectable(desc.debugName.c_str(), &isSelected))
                {
                    _overridingImageID = imageID;
                    _overridingDepthImageID = Renderer::DepthImageID::Invalid();

                    overridePreview = desc.debugName;

                    if (_overridingImageID == _overlayingImageID)
                    {
                        _overlayingImageID = Renderer::ImageID::Invalid();
                        overlayPreview = "NONE";
                    }
                }
            }

            // Depth images
            u32 numDepthImages = _renderer->GetNumDepthImages();
            for (u32 i = 0; i < numDepthImages; i++)
            {
                Renderer::DepthImageID depthImageID = Renderer::DepthImageID(i);
                Renderer::DepthImageDesc desc = _renderer->GetDepthImageDesc(depthImageID);

                bool isSelected = _overridingDepthImageID == depthImageID;

                if (ImGui::Selectable(desc.debugName.c_str(), &isSelected))
                {
                    _overridingImageID = Renderer::ImageID::Invalid();
                    _overridingDepthImageID = depthImageID;

                    overridePreview = desc.debugName;

                    if (_overridingDepthImageID == _overlayingDepthImageID)
                    {
                        _overlayingDepthImageID = Renderer::DepthImageID::Invalid();
                        overlayPreview = "NONE";
                    }
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Current overlay", overlayPreview.c_str())) // The second parameter is the label previewed before opening the combo.
        {
            // NONE entry
            {
                bool isSelected = _overlayingImageID == Renderer::ImageID::Invalid() && _overlayingDepthImageID == Renderer::DepthImageID::Invalid();
                if (ImGui::Selectable("NONE", &isSelected))
                {
                    _overlayingImageID = Renderer::ImageID::Invalid();
                    _overlayingDepthImageID = Renderer::DepthImageID::Invalid();

                    overlayPreview = "NONE";
                }
            }

            // Color images
            u32 numImages = _renderer->GetNumImages();
            for (u32 i = 0; i < numImages; i++)
            {
                Renderer::ImageID imageID = Renderer::ImageID(i);
                Renderer::ImageDesc desc = _renderer->GetImageDesc(imageID);

                // Don't display swapchain rendertargets
                if (StringUtils::BeginsWith(desc.debugName, "SwapChain"))
                {
                    continue;
                }

                // Don't display MainColor since that is what we copy into
                if (desc.debugName == "MainColor")
                {
                    continue;
                }

                bool isSelected = _overlayingImageID == imageID;

                if (ImGui::Selectable(desc.debugName.c_str(), &isSelected))
                {
                    _overlayingImageID = imageID;
                    _overlayingDepthImageID = Renderer::DepthImageID::Invalid();

                    overlayPreview = desc.debugName;

                    if (_overridingImageID == _overlayingImageID)
                    {
                        _overridingImageID = Renderer::ImageID::Invalid();
                        overridePreview = "NONE";
                    }
                }
            }

            // Depth images
            u32 numDepthImages = _renderer->GetNumDepthImages();
            for (u32 i = 0; i < numDepthImages; i++)
            {
                Renderer::DepthImageID depthImageID = Renderer::DepthImageID(i);
                Renderer::DepthImageDesc desc = _renderer->GetDepthImageDesc(depthImageID);

                bool isSelected = _overlayingDepthImageID == depthImageID;

                if (ImGui::Selectable(desc.debugName.c_str(), &isSelected))
                {
                    _overlayingImageID = Renderer::ImageID::Invalid();
                    _overlayingDepthImageID = depthImageID;

                    overlayPreview = desc.debugName;

                    if (_overridingDepthImageID == _overlayingDepthImageID)
                    {
                        _overridingDepthImageID = Renderer::DepthImageID::Invalid();
                        overridePreview = "NONE";
                    }
                }
            }

            ImGui::EndCombo();
        }

        // Controls
        ImGui::Spacing();
        ImGui::Text("Override controls");
        ImGui::Separator();

        ImGui::PushID("OverrideMip");
        if (ImGui::InputInt("Mip Level", &_selectedOverrideMip, 1, 1))
        {
            if (_selectedOverrideMip < 0)
                _selectedOverrideMip = 0;
        }
        ImGui::PopID();

        ImGui::Text("Overlay controls");
        ImGui::Separator();

        ImGui::InputFloat4("Color Multiplier", &_colorMultiplier.x, 3);
        ImGui::InputFloat4("Additive Color", &_additiveColor.x, 3);

        ImGui::PushID("OverlayMip");
        if (ImGui::InputInt("Mip Level", &_selectedOverlayMip, 1, 1))
        {
            if (_selectedOverlayMip < 0)
                _selectedOverlayMip = 0;
        }
        ImGui::PopID();
    }
    ImGui::End();
}

void RendertargetVisualizer::AddVisualizerPass(Renderer::RenderGraph* renderGraph, Renderer::DescriptorSet* globalDescriptorSet, Renderer::ImageID colorTarget, u8 frameIndex)
{
    struct RTVisualizerData
    {
        Renderer::RenderPassMutableResource target;
    };

    renderGraph->AddPass<RTVisualizerData>("RTVisualizer",
        [=](RTVisualizerData& data, Renderer::RenderGraphBuilder& builder)
        {
            data.target = builder.Write(colorTarget, Renderer::RenderGraphBuilder::WriteMode::RENDERTARGET, Renderer::RenderGraphBuilder::LoadMode::LOAD);

            return true;
        },
        [=](RTVisualizerData& data, Renderer::RenderGraphResources& resources, Renderer::CommandList& commandList)
        {
            GPU_SCOPED_PROFILER_ZONE(commandList, CModelPass);

            // Override
            if (_overridingImageID != Renderer::ImageID::Invalid())
            {
                RenderUtils::BlitParams blitParams;
                blitParams.input = _overridingImageID;
                blitParams.inputMipLevel = _selectedOverrideMip;
                
                blitParams.output = data.target;
                blitParams.sampler = _linearSampler;

                RenderUtils::Blit(_renderer, resources, commandList, frameIndex, blitParams);
            }
            else if (_overridingDepthImageID != Renderer::DepthImageID::Invalid())
            {
                RenderUtils::DepthBlitParams blitParams;
                blitParams.input = _overridingDepthImageID;

                blitParams.output = data.target;
                blitParams.sampler = _linearSampler;

                RenderUtils::DepthBlit(_renderer, resources, commandList, frameIndex, blitParams);
            }

            // Overlay
            if (_overlayingImageID != Renderer::ImageID::Invalid())
            {
                RenderUtils::OverlayParams overlayParams;
                overlayParams.overlayImage = _overlayingImageID;

                overlayParams.overlayMipLevel = _selectedOverlayMip;

                overlayParams.overlayColorMultiplier = _colorMultiplier;
                overlayParams.overlayAdditiveColor = _additiveColor;

                overlayParams.baseImage = data.target;
                overlayParams.sampler = _linearSampler;

                RenderUtils::Overlay(_renderer, resources, commandList, frameIndex, overlayParams);
            }
            else if (_overlayingDepthImageID != Renderer::DepthImageID::Invalid())
            {
                RenderUtils::DepthOverlayParams overlayParams;
                overlayParams.overlayImage = _overlayingDepthImageID;

                overlayParams.overlayColorMultiplier = _colorMultiplier;
                overlayParams.overlayAdditiveColor = _additiveColor;

                overlayParams.baseImage = data.target;
                overlayParams.sampler = _linearSampler;

                RenderUtils::DepthOverlay(_renderer, resources, commandList, frameIndex, overlayParams);
            }
        });
}

bool RendertargetVisualizer::GetOverridingImageID(Renderer::ImageID& imageID)
{
    if (_overridingImageID == Renderer::ImageID::Invalid())
        return false;

    imageID = _overridingImageID;

    return true;
}

bool RendertargetVisualizer::GetOverridingDepthImageID(Renderer::DepthImageID& imageID)
{
    if (_overridingDepthImageID == Renderer::DepthImageID::Invalid())
        return false;

    imageID = _overridingDepthImageID;

    return true;
}

void RendertargetVisualizer::CreatePermanentResources()
{
    Renderer::SamplerDesc samplerDesc;
    samplerDesc.filter = Renderer::SamplerFilter::MINIMUM_MIN_MAG_MIP_LINEAR;
    samplerDesc.addressU = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.addressV = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.addressW = Renderer::TextureAddressMode::CLAMP;
    samplerDesc.minLOD = 0.f;
    samplerDesc.maxLOD = 16.f;
    samplerDesc.mode = Renderer::SamplerReductionMode::MIN;

    _linearSampler = _renderer->CreateSampler(samplerDesc);
}
