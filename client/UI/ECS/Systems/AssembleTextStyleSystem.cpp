#include "AssembleTextStyleSystem.h"
#include <entity/registry.hpp>
#include "../Components/Text.h"
#include "../Components/TextEventStyles.h"
#include "../Components/TransformEvents.h"
#include "../Components/Dirty.h"
#include "../Components/NotCulled.h"

namespace UISystem
{
    void AssembleTextStyleSystem::Update(entt::registry& registry)
    {
        auto textView = registry.view<UIComponent::Text, UIComponent::TextEventStyles, UIComponent::TransformEvents, UIComponent::Dirty, UIComponent::NotCulled>();
        textView.each([&](UIComponent::Text& text, UIComponent::TextEventStyles& textStyles, UIComponent::TransformEvents& events)
        {            
            text.style = UI::TextStylesheet();

            const u8 highestSetBit = static_cast<u8>(std::log2(events.state));
            const u8 highestSetState = highestSetBit ? static_cast<u8>(std::pow(2,highestSetBit)) : 0;
            u8 key = highestSetState;
            while (true)
            {
                const auto itr = textStyles.styleMap.find(static_cast<UI::TransformEventState>(key));
                if (itr != textStyles.styleMap.end())
                {
                    text.style.Merge(itr->getSecond());
                }

                if (key == 0)
                    break;
                key >>= 1;
            }
        });
    }
}