#include "Widget.h"

namespace UI
{
    // Public
    Widget::Widget(const vec2& pos, const vec2& size)
        : _position(pos.x, pos.y, 0)
        , _localPosition(0, 0, 0)
        , _size(size)
        , _anchor(0, 0)
        , _parent(nullptr)
        , _children(16)
        , _isDirty(true)
    {

    }

    void Widget::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Widget", 0, asOBJ_REF | asOBJ_NOCOUNT);
        assert(r >= 0);
        {
            RegisterBase<Widget>();
        }
    }

    void Widget::SetPosition(const vec2& position, float depth)
    {
        float d = depth == 0 ? _position.z : depth;
        _position = vec3(position.x, position.y, d);
        SetDirty();
    }

    void Widget::SetSize(const vec2& size) { _size = size; }

    void Widget::SetAnchor(const vec2& anchor) { _anchor = anchor; }

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

    void Widget::SetDirty()
    {
        _isDirty = true;
    }

    // Protected
    void Widget::SetModelID(Renderer::ModelID modelID)
    {
        _modelID = modelID;
    }

    void Widget::SetTexture(std::string& texture)
    {
        _texture = texture;
        SetDirty();
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