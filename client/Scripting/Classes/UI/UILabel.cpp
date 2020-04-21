#include "UILabel.h"
#include "../../ScriptEngine.h"
#include "../../../Rendering/UIElementRegistry.h"

UILabel::UILabel(const vec2& pos, const vec2& size)
    : _label(pos, size)
    , UIWidget(&_label)
{
    UIElementRegistry::Instance()->AddUILabel(this);
}

void UILabel::RegisterType()
{
    i32 r = ScriptEngine::RegisterScriptClass("UILabel", 0, asOBJ_REF | asOBJ_NOCOUNT);
    assert(r >= 0);
    {
        r = ScriptEngine::RegisterScriptInheritance<UIWidget, UILabel>("UIWidget");
        r = ScriptEngine::RegisterScriptFunction("UILabel@ CreateLabel(vec2 pos = vec2(0, 0), vec2 size = vec2(100, 100))", asFUNCTION(UILabel::CreateLabel)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(vec3 color)", asMETHOD(UILabel, SetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float width)", asMETHOD(UILabel, SetOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(vec3 color)", asMETHOD(UILabel, SetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string texture)", asMETHOD(UILabel, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(UILabel, SetFont)); assert(r >= 0);
    }
}

std::string UILabel::GetTypeName()
{
    return "UILabel";
}

void UILabel::SetColor(const vec3& color)
{
    _label.SetColor(Color(color.r, color.g, color.b, 1));
}

void UILabel::SetOutlineWidth(f32 width)
{
    _label.SetOutlineWidth(width);
}

void UILabel::SetOutlineColor(const vec3& color)
{
    _label.SetOutlineColor(Color(color.r, color.g, color.b, 1));
}

void UILabel::SetText(std::string& text)
{
    _label.SetText(text);
}

void UILabel::SetFont(std::string& fontPath, f32 fontSize)
{
    _label.SetFont(fontPath, fontSize);
}

UILabel* UILabel::CreateLabel(const vec2& pos, const vec2& size)
{
    UILabel* label = new UILabel(pos, size);

    return label;
}