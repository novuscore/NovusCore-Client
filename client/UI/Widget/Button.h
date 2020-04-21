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

		std::string GetTypeName() override;

		Renderer::ModelID GetModelID();
		void SetModelID(Renderer::ModelID modelID);

		std::string& GetTexture();
		void SetTexture(std::string& texture);

		Renderer::TextureID GetTextureID();
		void SetTextureID(Renderer::TextureID textureID);

		const Color& GetColor();
		void SetColor(const Color& color);

		bool IsClickable();
		void SetClickable(bool value);

		std::string& GetText();
		void SetText(std::string& text);

		Label* GetLabel();

		void SetOnClick(asIScriptFunction* function);
		void OnClick();

		Renderer::ConstantBuffer<ButtonConstantBuffer>* GetConstantBuffer() { return _constantBuffer; }
			
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

