#pragma once
#include "Widget.h"
#include <Renderer/ConstantBuffer.h>

namespace UI
{
    class Label;

    class InputField : public Widget
    {

    public:
        struct InputFieldConstantBuffer
        {
            Color color; // 16 bytes

            u8 padding[240] = {};
        };

    public:
        InputField(const vec2& pos, const vec2& size);
        static void RegisterType();

        void SetColor(const Color& color);

        void AddText(const std::string& character);
        void RemoveCharacter();

        const std::string& GetText() const;
        void SetText(const std::string& text);
        void SetFont(const std::string& fontPath, f32 fontSize);
        void SetTextColor(const Color& color);

        Renderer::ConstantBuffer<InputFieldConstantBuffer>* GetConstantBuffer() const { return _constantBuffer; }
    private:
        void SetConstantBuffer(Renderer::ConstantBuffer<InputFieldConstantBuffer>* constantBuffer) { _constantBuffer = constantBuffer; }

    private:
        Color _color;

        Label* _label;

        Renderer::ConstantBuffer<InputFieldConstantBuffer>* _constantBuffer = nullptr;

        static InputField* CreateInputField(const vec2& pos, const vec2& size);

        friend class UIRenderer;
    };
}

