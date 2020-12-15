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
    enum QueryObjectType
    {
        None = 0,
        Terrain,
        MapObject,
        ComplexModelOpaque,
        ComplexModelTransparent
    };

    class Editor
    {
    public:
        Editor();

        void Update(f32 deltaTime);
        void DrawImguiMenuBar();

    private:
        void TerrainSelectionDrawImGui();
        void MapObjectSelectionDrawImGui();
        void ComplexModelSelectionDrawImGui();

        bool IsRayIntersectingAABB(const vec3& rayOrigin, const vec3& oneOverRayDir, const Geometry::AABoundingBox& boundingBox, f32& t);
        bool OnMouseClickLeft(Window* window, std::shared_ptr<Keybind> keybind);

        NDBCEditorHandler _ndbcEditorHandler;
    private:
        u32 _activeToken = 0;
        u32 _queriedToken = 0;
        bool _selectedObjectDataInitialized = false;

        struct SelectedTerrainData
        {
            Geometry::AABoundingBox boundingBox;
            std::vector<Geometry::Triangle> triangles;

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
            Geometry::AABoundingBox boundingBox;
            u32 instanceLookupDataID;
        } _selectedMapObjectData;

        struct SelectedComplexModelData
        {
            Geometry::AABoundingBox boundingBox;
            u32 drawCallDataID;
            u32 instanceID;
            bool isOpaque;
        } _selectedComplexModelData;
    };
}