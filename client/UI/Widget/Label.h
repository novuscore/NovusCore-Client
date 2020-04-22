#pragma once
#include "Widget.h"
#include <Renderer/Descriptors/ModelDesc.h>
#include <Renderer/Descriptors/TextureDesc.h>
#include <Renderer/Descriptors/FontDesc.h>
#include <Renderer/ConstantBuffer.h>

class UILabel;
class UIRenderer;

namespace Renderer
{
    struct Font;
}

namespace UI
{
    class Label : public Widget
    {
    public:
        struct LabelConstantBuffer
        {
            Color textColor; // 16 bytes
            Color outlineColor; // 16 bytes
            f32 outlineWidth; // 4 bytes

            u8 padding[220] = {};
        };

    public:
        Label(const vec2& pos, const vec2& size);
        static void RegisterType();

        std::string GetTypeName() override { return "Label"; }

        std::string& GetText() { return _text; }
        void SetText(std::string& text);

        u32 GetTextLength() { return static_cast<u32>(_text.length()); }
        u32 GetGlyphCount() { return _glyphCount; }

        const Color& GetColor() { return _color; }
        void SetColor(const Color& color);

        f32 GetOutlineWidth() { return _outlineWidth; }
        void SetOutlineWidth(f32 width);

        const Color& GetOutlineColor() { return _outlineColor; }
        void SetOutlineColor(const Color& color);

        std::string& GetFontPath() { return _fontPath; }
        f32 GetFontSize() { return _fontSize; }
        void SetFont(std::string& fontPath, f32 fontSize);

        Renderer::ConstantBuffer<LabelConstantBuffer>* GetConstantBuffer() { return _constantBuffer; }
    private:
        void SetConstantBuffer(Renderer::ConstantBuffer<LabelConstantBuffer>* constantBuffer) { _constantBuffer = constantBuffer; }

    private:
        std::string _text;
        u32 _glyphCount;

        Color _color;
        Color _outlineColor;
        f32 _outlineWidth;

        std::string _fontPath;
        f32 _fontSize;
        Renderer::Font* _font;

        std::vector<Renderer::ModelID> _models;
        std::vector<Renderer::TextureID> _textures;

        Renderer::ConstantBuffer<LabelConstantBuffer>* _constantBuffer = nullptr;

        static Label* CreateLabel(const vec2& pos, const vec2& size);

        friend class UIRenderer;
    };
}