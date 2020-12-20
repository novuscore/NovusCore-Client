#include "Editor.h"
#include "../Utils/ServiceLocator.h"
#include "../Utils/MapUtils.h"

#include "../Rendering/ClientRenderer.h"
#include "../Rendering/TerrainRenderer.h"
#include "../Rendering/MapObjectRenderer.h"
#include "../Rendering/CModelRenderer.h"
#include "../Rendering/DebugRenderer.h"
#include "../Rendering/PixelQuery.h"
#include "../Rendering/CameraFreelook.h"
#include "../ECS/Components/Singletons/NDBCSingleton.h"
#include "../ECS/Components/Singletons/MapSingleton.h"
#include "../ECS/Components/Singletons/TextureSingleton.h"
#include <CVar/CVarSystem.h>
#include <InputManager.h>
#include <GLFW/glfw3.h>
#include <entt.hpp>
#include <tracy/Tracy.hpp>

#include <glm/gtx/matrix_decompose.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "imgui/misc/cpp/imgui_stdlib.h"

namespace Editor
{
    AutoCVar_Int CVAR_EditorEnabled("editor.Enable", "enable editor mode for the client", 1, CVarFlags::EditCheckbox);

    Editor::Editor()
    {
        InputManager* inputManager = ServiceLocator::GetInputManager();
        inputManager->RegisterKeybind("Editor: Mouse Left Click", GLFW_MOUSE_BUTTON_LEFT, KEYBIND_ACTION_RELEASE, KEYBIND_MOD_NONE | KEYBIND_MOD_SHIFT, std::bind(&Editor::OnMouseClickLeft, this, std::placeholders::_1, std::placeholders::_2));
    }

    void Editor::Update(f32 deltaTime)
    {
        ZoneScoped;

        entt::registry* registry = ServiceLocator::GetGameRegistry();
        MapSingleton& mapSingleton = registry->ctx<MapSingleton>();
        NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();

        ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
        DebugRenderer* debugRenderer = clientRenderer->GetDebugRenderer();

        if (ImGui::Begin("Inspector Info"))
        {
            PixelQuery* pixelQuery = clientRenderer->GetPixelQuery();

            bool hasQueryToken = _queriedToken != 0;
            bool hasActiveToken = _activeToken != 0;
            bool hasNewSelection = false;

            if (hasQueryToken)
            {
                PixelQuery::PixelData pixelData;
                if (pixelQuery->GetQueryResult(_queriedToken, pixelData))
                {
                    // Here we free the currently active token (If set)
                    if (hasActiveToken)
                    {
                        pixelQuery->FreeToken(_activeToken);
                        _activeToken = 0;
                    }

                    if (pixelData.type == QueryObjectType::None)
                    {
                        pixelQuery->FreeToken(_queriedToken);
                        _queriedToken = 0;
                    }
                    else
                    {
                        _activeToken = _queriedToken;
                        _queriedToken = 0;

                        hasActiveToken = true;
                        hasNewSelection = true;
                    }
                }
            }

            PixelQuery::PixelData pixelData;
            if (hasActiveToken)
            {
                if (pixelQuery->GetQueryResult(_activeToken, pixelData))
                {
                    if (pixelData.type == QueryObjectType::Terrain)
                    {
                        if (hasNewSelection) 
                        {
                            NDBC::File* areaTableFile = ndbcSingleton.GetNDBCFile("AreaTable"_h);

                            const u32 packedChunkCellID = pixelData.value;
                            u32 cellID = packedChunkCellID & 0xffff;
                            u32 chunkID = packedChunkCellID >> 16;

                            Terrain::Chunk* chunk = mapSingleton.GetCurrentMap().GetChunkById(chunkID);
                            const Terrain::Cell& cell = chunk->cells[cellID];
                            const auto heightMinMax = std::minmax_element(cell.heightData, cell.heightData + Terrain::MAP_CELL_TOTAL_GRID_SIZE);

                            const u32 chunkX = chunkID % Terrain::MAP_CHUNKS_PER_MAP_STRIDE;
                            const u32 chunkY = chunkID / Terrain::MAP_CHUNKS_PER_MAP_STRIDE;

                            const u16 cellX = cellID % Terrain::MAP_CELLS_PER_CHUNK_SIDE;
                            const u16 cellY = cellID / Terrain::MAP_CELLS_PER_CHUNK_SIDE;

                            vec2 chunkOrigin;
                            chunkOrigin.x = Terrain::MAP_HALF_SIZE - (chunkX * Terrain::MAP_CHUNK_SIZE);
                            chunkOrigin.y = Terrain::MAP_HALF_SIZE - (chunkY * Terrain::MAP_CHUNK_SIZE);

                            vec3 min;
                            vec3 max;

                            // The reason for the flip in X and Y here is because in 2D X is Left and Right, Y is Forward and Backward.
                            // In our 3D coordinate space X is Forward and Backwards, Y is Left and Right.

                            min.x = chunkOrigin.y - (cellY * Terrain::MAP_CELL_SIZE);
                            min.y = chunkOrigin.x - (cellX * Terrain::MAP_CELL_SIZE);
                            min.z = *heightMinMax.first + 0.1f;

                            max.x = chunkOrigin.y - ((cellY + 1) * Terrain::MAP_CELL_SIZE);
                            max.y = chunkOrigin.x - ((cellX + 1) * Terrain::MAP_CELL_SIZE);
                            max.z = *heightMinMax.second + 0.1f;

                            _selectedTerrainData.boundingBox.min = glm::max(min, max);
                            _selectedTerrainData.boundingBox.max = glm::min(min, max);

                            vec3 center = (_selectedTerrainData.boundingBox.min + _selectedTerrainData.boundingBox.max) / vec3(2.0f, 2.0f, 2.0f);
                            _selectedTerrainData.triangles.clear();
                            _selectedTerrainData.triangles = Terrain::MapUtils::GetCellTrianglesFromWorldPosition(center);

                            for (auto& triangle : _selectedTerrainData.triangles)
                            {
                                // Offset Y slightly to not be directly drawn on top of the terrain
                                triangle.vert1.z += 0.1f;
                                triangle.vert2.z += 0.1f;
                                triangle.vert3.z += 0.1f;
                            }

                            _selectedTerrainData.chunkWorldPos.x = Terrain::MAP_HALF_SIZE - (chunkY * Terrain::MAP_CHUNK_SIZE);
                            _selectedTerrainData.chunkWorldPos.y = Terrain::MAP_HALF_SIZE - (chunkX * Terrain::MAP_CHUNK_SIZE);
                            _selectedTerrainData.chunkId = chunkID;
                            _selectedTerrainData.cellId = cellID;

                            _selectedTerrainData.chunk = chunk;
                            _selectedTerrainData.cell = &_selectedTerrainData.chunk->cells[_selectedTerrainData.cellId];

                            
                            _selectedTerrainData.zone = _selectedTerrainData.cell ? areaTableFile->GetRowById<NDBC::AreaTable>(_selectedTerrainData.cell->areaId) : nullptr;
                            _selectedTerrainData.area = nullptr;

                            if (_selectedTerrainData.zone && _selectedTerrainData.zone->parentId)
                            {
                                _selectedTerrainData.area = _selectedTerrainData.zone;
                                _selectedTerrainData.zone = areaTableFile->GetRowById<NDBC::AreaTable>(_selectedTerrainData.area->parentId);
                            }
                        }

                        TerrainSelectionDrawImGui();

                        debugRenderer->DrawAABB3D(_selectedTerrainData.boundingBox.min, _selectedTerrainData.boundingBox.max, 0xFF0000FF);
                        for (auto& triangle : _selectedTerrainData.triangles)
                        {
                            f32 steepnessAngle = triangle.GetSteepnessAngle();
                            u32 color = steepnessAngle <= 50 ? 0xff00ff00 : 0xff0000ff;

                            debugRenderer->DrawLine3D(triangle.vert1, triangle.vert2, color);
                            debugRenderer->DrawLine3D(triangle.vert2, triangle.vert3, color);
                            debugRenderer->DrawLine3D(triangle.vert3, triangle.vert1, color);
                        }
                    }
                    else if (pixelData.type == QueryObjectType::MapObject)
                    {
                        if (hasNewSelection)
                        {
                            _selectedMapObjectData.instanceLookupDataID = pixelData.value;

                            ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
                            TerrainRenderer* terrainRenderer = clientRenderer->GetTerrainRenderer();
                            MapObjectRenderer* mapObjectRenderer = terrainRenderer->GetMapObjectRenderer();

                            const std::vector<MapObjectRenderer::InstanceLookupData>& instanceLookupDatas = mapObjectRenderer->GetInstanceLookupData();
                            const std::vector<MapObjectRenderer::LoadedMapObject>& loadedMapObjects = mapObjectRenderer->GetLoadedMapObjects();

                            const MapObjectRenderer::InstanceLookupData& instanceLookupData = instanceLookupDatas[_selectedMapObjectData.instanceLookupDataID];
                            const MapObjectRenderer::LoadedMapObject& loadedMapObject = loadedMapObjects[instanceLookupData.loadedObjectID];
                            const mat4x4& instanceMatrix = mapObjectRenderer->GetInstances()[instanceLookupData.instanceID].instanceMatrix;

                            Geometry::AABoundingBox mapObjectAABB;
                            mapObjectAABB.min = vec3(Terrain::MAP_SIZE, Terrain::MAP_SIZE, Terrain::MAP_SIZE);
                            mapObjectAABB.max = vec3(-Terrain::MAP_SIZE, -Terrain::MAP_SIZE, -Terrain::MAP_SIZE);

                            for (const Terrain::CullingData& cullingData : loadedMapObject.cullingData)
                            {
                                vec3 minBoundingBox = cullingData.minBoundingBox;
                                vec3 maxBoundingBox = cullingData.maxBoundingBox;

                                for (u32 j = 0; j < 3; j++)
                                {
                                    if (minBoundingBox[j] < mapObjectAABB.min[j])
                                        mapObjectAABB.min[j] = minBoundingBox[j];

                                    if (maxBoundingBox[j] > mapObjectAABB.max[j])
                                        mapObjectAABB.max[j] = maxBoundingBox[j];
                                }
                            }

                            vec3 minBoundingBox = mapObjectAABB.min;
                            vec3 maxBoundingBox = mapObjectAABB.max;

                            vec3 center = (minBoundingBox + maxBoundingBox) * 0.5f;
                            vec3 extents = maxBoundingBox - center;

                            // transform center
                            vec3 transformedCenter = vec3(instanceMatrix * vec4(center, 1.0f));

                            // Transform extents (take maximum)
                            glm::mat3x3 absMatrix = glm::mat3x3(glm::abs(vec3(instanceMatrix[0])), glm::abs(vec3(instanceMatrix[1])), glm::abs(vec3(instanceMatrix[2])));
                            vec3 transformedExtents = absMatrix * extents;

                            // Transform to min/max box representation
                            _selectedMapObjectData.boundingBox.min = transformedCenter - transformedExtents;
                            _selectedMapObjectData.boundingBox.max = transformedCenter + transformedExtents;
                        }

                        MapObjectSelectionDrawImGui();
                        debugRenderer->DrawAABB3D(_selectedMapObjectData.boundingBox.min, _selectedMapObjectData.boundingBox.max, 0xFF0000FF);
                    }
                    else if (pixelData.type == QueryObjectType::ComplexModelOpaque || pixelData.type == QueryObjectType::ComplexModelTransparent)
                    {
                        if (hasNewSelection)
                        {
                            ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
                            CModelRenderer* cModelRenderer = clientRenderer->GetCModelRenderer();

                            bool isOpaque = pixelData.type == QueryObjectType::ComplexModelOpaque;

                            const std::vector<CModelRenderer::DrawCallData>& drawCallDatas = isOpaque ? cModelRenderer->GetOpaqueDrawCallData() : cModelRenderer->GetTransparentDrawCallData();
                            const CModelRenderer::DrawCallData& drawCallData = drawCallDatas[pixelData.value];

                            _selectedComplexModelData.isOpaque = isOpaque;
                            _selectedComplexModelData.drawCallDataID = pixelData.value;
                            _selectedComplexModelData.instanceID = drawCallData.instanceID;

                            const mat4x4& instanceMatrix = cModelRenderer->GetInstances()[drawCallData.instanceID].instanceMatrix;
                            const CModel::CullingData& cullingData = cModelRenderer->GetCullingData()[drawCallData.cullingDataID];
                            vec3 minBoundingBox = cullingData.minBoundingBox;
                            vec3 maxBoundingBox = cullingData.maxBoundingBox;

                            vec3 center = (minBoundingBox + maxBoundingBox) * 0.5f;
                            vec3 extents = maxBoundingBox - center;

                            // transform center
                            vec3 transformedCenter = vec3(instanceMatrix * vec4(center, 1.0f));

                            // Transform extents (take maximum)
                            glm::mat3x3 absMatrix = glm::mat3x3(glm::abs(vec3(instanceMatrix[0])), glm::abs(vec3(instanceMatrix[1])), glm::abs(vec3(instanceMatrix[2])));
                            vec3 transformedExtents = absMatrix * extents;

                            // Transform to min/max box representation
                            _selectedComplexModelData.boundingBox.min = transformedCenter - transformedExtents;
                            _selectedComplexModelData.boundingBox.max = transformedCenter + transformedExtents;
                        }

                        ComplexModelSelectionDrawImGui();
                        debugRenderer->DrawAABB3D(_selectedComplexModelData.boundingBox.min, _selectedComplexModelData.boundingBox.max, 0xFF0000FF);
                    }
                }
            }

            if (pixelData.type == QueryObjectType::None)
            {
                ImGui::TextWrapped("Welcome to the editor window. In the editor window you can see information about what you are currently viewing. To start viewing, click on a map tile, map object or complex model.");
            }
            else
            {
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::TextWrapped("You can clear your selection by using 'Shift + Mouse Left'");
            }
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

    void Editor::TerrainSelectionDrawImGui()
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

        NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();
        NDBC::File* areaTableFile = ndbcSingleton.GetNDBCFile("AreaTable"_h);

        Terrain::Chunk* chunk = _selectedTerrainData.chunk;
        Terrain::Cell* cell = _selectedTerrainData.cell;

        if (!chunk || !cell)
            return;

        const NDBC::AreaTable* zone = _selectedTerrainData.zone;
        const NDBC::AreaTable* area = _selectedTerrainData.area;

        if (zone && zone->parentId)
        {
            area = zone;
            zone = areaTableFile->GetRowById<NDBC::AreaTable>(area->parentId);
        }

        ImGui::Text("Selected Chunk (%u)", _selectedTerrainData.chunkId);
        ImGui::BulletText("Zone: %s", zone ? areaTableFile->GetStringTable()->GetString(zone->name).c_str() : "No Zone Name");
        ImGui::BulletText("Map Object Placements: %u", chunk->mapObjectPlacements.size());
        ImGui::BulletText("Complex Model Placements: %u", chunk->complexModelPlacements.size());

        ImGui::Spacing();
        ImGui::Spacing();

        bool hasLiquid = false;// chunk->liquidHeaders.size() > 0 ? chunk->liquidHeaders[_selectedTerrainData.cellId].packedData != 0 : false;
        ImGui::Text("Selected Cell (%u)", _selectedTerrainData.cellId);
        ImGui::BulletText("Area: %s", area ? areaTableFile->GetStringTable()->GetString(area->name).c_str() : "No Area Name");
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

    void Editor::MapObjectSelectionDrawImGui()
    {
        ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
        TerrainRenderer* terrainRenderer = clientRenderer->GetTerrainRenderer();
        MapObjectRenderer* mapObjectRenderer = terrainRenderer->GetMapObjectRenderer();

        const std::vector<MapObjectRenderer::InstanceLookupData>& instanceLookupDatas = mapObjectRenderer->GetInstanceLookupData();
        const std::vector<MapObjectRenderer::LoadedMapObject>& loadedMapObjects = mapObjectRenderer->GetLoadedMapObjects();

        const MapObjectRenderer::InstanceLookupData& instanceLookupData = instanceLookupDatas[_selectedMapObjectData.instanceLookupDataID];
        const MapObjectRenderer::LoadedMapObject& loadedMapObject = loadedMapObjects[instanceLookupData.loadedObjectID];
        const mat4x4& instanceMatrix = mapObjectRenderer->GetInstances()[instanceLookupData.instanceID].instanceMatrix;

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

    void Editor::ComplexModelSelectionDrawImGui()
    {
        ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
        CModelRenderer* cModelRenderer = clientRenderer->GetCModelRenderer();

        u32 loadedObjectIndex = cModelRenderer->GetModelIndexByDrawCallDataIndex(_selectedComplexModelData.drawCallDataID, _selectedComplexModelData.isOpaque);
        const CModelRenderer::LoadedComplexModel& loadedComplexModel = cModelRenderer->GetLoadedComplexModels()[loadedObjectIndex];
        const mat4x4& instanceMatrix = cModelRenderer->GetInstances()[_selectedComplexModelData.instanceID].instanceMatrix;

        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(instanceMatrix, scale, rotation, translation, skew, perspective);

        glm::vec3 euler = glm::eulerAngles(rotation);
        glm::vec3 eulerAsDeg = glm::degrees(euler);

        ImGui::Text("Complex Model");
        ImGui::Text("Model: %s", loadedComplexModel.debugName.c_str());
        ImGui::Text("Position: X: %.2f, Y: %.2f, Z: %.2f", translation.x, translation.y, translation.z);
        ImGui::Text("Scale: X: %.2f, Y: %.2f, Z: %.2f", scale.x, scale.y, scale.z);
        ImGui::Text("Rotation: X: %.2f, Y: %.2f, Z: %.2f", eulerAsDeg.x, eulerAsDeg.y, eulerAsDeg.z);
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

        ZoneScoped;

        ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
        InputManager* inputManager = ServiceLocator::GetInputManager();
        PixelQuery* pixelQuery = clientRenderer->GetPixelQuery();

        if (_queriedToken != 0)
        {
            pixelQuery->FreeToken(_queriedToken);
            _queriedToken = 0;
        }

        // Shift Click clears selection
        if (keybind->currentModifierMask & KEYBIND_MOD_SHIFT)
        {
            if (_activeToken != 0)
            {
                pixelQuery->FreeToken(_activeToken);
                _activeToken = 0;
            }

            return false;
        }

        hvec2 mousePosition = inputManager->GetMousePosition();
        u32 mousePosX = static_cast<u32>(mousePosition.x);
        u32 mousePosY = static_cast<u32>(mousePosition.y);

        // Check if we need to free the previous _queriedToken
        {
            if (_queriedToken != 0)
            {
                pixelQuery->FreeToken(_queriedToken);
                _queriedToken = 0;
            }
        }

        _queriedToken = pixelQuery->PerformQuery(uvec2(mousePosX, mousePosY));
        return true;
    }
}