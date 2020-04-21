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


		Renderer::ConstantBuffer<ButtonConstantBuffer>* GetConstantBuffer() { return _constantBuffer; }
			
	private:
		void SetConstantBuffer(Renderer::ConstantBuffer<ButtonConstantBuffer>* constantBuffer) { _constantBuffer = constantBuffer; }

	private:
		Color _color;
		bool _clickable;

		Label* _label;

		Renderer::ConstantBuffer<ButtonConstantBuffer>* _constantBuffer = nullptr;

		friend class UIRenderer;
	};
}

