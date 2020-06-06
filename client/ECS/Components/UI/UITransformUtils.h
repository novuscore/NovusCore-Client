#pragma once
#include <NovusTypes.h>
#include "UITransform.h"

namespace UITransformUtils
{
    vec2 GetScreenPosition(const UITransform& transform)
    {
        return transform.position + transform.localPosition;
    };

    vec2 GetMinBounds(const UITransform& transform)
    {
        const vec2 screenPosition = GetScreenPosition(transform);

        return vec2(screenPosition.x - (transform.localAnchor.x * transform.size.x), screenPosition.y - (transform.localAnchor.y * transform.size.y));
    };

    vec2 GetMaxBounds(const UITransform& transform)
    {
        const vec2 screenPosition = GetScreenPosition(transform);

        return vec2(screenPosition.x + (transform.localAnchor.x * transform.size.x), screenPosition.y + (transform.localAnchor.y * transform.size.y));
    }
};