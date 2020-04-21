#pragma once
#include <NovusTypes.h>
#include "UIWidget.h"
#include "../../../UI/Widget/Label.h"

class UILabel : public UIWidget
{
public:
    UILabel(const vec2& pos, const vec2& size);
    static void RegisterType();

    std::string GetTypeName() override;
    void SetColor(const vec3& color);
    void SetOutlineWidth(f32 width);
    void SetOutlineColor(const vec3& color);

    void SetText(std::string& text);
    void SetFont(std::string& fontPath, f32 fontSize);

    UI::Label* GetInternal() { return &_label; }
private:
    static UILabel* CreateLabel(const vec2& pos, const vec2& size);

private:
    UI::Label _label;
};