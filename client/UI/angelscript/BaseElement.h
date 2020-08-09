#pragma once
#include <NovusTypes.h>
#include <entity/entity.hpp>
#include <shared_mutex>

#include "../../Scripting/ScriptEngine.h"
#include "../ECS/Components/Transform.h"
#include "../ECS/Components/Visibility.h"
#include "../ECS/Components/Singletons/UIDataSingleton.h"
#include "LockToken.h"

namespace UIScripting
{
    class BaseElement
    {
        friend struct ::UISingleton::UIDataSingleton;
        friend class LockToken;

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
            r = ScriptEngine::RegisterScriptClassFunction("void SetPosition(vec2 position)", asMETHOD(T, SetPosition)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetLocalAnchor()", asMETHOD(T, GetLocalAnchor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetLocalAnchor(vec2 anchor)", asMETHOD(T, SetLocalAnchor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetSize()", asMETHOD(T, GetSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetSize(vec2 size)", asMETHOD(T, SetSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool GetFillParentSize()", asMETHOD(T, GetFillParentSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetFillParentSize(bool fillParent)", asMETHOD(T, SetFillParentSize)); assert(r >= 0);
            
            r = ScriptEngine::RegisterScriptClassFunction("uint8 GetDepthLayer()", asMETHOD(T, GetDepthLayer)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetDepthLayer(uint8 layer)", asMETHOD(T, SetDepthLayer)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("uint16 GetDepth()", asMETHOD(T, GetDepth)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetDepth(uint16 depth)", asMETHOD(T, SetDepth)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void SetParent(Transform@ parent)", asMETHOD(T, SetParent)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void UnsetParent()", asMETHOD(T, UnsetParent)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void Destroy()", asMETHOD(T, Destroy)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void SetExpandBoundsToChildren(bool enabled)", asMETHOD(T, SetExpandBoundsToChildren)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("bool IsVisible()", asMETHOD(T, IsVisible)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool IsLocallyVisible()", asMETHOD(T, IsLocallyVisible)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool IsParentVisible()", asMETHOD(T, IsParentVisible)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetVisible(bool visible)", asMETHOD(T, SetVisible)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("LockToken@ GetLock(uint8 lockState)", asMETHOD(T, GetLock)); assert(r >= 0);
        }

        const entt::entity GetEntityId() const { return _entityId; }
        const UI::UIElementType GetType() const { return _elementType; }

        // Transform Functions
        virtual void SetTransform(const vec2& position, const vec2& size);

        const vec2 GetScreenPosition() const;
        const vec2 GetLocalPosition() const;
        const vec2 GetParentPosition() const;
        virtual void SetPosition(const vec2& position);
        
        const vec2 GetAnchor() const;
        virtual void SetAnchor(const vec2& anchor);

        const vec2 GetLocalAnchor() const;
        virtual void SetLocalAnchor(const vec2& localAnchor);
        
        const vec2 GetSize() const;
        virtual void SetSize(const vec2& size);
        
        const bool GetFillParentSize();
        virtual void SetFillParentSize(bool fillParent);

        const UI::DepthLayer GetDepthLayer() const;
        virtual void SetDepthLayer(const UI::DepthLayer layer);

        const u16 GetDepth() const;
        virtual void SetDepth(const u16 depth);

        virtual void SetParent(BaseElement* parent);
        virtual void UnsetParent();

        const bool GetExpandBoundsToChildren();
        virtual void SetExpandBoundsToChildren(bool expand);

        const bool IsVisible() const;
        const bool IsLocallyVisible() const;
        const bool IsParentVisible() const;
        virtual void SetVisible(bool visible);
    
        void SetCollisionEnabled(bool enabled);

        virtual void Destroy();

protected:
        static void MarkDirty(entt::registry* registry, entt::entity entId);

        inline LockToken* GetLock(LockState state)
        {
            return new LockToken(_mutex, state);
        }

    protected:
        entt::entity _entityId;
        UI::UIElementType _elementType;

        std::shared_mutex _mutex;
    };
}
