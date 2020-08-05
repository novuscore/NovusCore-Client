#pragma once
#include <NovusTypes.h>
#include <entity/entity.hpp>

#include "../../Scripting/ScriptEngine.h"
#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Visibility.h"
#include "../ECS/Components/Singletons/UIDataSingleton.h"

namespace UIScripting
{
    class BaseElement
    {
        friend struct ::UISingleton::UIDataSingleton;

    public:
        BaseElement(UI::UIElementType elementType);

        static void RegisterType()
        {
            i32 r = ScriptEngine::RegisterScriptClass("Transform", 0, asOBJ_REF | asOBJ_NOCOUNT);
            assert(r >= 0);
            {
                RegisterBase<BaseElement>();
            }
        }

        template<class T>
        static void RegisterBase()
        {
            i32 r = ScriptEngine::RegisterScriptClassFunction("Entity GetEntityId()", asMETHOD(T, GetEntityId)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetTransform(vec2 position, vec2 size)", asMETHOD(T, SetTransform)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetScreenPosition()", asMETHOD(T, GetScreenPosition)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetLocalPosition()", asMETHOD(T, GetLocalPosition)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetParentPosition()", asMETHOD(T, GetParentPosition)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetPosition()", asMETHOD(T, GetPosition)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetPosition(vec2 position)", asMETHOD(T, SetPosition)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetLocalAnchor()", asMETHOD(T, GetLocalAnchor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetLocalAnchor(vec2 anchor)", asMETHOD(T, SetLocalAnchor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetSize()", asMETHOD(T, GetSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetSize(vec2 size)", asMETHOD(T, SetSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool GetFillParentSize()", asMETHOD(T, GetFillParentSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetFillParentSize(bool fillParent)", asMETHOD(T, SetFillParentSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("float GetDepth()", asMETHOD(T, GetDepth)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetDepth(uint16 depth)", asMETHOD(T, SetDepth)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void SetParent(Transform@ parent)", asMETHOD(T, SetParent)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void UnsetParent()", asMETHOD(T, UnsetParent)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void Destroy()", asMETHOD(T, Destroy)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void SetExpandBoundsToChildren(bool enabled)", asMETHOD(T, SetExpandBoundsToChildren)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("bool IsVisible()", asMETHOD(T, IsVisible)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool IsLocallyVisible()", asMETHOD(T, IsLocallyVisible)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool IsParentVisible()", asMETHOD(T, IsParentVisible)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetVisible(bool visible)", asMETHOD(T, SetVisible)); assert(r >= 0);
        }

        virtual const entt::entity GetEntityId() const
        {
            return _entityId;
        }

        virtual const UI::UIElementType GetType() const
        {
            return _elementType;
        }

        // Transform Functions
        virtual void SetTransform(const vec2& position, const vec2& size);

        virtual const vec2 GetScreenPosition() const
        {
            return _transform.position + _transform.localPosition;
        }
        virtual const vec2 GetLocalPosition() const
        {
            return _transform.parent ? _transform.localPosition : vec2(0, 0);
        }
        virtual const vec2 GetParentPosition() const
        {
            return _transform.parent ? _transform.position : vec2(0, 0);
        }
        virtual const vec2 GetPosition() const
        {
            return _transform.position + _transform.localPosition;
        }
        virtual void SetPosition(const vec2& position);
        
        virtual const vec2 GetAnchor() const
        {
            return _transform.anchor;
        }
        virtual void SetAnchor(const vec2& anchor);

        virtual const vec2 GetLocalAnchor() const
        {
            return _transform.localAnchor;
        }
        virtual void SetLocalAnchor(const vec2& localAnchor);
        
        virtual const vec2 GetSize() const
        {
            return _transform.size;
        }
        virtual void SetSize(const vec2& size);
        
        virtual const bool GetFillParentSize()
        {
            return _transform.fillParentSize;
        }
        virtual void SetFillParentSize(bool fillParent);

        virtual const u16 GetDepth() const
        {
            return _transform.sortData.depth;
        }
        virtual void SetDepth(const u16 depth);

        virtual void SetParent(BaseElement* parent);
        virtual void UnsetParent();

        virtual void SetExpandBoundsToChildren(bool expand);

        const bool IsVisible() const { return _visibility.visible && _visibility.parentVisible; }
        const bool IsLocallyVisible() const { return _visibility.visible; }
        const bool IsParentVisible() const { return _visibility.parentVisible; }
        virtual void SetVisible(bool visible);
    
        void SetCollisionEnabled(bool enabled);

        virtual void Destroy();

protected:
        static void MarkDirty(entt::registry* registry, entt::entity entId);
    
        static void UpdateChildTransforms(entt::registry* uiRegistry, UIComponent::Transform& parent);
        static void UpdateChildTransformsAngelScript(UISingleton::UIDataSingleton& uiDataSingleton, UIComponent::Transform& parent);

        static void UpdateChildVisibility(entt::registry* uiRegistry, const UIComponent::Transform& parent, bool parentVisibility);
        static void UpdateChildVisibilityAngelScript(UISingleton::UIDataSingleton& uiDataSingleton, const UIComponent::Transform& parent, bool parentVisibility);

        static void UpdateChildBounds(entt::registry* uiRegistry, UIComponent::Transform& transform);
        static void UpdateBounds(entt::registry* uiRegistry, UIComponent::Transform& parent);

    protected:
        entt::entity _entityId;
        UI::UIElementType _elementType;

        UIComponent::Transform _transform;
        UIComponent::Visibility _visibility;
    };
}
