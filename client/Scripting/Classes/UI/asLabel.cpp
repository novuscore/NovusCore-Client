#include "asLabel.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../UI/TextUtils.h"

#include "../../../ECS/Components/UI/Singletons/UIEntityPoolSingleton.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"

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
        r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(asLabel, SetFont)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(asLabel, GetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(asLabel, SetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(asLabel, GetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(Color color)", asMETHOD(asLabel, SetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetOutlineColor()", asMETHOD(asLabel, GetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float width)", asMETHOD(asLabel, SetOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetOutlineWidth()", asMETHOD(asLabel, GetOutlineWidth)); assert(r >= 0);
    }

    void asLabel::SetText(const std::string& text)
    {
        _text.text = text;

        UI::TextUtils::SetText(_entityId, text);
    }

    void asLabel::SetFont(const std::string& fontPath, f32 fontSize)
    {
        _text.fontPath = fontPath;

        UI::TextUtils::SetFont(_entityId, fontPath, fontSize);
    }

    void asLabel::SetColor(const Color& color)
    {
        _text.color = color;

        UI::TextUtils::SetColor(_entityId, color);
    }

    void asLabel::SetOutlineColor(const Color& outlineColor)
    {
        _text.outlineColor = outlineColor;

        UI::TextUtils::SetOutlineColor(_entityId, outlineColor);
    }

    void asLabel::SetOutlineWidth(f32 outlineWidth)
    {
        _text.outlineWidth = outlineWidth;

        UI::TextUtils::SetOutlineWidth(_entityId, outlineWidth);
    }

    asLabel* asLabel::CreateLabel()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIEntityPoolSingleton& entityPool = registry->ctx<UIEntityPoolSingleton>();

        asLabel* label = new asLabel(entityPool.GetId());

        return label;
    }
}