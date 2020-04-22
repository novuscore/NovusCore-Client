#include "Panel.h"
#include "../../Rendering/UIElementRegistry.h"

namespace UI
{
    // Public
    Panel::Panel(const vec2& pos, const vec2& size)
        : Widget(pos, size)
        , _color(1.0f, 1.0f, 1.0f, 1.0f), _clickable(true), _draggable(false), _isDragging(false), _deltaDragPosition(0, 0), _didDrag(false)
    {
        UIElementRegistry::Instance()->AddPanel(this);
    }

    void Panel::RegisterType()
    {
        i32 r = ScriptEngine::RegisterScriptClass("Panel", 0, asOBJ_REF | asOBJ_NOCOUNT);
        assert(r >= 0);
        {
            r = ScriptEngine::RegisterScriptInheritance<Widget, Panel>("Widget");
            r = ScriptEngine::RegisterScriptFunction("Panel@ CreatePanel(vec2 pos = vec2(0, 0), vec2 size = vec2(100, 100))", asFUNCTION(Panel::CreatePanel)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(Panel, SetColor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string texture)", asMETHOD(Panel, SetTexture)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetClickable(bool value)", asMETHOD(Panel, SetClickable)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool IsClickable()", asMETHOD(Panel, IsClickable)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetDraggable(bool value)", asMETHOD(Panel, SetDraggable)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("bool IsDraggable()", asMETHOD(Panel, IsDraggable)); assert(r >= 0);

            // Callback
            r = ScriptEngine::RegisterScriptFunctionDef("void OnPanelClickCallback(Panel@ panel)"); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void OnClick(OnPanelClickCallback@ cb)", asMETHOD(Panel, SetOnClick)); assert(r >= 0);
        }
    }

    void Panel::SetModelID(Renderer::ModelID modelID)
    {
        Widget::SetModelID(modelID);
    }

    void Panel::SetTexture(std::string& texture) 
    { 
        Widget::SetTexture(texture);
    }

    void Panel::SetTextureID(Renderer::TextureID textureID)
    {
        Widget::SetTextureID(textureID);
    }

    void Panel::SetColor(const Color& color)
    { 
        _color = color;
        SetDirty();
    }

    void Panel::SetClickable(bool value)
    {
        _clickable = value;
    }

    void Panel::SetDraggable(bool value)
    {
        _draggable = value;
    }

    void Panel::SetOnClick(asIScriptFunction* function)
    {
        _onClickCallback = function;
    }

    void Panel::OnClick()
    {
        if (!_onClickCallback)
            return;

        asIScriptContext* context = ScriptEngine::GetScriptContext();
        {
            context->Prepare(_onClickCallback);
            {
                context->SetArgObject(0, this);
            }
            context->Execute();
        }
    }

    void Panel::BeingDrag(const vec2& deltaDragPosition)
    {
        _deltaDragPosition = deltaDragPosition;
        _isDragging = true;
    }

    void Panel::SetDidDrag()
    {
        _didDrag = true;
    }

    void Panel::EndDrag()
    {
        _deltaDragPosition = vec2(0, 0);
        _didDrag = false;
        _isDragging = false;
    }

    Panel* Panel::CreatePanel(const vec2& pos, const vec2& size)
    {
        Panel* panel = new Panel(pos, size);
        return panel;
    }
}