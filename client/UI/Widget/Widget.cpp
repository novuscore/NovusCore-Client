#include "Widget.h"

namespace UI
{
    // Public
    Widget::Widget(const vec2& pos, const vec2& size)
        : _position(pos.x, pos.y, 0)
        , _size(size)
        , _parent(nullptr)
        , _children(16)
        , _isDirty(true) 
    {

    }

    const vec3& Widget::GetPosition() { return _position; }
    void Widget::SetPosition(const vec3& position)
    { 
        _position = position;
        SetDirty();
    }

    const vec2& Widget::GetSize() { return _size; }
    void Widget::SetSize(const vec2& size) { _size = size; }

    const vec2& Widget::GetAnchor() { return _anchor; }
    void Widget::SetAnchor(const vec2& anchor) { _anchor = anchor; }

    Widget* Widget::GetParent() { return _parent; }
    void Widget::SetParent(Widget* widget)
    {
        if (_parent)
        {
            _parent->RemoveChild(this);
        }

        _parent = widget;
        _parent->AddChild(_parent);
        _parent->SetDirty();
    }

    const std::vector<Widget*>& Widget::GetChildren() { return _children; }

    bool Widget::IsDirty()
    {
        return _isDirty;
    }
    void Widget::SetDirty()
    {
        _isDirty = true;
    }

    // Protected
    Renderer::ModelID Widget::GetModelID()
    {
        return _modelID;
    }
    void Widget::SetModelID(Renderer::ModelID modelID)
    {
        _modelID = modelID;
    }

    std::string& Widget::GetTexture()
    {
        return _texture;
    }
    void Widget::SetTexture(std::string& texture)
    {
        _texture = texture;
        SetDirty();
    }

    Renderer::TextureID Widget::GetTextureID()
    {
        return _textureID;
    }
    void Widget::SetTextureID(Renderer::TextureID textureID)
    {
        _textureID = textureID;
    }

    // Private
    void Widget::AddChild(Widget* child)
    {
        _children.push_back(child);
    }
    void Widget::RemoveChild(Widget* child)
    {
        auto iterator = std::find(_children.begin(), _children.end(), child);
        assert(iterator != _children.end());

        _children.erase(iterator);
    }
    void Widget::ResetDirty()
    {
        _isDirty = false;
    }
}