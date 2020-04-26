#pragma once
#include <NovusTypes.h>
#include "UIAddElementQueueSingleton.h"

struct UITransform
{
    struct UIChild
    {
        u32 entity;
        UIElementData::UIElementType type;
    };

public:
    UITransform() : position(), localPosition(), anchor(), size(), depth(), parent(), children(), isDirty(false) 
    { 
        children.reserve(8);
    }

    vec2 position;
    vec2 localPosition;
    vec2 anchor;
    vec2 size;
    u16 depth;
    u32 parent;
    std::vector<UIChild> children;

    bool isDirty;
};