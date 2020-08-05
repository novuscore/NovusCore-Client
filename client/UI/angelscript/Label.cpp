#include "Label.h"
#include "../../Scripting/ScriptEngine.h"
#include "../../Utils/ServiceLocator.h"

namespace UIScripting
{
    Label::Label() : BaseElement(UI::UIElementType::UITYPE_TEXT) { }
    
    void Label::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Label", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<BaseElement, Label>("Transform");
        r = ScriptEngine::RegisterScriptFunction("Label@ CreateLabel()", asFUNCTION(Label::CreateLabel)); assert(r >= 0);

        //Text Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(Label, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(Label, SetFont)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(Label, GetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(Label, SetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(Label, GetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(Color color)", asMETHOD(Label, SetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetOutlineColor()", asMETHOD(Label, GetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float width)", asMETHOD(Label, SetOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetOutlineWidth()", asMETHOD(Label, GetOutlineWidth)); assert(r >= 0);
    }

    void Label::SetText(const std::string& text)
    {
        _text.text = text;

        //UI::TextUtils::Transactions::SetTextTransaction(_entityId, text);
    }

    void Label::SetFont(const std::string& fontPath, f32 fontSize)
    {
        _text.fontPath = fontPath;

        //UI::TextUtils::Transactions::SetFontTransaction(_entityId, fontPath, fontSize);
    }

    void Label::SetColor(const Color& color)
    {
        _text.color = color;

        //UI::TextUtils::Transactions::SetColorTransaction(_entityId, color);
    }

    void Label::SetOutlineColor(const Color& outlineColor)
    {
        _text.outlineColor = outlineColor;

        //UI::TextUtils::Transactions::SetOutlineColorTransaction(_entityId, outlineColor);
    }

    void Label::SetOutlineWidth(f32 outlineWidth)
    {
        _text.outlineWidth = outlineWidth;

        //UI::TextUtils::Transactions::SetOutlineWidthTransaction(_entityId, outlineWidth);
    }

    void Label::SetHorizontalAlignment(UI::TextHorizontalAlignment alignment)
    {
        _text.horizontalAlignment = alignment;

        //UI::TextUtils::Transactions::SetHorizontalAlignmentTransaction(_entityId, alignment);
    }

    void Label::SetVerticalAlignment(UI::TextVerticalAlignment alignment)
    {
        _text.verticalAlignment = alignment;

        //UI::TextUtils::Transactions::SetVerticalAlignmentTransaction(_entityId, alignment);
    }

    Label* Label::CreateLabel()
    {
        Label* label = new Label();

        return label;
    }
}