#include "LoadTexturesSystem.h"
#include <entity/registry.hpp>
#include <tracy/Tracy.hpp>
#include "../../../Utils/ServiceLocator.h"
#include "../../render-lib/Renderer/Renderer.h"

#include "../Components/Image.h"
#include "../Components/Text.h"
#include "../Components/Dirty.h"


namespace UISystem
{
    void LoadTexturesSystem::Update(entt::registry& registry)
    {
        Renderer::Renderer* renderer = ServiceLocator::GetRenderer();

        auto imageView = registry.view<UIComponent::Image, UIComponent::Dirty>();
        imageView.each([&](UIComponent::Image& image)
        {
            ZoneScopedNC("LoadTexturesSystem::Update::ImageView", tracy::Color::RoyalBlue);
            if (image.style.texture.length() == 0)
                return;

            image.textureID = renderer->LoadTexture(Renderer::TextureDesc{ image.style.texture });
            
            if (!image.style.borderTexture.empty())
            {
                ZoneScopedNC("(Re)load Border", tracy::Color::RoyalBlue);
                image.borderID = renderer->LoadTexture(Renderer::TextureDesc{ image.style.borderTexture });
            }
        });

        auto textView = registry.view<UIComponent::Text, UIComponent::Dirty>();
        textView.each([&](UIComponent::Text& text)
        {
            ZoneScopedNC("UpdateRenderingSystem::Update::TextView", tracy::Color::SkyBlue);
            if (text.style.fontPath.length() == 0)
                return;
            
            text.font = Renderer::Font::GetFont(renderer, text.style.fontPath, text.style.fontSize);
        });
    }
}