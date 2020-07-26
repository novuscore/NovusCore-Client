#pragma once
#include <NovusTypes.h>
#include <entity/entity.hpp>
#include <entity/registry.hpp>
#include "../Utils/ServiceLocator.h"

#include "../ECS/Components/Singletons/ScriptSingleton.h"
#include "../ECS/Components/UI/UIText.h"
#include "../ECS/Components/UI/UIDirty.h"

namespace UI::TextUtils::Transactions
{
    inline static void MarkDirty(entt::entity entId)
    {
        entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();

        if (!uiRegistry->has<UIDirty>(entId))
            uiRegistry->emplace<UIDirty>(entId);
    }

    inline static void SetTextTransaction(entt::entity entId, const std::string& text)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, text]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.text = text;
                uiText.pushback = Math::Min(uiText.pushback, text.length() - 1);

                MarkDirty(entId);
            });
    }

    inline static void SetFontTransaction(entt::entity entId, const std::string& fontPath, f32 fontSize)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, fontPath, fontSize]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.fontPath = fontPath;
                uiText.fontSize = fontSize;

                MarkDirty(entId);
            });
    }

    inline static void SetHorizontalAlignmentTransaction(entt::entity entId, TextHorizontalAlignment alignment)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, alignment]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.horizontalAlignment = alignment;

                MarkDirty(entId);
            });
    }

    inline static void SetVerticalAlignmentTransaction(entt::entity entId, TextVerticalAlignment alignment)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, alignment]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.verticalAlignment = alignment;

                MarkDirty(entId);
            });
    }

    inline static void SetColorTransaction(entt::entity entId, const Color& color)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, color]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.color = color;

                MarkDirty(entId);
            });
    }

    inline static void SetOutlineColorTransaction(entt::entity entId, const Color& color)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, color]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.outlineColor = color;

                MarkDirty(entId);
            });
    }

    inline static void SetOutlineWidthTransaction(entt::entity entId, f32 width)
    {
        ServiceLocator::GetGameRegistry()->ctx<ScriptSingleton>().AddTransaction([entId, width]()
            {
                UIText& uiText = ServiceLocator::GetUIRegistry()->get<UIText>(entId);
                uiText.outlineWidth = width;

                MarkDirty(entId);
            });
    }
};