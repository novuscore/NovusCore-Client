#pragma once
#include <vector>

namespace UI 
{
    class Panel;
    class Label;
    class Button;
    class InputField;
}

class UIElementRegistry
{
public:
    static UIElementRegistry* Instance();

    std::vector<UI::Panel*>& GetPanels() { return _Panels; }
    void AddPanel(UI::Panel* panel) { _Panels.push_back(panel); }

    std::vector<UI::Label*>& GetLabels() { return _Labels; }
    void AddLabel(UI::Label* label) { _Labels.push_back(label); }

    std::vector<UI::Button*>& GetButtons() { return _Buttons; }
    void AddButton(UI::Button* button) { _Buttons.push_back(button); }

    std::vector<UI::InputField*>& GetInputFields() { return _InputFields; }
    void AddInputField(UI::InputField* inputField) { _InputFields.push_back(inputField); }

    void Clear();
    
private:
    UIElementRegistry() : _Panels(), _Labels(), _Buttons()
    { 
        _Panels.reserve(100);
        _Labels.reserve(100);
        _Buttons.reserve(100);
    }

    static UIElementRegistry* _instance;

    std::vector<UI::Panel*> _Panels;
    std::vector<UI::Label*> _Labels;
    std::vector<UI::Button*> _Buttons;
    std::vector<UI::InputField*> _InputFields;
};