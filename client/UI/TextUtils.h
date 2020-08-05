#pragma once
#include <NovusTypes.h>
#include "../ECS/Components/UI/UIText.h"

namespace UI::TextUtils
{
    inline static float GetHorizontalAlignment(TextHorizontalAlignment alignment)
    {
        switch (alignment)
        {
        case TextHorizontalAlignment::LEFT:
            return 0.f;
        case TextHorizontalAlignment::CENTER:
            return 0.5f;
        case TextHorizontalAlignment::RIGHT:
            return 1.f;
        default:
            assert(false); // We should never get here.
            return 0.f;
        }
    }

    inline static float GetVerticalAlignment(TextVerticalAlignment alignment)
    {
        switch (alignment)
        {
        case TextVerticalAlignment::TOP:
            return 0.f;
        case TextVerticalAlignment::CENTER:
            return 0.5f;
        case TextVerticalAlignment::BOTTOM:
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
    *   maxHeight: Max height of the text.
    */
    size_t CalculatePushback(const UIText& text, size_t writeHead, f32 bufferDecimal, f32 maxWidth, f32 maxHeight);
    size_t CalculateSinglelinePushback(const UI::UIText& text, const size_t writeHead, const f32 maxWidth, const f32 bufferDecimal);
    size_t CalculateMultilinePushback(const UIText& text, const size_t writeHead, const f32 maxWidth, const f32 maxHeight);


    /*
    *   Calculate Line Widths & Line Break points from pushback.
    *   text: Text to calculate for.
    *   maxWidth: Max width of a line.
    *   maxHeight: Max heights of all lines summed.
    *   lineWidths: Calculated line widths.
    *   lineBreakPoints: Calculated line breakpoints.
    */
    size_t CalculateLineWidthsAndBreaks(const UIText& text, f32 maxWidth, f32 maxHeight, std::vector<f32>& lineWidths, std::vector<size_t>& lineBreakPoints);
    /*
    *   Calculate Line Widths & Line Break points indepentent from pushback.
    */
    void CalculateAllLineWidthsAndBreaks(const UIText& text, f32 maxWidth, std::vector<f32>& lineWidths, std::vector<size_t>& lineBreakPoints);

};