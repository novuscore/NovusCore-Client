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

        virtual std::string GetTypeName();

        vec2 GetPosition();
        void SetPosition(const vec2& position, float depth);

        float GetDepth();

        vec2 GetScreenPosition();

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