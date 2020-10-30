#include "UITypeRegister.h"
#include "UIUtils.h"
#include "BaseElement.h"
#include "Panel.h"
#include "Label.h"
#include "Button.h"
#include "Inputfield.h"
#include "Checkbox.h"
#include "Slider.h"

namespace UI
{
    void RegisterTypes()
    {
        UIScripting::BaseElement::RegisterType();
        UIScripting::Panel::RegisterType();
        UIScripting::Label::RegisterType();
        UIScripting::Button::RegisterType();
        UIScripting::InputField::RegisterType();
        UIScripting::Checkbox::RegisterType();
        UIScripting::Slider::RegisterType();
        UIUtils::RegisterNamespace();
    }
}