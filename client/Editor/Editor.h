#pragma once
#include <NovusTypes.h>
#include <Math/Geometry.h>
#include "NDBC/NDBCEditorHandler.h"

class Window;
class Keybind;
class DebugRenderer;

namespace Terrain
{
    struct Chunk;
    struct Cell;
}

namespace NDBC
{
    struct AreaTable;
}

namespace Editor
{
    enum class SelectedBoundingBoxType
    {
        NONE,
        TERRAIN,
        MAP_OBJECT,
        COMPLEX_MODEL
    };

    struct SelectedBoundingBox
    {
        SelectedBoundingBoxType type = SelectedBoundingBoxType::NONE;
        Geometry::AABoundingBox aabb;
    };

    class Editor
    {
    public:
        Editor();

        void Update(f32 deltaTime);
        void DrawImguiMenuBar();

    private:
        void HandleTerrainBoundingBox(DebugRenderer* debugRenderer);

        bool IsRayIntersectingComplexModel(const vec3& rayOrigin, const vec3& oneOverRayDir, f32& outTime);
        bool IsRayIntersectingMapObject(const vec3& rayOrigin, const vec3& oneOverRayDir, f32& outTime);
        bool IsRayIntersectingTerrain(const vec3& rayOrigin, const vec3& oneOverRayDir, f32& outTime);
        bool IsRayIntersectingAABB(const vec3& rayOrigin, const vec3& oneOverRayDir, const Geometry::AABoundingBox& boundingBox, f32& t);
        bool OnMouseClickLeft(Window* window, std::shared_ptr<Keybind> keybind);

        SelectedBoundingBox _selectedBoundingBox;
        Geometry::AABoundingBox _boundingBoxTerrain;
        Geometry::AABoundingBox _boundingBoxMapObject;
        Geometry::AABoundingBox _boundingBoxComplexModel;
        NDBCEditorHandler _ndbcEditorHandler;


    private:
        bool _selectedObjectDataInitialized = false;

        struct SelectedTerrainData
        {
            vec3 center;

            vec2 adtCoords;
            vec2 chunkCoords;
            vec2 chunkWorldPos;
            vec2 cellCoords;

            u32 chunkId;
            u32 cellId;

            Terrain::Chunk* chunk;
            Terrain::Cell* cell;

            NDBC::AreaTable* zone;
            NDBC::AreaTable* area;
        } _selectedTerrainData;

        struct SelectedMapObjectData
        {
            u32 placementDetailsIndex;
        } _selectedMapObjectData;

        struct SelectedComplexModelData
        {
            u32 placementDetailsIndex;
        } _selectedComplexModelData;
    };
}