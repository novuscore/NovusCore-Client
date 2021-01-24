#include "UpdateTextModelSystem.h"
#include <entity/registry.hpp>
#include "../Components/Text.h"
#include "../Components/TextEventStyles.h"
#include "../Components/TransformEvents.h"
#include "../Components/Dirty.h"

namespace UISystem
{
    void UpdateTextModelSystem::Update(entt::registry& registry)
    {
        auto textView = registry.view<UIComponent::Text, UIComponent::TextEventStyles, UIComponent::TransformEvents, UIComponent::Dirty>();
        textView.each([&](UIComponent::Text& text, UIComponent::TextEventStyles& textStyles, UIComponent::TransformEvents& events)
        {

        });
    }
}