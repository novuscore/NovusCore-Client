#pragma once

struct UIVisibility
{
    UIVisibility() : visible(true), parentVisible(true) {}

    bool visible;
    bool parentVisible;
};
