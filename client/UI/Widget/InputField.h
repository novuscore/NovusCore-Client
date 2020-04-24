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

        const Color& GetColor() { return _color; }
        void SetColor(const Color& color);

        void AddText(const std::string& text);
        void RemovePreviousCharacter();
        void RemoveNextCharacter();

        void MovePointerLeft();
        void MovePointerRight();

        const std::string& GetText() const;
        void SetText(const std::string& text);
        void SetFont(const std::string& fontPath, f32 fontSize);
        void SetTextColor(const Color& color);

        const bool IsEnabled() const { return _enabled; }
        void SetEnabled(bool enabled);

        void SetOnSubmit(asIScriptFunction* function);
        void OnSubmit();

        void SetOnEnter(asIScriptFunction* function);
        void OnEnter();

        Renderer::ConstantBuffer<InputFieldConstantBuffer>* GetConstantBuffer() const { return _constantBuffer; }
    private:
        void SetConstantBuffer(Renderer::ConstantBuffer<InputFieldConstantBuffer>* constantBuffer) { _constantBuffer = constantBuffer; }

        static InputField* CreateInputField(const vec2& pos, const vec2& size);

    private:
        Color _color;

        bool _enabled;

        Label* _label;

        u32 _pointerIndex = 0;

        Renderer::ConstantBuffer<InputFieldConstantBuffer>* _constantBuffer = nullptr;
        
        asIScriptFunction* _onSubmitCallback;
        asIScriptFunction* _onEnterCallback;

        friend class UIRenderer;
    };
}

