#pragma once
#include <NovusTypes.h>
#include <entt.hpp>

#include "../../ScriptEngine.h"
#include "../../../ECS/Components/UI/UITransform.h"
#include "../../../ECS/Components/UI/UIDataSingleton.h"

namespace UI
{
    class asUITransform
    {
    public:
        asUITransform(entt::entity entityId, UIElementType elementType);

        static void RegisterType()
        {
            i32 r = ScriptEngine::RegisterScriptClass("UITransform", 0, asOBJ_REF | asOBJ_NOCOUNT);
            assert(r >= 0);
            {
                RegisterBase<asUITransform>();
            }
        }

        template<class T>
        static void RegisterBase()
        {
            i32 r = ScriptEngine::RegisterScriptClassFunction("vec2 GetLocalPosition()", asMETHOD(T, GetLocalPosition)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetParentPosition()", asMETHOD(T, GetParentPosition)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetPosition()", asMETHOD(T, GetPosition)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetPosition(vec2 position)", asMETHOD(T, SetPosition)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetLocalAnchor()", asMETHOD(T, GetLocalAnchor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetLocalAnchor(vec2 anchor)", asMETHOD(T, SetLocalAnchor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetSize()", asMETHOD(T, GetSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetSize(vec2 size)", asMETHOD(T, SetSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("float GetDepth()", asMETHOD(T, GetDepth)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetDepth(float depth)", asMETHOD(T, SetDepth)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void SetParent(UITransform@ parent)", asMETHOD(T, SetParent)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetMinBound()", asMETHOD(T, GetMinBound)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetMaxBound()", asMETHOD(T, GetMaxBound)); assert(r >= 0);
        }

        virtual const entt::entity GetEntityId() const
        {
            return _entityId;
        }

        virtual const UIElementType GetEntityType() const
        {
            return _elementType;
        }

        // Transform Functions
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
        
        virtual const u16 GetDepth() const
        {
            return _transform.depth;
        }
        virtual void SetDepth(const u16 depth);

        virtual void SetParent(asUITransform* parent);

        const vec2 GetMinBound() const;
        const vec2 GetMaxBound() const;

    private:
        static void UpdateChildPositions(entt::registry* uiRegistry, UITransform& parent);

        static void UpdateChildPositionsInAngelScript(UI::UIDataSingleton& uiDataSingleton, UITransform& parent);

    protected:
        entt::entity _entityId;
        UIElementType _elementType;

        UITransform _transform;

    };
}
