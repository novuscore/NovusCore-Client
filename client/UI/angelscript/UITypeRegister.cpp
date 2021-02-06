#include "UITypeRegister.h"
#include "UIUtils.h"
#include "BaseElement.h"
#include "EventElement.h"
#include "Panel.h"
#include "Label.h"
#include "Button.h"
#include "Inputfield.h"
#include "Checkbox.h"
#include "Slider.h"

#include "../Stylesheets/ImageStylesheet.h"
#include "../Stylesheets/TextStylesheet.h"

namespace UI
{
    static void Construct_BOX(u32 top, u32 right, u32 bottom, u32 left, Box* out)
    {
        *out = { top, right, bottom, left };
    }

    static void Construct_FBOX(f32 top, f32 right, f32 bottom, f32 left, FBox* out)
    {
        *out = { top, right, bottom, left };
    }

    void RegisterTypes()
    {
        /*
        *   TODO: Move.
        */
        u32 flags = asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS | asOBJ_APP_CLASS_CONSTRUCTOR | asOBJ_APP_CLASS_ASSIGNMENT | asOBJ_APP_CLASS_COPY_CONSTRUCTOR;

        //Box
        i32 r = ScriptEngine::RegisterScriptClass("Box", sizeof(Box), flags); assert(r >= 0);
        {
            r = ScriptEngine::RegisterScriptClassProperty("uint top", asOFFSET(Box, top)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassProperty("uint right", asOFFSET(Box, right)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassProperty("uint bottom", asOFFSET(Box, bottom)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassProperty("uint left", asOFFSET(Box, left)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassConstructor("void f(uint top, uint right, uint bottom, uint left)", asFUNCTION(Construct_BOX)); assert(r >= 0);
        }

        //FBox
        r = ScriptEngine::RegisterScriptClass("FBox", sizeof(FBox), flags | asOBJ_APP_CLASS_ALLFLOATS); assert(r >= 0);
        {
            r = ScriptEngine::RegisterScriptClassProperty("float top", asOFFSET(FBox, top)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassProperty("float right", asOFFSET(FBox, right)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassProperty("float bottom", asOFFSET(FBox, bottom)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassProperty("float left", asOFFSET(FBox, left)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassConstructor("void f(float top, float right, float bottom, float left)", asFUNCTION(Construct_FBOX)); assert(r >= 0);
        }

        UI::ImageStylesheet::RegisterType();
        UI::TextStylesheet::RegisterType();

        UIScripting::BaseElement::RegisterType();
        UIScripting::EventElement::RegisterType();
        UIScripting::Panel::RegisterType();
        UIScripting::Label::RegisterType();
        UIScripting::Button::RegisterType();
        UIScripting::InputField::RegisterType();
        UIScripting::Checkbox::RegisterType();
        UIScripting::Slider::RegisterType();
        UIUtils::RegisterNamespace();
    }
}