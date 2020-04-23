#pragma once
#include <NovusTypes.h>
#include <vector>
#include <assert.h>
#include <Renderer/Descriptors/ModelDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>
#include "../../Scripting/ScriptEngine.h"

class UIRenderer;

namespace UI
{
    class Widget
    {
    public:
        Widget(const vec2& pos, const vec2& size);
        static void RegisterType();

        template <class T>
        static void RegisterBase()
        {
            i32 r = ScriptEngine::RegisterScriptClassFunction("string GetTypeName()", asMETHOD(T, GetTypeName)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetPosition(vec2 pos, float depth = 0)", asMETHOD(T, SetPosition)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetSize(vec2 size)", asMETHOD(T, SetSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetPosition()", asMETHOD(T, GetPosition)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("float GetDepth()", asMETHOD(T, GetDepth)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetSize()", asMETHOD(T, GetSize)); assert(r >= 0);
        }

        virtual std::string GetTypeName() const { return "Widget"; }

        vec2 GetPosition() const { return vec2(_position.x, _position.y); }
        void SetPosition(const vec2& position, float depth);

        float GetDepth() const { return _position.z; }

        vec2 GetScreenPosition() const { return _position + _localPosition; }

        const vec2& GetSize() const { return _size; }
        void SetSize(const vec2& size);

        const vec2& GetAnchor() const { return _anchor; }
        void SetAnchor(const vec2& anchor);

        Widget* GetParent() const { return _parent; }
        void SetParent(Widget* widget);

        const std::vector<Widget*>& GetChildren() const { return _children; }

        bool IsDirty() const { return _isDirty; }
        void SetDirty();

    protected:
        Renderer::ModelID GetModelID() const { return _modelID; }
        void SetModelID(Renderer::ModelID modelID);

        const std::string& GetTexture() const { return _texture; }
        void SetTexture(std::string& texture);

        Renderer::TextureID GetTextureID() const { return _textureID; }
        void SetTextureID(Renderer::TextureID textureID);

    private:
        void AddChild(Widget* child);
        void RemoveChild(Widget* child);
        void ResetDirty();

    protected:
        vec3 _position;
        vec3 _localPosition;
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