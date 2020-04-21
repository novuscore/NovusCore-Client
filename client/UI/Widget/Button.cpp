#include "Button.h"
#include "Label.h"

namespace UI
{
    // Public
    Button::Button(const vec2& pos, const vec2& size)
        : Widget(pos, size)
        , _color(1.0f, 1.0f, 1.0f, 1.0f)
        , _clickable(true)
    {
        _label = new Label(pos, size);
        _label->SetParent(this);
    }

    // Private
    Renderer::ModelID Button::GetModelID()
    {
        return Widget::GetModelID();
    }
    void Button::SetModelID(Renderer::ModelID modelID)
    {
        Widget::SetModelID(modelID);
    }

    std::string& Button::GetTexture()
    {
        return Widget::GetTexture();
    }
    void Button::SetTexture(std::string& texture)
    {
        Widget::SetTexture(texture);
    }

    Renderer::TextureID Button::GetTextureID()
    {
        return Widget::GetTextureID();
    }
    void Button::SetTextureID(Renderer::TextureID textureID)
    {
        Widget::SetTextureID(textureID);
    }

    const Color& Button::GetColor()
    {
        return _color;
    }
    void Button::SetColor(const Color& color)
    {
        _color = color;
    }

    bool Button::IsClickable()
    {
        return _clickable;
    }

    void Button::SetClickable(bool value)
    {
        _clickable = value;
    }

    std::string& Button::GetText()
    {
        return _label->GetText();
    }
    void Button::SetText(std::string& text)
    {
        _label->SetText(text);
    }
}