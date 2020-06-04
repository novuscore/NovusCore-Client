#pragma once
#include <array>

template <typename T, size_t NumFrames>
struct FrameResource
{
    T& Get(size_t frame)
    {
        return items[frame % NumFrames];
    }

    std::array<T, NumFrames> items = { 0 };

    static constexpr size_t Num = NumFrames;
};