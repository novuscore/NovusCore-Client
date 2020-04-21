#include "Panel.h"

namespace UI
{
    // Public
    Panel::Panel(const vec2& pos, const vec2& size)
        : Widget(pos, size)
        , _color(1.0f, 1.0f, 1.0f, 1.0f), _clickable(true), _draggable(false), _isDragging(false), _deltaDragPosition(0, 0), _didDrag(false)
    {
    }

    // Private
    Renderer::ModelID Panel::GetModelID()
    {
        return Widget::GetModelID();
    }
    void Panel::SetModelID(Renderer::ModelID modelID)
    {
        Widget::SetModelID(modelID);
    }
    
    std::string& Panel::GetTexture() 
    { 
        return Widget::GetTexture(); 
    }
    void Panel::SetTexture(std::string& texture) 
    { 
        Widget::SetTexture(texture);
    }

    Renderer::TextureID Panel::GetTextureID()
    {
        return Widget::GetTextureID();
    }
    void Panel::SetTextureID(Renderer::TextureID textureID)
    {
        Widget::SetTextureID(textureID);
    }

    const Color& Panel::GetColor()
    { 
        return _color; 
    }

    void Panel::SetColor(const Color& color)
    { 
        _color = color;
        SetDirty();
    }

    bool Panel::IsClickable()
    {
        return _clickable;
    }
    void Panel::SetClickable(bool value)
    {
        _clickable = value;
    }

    bool Panel::IsDraggable()
    {
        return _draggable;
    }

    void Panel::SetDraggable(bool value)
    {
        _draggable = value;
    }

    bool Panel::IsDragging()
    {
        return _isDragging;
    }

    void Panel::BeingDrag(const vec2& deltaDragPosition)
    {
        _deltaDragPosition = deltaDragPosition;
        _isDragging = true;
    }

    const vec2& Panel::GetDeltaDragPosition()
    {
        return _deltaDragPosition;
    }

    bool Panel::DidDrag()
    {
        return _didDrag;
    }

    void Panel::SetDidDrag()
    {
        _didDrag = true;
    }

    void Panel::EndDrag()
    {
        _deltaDragPosition = vec2(0, 0);
        _didDrag = false;
        _isDragging = false;
    }
}