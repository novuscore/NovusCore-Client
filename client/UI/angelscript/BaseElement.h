#pragma once
#include <NovusTypes.h>
#include <entity/fwd.hpp>
#include "../../Scripting/ScriptEngine.h"
#include "../UITypes.h"

namespace UIScripting
{
    class BaseElement
    {
    public:
        BaseElement(UI::ElementType elementType, std::string name, bool collisionEnabled = true);

        virtual ~BaseElement() { }

        static void RegisterType()
        {
            i32 r = ScriptEngine::RegisterScriptClass("BaseElement", 0, asOBJ_REF | asOBJ_NOCOUNT);
            assert(r >= 0);
            {
                RegisterBase<BaseElement>();
            }
        }

        template<class T>
        static void RegisterBase()
        {
            i32 r = ScriptEngine::RegisterScriptClassFunction("Entity GetEntityId()", asMETHOD(T, GetEntityId)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetScreenPosition()", asMETHOD(T, GetScreenPosition)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetLocalPosition()", asMETHOD(T, GetLocalPosition)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetPosition(vec2 position)", asMETHOD(T, SetPosition)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetSize()", asMETHOD(T, GetSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetSize(vec2 size)", asMETHOD(T, SetSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool DoesFillParentSize()", asMETHOD(T, DoesFillParentSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetFillParentSize(bool fillParent)", asMETHOD(T, SetFillParentSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetFillParentSizeAxis(bool fillX, bool fillY)", asMETHOD(T, SetFillParentSizeAxis)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetFillBounds(FBox fillBounds)", asMETHOD(T, SetFillBounds)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void SetTransform(vec2 position, vec2 size)", asMETHOD(T, SetTransform)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetAnchor()", asMETHOD(T, GetAnchor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetAnchor(vec2 anchor)", asMETHOD(T, SetAnchor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetLocalAnchor()", asMETHOD(T, GetLocalAnchor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetLocalAnchor(vec2 anchor)", asMETHOD(T, SetLocalAnchor)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void SetPadding(FBox padding)", asMETHOD(T, SetPadding)); assert(r >= 0);
            
            r = ScriptEngine::RegisterScriptClassFunction("uint8 GetDepthLayer()", asMETHOD(T, GetDepthLayer)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetDepthLayer(uint8 layer)", asMETHOD(T, SetDepthLayer)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("uint16 GetDepth()", asMETHOD(T, GetDepth)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetDepth(uint16 depth)", asMETHOD(T, SetDepth)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("BaseElement@ GetParent()", asMETHOD(T, GetParent)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void AddChild(BaseElement@ child)", asMETHOD(T, AddChild)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void RemoveChild(BaseElement@ child)", asMETHOD(T, RemoveChild)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void RemoveFromParent()", asMETHOD(T, RemoveFromParent)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void Destroy(bool destroyChildren = true)", asMETHOD(T, Destroy)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void GetCollisionIncludesChildren()", asMETHOD(T, GetCollisionIncludesChildren)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetCollisionIncludesChildren(bool enabled)", asMETHOD(T, SetCollisionIncludesChildren)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("bool IsVisible()", asMETHOD(T, IsVisible)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool IsSelfVisible()", asMETHOD(T, IsSelfVisible)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool IsParentVisible()", asMETHOD(T, IsParentVisible)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetVisible(bool visible)", asMETHOD(T, SetVisible)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void SetCollisionEnabled(bool enabled)", asMETHOD(T, SetCollisionEnabled)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void MarkDirty()", asMETHOD(T, MarkDirty)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void MarkSelfDirty()", asMETHOD(T, MarkSelfDirty)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void MarkBoundsDirty()", asMETHOD(T, MarkBoundsDirty)); assert(r >= 0);
        }

        inline const entt::entity GetEntityId() const { return _entityId; }
        inline const UI::ElementType GetType() const { return _elementType; }

        // Transform Functions
        vec2 GetScreenPosition() const;
        vec2 GetLocalPosition() const;
        void SetPosition(const vec2& position);

        vec2 GetSize() const;
        void SetSize(const vec2& size);
        bool DoesFillParentSize() const;
        void SetFillParentSize(bool fillParent);
        void SetFillParentSizeAxis(bool fillX, bool fillY);
        void SetFillBounds(const UI::FBox& fillBounds);

        void SetTransform(const vec2& position, const vec2& size);

        vec2 GetAnchor() const;
        void SetAnchor(const vec2& anchor);

        vec2 GetLocalAnchor() const;
        void SetLocalAnchor(const vec2& localAnchor);
        
        void SetPadding(const UI::FBox& padding);

        UI::DepthLayer GetDepthLayer() const;
        void SetDepthLayer(const UI::DepthLayer layer);

        u16 GetDepth() const;
        void SetDepth(const u16 depth);

        const bool HasParent() const;
        BaseElement* GetParent() const;
        void AddChild(BaseElement* child);
        void RemoveChild(BaseElement* child);
        void RemoveFromParent();

        bool GetCollisionIncludesChildren() const;
        void SetCollisionIncludesChildren(bool expand);

        bool IsVisible() const;
        bool IsSelfVisible() const;
        bool IsParentVisible() const;
        void SetVisible(bool visible);
    
        void SetCollisionEnabled(bool enabled);

        void Destroy(bool destroyChildren = true);

        void MarkDirty();
        void MarkSelfDirty();
        void MarkBoundsDirty();

    protected:
        // Quick set up default children without doing all the checks and updates that SetParent() does. Used for elements like checkboxes, buttons & sliders.
        void InternalAddChild(BaseElement* element);

        entt::entity _entityId;
        UI::ElementType _elementType;
    };
}
