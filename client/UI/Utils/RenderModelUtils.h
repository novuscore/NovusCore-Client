#pragma once
#include <NovusTypes.h>
#include <entity/fwd.hpp>
#include "../UITypes.h"

#include "../Components/Singletons/UIDataSingleton.h"

namespace UIUtils::RenderModel
{
    inline void CreatePanelVertices(const entt::registry* registry, const vec2& pos, const vec2& size, const UI::FBox& texCoords, UI::UIVertex* vertices)
    {
        const UISingleton::UIDataSingleton& dataSingleton = registry->ctx<UISingleton::UIDataSingleton>();

        vec2 upperLeftPos = vec2(pos.x, pos.y);
        vec2 upperRightPos = vec2(pos.x + size.x, pos.y);
        vec2 lowerLeftPos = vec2(pos.x, pos.y + size.y);
        vec2 lowerRightPos = vec2(pos.x + size.x, pos.y + size.y);

        // Convert positions to UI render space (0.0 to 1.0).
        upperLeftPos /= dataSingleton.UIRESOLUTION;
        upperRightPos /= dataSingleton.UIRESOLUTION;
        lowerLeftPos /= dataSingleton.UIRESOLUTION;
        lowerRightPos /= dataSingleton.UIRESOLUTION;

        // Flip Y.
        upperLeftPos.y = 1.0f - upperLeftPos.y;
        upperRightPos.y = 1.0f - upperRightPos.y;
        lowerLeftPos.y = 1.0f - lowerLeftPos.y;
        lowerRightPos.y = 1.0f - lowerRightPos.y;

        // UI Vertices. (Pos, UV)
        vertices[0] = { upperLeftPos, vec2(texCoords.left, texCoords.top) };

        vertices[1] = { upperRightPos, vec2(texCoords.right, texCoords.top) };

        vertices[2] = { lowerLeftPos, vec2(texCoords.left, texCoords.bottom) };

        vertices[3] = { lowerRightPos, vec2(texCoords.right, texCoords.bottom) };
    }
};