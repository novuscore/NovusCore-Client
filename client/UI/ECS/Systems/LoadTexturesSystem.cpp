#include "LoadTexturesSystem.h"
#include <entity/registry.hpp>
#include <tracy/Tracy.hpp>
#include <Renderer/Renderer.h>
#include "../../../Utils/ServiceLocator.h"

#include "../Components/Image.h"
#include "../Components/Text.h"
#include "../Components/Dirty.h"
#include "../Components/NotCulled.h"


namespace UISystem
{
    void LoadTexturesSystem::Update(entt::registry& registry)
    {
        Renderer::Renderer* renderer = ServiceLocator::GetRenderer();

        auto imageView = registry.view<UIComponent::Image, UIComponent::Dirty, UIComponent::NotCulled>();
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

        auto textView = registry.view<UIComponent::Text, UIComponent::Dirty, UIComponent::NotCulled>();
        textView.each([&](UIComponent::Text& text)
        {
            ZoneScopedNC("UpdateRenderingSystem::Update::TextView", tracy::Color::SkyBlue);
            if (text.style.fontPath.length() == 0)
                return;
            
            text.font = Renderer::Font::GetFont(renderer, text.style.fontPath, text.style.fontSize);
        });
    }
}