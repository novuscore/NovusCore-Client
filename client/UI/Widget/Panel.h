#pragma once
#include "Widget.h"
#include <Renderer/ConstantBuffer.h>

class UIPanel;
class UIRenderer;

namespace UI
{
    class Panel : public Widget
    {
    public:
        struct PanelConstantBuffer
        {
            Color color; // 16 bytes

            u8 padding[240] = {};
        };

    public:
        Panel(const vec2& pos, const vec2& size);
        static void RegisterType();

        std::string GetTypeName() override { return "Panel"; }

        Renderer::ModelID GetModelID() { return Widget::GetModelID(); }
        void SetModelID(Renderer::ModelID modelID);

        std::string& GetTexture() { return Widget::GetTexture(); }
        void SetTexture(std::string& texture);

        Renderer::TextureID GetTextureID() { return Widget::GetTextureID(); }
        void SetTextureID(Renderer::TextureID textureID);

        const Color& GetColor() { return _color; }
        void SetColor(const Color& color);

        bool IsClickable() { return _clickable; }
        void SetClickable(bool value);

        bool IsDraggable() { return _draggable; }
        void SetDraggable(bool value);

        bool IsDragging() { return _isDragging; }
        bool DidDrag() { return _didDrag; }
        const vec2& GetDeltaDragPosition() { return _deltaDragPosition; }

        void SetOnClick(asIScriptFunction* function);
        void OnClick();

        Renderer::ConstantBuffer<PanelConstantBuffer>* GetConstantBuffer() { return _constantBuffer; }

    private:
        void SetConstantBuffer(Renderer::ConstantBuffer<PanelConstantBuffer>* constantBuffer) { _constantBuffer = constantBuffer; }

        static Panel* CreatePanel(const vec2& pos, const vec2& size);
    private:
        Color _color;
        bool _clickable;
        bool _draggable;
        bool _isDragging;
        bool _didDrag;
        vec2 _deltaDragPosition;

        void BeingDrag(const vec2& deltaDragPosition);
        void SetDidDrag();
        void EndDrag();

        Renderer::ConstantBuffer<PanelConstantBuffer>* _constantBuffer = nullptr;

        asIScriptFunction* _onClickCallback;

        friend class UIRenderer;
    };
}