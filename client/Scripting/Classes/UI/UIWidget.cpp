#include "UIWidget.h"

void UIWidget::RegisterType()
{
    i32 r = ScriptEngine::RegisterScriptClass("UIWidget", 0, asOBJ_REF | asOBJ_NOCOUNT);
    assert(r >= 0);
    {
        RegisterBase<UIWidget>();
    }
}

std::string UIWidget::GetTypeName()
{
    return "UIWidget";
}
void UIWidget::SetPosition(const vec2& pos, f32 depth)
{
    float d = depth == 0 ? _widget->GetPosition().z : depth;
    _widget->SetPosition(vec3(pos.x, pos.y, d));
}
void UIWidget::SetSize(const vec2& size)
{
    _widget->SetSize(size);
}
vec2 UIWidget::GetPosition()
{
    return _widget->GetPosition();
}
vec2 UIWidget::GetSize()
{
    return _widget->GetSize();
}
float UIWidget::GetDepth()
{
    return _widget->GetPosition().z;
}