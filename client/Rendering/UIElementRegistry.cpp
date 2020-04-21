#include "UIElementRegistry.h"
#include "../Scripting/Classes/UI/UIPanel.h"
#include "../Scripting/Classes/UI/UILabel.h"

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
    for (UIPanel* panel : _UIPanels)
    {
        delete panel;
    }
    _UIPanels.clear();

    for (UILabel* label : _UILabels)
    {
        delete label;
    }
    _UILabels.clear();
}
