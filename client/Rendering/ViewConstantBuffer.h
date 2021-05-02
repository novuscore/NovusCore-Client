#pragma once
#include <NovusTypes.h>

struct ViewConstantBuffer
{
    mat4x4 viewProjectionMatrix; // 64 bytes
    mat4x4 lastViewProjectionMatrix; // 64 bytes
    mat4x4 viewMatrix; // 64 bytes
    vec3 eye;
};