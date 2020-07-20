#pragma once
#include <NovusTypes.h>
#include <entity/entity.hpp>
#include <entity/registry.hpp>
#include "../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/ScriptSingleton.h"
#include "../ECS/Components/UI/UIText.h"
#include "../ECS/Components/UI/UIDirty.h"

namespace UI::TextUtils
{
    inline static void MarkDirty(entt::entity entId)
    {
        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();

        if (!uiRegistry->has<UIDirty>(entId))
            uiRegistry->emplace<UIDirty>(entId);
    }

    inline static void SetText(entt::entity entId, const std::string& text)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, text]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.text = text;

                MarkDirty(entId);
            });
    }

    inline static void SetFont(entt::entity entId, const std::string& fontPath, f32 fontSize)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, fontPath, fontSize]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.fontPath = fontPath;
                uiText.fontSize = fontSize;

                MarkDirty(entId);
            });
    }

    inline static void SetColor(entt::entity entId, const Color& color)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, color]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.color = color;

                MarkDirty(entId);
            });
    }

    inline static void SetOutlineColor(entt::entity entId, const Color& color)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, color]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.outlineColor = color;

                MarkDirty(entId);
            });
    }

    inline static void SetOutlineWidth(entt::entity entId, f32 width)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, width]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.outlineWidth = width;

                MarkDirty(entId);
            });
    }
};