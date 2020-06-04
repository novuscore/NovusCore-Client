#pragma once
#include <NovusTypes.h>

struct ViewConstantBuffer
{
    mat4x4 viewMatrix; // 64 bytes
    mat4x4 projMatrix; // 64 bytes

    u8 padding[128] = {};
};