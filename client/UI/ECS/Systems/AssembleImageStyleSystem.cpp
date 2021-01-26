#include "AssembleImageStyleSystem.h"
#include <entity/registry.hpp>
#include "../Components/Image.h"
#include "../Components/ImageEventStyles.h"
#include "../Components/TransformEvents.h"
#include "../Components/Dirty.h"


namespace UISystem
{
    void AssembleImageStyleSystem::Update(entt::registry& registry)
    {
        auto imageView = registry.view<UIComponent::Image, UIComponent::ImageEventStyles, UIComponent::TransformEvents, UIComponent::Dirty>();
        imageView.each([&](UIComponent::Image& image, UIComponent::ImageEventStyles& imageStyles, UIComponent::TransformEvents& events)
        {
            image.style = UI::ImageStylesheet();

            const u8 highestSetBit = static_cast<u8>(std::log2(events.state));
            const u8 highestSetState = highestSetBit ? static_cast<u8>(std::pow(2,highestSetBit)) : 0;
            u8 key = highestSetState;
            while (true)
            {
                const auto itr = imageStyles.styleMap.find(static_cast<UI::TransformEventState>(key));
                if (itr == imageStyles.styleMap.end())
                {
                    key >>= 1;
                    continue;
                }

                const UI::ImageStylesheet& eventStyle = itr->getSecond();

                /*
                *   Figure out which properties this style has that we are missing. Example:
                *   1011 ^ 1110 = 0101
                *   1110 & 0101 = 0100
                */
                const u8 missingProperties = eventStyle.overrideMask & (image.style.overrideMask ^ eventStyle.overrideMask);
                const auto IsMissingProperty = [&](UI::ImageStylesheet::OverrideMaskProperties property) { return (missingProperties & property); };

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::TEXTURE))
                    image.style.texture = eventStyle.texture;

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::TEXCOORD))
                    image.style.texCoord = eventStyle.texCoord;

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::COLOR))
                    image.style.color = eventStyle.color;

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::BORDER_TEXTURE))
                    image.style.borderTexture = eventStyle.borderTexture;

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::BORDER_SIZE))
                    image.style.borderSize = eventStyle.borderSize;

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::BORDER_INSET))
                    image.style.borderInset = eventStyle.borderInset;

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::SLICING_OFFSET))
                    image.style.slicingOffset = eventStyle.slicingOffset;

                image.style.overrideMask |= eventStyle.overrideMask;

                if (key == 0)
                    break;
                key >>= 1;
            }
        });
    }
}
