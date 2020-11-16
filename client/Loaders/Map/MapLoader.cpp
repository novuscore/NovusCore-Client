#include "MapLoader.h"
#include <Utils/ByteBuffer.h>
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <filesystem>
#include <entt.hpp>

#include "../NDBC/NDBC.h"
#include "../NDBC/NDBCLoader.h"
#include "../../ECS/Components/Singletons/MapSingleton.h"
#include "../../ECS/Components/Singletons/NDBCSingleton.h"

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
    NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();

    if (mapSingleton.mapDBCEntries.size() != 0)
    {
        NC_LOG_ERROR("MapLoader::Init can only be called once");
        return false;
    }

    if (!LoadMapDBC(mapSingleton, ndbcSingleton))
        return false;

    if (!LoadAreaTableDBC(mapSingleton, ndbcSingleton))
        return false;

    if (!LoadLightDBC(mapSingleton, ndbcSingleton))
        return false;

    if (!LoadLightParamsDBC(mapSingleton, ndbcSingleton))
        return false;

    if (!LoadLightIntBandDBC(mapSingleton, ndbcSingleton))
        return false;

    if (!LoadLightFloatBandDBC(mapSingleton, ndbcSingleton))
        return false;

    if (!LoadLightSkyboxDBC(mapSingleton, ndbcSingleton))
        return false;

    return true;
}

bool MapLoader::LoadMap(entt::registry* registry, const NDBC::Map* map)
{
    MapSingleton& mapSingleton = registry->ctx<MapSingleton>();
    NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();

    if (mapSingleton.currentMap.IsMapLoaded(map->id))
    {
        NC_LOG_WARNING("Tried to Load Current Map (%s)", mapSingleton.currentMap.name.data());
        return false; // Don't reload the map we're on
    }

    const NDBC::File& mapFile = ndbcSingleton.nameHashToDBCFile["Maps"_h];
    const std::string& mapInternalName = mapFile.stringTable->GetString(map->internalName);

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
            NC_LOG_ERROR("Failed to read map (%s)", mapInternalName.c_str());
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

bool MapLoader::LoadMapDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton)
{
    // Load Maps.ndbc
    {
        auto itr = ndbcSingleton.nameHashToDBCFile.find("Maps"_h);
        if (itr == ndbcSingleton.nameHashToDBCFile.end())
        {
            NC_LOG_ERROR("Maps.ndbc has not been loaded, please check your data folder.");
            return false;
        }

        if (!NDBCLoader::LoadDataIntoVector<NDBC::Map>(itr->second, mapSingleton.mapDBCEntries))
        {
            NC_LOG_ERROR("Failed to correctly load Maps.ndbc data");
            return false;
        }

        size_t numMaps = mapSingleton.mapDBCEntries.size();

        mapSingleton.mapIdToDBC.reserve(numMaps);
        mapSingleton.mapNameToDBC.reserve(numMaps);
        mapSingleton.mapInternalNameToDBC.reserve(numMaps);
        mapSingleton.mapNames.reserve(numMaps);

        for (NDBC::Map& map : mapSingleton.mapDBCEntries)
        {
            const std::string& mapName = itr->second.stringTable->GetString(map.name);
            u32 mapNameHash = itr->second.stringTable->GetStringHash(map.name);
            u32 mapInternalNameHash = itr->second.stringTable->GetStringHash(map.internalName);

            mapSingleton.mapIdToDBC[map.id] = &map;
            mapSingleton.mapNameToDBC[mapNameHash] = &map;
            mapSingleton.mapInternalNameToDBC[mapInternalNameHash] = &map;
            mapSingleton.mapNames.push_back(&mapName);
        }
    }

    return true;
}

bool MapLoader::LoadAreaTableDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton)
{
    // Load AreaTable.ndbc
    {
        auto itr = ndbcSingleton.nameHashToDBCFile.find("AreaTable"_h);
        if (itr == ndbcSingleton.nameHashToDBCFile.end())
        {
            NC_LOG_ERROR("AreaTable.ndbc has not been loaded, please check your data folder.");
            return false;
        }

        if (!NDBCLoader::LoadDataIntoVector<NDBC::AreaTable>(itr->second, mapSingleton.areaTableDBCEntries))
        {
            NC_LOG_ERROR("Failed to correctly load AreaTable.ndbc data");
            return false;
        }

        for (NDBC::AreaTable& area : mapSingleton.areaTableDBCEntries)
        {
            u32 areaNameHash = itr->second.stringTable->GetStringHash(area.name);

            mapSingleton.areaIdToDBC[area.id] = &area;
            mapSingleton.areaNameToDBC[areaNameHash] = &area;
        }
    }

    return true;
}

bool MapLoader::LoadLightDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton)
{
    // Load Light.ndbc
    {
        auto itr = ndbcSingleton.nameHashToDBCFile.find("Light"_h);
        if (itr == ndbcSingleton.nameHashToDBCFile.end())
        {
            NC_LOG_ERROR("Light.ndbc has not been loaded, please check your data folder.");
            return false;
        }

        if (!NDBCLoader::LoadDataIntoVector<NDBC::Light>(itr->second, mapSingleton.lightDBCEntries))
        {
            NC_LOG_ERROR("Failed to correctly load Light.ndbc data");
            return false;
        }

        for (NDBC::Light& light : mapSingleton.lightDBCEntries)
        {
            // Fix world position for non default lights
            if (light.position != vec3(0, 0, 0))
            {
                light.position.x = Terrain::MAP_HALF_SIZE - light.position.x;
                light.position.z = Terrain::MAP_HALF_SIZE - light.position.z;
            }

            mapSingleton.lightIdToDBC[light.id] = &light;
            mapSingleton.mapIdToLightDBC[light.mapId].push_back(&light);
        }
    }

    return true;
}

bool MapLoader::LoadLightParamsDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton)
{
    // Load LightParams.ndbc
    {
        auto itr = ndbcSingleton.nameHashToDBCFile.find("LightParams"_h);
        if (itr == ndbcSingleton.nameHashToDBCFile.end())
        {
            NC_LOG_ERROR("LightParams.ndbc has not been loaded, please check your data folder.");
            return false;
        }

        if (!NDBCLoader::LoadDataIntoVector<NDBC::LightParams>(itr->second, mapSingleton.lightParamsDBCEntries))
        {
            NC_LOG_ERROR("Failed to correctly load LightParams.ndbc data");
            return false;
        }

        for (NDBC::LightParams& lightParams : mapSingleton.lightParamsDBCEntries)
        {
            mapSingleton.lightParamsIdToDBC[lightParams.id] = &lightParams;
        }
    }

    return true;
}

bool MapLoader::LoadLightIntBandDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton)
{
    // Load LightIntBand.ndbc
    {
        auto itr = ndbcSingleton.nameHashToDBCFile.find("LightIntBand"_h);
        if (itr == ndbcSingleton.nameHashToDBCFile.end())
        {
            NC_LOG_ERROR("LightIntBand.ndbc has not been loaded, please check your data folder.");
            return false;
        }

        if (!NDBCLoader::LoadDataIntoVector<NDBC::LightIntBand>(itr->second, mapSingleton.lightIntBandDBCEntries))
        {
            NC_LOG_ERROR("Failed to correctly load LightIntBand.ndbc data");
            return false;
        }

        for (NDBC::LightIntBand& lightIntBand : mapSingleton.lightIntBandDBCEntries)
        {
            mapSingleton.lightIntBandIdToDBC[lightIntBand.id] = &lightIntBand;
        }
    }

    return true;
}

bool MapLoader::LoadLightFloatBandDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton)
{
    // Load LightFloatBand.ndbc
    {
        auto itr = ndbcSingleton.nameHashToDBCFile.find("LightFloatBand"_h);
        if (itr == ndbcSingleton.nameHashToDBCFile.end())
        {
            NC_LOG_ERROR("LightFloatBand.ndbc has not been loaded, please check your data folder.");
            return false;
        }

        if (!NDBCLoader::LoadDataIntoVector<NDBC::LightFloatBand>(itr->second, mapSingleton.lightFloatBandDBCEntries))
        {
            NC_LOG_ERROR("Failed to correctly load LightFloatBand.ndbc data");
            return false;
        }

        for (NDBC::LightFloatBand& lightFloatBand : mapSingleton.lightFloatBandDBCEntries)
        {
            mapSingleton.lightFloatBandIdToDBC[lightFloatBand.id] = &lightFloatBand;
        }
    }

    return true;
}

bool MapLoader::LoadLightSkyboxDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton)
{
    // Load LightSkybox.ndbc
    {
        auto itr = ndbcSingleton.nameHashToDBCFile.find("LightSkybox"_h);
        if (itr == ndbcSingleton.nameHashToDBCFile.end())
        {
            NC_LOG_ERROR("LightSkybox.ndbc has not been loaded, please check your data folder.");
            return false;
        }

        if (!NDBCLoader::LoadDataIntoVector<NDBC::LightSkybox>(itr->second, mapSingleton.lightSkyboxDBCEntries))
        {
            NC_LOG_ERROR("Failed to correctly load LightSkybox.ndbc data");
            return false;
        }

        for (NDBC::LightSkybox& lightSkybox : mapSingleton.lightSkyboxDBCEntries)
        {
            mapSingleton.lightSkyboxIdToDBC[lightSkybox.id] = &lightSkybox;
        }
    }

    return true;
}
