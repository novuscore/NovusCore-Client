#pragma once
#include <NovusTypes.h>
#include <entt.hpp>

#include "../../ScriptEngine.h"
#include "../../../ECS/Components/UI/UITransform.h"

namespace UI
{
    class asUITransform
    {
    public:
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

            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetAnchor()", asMETHOD(T, GetAnchor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetAnchor(vec2 anchor)", asMETHOD(T, SetAnchor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("vec2 GetSize()", asMETHOD(T, GetSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetSize(vec2 size)", asMETHOD(T, SetSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("float GetDepth()", asMETHOD(T, GetDepth)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetDepth(float depth)", asMETHOD(T, SetDepth)); assert(r >= 0);
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
        
        virtual const vec2 GetSize() const
        {
            return _transform.size;
        }
        virtual void SetSize(const vec2& size);
        
        virtual const u16 GetDepth() const
        {
            return _transform.depth;
        }
        virtual void SetDepth(const u16& depth);

    private:
        static void UpdateChildren(entt::registry* registry, UITransform& transform, vec2 position);

    protected:
        entt::entity _entityId;
        UITransform _transform;
    };
}
