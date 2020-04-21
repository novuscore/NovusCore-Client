#include "Label.h"

#include <algorithm>
#include "../../Rendering/UIElementRegistry.h"

namespace UI
{
    Label::Label(const vec2& pos, const vec2& size)
        : Widget(pos, size)
        , _color(1.0f, 1.0f, 1.0f, 1.0f)
    {
        UIElementRegistry::Instance()->AddLabel(this);
    }

    void Label::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Label", 0, asOBJ_REF | asOBJ_NOCOUNT);
        assert(r >= 0);
        {
            r = ScriptEngine::RegisterScriptInheritance<Widget, Label>("Widget");
            r = ScriptEngine::RegisterScriptFunction("Label@ CreateLabel(vec2 pos = vec2(0, 0), vec2 size = vec2(100, 100))", asFUNCTION(Label::CreateLabel)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetColor(vec3 color)", asMETHOD(Label, SetColor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float width)", asMETHOD(Label, SetOutlineWidth)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(vec3 color)", asMETHOD(Label, SetOutlineColor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetText(string texture)", asMETHOD(Label, SetText)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(Label, SetFont)); assert(r >= 0);
        }
    }

    std::string Label::GetTypeName()
    {
        return "Label";
    }

    // Private
    std::string& Label::GetText()
    { 
        return _text; 
    }

    void Label::SetText(std::string& text)
    { 
        _text = text;
        SetDirty();
        _glyphCount = static_cast<u32>(std::count_if(text.begin(), text.end(), [](char c)
        {
            return c != ' ';
        }));
    }

    u32 Label::GetTextLength()
    {
        return static_cast<u32>(_text.length());
    }

    u32 Label::GetGlyphCount()
    {
        return _glyphCount;
    }

    const Color& Label::GetColor()
    { 
        return _color;
    }

    void Label::SetColor(const Color& color)
    { 
        _color = color;
        SetDirty();
    }

    f32 Label::GetOutlineWidth()
    {
        return _outlineWidth;
    }

    void Label::SetOutlineWidth(f32 width)
    {
        _outlineWidth = width;
    }

    const Color& Label::GetOutlineColor()
    {
        return _outlineColor;
    }

    void Label::SetOutlineColor(const Color& color)
    {
        _outlineColor = color;
        SetDirty();
    }

    void Label::SetFont(std::string& fontPath, f32 fontSize)
    {
        if (_parent)
        {
            _localPosition.y = fontSize;
        }

        _fontPath = fontPath;
        _fontSize = fontSize;
        SetDirty();
    }

    std::string& Label::GetFontPath()
    {
        return _fontPath;
    }

    f32 Label::GetFontSize()
    {
        return _fontSize;
    }

    Label* Label::CreateLabel(const vec2& pos, const vec2& size)
    {
        Label* label = new Label(pos, size);

        return label;
    }
}
