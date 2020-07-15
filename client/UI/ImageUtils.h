#pragma once
#include <NovusTypes.h>
#include <entity/entity.hpp>
#include "../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/ScriptSingleton.h"
#include "../ECS/Components/UI/UIImage.h"
#include "../ECS/Components/UI/UIDirty.h"

namespace UI::ImageUtils
{
    inline static void MarkDirty(entt::entity entId)
    {
        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
        
        if (!uiRegistry->has<UIDirty>(entId))
            uiRegistry->emplace<UIDirty>(entId);
    }

    inline static void SetTexture(const std::string& texture, entt::entity entId)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([texture, entId]()
            {
                UIImage& image = ServiceLocator::GetUIRegistry()->get<UIImage>(entId);
                image.texture = texture;

                MarkDirty(entId);
            });
    }

    inline static void SetColor(const Color& color, entt::entity entId)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([color, entId]()
            {
                UIImage& image = ServiceLocator::GetUIRegistry()->get<UIImage>(entId);
                image.color = color;

                MarkDirty(entId);
            });
    }
};