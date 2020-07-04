#pragma once
#include <NovusTypes.h>
#include "UITypes.h"

struct UIChild
{
    u32 entity;
    UI::UIElementType type;
};

struct UITransform
{
    UITransform() : position(), localPosition(), anchor(), localAnchor(), size(), depth(), parent(), children(), asObject(nullptr), type(UI::UIElementType::UITYPE_NONE)
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

    UI::UIElementType type;
};
