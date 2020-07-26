#include "TextUtils.h"
#include <tracy/Tracy.hpp>

namespace UI::TextUtils
{
    size_t CalculatePushback(const UIText& text, size_t writeHead, f32 bufferDecimal, f32 maxWidth)
    {     
        ZoneScoped;
        /*
        *   TODO:
        *   - Move entire lines for multi-line fields.
        */

        if (!text.font)
            return 0;

        size_t oldPushback = Math::Min(text.pushback, text.text.length() - 1);

        f32 lineLength = 0.f;
        if (oldPushback <= writeHead)
        {
            size_t pushBackPoint = 0;
            bool reachedPercent = false;
            bool overflowed = false;
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
                    overflowed = true;
                    lineLength = 0.f;
                }
            }

            if (overflowed)
                return pushBackPoint;

            return oldPushback;
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

    size_t CalculateLineWidthsAndBreaks(const UIText& text, f32 maxWidth, f32 maxHeight, std::vector<f32>& lineWidths, std::vector<size_t> lineBreakPoints)
    {
        assert(text.font);
        ZoneScoped;

        lineWidths.clear();
        lineWidths.push_back(0);
        lineBreakPoints.clear();

        u32 maxLines = static_cast<u32>(text.textType == UI::TextType::SINGLELINE ? 1 : maxHeight / (text.fontSize * text.lineHeight));
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
}
