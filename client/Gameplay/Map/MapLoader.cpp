#include "MapLoader.h"
#include <Utils/ByteBuffer.h>
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <filesystem>

#include "../DBC/DBC.h"
#include "../../ECS/Components/Singletons/MapSingleton.h"
#include "../../ECS/Components/Singletons/DBCSingleton.h"

namespace fs = std::filesystem;

bool MapLoader::Init(entt::registry* registry)
{
    fs::path absolutePath = std::filesystem::absolute("Data/extracted/maps");
    if (!fs::is_directory(absolutePath))
    {
        NC_LOG_ERROR("Failed to find maps folder");
        return false;
    }

    MapSingleton& mapSingleton = registry->set<MapSingleton>();
    DBCSingleton& dbcSingleton = registry->ctx<DBCSingleton>();

    if (mapSingleton.mapDBCFiles.size() != 0)
    {
        NC_LOG_ERROR("MapLoader::Init can only be called once");
        return false;
    }

    auto itr = dbcSingleton.dbcs.find("Maps"_h);
    if (itr == dbcSingleton.dbcs.end())
    {
        NC_LOG_ERROR("Maps.ndbc has not been loaded, please check your data folder.");
        return false;
    }

    if (!ExtractMapDBC(itr->second, mapSingleton.mapDBCFiles, mapSingleton.mapsDBCStringTable))
    {
        NC_LOG_ERROR("Failed to correctly load Maps.ndbc data");
        return false;
    }

    for (DBC::Map& map : mapSingleton.mapDBCFiles)
    {
        u32 mapNameHash = mapSingleton.mapsDBCStringTable.GetStringHash(map.Name);
        u32 mapInternalNameHash = mapSingleton.mapsDBCStringTable.GetStringHash(map.InternalName);

        mapSingleton.mapIdToDBC[map.Id] = &map;
        mapSingleton.mapNameToDBC[mapNameHash] = &map;
        mapSingleton.mapInternalNameToDBC[mapInternalNameHash] = &map;
    }

    return true;
}

bool MapLoader::LoadMap(entt::registry* registry, u32 mapInternalNameHash)
{
    MapSingleton& mapSingleton = registry->ctx<MapSingleton>();
    DBCSingleton& dbcSingleton = registry->ctx<DBCSingleton>();

    if (mapSingleton.loadedMapHash == mapInternalNameHash)
    {
        return false; // Don't reload the map we're on
    }

    auto itr = mapSingleton.mapInternalNameToDBC.find(mapInternalNameHash);
    if (itr == mapSingleton.mapInternalNameToDBC.end())
    {
        NC_LOG_ERROR("Tried to Load Map with no entry in Maps.ndbc");
        return false;
    }

    const DBC::Map* map = mapSingleton.mapInternalNameToDBC[mapInternalNameHash];

    const std::string& mapInternalName = mapSingleton.mapsDBCStringTable.GetString(map->InternalName);
    fs::path absolutePath = std::filesystem::absolute("Data/extracted/maps/" + mapInternalName);
    if (!fs::is_directory(absolutePath))
    {
        NC_LOG_ERROR("Failed to find map folder for %s", mapInternalName);
        return false;
    }

    // Clear currently loaded map
    mapSingleton.currentMap.Clear();

    mapSingleton.currentMap.id = map->Id;
    mapSingleton.currentMap.name = mapInternalName;

    size_t loadedChunks = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(absolutePath))
    {
        auto file = std::filesystem::path(entry.path());
        if (file.extension() != ".nmap")
            continue;

        FileReader chunkFile(entry.path().string(), file.filename().string());
        if (!chunkFile.Open())
        {
            NC_LOG_ERROR("Failed to load all maps");
            return false;
        }

        Terrain::Chunk chunk;
        StringTable chunkStringTable;
        if (!ExtractChunkData(chunkFile, chunk, chunkStringTable))
        {
            NC_LOG_ERROR("Failed to load all maps");
            return false;
        }

        std::vector<std::string> splitName = StringUtils::SplitString(file.filename().string(), '_');
        size_t numberOfSplits = splitName.size();

        std::string mapInternalName = splitName[0];
        for (size_t i = 1; i < numberOfSplits - 2; i++)
        {
            mapInternalName += "_" + splitName[i];
        }

        auto itr = mapSingleton.mapInternalNameToDBC.find(mapInternalNameHash);
        if (itr == mapSingleton.mapInternalNameToDBC.end())
        {
            NC_LOG_ERROR("Tried to Load Map with no entry in Maps.ndbc (%s, %s)", splitName[0].c_str(), splitName[1].c_str());
            continue;
        }

        u16 x = std::stoi(splitName[numberOfSplits - 2]);
        u16 y = std::stoi(splitName[numberOfSplits - 1]);
        u32 chunkId = x + (y * Terrain::MAP_CHUNKS_PER_MAP_STRIDE);

        mapSingleton.currentMap.chunks[chunkId] = chunk;
        mapSingleton.currentMap.stringTables[chunkId].CopyFrom(chunkStringTable);

        loadedChunks++;
    }

    if (loadedChunks == 0)
    {
        NC_LOG_ERROR("0 maps found in (%s)", absolutePath.string().c_str());
        return false;
    }

    NC_LOG_SUCCESS("Loaded %u chunks", loadedChunks);
    return true;
}

bool MapLoader::ExtractMapDBC(DBC::File& file, std::vector<DBC::Map>& maps, StringTable& stringTable)
{
    u32 numMaps = 0;
    file.buffer->GetU32(numMaps);

    if (numMaps == 0)
        return false;
    
    // Resize our vector and fill it with map data
    maps.resize(numMaps);
    file.buffer->GetBytes(reinterpret_cast<u8*>(&maps[0]), sizeof(DBC::Map) * numMaps);

    stringTable.Deserialize(file.buffer.get());
    assert(stringTable.GetNumStrings() > 0); // We always expect to have at least 2 strings per map in our stringtable, a path for the base texture
    return true;
}

bool MapLoader::ExtractChunkData(FileReader& reader, Terrain::Chunk& chunk, StringTable& stringTable)
{
    Bytebuffer buffer(nullptr, reader.Length());
    reader.Read(&buffer, buffer.size);

    buffer.Get<Terrain::ChunkHeader>(chunk.chunkHeader);

    if (chunk.chunkHeader.token != Terrain::MAP_CHUNK_TOKEN)
    {
        NC_LOG_FATAL("Tried to load a map chunk file with the wrong token");
    }

    if (chunk.chunkHeader.version != Terrain::MAP_CHUNK_VERSION)
    {
        NC_LOG_FATAL("Loaded map chunk has version of %u instead of expected version of %u, try reextracting your data", chunk.chunkHeader.version, Terrain::MAP_CHUNK_VERSION);
    }

    buffer.Get<Terrain::HeightHeader>(chunk.heightHeader);
    buffer.Get<Terrain::HeightBox>(chunk.heightBox);

    buffer.GetBytes(reinterpret_cast<u8*>(&chunk.cells[0]), sizeof(Terrain::Cell) * Terrain::MAP_CELLS_PER_CHUNK);

    buffer.Get<u32>(chunk.alphaMapStringID);

    u32 numMapObjectPlacements;
    buffer.Get<u32>(numMapObjectPlacements);

    if (numMapObjectPlacements > 0)
    {
        chunk.mapObjectPlacements.resize(numMapObjectPlacements);
        buffer.GetBytes(reinterpret_cast<u8*>(&chunk.mapObjectPlacements[0]), sizeof(Terrain::MapObjectPlacement) * numMapObjectPlacements);
    }
    
    stringTable.Deserialize(&buffer);
    assert(stringTable.GetNumStrings() > 0); // We always expect to have at least 1 string in our stringtable, a path for the base texture
    return true;
}