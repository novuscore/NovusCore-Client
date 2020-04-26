#include "asLabel.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/UI/UIEntityPoolSingleton.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"
#include "../../../ECS/Components/UI/UIAddElementQueueSingleton.h"

namespace UI
{
    void asLabel::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Label", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptFunction("Label@ CreateLabel()", asFUNCTION(asLabel::CreateLabel)); assert(r >= 0);

        // Transform Functions
        r = ScriptEngine::RegisterScriptClassFunction("vec2 GetPosition()", asMETHOD(asLabel, GetPosition)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetPosition(vec2 pos)", asMETHOD(asLabel, SetPosition)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("vec2 GetLocalPosition()", asMETHOD(asLabel, GetLocalPosition)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetLocalPosition(vec2 localPosition)", asMETHOD(asLabel, SetLocalPosition)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("vec2 GetAnchor()", asMETHOD(asLabel, GetAnchor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetAnchor(vec2 anchor)", asMETHOD(asLabel, SetAnchor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("vec2 GetSize()", asMETHOD(asLabel, GetSize)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetSize(vec2 size)", asMETHOD(asLabel, SetSize)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetDepth()", asMETHOD(asLabel, GetDepth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetDepth(float depth)", asMETHOD(asLabel, SetDepth)); assert(r >= 0);

        //Text Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(asLabel, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(asLabel, GetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(asLabel, SetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(asLabel, GetColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(Color color)", asMETHOD(asLabel, SetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetOutlineColor()", asMETHOD(asLabel, GetOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float width)", asMETHOD(asLabel, SetOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetOutlineWidth()", asMETHOD(asLabel, GetOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(asLabel, SetFont)); assert(r >= 0);
    }

    void asLabel::SetPosition(const vec2& position)
    {
        _transform.position = position;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([position, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.isDirty = true;
                uiTransform.position = position;
            });
    }

    void asLabel::SetLocalPosition(const vec2& localPosition)
    {
        _transform.localPosition = localPosition;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([localPosition, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.isDirty = true;
                uiTransform.localPosition = localPosition;
            });
    }

    void asLabel::SetAnchor(const vec2& anchor)
    {
        _transform.anchor = anchor;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([anchor, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.isDirty = true;
                uiTransform.anchor = anchor;
            });
    }

    void asLabel::SetSize(const vec2& size)
    {
        _transform.size = size;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([size, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.isDirty = true;
                uiTransform.size = size;
            });
    }

    void asLabel::SetDepth(const u16& depth)
    {
        _transform.depth = depth;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([depth, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UITransform& uiTransform = uiRegistry->get<UITransform>(entId);

                uiTransform.isDirty = true;
                uiTransform.depth = depth;
            });
    }

    void asLabel::SetText(const std::string& text)
    {
        _text.text = text;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([text, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.text = text;
                uiText.isDirty = true;
            });
    }

    void asLabel::SetColor(const Color& color)
    {
        _text.color = color;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([color, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.color = color;
                uiText.isDirty = true;
            });
    }

    void asLabel::SetOutlineColor(const Color& outlineColor)
    {
        _text.outlineColor = outlineColor;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([outlineColor, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.outlineColor = outlineColor;
                uiText.isDirty = true;
            });
    }

    void asLabel::SetOutlineWidth(f32 outlineWidth)
    {
        _text.outlineWidth = outlineWidth;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([outlineWidth, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.outlineWidth = outlineWidth;
                uiText.isDirty = true;
            });
    }

    void asLabel::SetFont(std::string fontPath, f32 fontSize)
    {
        _text.fontPath = fontPath;

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        entt::entity entId = _entityId;

        gameRegistry->ctx<ScriptSingleton>().AddTransaction([fontPath, fontSize, entId]()
            {
                entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
                UIText& uiText = uiRegistry->get<UIText>(entId);

                uiText.fontPath = fontPath;
                uiText.fontSize = fontSize;
                uiText.isDirty = true;
            });
    }

    asLabel* asLabel::CreateLabel()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIEntityPoolSingleton& entityPool = registry->ctx<UIEntityPoolSingleton>();
        UIAddElementQueueSingleton& addElementQueue = registry->ctx<UIAddElementQueueSingleton>();

        UIElementData elementData;
        entityPool.entityIdPool.try_dequeue(elementData.entityId);
        elementData.type = UIElementData::UIElementType::UITYPE_TEXT;

        asLabel* label = new asLabel();
        label->_entityId = elementData.entityId;

        elementData.asObject = label;

        addElementQueue.elementPool.enqueue(elementData);
        return label;
    }
}