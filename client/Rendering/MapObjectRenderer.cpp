#include "MapObjectRenderer.h"
#include <filesystem>
#include <Renderer/Renderer.h>

#include "../Gameplay/Map/Chunk.h"
#include "../Gameplay/Map/MapObjectRoot.h"
#include "../Gameplay/Map/MapObject.h"
#include <Utils\FileReader.h>

MapObjectRenderer::MapObjectRenderer(Renderer::Renderer* renderer)
{
    CreatePermanentResources();
}

void MapObjectRenderer::Update(f32 deltaTime)
{

}

void MapObjectRenderer::AddMapObjectDepthPrepass(Renderer::RenderGraph* renderGraph, Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::DepthImageID depthTarget, u8 frameIndex)
{

}

void MapObjectRenderer::AddMapObjectPass(Renderer::RenderGraph* renderGraph, Renderer::Buffer<ViewConstantBuffer>* viewConstantBuffer, Renderer::ImageID renderTarget, Renderer::DepthImageID depthTarget, u8 frameIndex)
{

}

void MapObjectRenderer::LoadMapObjects(Terrain::Chunk& chunk, StringTable& stringTable)
{
    for (Terrain::MapObjectPlacement& placement : chunk.mapObjectPlacements)
    {
        u32 mapObjectID = LoadMapObject(placement.nameID, stringTable);
    }
}

void MapObjectRenderer::CreatePermanentResources()
{

}

namespace fs = std::filesystem;
u32 MapObjectRenderer::LoadMapObject(u32 nameID, StringTable& stringTable)
{
    // Placements reference a path to a MapObject, several placements can reference the same object
    // Because of this we want only the first load to actually load the object, subsequent loads should just return the id to the already loaded version
    u32 nameHash = stringTable.GetStringHash(nameID);

    auto it = _nameHashToIndexMap.find(nameHash);
    if (it != _nameHashToIndexMap.end())
    {
        return it->second;
    }

    u32 nextID = static_cast<u32>(_loadedMapObjects.size());
    LoadedMapObject& loadedMapObject = _loadedMapObjects.emplace_back();

    // -- Read MapObjectRoot --
    const std::string& nmorName = stringTable.GetString(nameID);
    if (!StringUtils::EndsWith(nmorName, ".nmor"))
    {
        NC_LOG_FATAL("For some reason, a Chunk had a MapObjectPlacement with a reference to a file that didn't end with .nmor");
    }

    fs::path nmorPath = "Data/extracted/MapObjects/" + nmorName;
    nmorPath.make_preferred();
    nmorPath = fs::absolute(nmorPath);

    FileReader nmorFile(nmorPath.string(), nmorPath.filename().string());
    if (!nmorFile.Open())
    {
        NC_LOG_FATAL("Failed to load Map Object Root file: %s", nmorPath.string());
    }
    
    Bytebuffer nmorBuffer(nullptr, nmorFile.Length());
    nmorFile.Read(nmorBuffer, nmorBuffer.size);
    nmorFile.Close();

    Terrain::MapObjectRoot mapObjectRoot;

    // Read header
    nmorBuffer.Get<Terrain::MapObjectRootHeader>(mapObjectRoot.header);

    if (mapObjectRoot.header.token != Terrain::MAP_OBJECT_ROOT_TOKEN)
    {
        NC_LOG_FATAL("We opened MapObjectRoot file (%s) with invalid token %u instead of expected token %u", nmorPath.string(), mapObjectRoot.header.token, Terrain::MAP_OBJECT_ROOT_TOKEN);
    }

    if (mapObjectRoot.header.version != Terrain::MAP_OBJECT_ROOT_VERSION)
    {
        NC_LOG_FATAL("We opened MapObjectRoot file (%s) with invalid version %u instead of expected version %u, rerun dataextractor", nmorPath.string(), mapObjectRoot.header.version, Terrain::MAP_OBJECT_ROOT_VERSION);
    }

    // Read number of materials
    u32 numMaterials;
    nmorBuffer.Get<u32>(numMaterials);

    // Read materials
    mapObjectRoot.materials.resize(numMaterials);
    nmorBuffer.GetBytes(reinterpret_cast<u8*>(mapObjectRoot.materials.data()), numMaterials * sizeof(Terrain::MapObjectMaterial));

    // Read number of groups
    nmorBuffer.Get<u32>(mapObjectRoot.numMapObjects);

    // -- Read MapObjects --
    std::string nmorNameWithoutExtension = nmorName.substr(0, nmorName.length() - 5); // Remove .nmor
    std::stringstream ss;

    for (u32 i = 0; i < mapObjectRoot.numMapObjects; i++)
    {
        ss << nmorNameWithoutExtension << "_" << std::setw(3) << std::setfill('0') << i << ".nmo";

        fs::path nmoPath = "Data/extracted/MapObjects/" + ss.str();
        nmoPath.make_preferred();
        nmoPath = fs::absolute(nmoPath);

        FileReader nmoFile(nmoPath.string(), nmoPath.filename().string());
        if (!nmoFile.Open())
        {
            NC_LOG_FATAL("Failed to load Map Object file: %s", nmoPath.string());
        }

        Bytebuffer nmoBuffer(nullptr, nmoFile.Length());
        nmoFile.Read(nmoBuffer, nmoBuffer.size);
        nmoFile.Close();

        Terrain::MapObject mapObject;

        // Read header
        nmoBuffer.Get<Terrain::MapObjectHeader>(mapObject.header);

        if (mapObject.header.token != Terrain::MAP_OBJECT_TOKEN)
        {
            NC_LOG_FATAL("We opened MapObject file (%s) with invalid token %u instead of expected token %u", nmoPath.string(), mapObject.header.token, Terrain::MAP_OBJECT_TOKEN);
        }

        if (mapObject.header.version != Terrain::MAP_OBJECT_VERSION)
        {
            NC_LOG_FATAL("We opened MapObject file (%s) with invalid version %u instead of expected version %u, rerun dataextractor", nmoPath.string(), mapObject.header.version, Terrain::MAP_OBJECT_VERSION);
        }

        // Read number of indices
        u32 numIndices;
        nmoBuffer.Get<u32>(numIndices);

        mapObject.indices.resize(numIndices);

        // Read indices
        nmoBuffer.GetBytes(reinterpret_cast<u8*>(mapObject.indices.data()), numIndices * sizeof(u16));

        // Read number of vertices
        u32 numVertices;
        nmoBuffer.Get<u32>(numVertices);

        mapObject.vertexPositions.resize(numVertices);
        mapObject.vertexNormals.resize(numVertices);
        mapObject.vertexUVs.resize(numVertices);

        // Read vertices
        nmoBuffer.GetBytes(reinterpret_cast<u8*>(mapObject.vertexPositions.data()), numVertices * sizeof(vec3));
        nmoBuffer.GetBytes(reinterpret_cast<u8*>(mapObject.vertexNormals.data()), numVertices * sizeof(vec3));
        nmoBuffer.GetBytes(reinterpret_cast<u8*>(mapObject.vertexUVs.data()), numVertices * sizeof(vec2));

        // Read number of triangle data
        u32 numTriangleData;
        nmoBuffer.Get<u32>(numTriangleData);
        mapObject.triangleData.resize(numTriangleData);
        
        // Read triangle data
        nmoBuffer.GetBytes(reinterpret_cast<u8*>(mapObject.triangleData.data()), numTriangleData * sizeof(Terrain::TriangleData));

        // Read number of RenderBatches
        u32 numRenderBatches;
        nmoBuffer.Get<u32>(numRenderBatches);

        mapObject.renderBatches.resize(numRenderBatches);

        // Read RenderBatches
        nmoBuffer.GetBytes(reinterpret_cast<u8*>(mapObject.renderBatches.data()), numRenderBatches * sizeof(Terrain::RenderBatch));

        // Find out how many different materials this MapObject uses
        std::vector<u8> usedMaterialIDs;
        usedMaterialIDs.reserve(8);

        for (Terrain::TriangleData& triangleData : mapObject.triangleData)
        {
            u8 materialID = triangleData.materialID;
            bool found = false;

            for (u8 usedMaterialID : usedMaterialIDs)
            {
                if (usedMaterialID == materialID)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                usedMaterialIDs.push_back(materialID);
            }
        }

        // Initialize primitive models, one per materialID in use
        loadedMapObject.modelIDs.reserve(usedMaterialIDs.size());

        /*for (u8 material : usedMaterialIDs)
        {
            Renderer::PrimitiveModelDesc desc;
            desc.debugName = "TODO";

            desc.


            Renderer::ModelID& modelID = loadedMapObject.modelIDs.emplace_back();
            modelID = _renderer->CreatePrimitiveModel(desc);
        }*/

        ss.clear();
        ss.str("");
    }

    


    return nextID;
}
