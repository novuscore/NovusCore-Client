#pragma once
#include <NovusTypes.h>
#include <vector>
#include <assert.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>

class UIRenderer;

namespace UI
{
    class Widget
    {
    public:
        Widget(const vec2& pos, const vec2& size);

        const vec3& GetPosition();
        void SetPosition(const vec3& position);

        const vec2& GetSize();
        void SetSize(const vec2& size);

        const vec2& GetAnchor();
        void SetAnchor(const vec2& anchor);

        Widget* GetParent();
        void SetParent(Widget* widget);

        const std::vector<Widget*>& GetChildren();

        bool IsDirty();
        void SetDirty();

    protected:
        Renderer::ModelID GetModelID();
        void SetModelID(Renderer::ModelID modelID);

        std::string& GetTexture();
        void SetTexture(std::string& texture);

        Renderer::TextureID GetTextureID();
        void SetTextureID(Renderer::TextureID textureID);

    private:
        void AddChild(Widget* child);
        void RemoveChild(Widget* child);
        void ResetDirty();

    private:
        vec3 _position;
        vec2 _size;
        vec2 _anchor;
        Widget* _parent;
        std::vector<Widget*> _children;
        bool _isDirty;

        Renderer::ModelID _modelID = Renderer::ModelID::Invalid();

        std::string _texture;
        Renderer::TextureID _textureID = Renderer::TextureID::Invalid();

        friend class UIRenderer;
    };
}