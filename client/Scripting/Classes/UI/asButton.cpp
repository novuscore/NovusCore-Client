#include "asButton.h"
#include "asLabel.h"
#include "asPanel.h"
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/UI/UIEntityPoolSingleton.h"
#include "../../../ECS/Components/Singletons/ScriptSingleton.h"
#include "../../../ECS/Components/UI/UIAddElementQueueSingleton.h"

namespace UI
{
    asButton::asButton(entt::entity entityId) : asUITransform(entityId, UIElementData::UIElementType::UITYPE_BUTTON) 
    {
        _panel = asPanel::CreatePanel();
        _panel->SetParent(this);
        _panel->SetSize(GetSize());

        _label = asLabel::CreateLabel();
        _label->SetParent(_panel);
        _label->SetSize(GetSize());
    }
    
    void asButton::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Button", 0, asOBJ_REF | asOBJ_NOCOUNT);
        r = ScriptEngine::RegisterScriptInheritance<asUITransform, asButton>("UITransform");
        r = ScriptEngine::RegisterScriptFunction("Button@ CreateButton()", asFUNCTION(asButton::CreateButton)); assert(r >= 0);

        //Button Functions.
        r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(asButton, IsClickable)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(asButton, GetText)); assert(r >= 0);

        //Label Functions
        r = ScriptEngine::RegisterScriptClassFunction("void SetText(string text)", asMETHOD(asButton, SetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("string GetText()", asMETHOD(asButton, GetText)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(asButton, SetTextColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetColor()", asMETHOD(asButton, GetTextColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(Color color)", asMETHOD(asButton, SetTextOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("Color GetOutlineColor()", asMETHOD(asButton, GetTextOutlineColor)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(float width)", asMETHOD(asButton, SetTextOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("float GetOutlineWidth()", asMETHOD(asButton, GetTextOutlineWidth)); assert(r >= 0);
        r = ScriptEngine::RegisterScriptClassFunction("void SetFont(string fontPath, float fontSize)", asMETHOD(asButton, SetTextFont)); assert(r >= 0);
    }

    const bool asButton::IsClickable() const
    {
        return _panel->IsClickable();
    }

    void asButton::SetOnClickCallback(asIScriptFunction* callback)
    {
        _panel->SetOnClickCallback(callback);
    }

    void asButton::SetText(const std::string& text)
    {
        _label->SetText(text);
    }
    const std::string& asButton::GetText() const
    {
        return _label->GetText();
    }

    void asButton::SetTextColor(const Color& color)
    {
        _label->SetColor(color);
    }
    const Color& asButton::GetTextColor() const
    {
        return _label->GetColor();
    }

    void asButton::SetTextOutlineColor(const Color& outlineColor)
    {
        _label->SetOutlineColor(outlineColor);
    }
    const Color& asButton::GetTextOutlineColor() const
    {
        return _label->GetOutlineColor();
    }

    void asButton::SetTextOutlineWidth(f32 outlineWidth)
    {
        _label->SetOutlineWidth(outlineWidth);
    }
    const f32 asButton::GetTextOutlineWidth() const
    {
        return _label->GetOutlineWidth();
    }

    void asButton::SetTextFont(std::string fontPath, f32 fontSize)
    {
        _label->SetFont(fontPath, fontSize);
    }

    asButton* asButton::CreateButton()
    {
        entt::registry* registry = ServiceLocator::GetUIRegistry();
        UIEntityPoolSingleton& entityPool = registry->ctx<UIEntityPoolSingleton>();
        UIAddElementQueueSingleton& addElementQueue = registry->ctx<UIAddElementQueueSingleton>();

        UIElementData elementData;
        entityPool.entityIdPool.try_dequeue(elementData.entityId);
        elementData.type = UIElementData::UIElementType::UITYPE_BUTTON;

        asButton* button = new asButton(elementData.entityId);

        elementData.asObject = button;

        addElementQueue.elementPool.enqueue(elementData);
        return button;
    }
}