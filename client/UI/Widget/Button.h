#pragma once
#include "Widget.h"
#include <Renderer/ConstantBuffer.h>

class UIRenderer;

namespace UI
{
	class Label;

	class Button : public Widget
	{
	public:
		struct ButtonConstantBuffer
		{
			Color color; // 16 bytes

			u8 padding[240] = {};
		};

	public:
		Button(const vec2& pos, const vec2& size);
		static void RegisterType();

        std::string GetTypeName() const override { return "Button"; }

		const Color& GetColor() const { return _color; }
		void SetColor(const Color& color);

		bool IsClickable() const { return _clickable; }
		void SetClickable(bool value);

        const std::string& GetText() const;
		void SetText(std::string& text);
        void SetFont(std::string& fontPath, f32 fontSize);
        void SetTextColor(const Color& color);

		Label* GetLabel() const { return _label; }

		void SetOnClick(asIScriptFunction* function);
		void OnClick();

		Renderer::ConstantBuffer<ButtonConstantBuffer>* GetConstantBuffer() const { return _constantBuffer; }
			
	private:
		void SetConstantBuffer(Renderer::ConstantBuffer<ButtonConstantBuffer>* constantBuffer) { _constantBuffer = constantBuffer; }

		static Button* CreateButton(const vec2& pos, const vec2& size);
	private:
		Color _color;
		bool _clickable;

		Label* _label;

		Renderer::ConstantBuffer<ButtonConstantBuffer>* _constantBuffer = nullptr;

		asIScriptFunction* _onClickCallback;

		friend class UIRenderer;
	};
}

