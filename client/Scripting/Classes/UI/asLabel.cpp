#include "asLabel.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"

#include "../../../ECS/Components/UI/Singletons/UIEntityPoolSingleton.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"

#include "../../../ECS/Components/UI/UIDirty.h"

namespace UI
{
    asLabel::asLabel(entt::entity entityId) : asUITransform(entityId, UIElementType::UITYPE_TEXT) { }
    
    void asLabel::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Label", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<asUITransform, asLabel>("UITransform");
        r = ScriptEngine::RegisterScriptFunction("Label@ CreateLabel()", asFUNCTION(asLabel::CreateLabel)); assert(r >= 0);

        //Text Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(asLabel, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(asLabel, GetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(asLabel, SetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(asLabel, GetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(Color color)", asMETHOD(asLabel, SetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetOutlineColor()", asMETHOD(asLabel, GetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float width)", asMETHOD(asLabel, SetOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetOutlineWidth()", asMETHOD(asLabel, GetOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(asLabel, SetFont)); assert(r >= 0);
    }

    void asLabel::SetText(const std::string& text)
    {
        _text.text = text;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([text, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.text = text;
                MarkDirty(uiRegistry, entId)
            });
    }

    void asLabel::SetColor(const Color& color)
    {
        _text.color = color;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([color, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.color = color;
                MarkDirty(uiRegistry, entId)
            });
    }

    void asLabel::SetOutlineColor(const Color& outlineColor)
    {
        _text.outlineColor = outlineColor;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([outlineColor, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.outlineColor = outlineColor;
                MarkDirty(uiRegistry, entId)
            });
    }

    void asLabel::SetOutlineWidth(f32 outlineWidth)
    {
        _text.outlineWidth = outlineWidth;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([outlineWidth, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.outlineWidth = outlineWidth;
                MarkDirty(uiRegistry, entId)
            });
    }

    void asLabel::SetFont(const std::string& fontPath, f32 fontSize)
    {
        _text.fontPath = fontPath;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;
        gameRegistry->ctx<ScriptSingleton>().AddTransaction([fontPath, fontSize, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.fontPath = fontPath;
                uiText.fontSize = fontSize;
                MarkDirty(uiRegistry, entId)
            });
    }

    asLabel* asLabel::CreateLabel()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIEntityPoolSingleton& entityPool = registry->ctx<UIEntityPoolSingleton>();

        asLabel* label = new asLabel(entityPool.DeQeueueId());

        return label;
    }
}