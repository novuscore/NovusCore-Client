/*
    MIT License

    Copyright (c) 2018-2019 NovusCore

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#pragma once
#include <NovusTypes.h>
#include <Utils/FileReader.h>
#include <entity/fwd.hpp>
#include <vector>

class StringTable;
namespace Terrain
{
    struct Chunk;
    struct MapHeader;
}

namespace NDBC
{
    struct File;
    struct Map;
    struct AreaTable;
}

struct MapSingleton;
struct NDBCSingleton;
class MapLoader
{
public:
    MapLoader() { }

    static bool Init(entt::registry* registry);
    static bool LoadMap(entt::registry* registry, const NDBC::Map* map);

private:
    static bool ExtractHeaderData(FileReader& reader, Terrain::MapHeader& header);
    static bool ExtractChunkData(FileReader& reader, Terrain::Chunk& chunk, StringTable& stringTable);

    static bool LoadMapDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton);
    static bool LoadAreaTableDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton);
    static bool LoadLightDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton);
    static bool LoadLightParamsDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton);
    static bool LoadLightIntBandDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton);
    static bool LoadLightFloatBandDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton);
    static bool LoadLightSkyboxDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton);
};
