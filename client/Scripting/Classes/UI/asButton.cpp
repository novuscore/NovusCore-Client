#include "asButton.h"
#include "asLabel.h"
#include "asPanel.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../UI/TransformEventUtilsTransactions.h"

#include "../../../ECS/Components/Singletons/ScriptSingleton.h"

namespace UI
{
    asButton::asButton() : asUITransform(UIElementType::UITYPE_BUTTON) 
    {
        _panel = asPanel::CreatePanel();
        _panel->SetFillParentSize(true);
        _panel->SetAnchor(vec2(0.5, 0.5));
        _panel->SetLocalAnchor(vec2(0.5, 0.5));
        _panel->SetParent(this);
        _panel->SetCollisionEnabled(false);

        _label = asLabel::CreateLabel();
        _label->SetFillParentSize(true);
        _label->SetAnchor(vec2(0.5, 0.5));
        _label->SetLocalAnchor(vec2(0.5, 0.5));
        _label->SetParent(this);
        _label->SetCollisionEnabled(false);
        _label->SetHorizontalAlignment(TextHorizontalAlignment::CENTER);
    }
    
    void asButton::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Button", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<asUITransform, asButton>("UITransform");
        r = ScriptEngine::RegisterScriptFunction("Button@ CreateButton()", asFUNCTION(asButton::CreateButton)); assert(r >= 0);

        //Button Functions.
        r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(asButton, IsClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptFunctionDef("void ButtonEventCallback(Button@ button)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnClick(ButtonEventCallback@ cb)", asMETHOD(asButton, SetOnClickCallback)); assert(r >= 0);

        //Label Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(asButton, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(asButton, GetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTextColor(Color color)", asMETHOD(asButton, SetTextColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetTextColor()", asMETHOD(asButton, GetTextColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(Color color)", asMETHOD(asButton, SetTextOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetOutlineColor()", asMETHOD(asButton, GetTextOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float width)", asMETHOD(asButton, SetTextOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetOutlineWidth()", asMETHOD(asButton, GetTextOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(asButton, SetTextFont)); assert(r >= 0);

        //Panel Functions.
        r = ScriptEngine::RegisterScriptClassFunction("string GetTexture()", asMETHOD(asButton, GetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string Texture)", asMETHOD(asButton, SetTexture)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(asButton, GetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(asButton, SetColor)); assert(r >= 0);
    }

    void asButton::SetOnClickCallback(asIScriptFunction* callback)
    {
        _events.onClickCallback = callback;
        _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_CLICKABLE);

        UI::TransformEventUtils::Transactions::SetOnClickCallbackTransaction(_entityId, callback);
    }

    void asButton::SetText(const std::string& text)
    {
        _label->SetText(text);
    }
    const std::string asButton::GetText() const
    {
        return _label->GetText();
    }

    void asButton::SetTextColor(const Color& color)
    {
        _label->SetColor(color);
    }
    const Color& asButton::GetTextColor() const
    {
        return _label->GetColor();
    }

    void asButton::SetTextOutlineColor(const Color& outlineColor)
    {
        _label->SetOutlineColor(outlineColor);
    }
    const Color& asButton::GetTextOutlineColor() const
    {
        return _label->GetOutlineColor();
    }

    void asButton::SetTextOutlineWidth(f32 outlineWidth)
    {
        _label->SetOutlineWidth(outlineWidth);
    }
    const f32 asButton::GetTextOutlineWidth() const
    {
        return _label->GetOutlineWidth();
    }

    void asButton::SetTextFont(std::string fontPath, f32 fontSize)
    {
        _label->SetFont(fontPath, fontSize);
    }

    void asButton::SetTexture(const std::string& texture)
    {
        _panel->SetTexture(texture);
    }

    const std::string& asButton::GetTexture() const
    {
        return _panel->GetTexture();
    }

    void asButton::SetColor(const Color& color)
    {
        _panel->SetColor(color);
    }

    const Color asButton::GetColor() const
    {
        return _panel->GetColor();
    }

    asButton* asButton::CreateButton()
    {
        asButton* button = new asButton();

        return button;
    }
}