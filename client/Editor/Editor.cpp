#include "Editor.h"
#include "../Utils/ServiceLocator.h"
#include "../Utils/MapUtils.h"

#include "../Rendering/ClientRenderer.h"
#include "../Rendering/TerrainRenderer.h"
#include "../Rendering/MapObjectRenderer.h"
#include "../Rendering/CModelRenderer.h"
#include "../Rendering/DebugRenderer.h"
#include "../Rendering/CameraFreelook.h"
#include "../ECS/Components/Singletons/NDBCSingleton.h"
#include "../ECS/Components/Singletons/MapSingleton.h"
#include "../ECS/Components/Singletons/TextureSingleton.h"
#include <CVar/CVarSystem.h>
#include <InputManager.h>
#include <GLFW/glfw3.h>
#include <entt.hpp>

#include <glm/gtx/matrix_decompose.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "imgui/misc/cpp/imgui_stdlib.h"

namespace Editor
{
    AutoCVar_Int CVAR_EditorEnabled("editor.Enable", "enable editor mode for the client", 1, CVarFlags::EditCheckbox);

    AutoCVar_Int CVAR_EditorRayEnabled("editor.ray.Enable", "enable ray for debugging screen to world", 0, CVarFlags::EditCheckbox);
    AutoCVar_VecFloat CVAR_EditorRayStartPosition("editor.ray.StartPosition", "", vec4(0, 0, 0, 0), CVarFlags::Noedit);
    AutoCVar_VecFloat CVAR_EditorRayEndPosition("editor.ray.EndPosition", "", vec4(0, 0, 0, 0), CVarFlags::Noedit);

    Editor::Editor()
    {
        InputManager* inputManager = ServiceLocator::GetInputManager();
        inputManager->RegisterKeybind("Editor: Mouse Left Click", GLFW_MOUSE_BUTTON_LEFT, KEYBIND_ACTION_RELEASE, KEYBIND_MOD_NONE | KEYBIND_MOD_SHIFT, std::bind(&Editor::OnMouseClickLeft, this, std::placeholders::_1, std::placeholders::_2));
    }

    void Editor::Update(f32 deltaTime)
    {
        ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
        DebugRenderer* debugRenderer = clientRenderer->GetDebugRenderer();

        if (CVAR_EditorRayEnabled.Get())
        {
            debugRenderer->DrawLine3D(CVAR_EditorRayStartPosition.Get(), CVAR_EditorRayEndPosition.Get(), 0x00FF00FF);
        }

        if (ImGui::Begin("Editor Info"))
        {
            if (_selectedBoundingBox.type != SelectedBoundingBoxType::NONE)
            {
                if (_selectedBoundingBox.type == SelectedBoundingBoxType::TERRAIN)
                {
                    HandleTerrainBoundingBox(debugRenderer);
                }
                else if (_selectedBoundingBox.type == SelectedBoundingBoxType::MAP_OBJECT)
                {

                    ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
                    TerrainRenderer* terrainRenderer = clientRenderer->GetTerrainRenderer();
                    MapObjectRenderer* mapObjectRenderer = terrainRenderer->GetMapObjectRenderer();
                    const std::vector<Terrain::PlacementDetails>& mapObjectPlacementDetails = mapObjectRenderer->GetPlacementDetails();
                    const std::vector<MapObjectRenderer::LoadedMapObject>& loadedMapObjects = mapObjectRenderer->GetLoadedMapObjects();

                    const Terrain::PlacementDetails& placementDetails = mapObjectPlacementDetails[_selectedMapObjectData.placementDetailsIndex];
                    const MapObjectRenderer::LoadedMapObject& loadedMapObject = loadedMapObjects[placementDetails.loadedIndex];
                    const mat4x4& instanceMatrix = mapObjectRenderer->GetInstances()[placementDetails.instanceIndex].instanceMatrix;

                    glm::vec3 scale;
                    glm::quat rotation;
                    glm::vec3 translation;
                    glm::vec3 skew;
                    glm::vec4 perspective;
                    glm::decompose(instanceMatrix, scale, rotation, translation, skew, perspective);

                    glm::vec3 euler = glm::eulerAngles(rotation);
                    glm::vec3 eulerAsDeg = glm::degrees(euler);

                    ImGui::Text("Map Object");
                    ImGui::Text("Model: %s", loadedMapObject.debugName.c_str());
                    ImGui::Text("Position: X: %.2f, Y: %.2f, Z: %.2f", translation.x, translation.y, translation.z);
                    ImGui::Text("Scale: X: %.2f, Y: %.2f, Z: %.2f", scale.x, scale.y, scale.z);
                    ImGui::Text("Rotation: X: %.2f, Y: %.2f, Z: %.2f", eulerAsDeg.x, eulerAsDeg.y, eulerAsDeg.z);
                }
                else if (_selectedBoundingBox.type == SelectedBoundingBoxType::COMPLEX_MODEL)
                {

                    ImGui::Text("COMPLEX MODEL");
                }

                debugRenderer->DrawAABB3D(_selectedBoundingBox.aabb.min, _selectedBoundingBox.aabb.max, 0xFFFF00FF);
            }
            else
            {
                ImGui::TextWrapped("Welcome to the editor window. In the editor window you can see information about what you are currently viewing. To start viewing, click on a map tile, map object or complex model.");
            }

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::TextWrapped("You can clear your selection by using 'Shift + Mouse Left'");
        }

        ImGui::End();

        _ndbcEditorHandler.Draw();
    }

    void Editor::DrawImguiMenuBar()
    {
        if (ImGui::BeginMenu("Editor"))
        {
            _ndbcEditorHandler.DrawImGuiMenuBar();

            ImGui::EndMenu();
        }
    }

    void Editor::HandleTerrainBoundingBox(DebugRenderer* debugRenderer)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

        if (!_selectedObjectDataInitialized)
        {
            _selectedObjectDataInitialized = true;
            _selectedTerrainData.center = (_selectedBoundingBox.aabb.min + _selectedBoundingBox.aabb.max) / vec3(2.0f, 2.0f, 2.0f);

            _selectedTerrainData.adtCoords = Terrain::MapUtils::WorldPositionToADTCoordinates(_selectedTerrainData.center);
            _selectedTerrainData.chunkCoords = Terrain::MapUtils::GetChunkFromAdtPosition(_selectedTerrainData.adtCoords);

            vec2 chunkCoordsFloored = glm::floor(_selectedTerrainData.chunkCoords);
            vec2 chunkRemainder = _selectedTerrainData.chunkCoords - chunkCoordsFloored;

            _selectedTerrainData.chunkWorldPos.x = Terrain::MAP_HALF_SIZE - (chunkCoordsFloored.y * Terrain::MAP_CHUNK_SIZE);
            _selectedTerrainData.chunkWorldPos.y = Terrain::MAP_HALF_SIZE - (chunkCoordsFloored.x * Terrain::MAP_CHUNK_SIZE);
            _selectedTerrainData.chunkId = Terrain::MapUtils::GetChunkIdFromChunkPos(_selectedTerrainData.chunkCoords);

            _selectedTerrainData.cellCoords = (chunkRemainder * Terrain::MAP_CHUNK_SIZE) / Terrain::MAP_CELL_SIZE;
            _selectedTerrainData.cellId = Terrain::MapUtils::GetCellIdFromCellPos(_selectedTerrainData.cellCoords);

            auto itr = mapSingleton.currentMap.chunks.find(_selectedTerrainData.chunkId);
            if (itr == mapSingleton.currentMap.chunks.end())
                return;

            _selectedTerrainData.chunk = &itr->second;
            _selectedTerrainData.cell = &_selectedTerrainData.chunk->cells[_selectedTerrainData.cellId];

            _selectedTerrainData.zone = _selectedTerrainData.cell ? mapSingleton.areaIdToDBC[_selectedTerrainData.cell->areaId] : nullptr;
            _selectedTerrainData.area = nullptr;
            
            if (_selectedTerrainData.zone && _selectedTerrainData.zone->parentId)
            {
                _selectedTerrainData.area = _selectedTerrainData.zone;
                _selectedTerrainData.zone = mapSingleton.areaIdToDBC[_selectedTerrainData.area->parentId];
            }
        }

        NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();
        const NDBC::File& areaTableFile = ndbcSingleton.nameHashToDBCFile["AreaTable"_h];

        // Draw AABB of current Chunk
        {
            if (_selectedTerrainData.chunk)
            {
                Geometry::AABoundingBox chunkAABB;
                chunkAABB.min = vec3(_selectedTerrainData.chunkWorldPos.x, _selectedTerrainData.chunkWorldPos.y, _selectedTerrainData.chunk->heightHeader.gridMinHeight);
                chunkAABB.max = vec3(_selectedTerrainData.chunkWorldPos.x - Terrain::MAP_CHUNK_SIZE, _selectedTerrainData.chunkWorldPos.y - Terrain::MAP_CHUNK_SIZE, _selectedTerrainData.chunk->heightHeader.gridMaxHeight);

                debugRenderer->DrawAABB3D(chunkAABB.min, chunkAABB.max, 0xFF0000FF);
            }
        }

        Terrain::Chunk* chunk = _selectedTerrainData.chunk;
        Terrain::Cell* cell = _selectedTerrainData.cell;

        if (!chunk || !cell)
            return;

        const NDBC::AreaTable* zone = _selectedTerrainData.zone;
        const NDBC::AreaTable* area = _selectedTerrainData.area;

        if (zone && zone->parentId)
        {
            area = zone;
            zone = mapSingleton.areaIdToDBC[area->parentId];
        }

        ImGui::Text("Selected Chunk (%u)", _selectedTerrainData.chunkId);
        ImGui::BulletText("Zone: %s", zone ? areaTableFile.stringTable->GetString(zone->name).c_str() : "No Zone Name");
        ImGui::BulletText("Map Object Placements: %u", chunk->mapObjectPlacements.size());
        ImGui::BulletText("Complex Model Placements: %u", chunk->complexModelPlacements.size());

        ImGui::Spacing();
        ImGui::Spacing();

        bool hasLiquid = chunk->liquidHeaders.size() > 0 ? chunk->liquidHeaders[_selectedTerrainData.cellId].packedData != 0 : false;
        ImGui::Text("Selected Cell (%u)", _selectedTerrainData.cellId);
        ImGui::BulletText("Area: %s", area ? areaTableFile.stringTable->GetString(area->name).c_str() : "No Area Name");
        ImGui::BulletText("Area Id: %u, Has Holes: %u, Has Liquid: %u", cell->areaId, cell->hole > 0, hasLiquid);

        ImGui::Spacing();
        ImGui::Spacing();

        TextureSingleton& textureSingleton = registry->ctx<TextureSingleton>();
        for (u32 i = 0; i < 4; i++)
        {
            if (cell)
            {
                Terrain::LayerData& layerData = cell->layers[i];
                if (layerData.textureId != layerData.TextureIdInvalid)
                {
                    const std::string& texture = textureSingleton.textureHashToPath[layerData.textureId];
                    ImGui::BulletText("Texture %u: %s", i, texture.c_str());
                    continue;
                }
            }
                
            ImGui::BulletText("Texture %u: Unused", i);
        }
    }

    bool Editor::IsRayIntersectingComplexModel(const vec3& rayOrigin, const vec3& oneOverRayDir, f32& outTime)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

        ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
        TerrainRenderer* terrainRenderer = clientRenderer->GetTerrainRenderer();
        CModelRenderer* cmodelRenderer = terrainRenderer->GetComplexModelRenderer();

        vec3 aabbCenter = (_selectedBoundingBox.aabb.min + _selectedBoundingBox.aabb.max) / vec3(2.0f, 2.0f, 2.0f);
        vec2 adtCoords = Terrain::MapUtils::WorldPositionToADTCoordinates(aabbCenter);
        vec2 chunkCoords = Terrain::MapUtils::GetChunkFromAdtPosition(adtCoords);
        u32 chunkId = Terrain::MapUtils::GetChunkIdFromChunkPos(chunkCoords);

        const Terrain::Chunk& chunk = mapSingleton.currentMap.chunks[chunkId];
        u32 numPlacementsOnChunk = static_cast<u32>(chunk.complexModelPlacements.size());
        u32 chunkMapPlacementDetailsOffset = cmodelRenderer->GetChunkPlacementDetailsOffset(chunkId);
        const std::vector<Terrain::PlacementDetails>& complexModelPlacementDetails = cmodelRenderer->GetPlacementDetails();

        const std::vector<Terrain::PlacementDetails>& complexPlacementDetails = cmodelRenderer->GetPlacementDetails();
        const std::vector<CModelRenderer::Instance>& complexModelInstances = cmodelRenderer->GetInstances();
        const std::vector<CModelRenderer::LoadedComplexModel>& loadedComplexModels = cmodelRenderer->GetLoadedComplexModels();
        const std::vector<CModel::CullingData>& complexModelCullingDatas = cmodelRenderer->GetCullingData();

        bool didIntersectAtleastOnce = false;

        for (u32 i = 0; i < complexPlacementDetails.size(); i++)
        {
            const Terrain::PlacementDetails& placementDetails = complexPlacementDetails[i];

            const CModelRenderer::Instance& instance = complexModelInstances[placementDetails.instanceIndex];
            const CModelRenderer::LoadedComplexModel& loadedComplexModel = loadedComplexModels[placementDetails.loadedIndex];

            // Particle Emitters have no culling data
            if (loadedComplexModel.cullingDataID == std::numeric_limits<u32>().max())
                continue;

            const CModel::CullingData& cullingData = complexModelCullingDatas[loadedComplexModel.cullingDataID];

            vec3 minBoundingBox = cullingData.minBoundingBox;
            vec3 maxBoundingBox = cullingData.maxBoundingBox;

            vec3 center = (minBoundingBox + maxBoundingBox) * 0.5f;
            vec3 extents = maxBoundingBox - center;

            // transform center
            const mat4x4& m = instance.instanceMatrix;
            vec3 transformedCenter = vec3(m * vec4(center, 1.0f));

            // Transform extents (take maximum)
            glm::mat3x3 absMatrix = glm::mat3x3(glm::abs(vec3(m[0])), glm::abs(vec3(m[1])), glm::abs(vec3(m[2])));
            vec3 transformedExtents = absMatrix * extents;

            // Transform to min/max box representation
            Geometry::AABoundingBox aabb;
            aabb.min = transformedCenter - transformedExtents;
            aabb.max = transformedCenter + transformedExtents;

            f32 t = 0;
            if (IsRayIntersectingAABB(rayOrigin, oneOverRayDir, aabb, t))
            {
                if (t >= 0.0f && t < outTime)
                {
                    outTime = t;
                    _boundingBoxComplexModel = aabb;
                    _selectedComplexModelData.placementDetailsIndex = i;

                    didIntersectAtleastOnce = true;
                }
            }
        }

        return didIntersectAtleastOnce;
    }
    bool Editor::IsRayIntersectingMapObject(const vec3& rayOrigin, const vec3& oneOverRayDir, f32& outTime)
    {
        ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
        TerrainRenderer* terrainRenderer = clientRenderer->GetTerrainRenderer();
        MapObjectRenderer* mapObjectRenderer = terrainRenderer->GetMapObjectRenderer();

        const std::vector<Terrain::PlacementDetails>& mapObjectPlacementDetails = mapObjectRenderer->GetPlacementDetails();
        const std::vector<MapObjectRenderer::InstanceData>& mapObjectInstanceDatas = mapObjectRenderer->GetInstances();
        const std::vector<MapObjectRenderer::LoadedMapObject>& loadedMapObjects = mapObjectRenderer->GetLoadedMapObjects();

        bool didIntersectAtleastOnce = false;

        for (u32 i = 0; i < mapObjectPlacementDetails.size(); i++)
        {
            const Terrain::PlacementDetails& placementDetails = mapObjectPlacementDetails[i];

            const MapObjectRenderer::InstanceData& instanceData = mapObjectInstanceDatas[placementDetails.instanceIndex];
            const MapObjectRenderer::LoadedMapObject& loadedMapObject = loadedMapObjects[placementDetails.loadedIndex];

            const mat4x4& m = instanceData.instanceMatrix;
            glm::mat3x3 absMatrix = glm::mat3x3(glm::abs(vec3(m[0])), glm::abs(vec3(m[1])), glm::abs(vec3(m[2])));

            Geometry::AABoundingBox mapObjectAABB;
            mapObjectAABB.min = vec3(Terrain::MAP_SIZE, Terrain::MAP_SIZE, Terrain::MAP_SIZE);
            mapObjectAABB.max = vec3(-Terrain::MAP_SIZE, -Terrain::MAP_SIZE, -Terrain::MAP_SIZE);

            for (const Terrain::CullingData& cullingData : loadedMapObject.cullingData)
            {
                vec3 minBoundingBox = cullingData.minBoundingBox;
                vec3 maxBoundingBox = cullingData.maxBoundingBox;

                vec3 center = (minBoundingBox + maxBoundingBox) * 0.5f;
                vec3 extents = maxBoundingBox - center;

                // transform center
                vec3 transformedCenter = vec3(m * vec4(center, 1.0f));
                vec3 transformedExtents = absMatrix * extents;

                // Transform to min/max box representation
                Geometry::AABoundingBox aabb;
                aabb.min = transformedCenter - transformedExtents;
                aabb.max = transformedCenter + transformedExtents;

                for (u32 j = 0; j < 3; j++)
                {
                    if (aabb.min[j] < mapObjectAABB.min[j])
                        mapObjectAABB.min[j] = aabb.min[j];

                    if (aabb.max[j] > mapObjectAABB.max[j])
                        mapObjectAABB.max[j] = aabb.max[j];
                }
            }

            f32 t = 0;
            if (IsRayIntersectingAABB(rayOrigin, oneOverRayDir, mapObjectAABB, t))
            {
                if (t >= 0.0f && t < outTime)
                {
                    outTime = t;
                    _boundingBoxMapObject = mapObjectAABB;
                    _selectedMapObjectData.placementDetailsIndex = i;

                    didIntersectAtleastOnce = true;
                }
            }
        }

        return didIntersectAtleastOnce;
    }
    bool Editor::IsRayIntersectingTerrain(const vec3& rayOrigin, const vec3& oneOverRayDir, f32& outTime)
    {
        ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
        TerrainRenderer* terrainRenderer = clientRenderer->GetTerrainRenderer();

        std::vector<Geometry::AABoundingBox> terrainBoundingBoxes = terrainRenderer->GetBoundingBoxes();

        bool didIntersectAtleastOne = false;

        f32 t = 0;
        for (Geometry::AABoundingBox& boundingBox : terrainBoundingBoxes)
        {
            if (IsRayIntersectingAABB(rayOrigin, oneOverRayDir, boundingBox, t))
            {
                if (t >= 0.0f && t < outTime)
                {
                    outTime = t;
                    _boundingBoxTerrain = boundingBox;

                    didIntersectAtleastOne = true;
                }
            }
        }

        return didIntersectAtleastOne;
    }
    bool Editor::IsRayIntersectingAABB(const vec3& rayOrigin, const vec3& oneOverRayDir, const Geometry::AABoundingBox& aabb, f32& t)
    {
        f32 t1 = (aabb.min.x - rayOrigin.x) * oneOverRayDir.x;
        f32 t2 = (aabb.max.x - rayOrigin.x) * oneOverRayDir.x;
        f32 t3 = (aabb.min.y - rayOrigin.y) * oneOverRayDir.y;
        f32 t4 = (aabb.max.y - rayOrigin.y) * oneOverRayDir.y;
        f32 t5 = (aabb.min.z - rayOrigin.z) * oneOverRayDir.z;
        f32 t6 = (aabb.max.z - rayOrigin.z) * oneOverRayDir.z;

        f32 tMin = glm::max(glm::max(glm::min(t1, t2), glm::min(t3, t4)), glm::min(t5, t6));
        f32 tMax = glm::min(glm::min(glm::max(t1, t2), glm::max(t3, t4)), glm::max(t5, t6));

        if (tMax < 0 || tMin > tMax)
        {
            t = tMax;
            return false;
        }

        t = tMin;
        return true;
    }

    bool Editor::OnMouseClickLeft(Window* window, std::shared_ptr<Keybind> keybind)
    {
        if (!CVAR_EditorEnabled.Get())
            return false;

        CameraFreeLook* camera = ServiceLocator::GetCameraFreeLook();
        if (!camera->IsActive())
            return false;

        if (camera->IsMouseCaptured())
            return false;

        if (ImGui::GetCurrentContext()->HoveredWindow)
            return false;

        // Shift Click clears selection
        if (keybind->currentModifierMask & KEYBIND_MOD_SHIFT)
        {
            if (CVAR_EditorRayEnabled.Get())
            {
                CVAR_EditorRayStartPosition.Set(vec4(0, 0, 0, 0));
                CVAR_EditorRayEndPosition.Set(vec4(0, 0, 0, 0));
            }

            _selectedBoundingBox.type = SelectedBoundingBoxType::NONE;
            return false;
        }

        ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
        InputManager* inputManager = ServiceLocator::GetInputManager();
        hvec2 mousePosition = inputManager->GetMousePosition();

        f32 mousePosX = Math::Map(mousePosition.x, 0, static_cast<f32>(clientRenderer->WIDTH), -1, 1);
        f32 mousePosY = -Math::Map(mousePosition.y, 0, static_cast<f32>(clientRenderer->HEIGHT), -1, 1);

        const mat4x4 m = glm::inverse(camera->GetViewProjectionMatrix());

        const vec3 start = DebugRenderer::UnProject(vec3(mousePosX, mousePosY, 1.0f), m);
        const vec3 end = DebugRenderer::UnProject(vec3(mousePosX, mousePosY, 0.0f), m);
        const vec3 dir = glm::normalize(end - start);
        const vec3 dirFrac = vec3(1.0f, 1.0f, 1.0f) / dir;

        if (CVAR_EditorRayEnabled.Get())
        {
            CVAR_EditorRayStartPosition.Set(vec4(start, 0));
            CVAR_EditorRayEndPosition.Set(vec4(end, 0));
        }

        std::vector<f32> rayIntersectionTimes = { 100000.0f, 100000.0f, 100000.0f };

        bool didRayIntersectTerrain = IsRayIntersectingTerrain(start, dirFrac, rayIntersectionTimes[0]);
        bool didRayIntersectMapObject = IsRayIntersectingMapObject(start, dirFrac, rayIntersectionTimes[1]);
        bool didRayIntersectComplexModel = IsRayIntersectingComplexModel(start, dirFrac, rayIntersectionTimes[2]);

        bool didRayIntersect = didRayIntersectTerrain || didRayIntersectMapObject || didRayIntersectComplexModel;

        if (didRayIntersect)
        {
            // Figure out which one to select
            {
                // Rules to help make selecting Map Objects & Complex Models easier (Later we will use the GPU for Pixel Perfect Accuracy, so this is just temporary)
                if (didRayIntersectTerrain && (didRayIntersectMapObject || didRayIntersectComplexModel))
                    rayIntersectionTimes[0] = 100000.0f;

                f32 minimumTimeToIntersect = 100000.0f;
                u32 index = std::numeric_limits<u32>().max();

                for (u32 i = 0; i < rayIntersectionTimes.size(); i++)
                {
                    f32 rayIntersectionTime = rayIntersectionTimes[i];
                    if (rayIntersectionTime < minimumTimeToIntersect)
                    {
                        minimumTimeToIntersect = rayIntersectionTime;
                        index = i;
                    }
                }

                switch (index)
                {
                    case 0: // Terrain
                    {
                        _selectedBoundingBox.type = SelectedBoundingBoxType::TERRAIN;
                        _selectedBoundingBox.aabb = _boundingBoxTerrain;
                        break;
                    }

                    case 1: // Map Object
                    {
                        _selectedBoundingBox.type = SelectedBoundingBoxType::MAP_OBJECT;
                        _selectedBoundingBox.aabb = _boundingBoxMapObject;
                        break;
                    }

                    case 2: // Complex Model
                    {
                        _selectedBoundingBox.type = SelectedBoundingBoxType::COMPLEX_MODEL;
                        _selectedBoundingBox.aabb = _boundingBoxComplexModel;
                        break;
                    }

                    default:
                    {
                        break;
                    }
                }
            }

            _selectedObjectDataInitialized = false;
        }

        return didRayIntersect;
    }
}