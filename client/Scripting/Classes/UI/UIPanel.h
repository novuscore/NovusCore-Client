#pragma once
#include <NovusTypes.h>
#include "UIWidget.h"
#include "../../../UI/Widget/Panel.h"

class UIPanel : public UIWidget
{
public:
    UIPanel(const vec2& pos, const vec2& size);
    static void RegisterType();

    std::string GetTypeName() override;
    void SetColor(const vec4& color);

    void SetTexture(std::string& texture);

    bool IsClickable();
    void SetClickable(bool value);

    bool IsDragable();
    void SetDragable(bool value);

    void SetOnClick(asIScriptFunction* function);
    void OnClick();

    UI::Panel* GetInternal();
private:
    static UIPanel* CreatePanel(const vec2& pos, const vec2& size);

private:
    UI::Panel _panel;
    asIScriptFunction* _onClickCallback;
};