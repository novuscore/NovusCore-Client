#include "TextUtils.h"
#include <tracy/Tracy.hpp>

namespace UIUtils::Text
{
    size_t CalculatePushback(const UIComponent::Text& text, size_t writeHead, f32 bufferDecimal, f32 maxWidth, f32 maxHeight)
    {     
        ZoneScoped;
        /*
        *   TODO:
        *   - Move entire lines for multi-line fields.
        */

        if (!text.font)
            return 0;

        if (text.isMultiline)
            return CalculateMultilinePushback(text, writeHead, maxWidth, maxHeight);
        else
            return CalculateSinglelinePushback(text, writeHead, maxWidth, bufferDecimal);
    }

    size_t CalculateSinglelinePushback(const UIComponent::Text& text, const size_t writeHead, const f32 maxWidth, const f32 bufferDecimal)
    {
        size_t oldPushback = Math::Min(text.pushback, text.text.length() - 1);

        f32 lineLength = 0.f;
        if (oldPushback <= writeHead)
        {
            size_t pushBackPoint = oldPushback;
            bool reachedPercent = false;
            for (size_t i = oldPushback; i < writeHead; i++)
            {
                if (std::isspace(text.text[i]))
                    lineLength += text.fontSize * 0.15f;
                else
                    lineLength += text.font->GetChar(text.text[i]).advance;

                if (!reachedPercent && lineLength > maxWidth * bufferDecimal)
                {
                    reachedPercent = true;
                    pushBackPoint = i;
                }
                else if (lineLength > maxWidth)
                {
                    reachedPercent = false;
                    lineLength = 0.f;
                }
            }

            return pushBackPoint;
        }

        for (size_t i = oldPushback; i > 0; i--)
        {
            if (std::isspace(text.text[i]))
                lineLength += text.fontSize * 0.15f;
            else
                lineLength += text.font->GetChar(text.text[i]).advance;

            if (lineLength > maxWidth * bufferDecimal)
                return i;
        }

        return 0;
    }

    size_t CalculateMultilinePushback(const UIComponent::Text& text, const size_t writeHead, const f32 maxWidth, const f32 maxHeight)
    {
        u32 maxLines = static_cast<u32>(maxHeight / (text.fontSize * text.lineHeight));
        std::vector<f32> lineWidths;
        std::vector<size_t> lineBreakPoints;
        CalculateAllLineWidthsAndBreaks(text, maxWidth, lineWidths, lineBreakPoints);

        if (lineWidths.size() <= maxLines)
            return 0;

        size_t writeHeadLine = 0;
        size_t pushbackLine = 0;
        for (size_t i = lineBreakPoints.size()-1; i > 0; i--)
        {
            if (lineBreakPoints[i] > writeHead)
                writeHeadLine = i - 1;
            if (lineBreakPoints[i] > text.pushback)
                writeHeadLine = i - 1;
        }

        if (writeHeadLine < pushbackLine)
        {
            pushbackLine = writeHeadLine;
        }
        else if (writeHeadLine > pushbackLine + maxLines)
        {
            pushbackLine = writeHead - maxLines;
        }

        return lineBreakPoints[pushbackLine];
    }
    
    size_t CalculateLineWidthsAndBreaks(const UIComponent::Text& text, f32 maxWidth, f32 maxHeight, std::vector<f32>& lineWidths, std::vector<size_t>& lineBreakPoints)
    {
        ZoneScoped;
        assert(text.font);

        lineWidths.clear();
        lineWidths.push_back(0);
        lineBreakPoints.clear();

        u32 maxLines = static_cast<u32>(text.isMultiline ? 1 : maxHeight / (text.fontSize * text.lineHeight));
        size_t lastWordStart = 0;
        f32 wordWidth = 0.f;

        auto BreakLine = [&](f32 newLineWidth, size_t breakPoint)
        {
            lineWidths.push_back(newLineWidth);
            lineBreakPoints.push_back(breakPoint);
        };

        for (size_t i = text.pushback; i < text.text.length(); i++)
        {
            // Handle line break character.
            if (text.text[i] == '\n')
            {
                //If we have reached max amount of lines then the final character will be the one before this.
                if (lineWidths.size() == maxLines)
                    return i - 1;

                BreakLine(0.f, i);
                lastWordStart = i + 1;
                wordWidth = 0.f;

                continue;
            }

            f32 advance = 0.f;
            if (std::isspace(text.text[i]))
            {
                advance = text.fontSize * 0.15f;
                lastWordStart = i + 1;
                wordWidth = 0.f;
            }
            else
            {
                advance = text.font->GetChar(text.text[i]).advance;
                wordWidth += advance;
            }

            // Check if adding this character would break the line
            if (lineWidths.back() + advance > maxWidth)
            {
                //If we have reached max amount of lines then the final fitting character will be the last one to prevent overflow.
                if (lineWidths.size() == maxLines)
                    return i - 1;

                // If the word takes up less than a line break before it else just break in the middle of it.
                if (wordWidth < maxWidth)
                {
                    lineWidths.back() -= wordWidth;
                    BreakLine(wordWidth, lastWordStart);
                }
                else
                {
                    BreakLine(0, i);
                }
            }

            lineWidths.back() += advance;
        }

        return text.text.length();
    }

    void CalculateAllLineWidthsAndBreaks(const UIComponent::Text& text, f32 maxWidth, std::vector<f32>& lineWidths, std::vector<size_t>& lineBreakPoints)
    {
        ZoneScoped;
        assert(text.font);

        lineWidths.clear();
        lineWidths.push_back(0);
        lineBreakPoints.clear();

        size_t lastWordStart = 0;
        f32 wordWidth = 0.f;

        auto BreakLine = [&](f32 newLineWidth, size_t breakPoint)
        {
            lineWidths.push_back(newLineWidth);
            lineBreakPoints.push_back(breakPoint);
        };

        for (size_t i = 0; i < text.text.length(); i++)
        {
            // Handle line break character.
            if (text.text[i] == '\n')
            {
                BreakLine(0.f, i);
                lastWordStart = i + 1;
                wordWidth = 0.f;

                continue;
            }

            f32 advance = 0.f;
            if (std::isspace(text.text[i]))
            {
                advance = text.fontSize * 0.15f;
                lastWordStart = i + 1;
                wordWidth = 0.f;
            }
            else
            {
                advance = text.font->GetChar(text.text[i]).advance;
                wordWidth += advance;
            }

            // Check if adding this character would break the line
            if (lineWidths.back() + advance > maxWidth)
            {
                // If the word takes up less than a line break before it else just break in the middle of it.
                if (wordWidth < maxWidth)
                {
                    lineWidths.back() -= wordWidth;
                    BreakLine(wordWidth, lastWordStart);
                }
                else
                {
                    BreakLine(0, i);
                }
            }

            lineWidths.back() += advance;
        }
    }
}
