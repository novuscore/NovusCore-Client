#pragma once
#include <NovusTypes.h>
#include <Utils/FileReader.h>
#include <entity/fwd.hpp>

class StringTable;
namespace DBC
{
    struct File;
}

class DBCLoader
{
public:
    DBCLoader() { }
    static bool Load(entt::registry* registry);

private:
    static bool ExtractFileData(FileReader& reader, DBC::File& file);
};