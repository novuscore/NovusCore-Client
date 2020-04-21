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

        bool IsDraggable();
        void SetDraggable(bool value);

        Renderer::ConstantBuffer<PanelConstantBuffer>* GetConstantBuffer() { return _constantBuffer; }

    private:
        void SetConstantBuffer(Renderer::ConstantBuffer<PanelConstantBuffer>* constantBuffer) { _constantBuffer = constantBuffer; }

    private:
        Color _color;
        bool _clickable;
        bool _draggable;
        bool _isDragging;
        bool _didDrag;
        vec2 _deltaDragPosition;

        bool IsDragging();
        void BeingDrag(const vec2& deltaDragPosition);
        const vec2& GetDeltaDragPosition();
        bool DidDrag();
        void SetDidDrag();
        void EndDrag();

        Renderer::ConstantBuffer<PanelConstantBuffer>* _constantBuffer = nullptr;

        friend class ::UIPanel;
        friend class UIRenderer;
    };
}