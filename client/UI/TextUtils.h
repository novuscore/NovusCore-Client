#pragma once
#include <NovusTypes.h>
#include "../ECS/Components/UI/UIText.h"

namespace UI::TextUtils
{
    inline static float GetTextAlignment(TextAlignment alignment)
    {
        switch (alignment)
        {
        case TextAlignment::LEFT:
            return 0.f;
        case TextAlignment::CENTER:
            return 0.5f;
        case TextAlignment::RIGHT:
            return 1.f;
        default:
            assert(false); // We should never get here.
            return 0.f;
        }
    }

    /*
    *   Calculate Pushback index.
    *   text: Text to calculate pushback for.
    *   writeHead: Position of the writeHead.
    *   bufferDecimal: How much extra text we want before or ahead of the writeHead atleast when moving the pushback.
    *   maxWidth: Max width of a line.
    */
    size_t CalculatePushback(const UIText& text, size_t writeHead, f32 bufferDecimal, f32 maxWidth);

    /*
    *   Calculate Line Widths & Line Break points.
    *   text: Text to calculate for.
    *   maxWidth: Max width of a line.
    *   maxHeight: Max heights of all lines summed.
    *   lineWidths: Calculated line widths.
    *   lineBreakPoints: Calculated line breakpoints.
    */
    size_t CalculateLineWidthsAndBreaks(const UIText& text, f32 maxWidth, f32 maxHeight, std::vector<f32>& lineWidths, std::vector<size_t> lineBreakPoints);
};