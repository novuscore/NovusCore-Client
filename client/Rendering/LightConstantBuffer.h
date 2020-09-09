#pragma once
#include <NovusTypes.h>

struct LightConstantBuffer
{
    vec4 ambientColor;
    vec4 lightColor;
    vec4 lightDir;
};