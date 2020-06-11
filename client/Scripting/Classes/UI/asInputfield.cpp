#include "asInputfield.h"
#include "asLabel.h"
#include "asPanel.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/UI/UIEntityPoolSingleton.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"
#include "../../../ECS/Components/UI/UIAddElementQueueSingleton.h"

namespace UI
{
    asInputfield::asInputfield(entt::entity entityId) : asUITransform(entityId, UIElementData::UIElementType::UITYPE_INPUTFIELD) 
    {
        _panel = asPanel::CreatePanel();
        _panel->SetParent(this);

        _label = asLabel::CreateLabel();
        _label->SetParent(this);
        _label->SetAnchor(vec2(0, 1));
    }

    void asInputfield::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Inputfield", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<asUITransform, asInputfield>("UITransform");
        r = ScriptEngine::RegisterScriptFunction("Inputfield@ CreateInputfield()", asFUNCTION(asInputfield::CreateInputfield)); assert(r >= 0);

        // TransformEvents Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetEventFlag(int8 flags)", asMETHOD(asInputfield, SetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void UnsetEventFlag(int8 flags)", asMETHOD(asInputfield, UnsetEventFlag)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("bool IsFocusable()", asMETHOD(asInputfield, IsFocusable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptFunctionDef("void InputfieldEventCallback(Inputfield@ inputfield)"); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void OnFocused(InputfieldEventCallback@ cb)", asMETHOD(asInputfield, SetOnFocusCallback)); assert(r >= 0);

    }

    void asInputfield::SetSize(const vec2& size)
    {
        asUITransform::SetSize(size);

        //This should be replaced by a system for filling parent size.
        _panel->SetSize(size);
        _label->SetSize(size);
    }

    void asInputfield::SetOnFocusCallback(asIScriptFunction* callback)
    {
        _events.onFocusedCallback = callback;
        _events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([callback, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransformEvents& events = uiRegistry->get<UITransformEvents>(entId);

                events.onFocusedCallback = callback;
                events.SetFlag(UITransformEventsFlags::UIEVENTS_FLAG_FOCUSABLE);
            });
    }

    asInputfield* asInputfield::CreateInputfield()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIEntityPoolSingleton& entityPool = registry->ctx<UIEntityPoolSingleton>();
        UIAddElementQueueSingleton& addElementQueue = registry->ctx<UIAddElementQueueSingleton>();

        UIElementData elementData;
        entityPool.entityIdPool.try_dequeue(elementData.entityId);
        elementData.type = UIElementData::UIElementType::UITYPE_INPUTFIELD;

        asInputfield* inputfield = new asInputfield(elementData.entityId);

        elementData.asObject = inputfield;

        addElementQueue.elementPool.enqueue(elementData);
        return inputfield;
    }
}