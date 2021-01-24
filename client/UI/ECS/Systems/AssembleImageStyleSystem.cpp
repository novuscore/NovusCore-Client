#include "AssembleImageStyleSystem.h"
#include <entity/registry.hpp>
#include "../Components/Image.h"
#include "../Components/ImageEventStyles.h"
#include "../Components/TransformEvents.h"
#include "../Components/Dirty.h"


namespace UISystem
{
    /*
    *   This is the most AAA looking programming I have done.
    *   I both hate & love it.
    *       - Grim.
    */
    void AssembleImageStyleSystem::Update(entt::registry& registry)
    {
        auto imageView = registry.view<UIComponent::Image, UIComponent::ImageEventStyles, UIComponent::TransformEvents, UIComponent::Dirty>();
        imageView.each([&](UIComponent::Image& image, UIComponent::ImageEventStyles& imageStyles, UIComponent::TransformEvents& events)
        {
            image.style = UI::ImageStylesheet();

            const u8 highestSetState = static_cast<u8>(std::log2(events.state)); 
            
            /*
            *   Problem with this:
            *   It is a very manual process and if someone adds in a new property this must be updated for it.
            *   Is there a meta-programming solution?
            */
            for (u8 key = highestSetState; key > 0; key >>= 1)
            {
                auto itr = imageStyles.styleMap.find(static_cast<UI::TransformEventState>(key));
                if (itr == imageStyles.styleMap.end())
                    continue;

                const UI::ImageStylesheet& eventStyle = itr->getSecond();
                const u8 existingOverride = image.style.overrideMask;
                
                /*
                *   Figure out which properties this style has that we are missing. Example:
                *   1011 ^ 1110 = 0101
                *   1110 & 0101 = 0100
                */
                const u8 missingProperties = eventStyle.overrideMask & (existingOverride ^ eventStyle.overrideMask);
                const auto IsMissingProperty = [&](UI::ImageStylesheet::OverrideMaskProperties property) { return missingProperties & property; };

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::TEXTURE))
                    image.style.SetTexture(eventStyle.texture);

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::TEXCOORD))
                    image.style.SetTexCoord(eventStyle.texCoord);

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::COLOR))
                    image.style.SetColor(eventStyle.color);

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::BORDER_TEXTURE))
                    image.style.SetBorderTexture(eventStyle.borderTexture);

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::BORDER_SIZE))
                    image.style.SetBorderSize(eventStyle.borderSize);

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::BORDER_INSET))
                    image.style.SetBorderInset(eventStyle.borderInset);

                if (IsMissingProperty(UI::ImageStylesheet::OverrideMaskProperties::SLICING_OFFSET))
                    image.style.SetSlicingOffset(eventStyle.slicingOffset);

                // Assert that we have the sum of all properties of both parts. If this fails someone has forgotten to implement the assembling of a property.
                if ((existingOverride | eventStyle.overrideMask) != image.style.overrideMask)
                {
                    DebugHandler::PrintFatal("Overriden property not implemented in AssembleImageStyleSystem. Expected mask: %d. Recieved mask: %d", (existingOverride | itr->second.overrideMask), image.style.overrideMask);
                }
            }
        });
    }
}