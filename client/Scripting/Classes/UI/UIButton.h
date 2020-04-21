#pragma once
#include <NovusTypes.h>
#include "UIWidget.h"
#include "../../../UI/Widget/Button.h"

class UIButton : public UIWidget
{
public:
    UIButton(const vec2& pos, const vec2& size);
    static void RegisterType();

    std::string GetTypeName() override;
    
    void SetText(std::string& text);

    void SetColor(const vec4& color);

    void SetTexture(std::string& texture);

    bool IsClickable();
    void SetClickable(bool value);

    void SetOnClick(asIScriptFunction* function);
    void OnClick();

    UI::Button* GetInternal() { return &_button; };

private:
    static UIButton* CreateButton(const vec2& pos, const vec2& size);

private:
    UI::Button _button;
    asIScriptFunction* _onClickCallback;
};