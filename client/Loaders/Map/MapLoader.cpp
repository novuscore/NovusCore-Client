#include "MapLoader.h"
#include <Utils/ByteBuffer.h>
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <filesystem>
#include <entt.hpp>

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

    if (mapSingleton.mapDBCEntries.size() != 0)
    {
        NC_LOG_ERROR("MapLoader::Init can only be called once");
        return false;
    }

    // Load Maps.ndbc
    {
        auto itr = dbcSingleton.nameHashToDBCFile.find("Maps"_h);
        if (itr == dbcSingleton.nameHashToDBCFile.end())
        {
            NC_LOG_ERROR("Maps.ndbc has not been loaded, please check your data folder.");
            return false;
        }

        if (!ExtractMapDBC(itr->second, mapSingleton.mapDBCEntries))
        {
            NC_LOG_ERROR("Failed to correctly load Maps.ndbc data");
            return false;
        }

        for (DBC::Map& map : mapSingleton.mapDBCEntries)
        {
            u32 mapNameHash = dbcSingleton.stringTable.GetStringHash(map.name);
            u32 mapInternalNameHash = dbcSingleton.stringTable.GetStringHash(map.internalName);

            mapSingleton.mapIdToDBC[map.id] = &map;
            mapSingleton.mapNameToDBC[mapNameHash] = &map;
            mapSingleton.mapInternalNameToDBC[mapInternalNameHash] = &map;
        }
    }

    // Load AreaTable.ndbc
    {
        auto itr = dbcSingleton.nameHashToDBCFile.find("AreaTable"_h);
        if (itr == dbcSingleton.nameHashToDBCFile.end())
        {
            NC_LOG_ERROR("AreaTable.ndbc has not been loaded, please check your data folder.");
            return false;
        }

        if (!ExtractAreaTableDBC(itr->second, mapSingleton.areaTableDBCEntries))
        {
            NC_LOG_ERROR("Failed to correctly load Maps.ndbc data");
            return false;
        }

        for (DBC::AreaTable& area : mapSingleton.areaTableDBCEntries)
        {
            u32 areaNameHash = dbcSingleton.stringTable.GetStringHash(area.name);

            mapSingleton.areaIdToDBC[area.id] = &area;
            mapSingleton.areaNameToDBC[areaNameHash] = &area;
        }
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

    const std::string& mapInternalName = dbcSingleton.stringTable.GetString(map->internalName);
    fs::path absolutePath = std::filesystem::absolute("Data/extracted/maps/" + mapInternalName);
    if (!fs::is_directory(absolutePath))
    {
        NC_LOG_ERROR("Failed to find map folder for %s", mapInternalName.c_str());
        return false;
    }

    // Clear currently loaded map
    mapSingleton.currentMap.Clear();

    mapSingleton.currentMap.id = map->id;
    mapSingleton.currentMap.name = mapInternalName;

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
            NC_LOG_ERROR("Failed to map (%s)", mapInternalName.c_str());
            return false;
        }

        if (!ExtractHeaderData(mapHeaderFile, mapSingleton.currentMap.header))
        {
            NC_LOG_ERROR("Failed to load map header for (%s)", mapInternalName.c_str());
            return false;
        }

        nmapFound = true;
        break;
    }

    if (!nmapFound)
    {
        NC_LOG_ERROR("Failed to find nmap file for map (%s)", mapInternalName.c_str());
        return false;
    }

    // Load Chunks if map does not use Map Object as base
    if (!mapSingleton.currentMap.header.flags.UseMapObjectInsteadOfTerrain)
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
                NC_LOG_ERROR("Failed to load map chunk (%s)", file.filename().string().c_str());
                return false;
            }

            Terrain::Chunk chunk;
            StringTable chunkStringTable;
            if (!ExtractChunkData(chunkFile, chunk, chunkStringTable))
            {
                NC_LOG_ERROR("Failed to load map chunk for (%s)", file.filename().string().c_str());
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

            mapSingleton.currentMap.chunks[chunkId] = chunk;
            mapSingleton.currentMap.stringTables[chunkId].CopyFrom(chunkStringTable);

            loadedChunks++;
        }

        if (loadedChunks == 0)
        {
            NC_LOG_ERROR("0 map chunks found in (%s)", absolutePath.string().c_str());
            return false;
        }
    }
    
    NC_LOG_SUCCESS("Loaded Map (%s)", mapInternalName.c_str());
    return true;
}

bool MapLoader::ExtractMapDBC(DBC::File& file, std::vector<DBC::Map>& maps)
{
    u32 numMaps = 0;
    file.buffer->GetU32(numMaps);

    if (numMaps == 0)
        return false;
    
    // Resize our vector and fill it with map data
    maps.resize(numMaps);
    file.buffer->GetBytes(reinterpret_cast<u8*>(&maps[0]), sizeof(DBC::Map) * numMaps);
    return true;
}

bool MapLoader::ExtractAreaTableDBC(DBC::File& file, std::vector<DBC::AreaTable>& areas)
{
    u32 numAreas = 0;
    file.buffer->GetU32(numAreas);

    if (numAreas == 0)
        return false;

    // Resize our vector and fill it with map data
    areas.resize(numAreas);
    file.buffer->GetBytes(reinterpret_cast<u8*>(&areas[0]), sizeof(DBC::AreaTable) * numAreas);
    return true;
}

bool MapLoader::ExtractHeaderData(FileReader& reader, Terrain::MapHeader& header)
{
    Bytebuffer buffer(nullptr, reader.Length());
    reader.Read(&buffer, buffer.size);

    if (!buffer.GetU32(header.token))
        return false;

    if (!buffer.GetU32(header.version))
        return false;

    if (!buffer.Get(header.flags))
        return false;

    if (header.flags.UseMapObjectInsteadOfTerrain)
    {
        buffer.GetString(header.mapObjectName);

        if (header.mapObjectName.length() == 0)
            return false;

        if (!buffer.Get(header.mapObjectPlacement))
            return false;
    }

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
        if (chunk.chunkHeader.version < Terrain::MAP_CHUNK_VERSION)
        {
            NC_LOG_FATAL("Loaded map chunk with too old version %u instead of expected version of %u, rerun dataextractor", chunk.chunkHeader.version, Terrain::MAP_CHUNK_VERSION);
        }
        else
        {
            NC_LOG_FATAL("Loaded map chunk with too new version %u instead of expected version of %u, update your client", chunk.chunkHeader.version, Terrain::MAP_CHUNK_VERSION);
        }
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
        buffer.GetBytes(reinterpret_cast<u8*>(&chunk.mapObjectPlacements[0]), sizeof(Terrain::Placement) * numMapObjectPlacements);
    }

    u32 numComplexModelPlacements;
    buffer.Get<u32>(numComplexModelPlacements);

    if (numComplexModelPlacements > 0)
    {
        chunk.complexModelPlacements.resize(numComplexModelPlacements);
        buffer.GetBytes(reinterpret_cast<u8*>(&chunk.complexModelPlacements[0]), sizeof(Terrain::Placement) * numComplexModelPlacements);
    }

    // Read Liquid Headers
    {
        u16 numLiquidHeaders;
        buffer.Get<u16>(numLiquidHeaders);

        if (numLiquidHeaders > 0)
        {
            chunk.liquidHeaders.resize(numLiquidHeaders);
            buffer.GetBytes(reinterpret_cast<u8*>(&chunk.liquidHeaders[0]), sizeof(Terrain::CellLiquidHeader) * numLiquidHeaders);
        }
    }

    // Read Liquid Instances
    {
        u16 numLiquidInstances;
        buffer.Get<u16>(numLiquidInstances);

        if (numLiquidInstances > 0)
        {
            chunk.liquidInstances.resize(numLiquidInstances);
            buffer.GetBytes(reinterpret_cast<u8*>(&chunk.liquidInstances[0]), sizeof(Terrain::CellLiquidInstance) * numLiquidInstances);
        }
    }

    // Read Liquid Attributes
    {
        u8 numLiquidAttributes;
        buffer.Get<u8>(numLiquidAttributes);

        if (numLiquidAttributes > 0)
        {
            chunk.liquidAttributes.resize(numLiquidAttributes);
            buffer.GetBytes(reinterpret_cast<u8*>(&chunk.liquidAttributes[0]), sizeof(Terrain::CellLiquidAttributes) * numLiquidAttributes);
        }
    }

    // Read Liquid BitMask Data
    {
        u16 numLiquidBitMaskBytes;
        buffer.Get<u16>(numLiquidBitMaskBytes);

        if (numLiquidBitMaskBytes > 0)
        {
            chunk.liquidBitMaskForPatchesData.resize(numLiquidBitMaskBytes);
            buffer.GetBytes(reinterpret_cast<u8*>(&chunk.liquidBitMaskForPatchesData[0]), sizeof(u8) * numLiquidBitMaskBytes);
        }
    }

    // Read Liquid Vertex Data
    {
        u32 numLiquidVertexDataBytes;
        buffer.Get<u32>(numLiquidVertexDataBytes);

        if (numLiquidVertexDataBytes > 0)
        {
            chunk.liquidvertexData.resize(numLiquidVertexDataBytes);
            buffer.GetBytes(reinterpret_cast<u8*>(&chunk.liquidvertexData[0]), sizeof(u8) * numLiquidVertexDataBytes);
        }
    }
    
    stringTable.Deserialize(&buffer);
    assert(stringTable.GetNumStrings() > 0); // We always expect to have at least 1 string in our stringtable, a path for the base texture
    return true;
}