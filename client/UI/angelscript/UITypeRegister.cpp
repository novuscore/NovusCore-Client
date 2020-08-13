#include "UITypeRegister.h"
#include "RegistryUtils.h"
#include "BaseElement.h"
#include "Panel.h"
#include "Label.h"
#include "Button.h"
#include "Inputfield.h"
#include "Checkbox.h"

namespace UI
{
    void RegisterTypes()
    {
        UIUtils::Registry::RegisterNamespace();
        UIScripting::BaseElement::RegisterType();
        UIScripting::Panel::RegisterType();
        UIScripting::Label::RegisterType();
        UIScripting::Button::RegisterType();
        UIScripting::InputField::RegisterType();
        UIScripting::Checkbox::RegisterType();
    }
}