#pragma once

namespace UI
{
    enum class RenderType
    {
        None,
        Image,
        Text
    };
}

namespace UIComponent
{
    struct Renderable
    {
        UI::RenderType renderType = UI::RenderType::None;
    };
}
