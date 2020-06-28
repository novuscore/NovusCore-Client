#pragma once
#include <NovusTypes.h>

enum class UIElementType
{
    UITYPE_NONE,

    UITYPE_PANEL,
    UITYPE_TEXT,
    UITYPE_BUTTON,
    UITYPE_INPUTFIELD
};

struct UIElementData
{
    entt::entity entityId;
    UIElementType type;
    void* asObject;
};

struct UITransform
{
    struct UIChild
    {
        u32 entity;
        UIElementType type;
    };

public:
    UITransform() : position(), localPosition(), anchor(), localAnchor(), size(), depth(), parent(), children(), type(UIElementType::UITYPE_NONE), isDirty(false), asObject(nullptr)
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
    void* asObject;

    UIElementType type;
    bool isDirty;
};
