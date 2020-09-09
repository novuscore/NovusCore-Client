#pragma once
#include <NovusTypes.h>
#include <Utils/FileReader.h>
#include <entity/fwd.hpp>

class StringTable;
class TextureLoader
{
public:
    TextureLoader() { }

    static bool Load(entt::registry* registry);
};