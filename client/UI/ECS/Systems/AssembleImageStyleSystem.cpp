#include "AssembleImageStyleSystem.h"
#include <entity/registry.hpp>
#include "../Components/Image.h"
#include "../Components/ImageEventStyles.h"
#include "../Components/TransformEvents.h"
#include "../Components/Dirty.h"
#include "../Components/NotCulled.h"


namespace UISystem
{
    void AssembleImageStyleSystem::Update(entt::registry& registry)
    {
        auto imageView = registry.view<UIComponent::Image, UIComponent::ImageEventStyles, UIComponent::TransformEvents, UIComponent::Dirty, UIComponent::NotCulled>();
        imageView.each([&](UIComponent::Image& image, UIComponent::ImageEventStyles& imageStyles, UIComponent::TransformEvents& events)
        {
            image.style = UI::ImageStylesheet();

            if (events.HasState(UI::TransformEventState::STATE_DISABLED))
            {
                const auto itr = imageStyles.styleMap.find(UI::TransformEventState::STATE_DISABLED);
                if (itr != imageStyles.styleMap.end())
                    image.style = itr->getSecond();

                image.style.Merge(imageStyles.styleMap[UI::TransformEventState::STATE_NORMAL]);
                return;
            }

            const u8 highestSetBit = static_cast<u8>(std::log2(events.state));
            const u8 highestSetState = highestSetBit ? static_cast<u8>(std::pow(2,highestSetBit)) : 0;
            u8 key = highestSetState;
            while (true)
            {
                const auto itr = imageStyles.styleMap.find(static_cast<UI::TransformEventState>(key));
                if (itr != imageStyles.styleMap.end())
                {
                    image.style.Merge(itr->getSecond());
                }

                if (key == 0)
                    break;
                key >>= 1;
            }
        });
    }
}
