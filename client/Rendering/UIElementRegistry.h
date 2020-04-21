#pragma once
#include <vector>

class UIPanel;
class UILabel;
class UIButton;
class UIElementRegistry
{
public:
    static UIElementRegistry* Instance();

    std::vector<UIPanel*>& GetUIPanels() { return _UIPanels; }
    void AddUIPanel(UIPanel* panel) { _UIPanels.push_back(panel); }

    std::vector<UILabel*>& GetUILabels() { return _UILabels; }
    void AddUILabel(UILabel* label) { _UILabels.push_back(label); }

    std::vector<UIButton*>& GetUIButtons() { return _UIButtons; }
    void AddUIButton(UIButton* button) { _UIButtons.push_back(button); }

    void Clear();
    
private:
    UIElementRegistry() : _UIPanels(), _UILabels(), _UIButtons()
    { 
        _UIPanels.reserve(100);
        _UILabels.reserve(100);
        _UIButtons.reserve(100);
    }

    static UIElementRegistry* _instance;

    std::vector<UIPanel*> _UIPanels;
    std::vector<UILabel*> _UILabels;
    std::vector<UIButton*> _UIButtons;
};