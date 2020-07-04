#pragma once
#include <NovusTypes.h>
#include "../../../UI/UITypes.h"

struct UIChild
{
    u32 entity;
    UI::UIElementType type;
};

struct UITransform
{
    UITransform()
    { 
        children.reserve(8);
    }

    vec2 position = vec2(0,0);
    vec2 localPosition = vec2(0, 0);
    vec2 anchor = vec2(0, 0);
    vec2 localAnchor = vec2(0, 0);
    vec2 size = vec2(0, 0);
    union
    {
        u64 sortKey = 0;
        struct
        {
            u16 depth;
            UI::UIElementType type;
            entt::entity entId;
        } sortData;
    };
    u32 parent = 0;
    std::vector<UIChild> children;
    void* asObject = nullptr;

};
