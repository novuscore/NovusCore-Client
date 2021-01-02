#include "UITypeRegister.h"
#include "UIUtils.h"
#include "BaseElement.h"
#include "Panel.h"
#include "Label.h"
#include "Button.h"
#include "Inputfield.h"
#include "Checkbox.h"
#include "Slider.h"

#include "../ECS/Components/Image.h"

namespace UI
{
    static void Construct_BOX(u32 top, u32 right, u32 bottom, u32 left, Box* out)
    {
        out->top = top;
        out->right = right;
        out->bottom = bottom;
        out->left = left;
    }

    static void Construct_FBOX(f32 top, f32 right, f32 bottom, f32 left, FBox* out)
    {
        out->top = top;
        out->right = right;
        out->bottom = bottom;
        out->left = left;
    }

    static void Construct_IMAGESTYLESHEET(void* memory)
    {
        new(memory) ImageStylesheet();
    }

    static void Construct_TEXTSTYLESHEET(void* memory)
    {
        new(memory) TextStylesheet();
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

        // ImageStylesheet
        r = ScriptEngine::RegisterScriptClass("ImageStylesheet", sizeof(UI::ImageStylesheet), flags);
        assert(r >= 0);
        {        
            r = ScriptEngine::RegisterScriptClassFunction("void SetTexture(string texture)", asMETHOD(UI::ImageStylesheet, SetTexture)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetTexCoord(FBox texCoord)", asMETHOD(UI::ImageStylesheet, SetTexCoord)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(UI::ImageStylesheet, SetColor)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void SetBorderTexture(string texture)", asMETHOD(UI::ImageStylesheet, SetBorderTexture)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetBorderSize(Box borderSize)", asMETHOD(UI::ImageStylesheet, SetBorderSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetBorderInset(Box borderInset)", asMETHOD(UI::ImageStylesheet, SetBorderInset)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassConstructor("void f()", asFUNCTION(Construct_IMAGESTYLESHEET)); assert(r >= 0);
        }

        // TextStylesheet
        r = ScriptEngine::RegisterScriptClass("TextStylesheet", sizeof(UI::TextStylesheet), flags);
        assert(r >= 0);
        {        
            r = ScriptEngine::RegisterScriptClassFunction("void SetFontPath(string font)", asMETHOD(UI::TextStylesheet, SetFontPath)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetFontSize(f32 size)", asMETHOD(UI::TextStylesheet, SetFontSize)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetLineHeightMultiplier(f32 lineHeightMultiplier)", asMETHOD(UI::TextStylesheet, SetLineHeightMultiplier)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassFunction("void SetColor(Color color)", asMETHOD(UI::TextStylesheet, SetColor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineColor(Color color)", asMETHOD(UI::TextStylesheet, SetOutlineColor)); assert(r >= 0);
            r = ScriptEngine::RegisterScriptClassFunction("void SetOutlineWidth(f32 outlineWidth)", asMETHOD(UI::TextStylesheet, SetOutlineWidth)); assert(r >= 0);

            r = ScriptEngine::RegisterScriptClassConstructor("void f()", asFUNCTION(Construct_TEXTSTYLESHEET)); assert(r >= 0);
        }

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