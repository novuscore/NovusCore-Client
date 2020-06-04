#include "MapLoader.h"
#include <Utils/ByteBuffer.h>
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <filesystem>

#include "../ECS/Components/Singletons/MapSingleton.h"
//#include "../ECS/Components/Singletons/DBCDatabaseCacheSingleton.h"

bool MapLoader::Load(entt::registry& registry)
{
    //size_t test = sizeof(NovusAdt);
    std::filesystem::path absolutePath = std::filesystem::absolute("Data/extracted/maps");
    if (!std::filesystem::is_directory(absolutePath))
    {
        NC_LOG_ERROR("Failed to find maps folder");
        return false;
    }

    MapSingleton& mapSingleton = registry.set<MapSingleton>();
    //DBCDatabaseCacheSingleton& dbcCache = registry.ctx<DBCDatabaseCacheSingleton>();

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
        StringTable stringTable;
        if (!ExtractChunkData(chunkFile, chunk, stringTable))
        {
            NC_LOG_ERROR("Failed to load all maps");
            return false;
        }

        std::vector<std::string> splitName = StringUtils::SplitString(file.filename().string(), '_');
        size_t numberOfSplits = splitName.size();

        std::string mapInternalName = splitName[0];
        //MapData mapData;
        //if (!dbcCache.cache->GetMapDataFromInternalName(mapInternalName, mapData))
        //    continue;

        u16 mapId = 0;// mapData.id;

        if (mapSingleton.maps.find(mapId) == mapSingleton.maps.end())
        {
            mapSingleton.maps[mapId].id = mapId;
            mapSingleton.maps[mapId].name = mapInternalName;// mapData.name;
        }

        u16 x = std::stoi(splitName[numberOfSplits - 2]);
        u16 y = std::stoi(splitName[numberOfSplits - 1]);

        int chunkId = x + (y * Terrain::MAP_CHUNKS_PER_MAP_SIDE);
        mapSingleton.maps[mapId].chunks[chunkId] = chunk;
        mapSingleton.maps[mapId].stringTables[chunkId].CopyFrom(stringTable);

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

bool MapLoader::ExtractChunkData(FileReader& reader, Terrain::Chunk& chunk, StringTable& stringTable)
{
    ByteBuffer buffer(nullptr, reader.Length());
    reader.Read(buffer, buffer.Size);

    buffer.Get<Terrain::Chunk>(chunk);
    stringTable.Deserialize(buffer);

    return true;
}