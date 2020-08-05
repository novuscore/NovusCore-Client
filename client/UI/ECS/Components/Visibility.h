#pragma once

namespace UIComponent
{
    struct Visibility
    {
        Visibility() : visible(true), parentVisible(true) {}

        bool visible;
        bool parentVisible;
    };
}
