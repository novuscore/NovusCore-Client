#include "TextUtils.h"
#include <tracy/Tracy.hpp>

namespace UIUtils::Text
{
    size_t CalculatePushback(const UIComponent::Text* text, size_t writeHead, f32 bufferDecimal, f32 maxWidth, f32 maxHeight)
    {     
        ZoneScoped;

        if (!text->font)
            return 0;

        if (text->isMultiline)
            return CalculateMultilinePushback(text, writeHead, maxWidth, maxHeight);
        else
            return CalculateSinglelinePushback(text, writeHead, maxWidth, bufferDecimal);
    }

    size_t CalculateSinglelinePushback(const UIComponent::Text* text, const size_t writeHead, const f32 maxWidth, const f32 bufferDecimal)
    {
        if (text->text.length() == 0)
            return 0;

        auto GetAdvance = [&](char c) { return std::isspace(c) ? text->style.fontSize * 0.15f : text->font->GetChar(c).advance; };

        size_t oldPushback = Math::Min(text->pushback, text->text.length() - 1);
        size_t finalCharacter = oldPushback;

        f32 lineLength = 0.f;
        bool overflowed = false;
        for (; finalCharacter < text->text.length(); finalCharacter++)
        {
            lineLength += GetAdvance(text->text[finalCharacter]);

            if (lineLength >= maxWidth)
            {
                overflowed = true;
                break;
            }
        }

        if (writeHead >= oldPushback && (!overflowed || writeHead <= finalCharacter))
            return oldPushback;

        overflowed = writeHead >= finalCharacter;
        const f32 bufferSpace = maxWidth * (overflowed ? 1.f - bufferDecimal : bufferDecimal);
        lineLength = 0.f;

        for (size_t i = writeHead - 1; i > 0; --i)
        {
            lineLength += GetAdvance(text->text[i]);

            if (lineLength > bufferSpace)
                return i + 1;
        }

        return 0;
    }
    size_t CalculateMultilinePushback(const UIComponent::Text* text, const size_t writeHead, const f32 maxWidth, const f32 maxHeight)
    {
        u32 maxLines = static_cast<u32>(maxHeight / (text->style.fontSize * text->style.lineHeightMultiplier));
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
            if (lineBreakPoints[i] > text->pushback)
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
    
    size_t CalculateLineWidthsAndBreaks(const UIComponent::Text* text, f32 maxWidth, f32 maxHeight, std::vector<f32>& lineWidths, std::vector<size_t>& lineBreakPoints)
    {
        ZoneScoped;
        assert(text->font);

        if (text->text.length() == 0)
            return 0;

        lineWidths.clear();
        lineWidths.push_back(0);
        lineBreakPoints.clear();

        const u32 MaxLines = text->isMultiline ? (u32)(maxHeight / (text->style.fontSize * text->style.lineHeightMultiplier)) : 1;
        size_t wordStart = 0;
        f32 wordWidth = 0.f;
        f32 advance = 0.f;

        const auto IsLastLine = [&]() { return lineWidths.size() == MaxLines; };
        const auto BreakWord = [&](size_t i)
        {
            wordStart = i + 1;
            wordWidth = 0.f;
        };
        const auto BreakLine = [&](size_t i)
        {
            lineWidths.push_back(0);
            lineBreakPoints.push_back(i);
        };

        for (size_t i = text->pushback; i < text->text.length(); i++)
        {
            if (text->text[i] == '\n')
            {
                if (IsLastLine())
                    return i;

                BreakWord(i);
                BreakLine(i);
                continue;
            }

            if (std::isspace(text->text[i]))
            {
                advance = text->style.fontSize * 0.15f;
                BreakWord(i);
            }
            else
            {
                advance = text->font->GetChar(text->text[i]).advance;
                wordWidth += advance;
            }

            if (lineWidths.back() + advance > maxWidth)
            {
                if (IsLastLine())
                    return i;

                if (wordWidth > maxWidth)
                    BreakLine(i);
                else
                {
                    BreakLine(wordStart);
                    advance = wordWidth;
                }

            }

            lineWidths.back() += advance;
        }

        return text->text.length();
    }

    void CalculateAllLineWidthsAndBreaks(const UIComponent::Text* text, f32 maxWidth, std::vector<f32>& lineWidths, std::vector<size_t>& lineBreakPoints)
    {
        ZoneScoped;
        assert(text->font);

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

        for (size_t i = 0; i < text->text.length(); i++)
        {
            // Handle line break character.
            if (text->text[i] == '\n')
            {
                BreakLine(0.f, i);
                lastWordStart = i + 1;
                wordWidth = 0.f;

                continue;
            }

            f32 advance = 0.f;
            if (std::isspace(text->text[i]))
            {
                advance = text->style.fontSize * 0.15f;
                lastWordStart = i + 1;
                wordWidth = 0.f;
            }
            else
            {
                advance = text->font->GetChar(text->text[i]).advance;
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
