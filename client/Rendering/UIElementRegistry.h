#pragma once
#include <vector>

class UIPanel;
class UILabel;
class UIElementRegistry
{
public:
    static UIElementRegistry* Instance();

    std::vector<UIPanel*>& GetUIPanels() { return _UIPanels; }
    void AddUIPanel(UIPanel* panel) { _UIPanels.push_back(panel); }

    std::vector<UILabel*>& GetUILabels() { return _UILabels; }
    void AddUILabel(UILabel* label) { _UILabels.push_back(label); }

    void Clear();
    
private:
    UIElementRegistry() : _UIPanels(), _UILabels() 
    { 
        _UIPanels.reserve(100);
        _UILabels.reserve(100);
    }

    static UIElementRegistry* _instance;

    std::vector<UIPanel*> _UIPanels;
    std::vector<UILabel*> _UILabels;
};