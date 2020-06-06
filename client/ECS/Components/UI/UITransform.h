#pragma once
#include <NovusTypes.h>

struct UIElementData
{
    enum class UIElementType
    {
        UITYPE_PANEL,
        UITYPE_TEXT,
        UITYPE_BUTTON,
        UITYPE_INPUTFIELD
    };

    entt::entity entityId;
    UIElementType type;
    void* asObject;
};

struct UITransform
{
    struct UIChild
    {
        u32 entity;
        UIElementData::UIElementType type;
    };

public:
    UITransform() : position(), localPosition(), anchor(), localAnchor(), size(), depth(), parent(), children(), isDirty(false) 
    { 
        children.reserve(8);
    }

    vec2 position;
    vec2 localPosition;
    vec2 anchor;
    vec2 localAnchor;
    vec2 size;
    u16 depth;
    u32 parent;
    std::vector<UIChild> children;

    bool isDirty;
};
