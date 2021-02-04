#include "MapUtils.h"
#include "../ECS/Components/Singletons/NDBCSingleton.h"

#include <Utils/FileReader.h>
#include <filesystem>
namespace fs = std::filesystem;

bool Terrain::MapUtils::LoadMap(entt::registry* registry, const NDBC::Map* map)
{
    MapSingleton& mapSingleton = registry->ctx<MapSingleton>();
    NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();

    Terrain::Map& currentMap = mapSingleton.GetCurrentMap();

    if (currentMap.IsMapLoaded(map->id))
    {
        DebugHandler::PrintWarning("Tried to Load Current Map (%s)", currentMap.name.data());
        return false; // Don't reload the map we're on
    }

    NDBC::File* mapFile = ndbcSingleton.GetNDBCFile("Maps"_h);
    const std::string& mapInternalName = mapFile->GetStringTable()->GetString(map->internalName);

    fs::path absolutePath = std::filesystem::absolute("Data/extracted/maps/" + mapInternalName);
    if (!fs::is_directory(absolutePath))
    {
        DebugHandler::PrintError("Failed to find map folder for %s", mapInternalName.c_str());
        return false;
    }

    // Clear currently loaded map
    currentMap.Clear();

    currentMap.id = map->id;
    currentMap.name = mapInternalName;

    bool nmapFound = false;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(absolutePath))
    {
        auto file = std::filesystem::path(entry.path());

        if (file.extension() != ".nmap")
            continue;

        std::string fileName = file.filename().replace_extension("").string();
        if (fileName != mapInternalName)
            continue;

        FileReader mapHeaderFile(entry.path().string(), file.filename().string());
        if (!mapHeaderFile.Open())
        {
            DebugHandler::PrintError("Failed to read map (%s)", mapInternalName.c_str());
            return false;
        }

        if (!Terrain::MapHeader::Read(mapHeaderFile, currentMap.header))
        {
            DebugHandler::PrintError("Failed to load map header for (%s)", mapInternalName.c_str());
            return false;
        }

        nmapFound = true;
        break;
    }

    if (!nmapFound)
    {
        DebugHandler::PrintError("Failed to find nmap file for map (%s)", mapInternalName.c_str());
        return false;
    }

    // Load Chunks if map does not use Map Object as base
    if (!currentMap.header.flags.UseMapObjectInsteadOfTerrain)
    {
        size_t loadedChunks = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(absolutePath))
        {
            auto file = std::filesystem::path(entry.path());
            if (file.extension() != ".nchunk")
                continue;

            // Make sure filename is the same, multiple maps can have chunks in the same folder
            std::string fileName = file.filename().string();
            if (strncmp(fileName.c_str(), mapInternalName.c_str(), mapInternalName.length()) != 0)
                continue;

            FileReader chunkFile(entry.path().string(), file.filename().string());
            if (!chunkFile.Open())
            {
                DebugHandler::PrintError("Failed to load map chunk (%s)", file.filename().string().c_str());
                return false;
            }

            Terrain::Chunk chunk;
            StringTable chunkStringTable;
            if (!Terrain::Chunk::Read(chunkFile, chunk, chunkStringTable))
            {
                DebugHandler::PrintError("Failed to load map chunk for (%s)", file.filename().string().c_str());
                return false;
            }

            std::vector<std::string> splitName = StringUtils::SplitString(file.filename().string(), '_');
            size_t numberOfSplits = splitName.size();

            std::string mapInternalName = splitName[0];
            for (size_t i = 1; i < numberOfSplits - 2; i++)
            {
                mapInternalName += "_" + splitName[i];
            }

            u16 x = std::stoi(splitName[numberOfSplits - 2]);
            u16 y = std::stoi(splitName[numberOfSplits - 1]);
            u32 chunkId = x + (y * Terrain::MAP_CHUNKS_PER_MAP_STRIDE);

            currentMap.chunks[chunkId] = chunk;
            currentMap.stringTables[chunkId].CopyFrom(chunkStringTable);

            Terrain::MapUtils::AlignCellBorders(currentMap.chunks[chunkId]);

            loadedChunks++;
        }

        if (loadedChunks == 0)
        {
            DebugHandler::PrintError("0 map chunks found in (%s)", absolutePath.string().c_str());
            return false;
        }

        Terrain::MapUtils::AlignChunkBorders(currentMap);
    }

    DebugHandler::PrintSuccess("Loaded Map (%s)", mapInternalName.c_str());
    return true;
}