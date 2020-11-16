#pragma once
#include <NovusTypes.h>
#include <Math/Geometry.h>
#include "NDBC/NDBCEditorHandler.h"

class Window;
class Keybind;
class DebugRenderer;
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

        bool IsRayIntersectingMapObject(const vec3& rayOrigin, const vec3& oneOverRayDir, const Geometry::AABoundingBox* terrainAABB, f32& t);
        bool IsRayIntersectingTerrain(const vec3& rayOrigin, const vec3& oneOverRayDir);
        bool IsRayIntersectingAABB(const vec3& rayOrigin, const vec3& oneOverRayDir, const Geometry::AABoundingBox& boundingBox, f32& t);
        bool OnMouseClickLeft(Window* window, std::shared_ptr<Keybind> keybind);

        SelectedBoundingBox _selectedBoundingBox;
        NDBCEditorHandler _ndbcEditorHandler;
    };
}