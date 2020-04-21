#include "UIElementRegistry.h"
#include "../UI/Widget/Panel.h"
#include "../UI/Widget/Label.h"
#include "../UI/Widget/Button.h"

UIElementRegistry* UIElementRegistry::_instance = nullptr;
UIElementRegistry* UIElementRegistry::Instance()
{
    if (!_instance)
    {
        _instance = new UIElementRegistry();
    }

    return _instance;
}

void UIElementRegistry::Clear()
{
    for (UI::Panel* panel : _Panels)
    {
        delete panel;
    }
    _Panels.clear();

    for (UI::Label* label : _Labels)
    {
        delete label;
    }
    _Labels.clear();

    for (UI::Button* button : _Buttons)
    {
        delete button;
    }
    _Buttons.clear();
}
