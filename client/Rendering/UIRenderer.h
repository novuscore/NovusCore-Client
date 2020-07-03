#pragma once
#include <NovusTypes.h>

#include <Renderer/Descriptors/ImageDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include <Renderer/Descriptors/SamplerDesc.h>
#include <Renderer/ConstantBuffer.h>

namespace Renderer
{
    class RenderGraph;
    class Renderer;
}

class Window;
class Keybind;
class UIRenderer
{
public:
    UIRenderer(Renderer::Renderer* renderer);

    void ClearWidgets();

    void DestroyWidget(entt::entity entId);

    void Update(f32 deltaTime);
    void AddUIPass(Renderer::RenderGraph* renderGraph, Renderer::ImageID renderTarget, u8 frameIndex);

private:
    bool OnMouseClick(Window* window, std::shared_ptr<Keybind> keybind);
    void OnMousePositionUpdate(Window* window, f32 x, f32 y);
    bool OnKeyboardInput(Window* window, i32 key, i32 actionMask, i32 modifierMask);
    bool OnCharInput(Window* window, u32 unicodeKey);

    void CreatePermanentResources();

    // Helper functions
    Renderer::TextureID ReloadTexture(const std::string& texturePath);
    void CalculateVertices(const vec3& pos, const vec2& size, std::vector<Renderer::Vertex>& vertices);

private:
    Renderer::Renderer* _renderer;

    Renderer::SamplerID _linearSampler;

    entt::entity _focusedWidget;
};