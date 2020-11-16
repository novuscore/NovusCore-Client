#include "Editor.h"
#include "../Utils/ServiceLocator.h"
#include "../Utils/MapUtils.h"

#include "../Rendering/ClientRenderer.h"
#include "../Rendering/TerrainRenderer.h"
#include "../Rendering/MapObjectRenderer.h"
#include "../Rendering/DebugRenderer.h"
#include "../Rendering/CameraFreelook.h"
#include "../ECS/Components/Singletons/NDBCSingleton.h"
#include "../ECS/Components/Singletons/MapSingleton.h"
#include "../ECS/Components/Singletons/TextureSingleton.h"
#include <CVar/CVarSystem.h>
#include <InputManager.h>
#include <GLFW/glfw3.h>
#include <entt.hpp>

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

                }
                else if (_selectedBoundingBox.type == SelectedBoundingBoxType::COMPLEX_MODEL)
                {

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
        vec3 aabbCenter = (_selectedBoundingBox.aabb.min + _selectedBoundingBox.aabb.max) / vec3(2.0f, 2.0f, 2.0f);

        vec2 adtCoords = Terrain::MapUtils::WorldPositionToADTCoordinates(aabbCenter);
        vec2 chunkCoords = Terrain::MapUtils::GetChunkFromAdtPosition(adtCoords);
        vec2 chunkRemainder = chunkCoords - glm::floor(chunkCoords);
        vec2 chunkWorldPos = vec2(Terrain::MAP_HALF_SIZE, Terrain::MAP_HALF_SIZE) - (glm::floor(chunkCoords) * vec2(Terrain::MAP_CHUNK_SIZE, Terrain::MAP_CHUNK_SIZE));
        u32 chunkId = Terrain::MapUtils::GetChunkIdFromChunkPos(chunkCoords);

        vec2 cellCoords = (chunkRemainder * Terrain::MAP_CHUNK_SIZE) / Terrain::MAP_CELL_SIZE;
        u32 cellId = Terrain::MapUtils::GetCellIdFromCellPos(cellCoords);

        entt::registry* registry = ServiceLocator::GetGameRegistry();
        MapSingleton& mapSingleton = registry->ctx<MapSingleton>();
        NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();

        const NDBC::File& areaTableFile = ndbcSingleton.nameHashToDBCFile["AreaTable"_h];

        auto itr = mapSingleton.currentMap.chunks.find(chunkId);
        if (itr == mapSingleton.currentMap.chunks.end())
            return;

        Terrain::Chunk* chunk = &itr->second;
        Terrain::Cell* cell = chunk ? &chunk->cells[cellId] : nullptr;

        // Draw AABB of current Chunk
        {
            if (chunk)
            {
                Geometry::AABoundingBox chunkAABB;
                chunkAABB.min = vec3(chunkWorldPos.y, chunk->heightHeader.gridMinHeight, chunkWorldPos.x);
                chunkAABB.max = vec3(chunkWorldPos.y - Terrain::MAP_CHUNK_SIZE, chunk->heightHeader.gridMaxHeight, chunkWorldPos.x - Terrain::MAP_CHUNK_SIZE);

                debugRenderer->DrawAABB3D(chunkAABB.min, chunkAABB.max, 0xFF0000FF);
            }
        }

        const NDBC::AreaTable* zone = cell ? mapSingleton.areaIdToDBC[cell->areaId] : nullptr;
        const NDBC::AreaTable* area = nullptr;

        if (zone && zone->parentId)
        {
            area = zone;
            zone = mapSingleton.areaIdToDBC[area->parentId];
        }

        ImGui::Text("Selected Chunk (%u)", chunkId);
        ImGui::BulletText("Zone: %s", zone ? areaTableFile.stringTable->GetString(zone->name).c_str() : "No Zone Name");
        ImGui::BulletText("Map Object Placements: %u", chunk ? chunk->mapObjectPlacements.size() : 0);
        ImGui::BulletText("Complex Model Placements: %u", chunk ? chunk->complexModelPlacements.size() : 0);

        ImGui::Spacing();
        ImGui::Spacing();

        bool hasLiquid = chunk ? (chunk->liquidHeaders.size() > 0 ? chunk->liquidHeaders[cellId].packedData != 0 : false) : false;
        ImGui::Text("Selected Cell (%u)", cellId);
        ImGui::BulletText("Area: %s", area ? areaTableFile.stringTable->GetString(area->name).c_str() : "No Area Name");
        ImGui::BulletText("Area Id: %u, Has Holes: %u, Has Liquid: %u", cell ? cell->areaId : 0, cell ? cell->hole > 0 : 0, hasLiquid);

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

    bool Editor::IsRayIntersectingMapObject(const vec3& rayOrigin, const vec3& oneOverRayDir, const Geometry::AABoundingBox* terrainAABB, f32& t)
    {
        ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
        TerrainRenderer* terrainRenderer = clientRenderer->GetTerrainRenderer();
        MapObjectRenderer* mapObjectRenderer = terrainRenderer->GetMapObjectRenderer();

        std::vector<Terrain::CullingData> mapObjectCullingData = mapObjectRenderer->GetCullingData();
        std::vector<Geometry::AABoundingBox> intersectedBoundingBoxes;
        std::vector<f32> timeToIntersection;

        intersectedBoundingBoxes.reserve(16);
        timeToIntersection.reserve(16);

        Geometry::AABoundingBox* selectedBoundingBox = nullptr;

        f32 timeToIntersect = 0;
        for (Terrain::CullingData& cullingData : mapObjectCullingData)
        {
            Geometry::AABoundingBox aabb;
            aabb.min = cullingData.minBoundingBox;
            aabb.max = cullingData.maxBoundingBox;
            
            if (IsRayIntersectingAABB(rayOrigin, oneOverRayDir, aabb, t))
            {
                intersectedBoundingBoxes.push_back(aabb);
                timeToIntersection.push_back(t);
            }
        }

        f32 smallestT = std::numeric_limits<f32>().max();
        for (u32 i = 0; i < intersectedBoundingBoxes.size(); i++)
        {
            f32& currentT = timeToIntersection[i];
            if (currentT < smallestT)
            {
                smallestT = currentT;
                selectedBoundingBox = &intersectedBoundingBoxes[i];
            }
        }

        if (selectedBoundingBox)
        {
            _selectedBoundingBox.type = SelectedBoundingBoxType::MAP_OBJECT;
            _selectedBoundingBox.aabb.min = selectedBoundingBox->min;
            _selectedBoundingBox.aabb.max = selectedBoundingBox->max;
        }

        return selectedBoundingBox != selectedBoundingBox;
    }
    bool Editor::IsRayIntersectingTerrain(const vec3& rayOrigin, const vec3& oneOverRayDir)
    {
        ClientRenderer* clientRenderer = ServiceLocator::GetClientRenderer();
        TerrainRenderer* terrainRenderer = clientRenderer->GetTerrainRenderer();

        std::vector<Geometry::AABoundingBox> terrainBoundingBoxes = terrainRenderer->GetBoundingBoxes();
        std::vector<Geometry::AABoundingBox*> intersectedBoundingBoxes;
        std::vector<f32> timeToIntersection;

        intersectedBoundingBoxes.reserve(16);
        timeToIntersection.reserve(16);

        Geometry::AABoundingBox* selectedBoundingBox = nullptr;

        f32 t = 0;
        for (Geometry::AABoundingBox& boundingBox : terrainBoundingBoxes)
        {
            if (IsRayIntersectingAABB(rayOrigin, oneOverRayDir, boundingBox, t))
            {
                intersectedBoundingBoxes.push_back(&boundingBox);
                timeToIntersection.push_back(t);
            }
        }

        f32 smallestT = std::numeric_limits<f32>().max();
        for (u32 i = 0; i < intersectedBoundingBoxes.size(); i++)
        {
            f32& currentT = timeToIntersection[i];
            if (currentT < smallestT)
            {
                smallestT = currentT;
                selectedBoundingBox = intersectedBoundingBoxes[i];
            }
        }

        if (selectedBoundingBox)
        {
            /* Check if we have hit any WMO before the terrain */
            
            f32 mapObjectTimeForRayIntersection = std::numeric_limits<f32>().max();
            if (IsRayIntersectingMapObject(rayOrigin, oneOverRayDir, selectedBoundingBox, mapObjectTimeForRayIntersection))
            {

            }

            _selectedBoundingBox.type = SelectedBoundingBoxType::TERRAIN;
            _selectedBoundingBox.aabb.min = selectedBoundingBox->min;
            _selectedBoundingBox.aabb.max = selectedBoundingBox->max;
        }

        return selectedBoundingBox != selectedBoundingBox;
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

        if (IsRayIntersectingTerrain(start, dirFrac))
            return true;

        return false;
    }
}