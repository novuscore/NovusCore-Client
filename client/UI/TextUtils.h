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

    inline static void SetText(const std::string& text, entt::entity entId)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([text, entId]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.text = text;

                MarkDirty(entId);
            });
    }

    inline static void SetFont(const std::string& fontPath, f32 fontSize, entt::entity entId)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([fontPath, fontSize, entId]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.fontPath = fontPath;
                uiText.fontSize = fontSize;

                MarkDirty(entId);
            });
    }

    inline static void SetColor(const Color& color, entt::entity entId)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([color, entId]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.color = color;

                MarkDirty(entId);
            });
    }

    inline static void SetOutlineColor(const Color& color, entt::entity entId)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([color, entId]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.outlineColor = color;

                MarkDirty(entId);
            });
    }

    inline static void SetOutlineWidth(f32 width, entt::entity entId)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([width, entId]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.outlineWidth = width;

                MarkDirty(entId);
            });
    }
};