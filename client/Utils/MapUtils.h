#pragma once
#include <NovusTypes.h>
#include <entt.hpp>
#include "ServiceLocator.h"
#include "../Gameplay/Map/Chunk.h"
#include "../ECS/Components/Singletons/MapSingleton.h"

namespace Terrain
{
    namespace MapUtils
    {
        struct Triangle
        {
            vec3 vert1;
            vec3 vert2;
            vec3 vert3;
        };

        struct AABoundingBox
        {
            vec3 min;
            vec3 max;
        };

        inline vec2 WorldPositionToADTCoordinates(const vec3& position) // TODO: Rename this to be poggers later
        {
            // This is translated to remap positions [-17066 .. 17066] to [0 ..  34132]
            // This is because we want the Chunk Pos to be between [0 .. 64] and not [-32 .. 32]

            // We have to flip "X" and "Z" here. (Using minus below we negate the axis)
            // Wow: (North <- South, West <- East, Up) RH
            // Novus: (West -> East, Up, North <- South) LH

            return vec2(Terrain::MAP_HALF_SIZE - position.z, Terrain::MAP_HALF_SIZE - position.x);;
        }

        inline vec2 GetChunkFromAdtPosition(const vec2& adtPosition)
        {
            return adtPosition / Terrain::MAP_CHUNK_SIZE;
        }

        inline u32 GetChunkIdFromChunkPos(const vec2& chunkPos)
        {
            return Math::FloorToInt(chunkPos.x) + (Math::FloorToInt(chunkPos.y) * Terrain::MAP_CHUNKS_PER_MAP_STRIDE);
        }

        inline vec2 GetCellPosFromChunkRemainder(const vec2& chunkRemainder, vec2& outCellRemainder)
        {
            outCellRemainder = vec2(fmod(chunkRemainder.y, Terrain::MAP_CELL_SIZE), fmod(chunkRemainder.x, Terrain::MAP_CELL_SIZE));
            return glm::floor(chunkRemainder / Terrain::MAP_CELL_SIZE);
        }

        inline u32 GetCellIdFromCellPos(const vec2& cellPos)
        {
            return Math::FloorToInt(cellPos.x) + (Math::FloorToInt(cellPos.y) * Terrain::MAP_CELLS_PER_CHUNK_SIDE);
        }

        inline vec2 GetPatchPosFromCellRemainder(const vec2& cellRemainder, vec2& outPatchRemainder)
        {
            outPatchRemainder = vec2(fmod(cellRemainder.x, Terrain::MAP_PATCH_SIZE), fmod(cellRemainder.y, Terrain::MAP_PATCH_SIZE));
            return glm::floor(cellRemainder / Terrain::MAP_PATCH_SIZE);
        }

        inline f32 Sign(vec2 p1, vec2 p2, vec2 p3)
        {
            return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
        }

        inline bool IsPointInTriangle(vec2 v1, vec2 v2, vec2 v3, vec2 pt)
        {
            float d1, d2, d3;
            bool has_neg, has_pos;

            d1 = Sign(pt, v1, v2);
            d2 = Sign(pt, v2, v3);
            d3 = Sign(pt, v3, v1);

            has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

            return !(has_neg && has_pos);
        }

        inline vec2 GetLocalPosFromVertexId(i32 vertexId)
        {
            f32 aX = static_cast<f32>(vertexId % 17);
            f32 aZ = static_cast<f32>(glm::floor(vertexId / 17.0f));

            bool isOddRow = aX > 8.f;
            aX = aX - (8.5f * isOddRow);
            aZ = aZ + (0.5f * isOddRow);

            return vec2(aX, aZ);
        }

        inline ivec3 GetVertexIDsFromPatchPos(const vec2& patchPos, const vec2& patchRemainder, vec2& outB, vec2& outC)
        {
            // This is what our height data looks like
            // 0     1     2     3     4     5     6     7     8
            //    9    10    11    12    13    14    15    16
            // 17    18   19    20    21    22    23    24     25
            //    26    27    28    29    30    31   32    33
            // 34    35    36    37    38    39    40   41     42
            //    43    44    45    46    47    48    49    50
            // 51    52    53    54    55    56    57    58    59
            //    60    61    62    63    64    65    66    67
            // 68    69    70    71    72    73    74    75    76
            //    77    78    79    80    81    82    83    84
            // 85    86    87    88    89    90    91    92    93
            //    94    95    96    97    98    99    100   101
            // 102   103   104   105   106   107   108   109   110
            //    111   112   113   114   115   116   117   118
            // 119   120   121   122   123   124   125   126   127
            //    128   129   130   131   132   133   134   135
            // 136   137   138   139   140   141   142   143   144

            // Using patchPos we need to build a square looking something like this depending on what cell we're on
            // TL     TR
            //     C
            // BL     BR
            // TL = TopLeft, TR = TopRight, C = Center, BL = BottomLeft, BR = BottomRight

            u16 topLeftVertex = (Math::FloorToInt(patchPos.y) * Terrain::MAP_CELL_TOTAL_GRID_STRIDE) + Math::FloorToInt(patchPos.x);

            // Top Right is always +1 from Top Left
            u16 topRightVertex = topLeftVertex + 1;

            // Bottom Left is a full rowStride from the Top Left vertex
            u16 bottomLeftVertex = topLeftVertex + Terrain::MAP_CELL_TOTAL_GRID_STRIDE;

            // Bottom Right is always +1 from Bottom Left
            u16 bottomRightVertex = bottomLeftVertex + 1;

            // Center is always + cellStride + 1 from Top Left
            u16 centerVertex = topLeftVertex + Terrain::MAP_CELL_OUTER_GRID_STRIDE;

            // The next step is to use the patchRemainder to figure out which of these triangles we are on: https://imgur.com/i9aHwus
            ivec3 vertexIds = vec3(centerVertex, 0, 0);

            // We swap X, Y here to get the values in ADT Space
            constexpr vec2 topLeft = vec2(0, 0);
            constexpr vec2 topRight = vec2(Terrain::MAP_PATCH_SIZE, 0);
            constexpr vec2 center = vec2(Terrain::MAP_PATCH_HALF_SIZE, Terrain::MAP_PATCH_HALF_SIZE);
            constexpr vec2 bottomLeft = vec2(0, Terrain::MAP_PATCH_SIZE);
            constexpr vec2 bottomRight = vec2(Terrain::MAP_PATCH_SIZE, Terrain::MAP_PATCH_SIZE);

            vec2 patchRemainderPos = patchRemainder * Terrain::MAP_PATCH_SIZE;

            // Check North
            if (IsPointInTriangle(topLeft, topRight, center, patchRemainderPos))
            {
                vertexIds.y = topLeftVertex;
                vertexIds.z = topRightVertex;

                outB = topLeft;
                outC = topRight;
            }
            // Check East
            else if (IsPointInTriangle(topRight, bottomRight, center, patchRemainderPos))
            {
                vertexIds.y = topRightVertex;
                vertexIds.z = bottomRightVertex;

                outB = topRight;
                outC = bottomRight;
            }
            // Check South
            else if (IsPointInTriangle(bottomRight, bottomLeft, center, patchRemainderPos))
            {
                vertexIds.y = bottomRightVertex;
                vertexIds.z = bottomLeftVertex;

                outB = bottomRight;
                outC = bottomLeft;
            }
            // Check West
            else if (IsPointInTriangle(bottomLeft, topLeft, center, patchRemainderPos))
            {
                vertexIds.y = bottomLeftVertex;
                vertexIds.z = topLeftVertex;

                outB = bottomLeft;
                outC = topLeft;
            }

            return vertexIds;
        }

        

        inline bool GetVerticesFromWorldPosition(const vec3& position, Triangle& triangle)
        {
            entt::registry* registry = ServiceLocator::GetGameRegistry();
            MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

            vec2 adtPos = Terrain::MapUtils::WorldPositionToADTCoordinates(position);

            vec2 chunkPos = Terrain::MapUtils::GetChunkFromAdtPosition(adtPos);
            vec2 chunkRemainder = chunkPos - glm::floor(chunkPos);
            u32 chunkId = GetChunkIdFromChunkPos(chunkPos);

            Terrain::Map& currentMap = mapSingleton.currentMap;
            auto chunkItr = currentMap.chunks.find(chunkId);
            if (chunkItr == currentMap.chunks.end())
                return false;

            Terrain::Chunk& currentChunk = chunkItr->second;

            vec2 cellPos = (chunkRemainder * Terrain::MAP_CHUNK_SIZE) / Terrain::MAP_CELL_SIZE;
            vec2 cellRemainder = cellPos - glm::floor(cellPos);
            u32 cellId = GetCellIdFromCellPos(cellPos);

            vec2 patchPos = (cellRemainder * Terrain::MAP_CELL_SIZE) / Terrain::MAP_PATCH_SIZE;
            vec2 patchRemainder = patchPos - glm::floor(patchPos);

            // NOTE: Order of A, B and C is important, don't swap them around without understanding how it works
            vec2 a = vec2(Terrain::MAP_PATCH_HALF_SIZE, Terrain::MAP_PATCH_HALF_SIZE);
            vec2 b = vec2(0, 0);
            vec2 c = vec2(0, 0);
            
            ivec3 vertexIds = GetVertexIDsFromPatchPos(patchPos, patchRemainder, b, c);

            // X, Y here maps to our Z, X
            vec2 chunkWorldPos = glm::floor(chunkPos) * Terrain::MAP_CHUNK_SIZE;
            vec2 cellWorldPos = glm::floor(cellPos) * Terrain::MAP_CELL_SIZE;
            vec2 patchWorldPos = glm::floor(patchPos) * Terrain::MAP_PATCH_SIZE;

            // Below we subtract Terrain::MAP_HALF_SIZE to go from ADT Coordinate back to World Space
            f32 x = chunkWorldPos.y + cellWorldPos.y + patchWorldPos.y;
            f32 z = chunkWorldPos.x + cellWorldPos.x + patchWorldPos.x;

            // Calculate Vertex A
            {
                triangle.vert1.x = Terrain::MAP_HALF_SIZE - (x + a.y);
                triangle.vert1.y = currentChunk.cells[cellId].heightData[vertexIds.x];
                triangle.vert1.z = Terrain::MAP_HALF_SIZE - (z + a.x);
            }

            // Calculate Vertex B
            {
                triangle.vert2.x = Terrain::MAP_HALF_SIZE - (x + b.y);
                triangle.vert2.y = currentChunk.cells[cellId].heightData[vertexIds.y];
                triangle.vert2.z = Terrain::MAP_HALF_SIZE - (z + b.x);
            }

            // Calculate Vertex C
            {
                triangle.vert3.x = Terrain::MAP_HALF_SIZE - (x + c.y);
                triangle.vert3.y = currentChunk.cells[cellId].heightData[vertexIds.z];
                triangle.vert3.z = Terrain::MAP_HALF_SIZE - (z + c.x);
            }

            return true;
        }
        inline std::vector<Triangle> GetCellTrianglesFromWorldPosition(const vec3& position)
        {
            entt::registry* registry = ServiceLocator::GetGameRegistry();
            MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

            std::vector<Triangle> triangles;
            triangles.reserve(256);

            vec2 adtPos = Terrain::MapUtils::WorldPositionToADTCoordinates(position);

            vec2 chunkPos = Terrain::MapUtils::GetChunkFromAdtPosition(adtPos);
            vec2 chunkRemainder = chunkPos - glm::floor(chunkPos);
            u32 chunkId = GetChunkIdFromChunkPos(chunkPos);

            Terrain::Map& currentMap = mapSingleton.currentMap;
            auto chunkItr = currentMap.chunks.find(chunkId);
            if (chunkItr == currentMap.chunks.end())
                return triangles;

            Terrain::Chunk& currentChunk = chunkItr->second;

            vec2 cellPos = (chunkRemainder * Terrain::MAP_CHUNK_SIZE) / Terrain::MAP_CELL_SIZE;
            vec2 cellRemainder = cellPos - glm::floor(cellPos);
            u32 cellId = GetCellIdFromCellPos(cellPos);

            vec2 patchPos = (cellRemainder * Terrain::MAP_CELL_SIZE) / Terrain::MAP_PATCH_SIZE;
            vec2 patchRemainder = patchPos - glm::floor(patchPos);
            
            for (u16 x = 0; x < 8; x++)
            {
                for (u16 y = 0; y < 8; y++)
                {
                    for (u16 i = 0; i < 4; i++)
                    {
                        // Default Draw North
                        vec2 remainder = vec2(0, Terrain::MAP_PATCH_HALF_SIZE) / Terrain::MAP_PATCH_SIZE;

                        if (i == 1)
                        {
                            // Draw East
                            remainder = vec2(Terrain::MAP_PATCH_HALF_SIZE, Terrain::MAP_PATCH_SIZE) / Terrain::MAP_PATCH_SIZE;
                        }
                        else if (i == 2)
                        {
                            // Draw South
                            remainder = vec2(Terrain::MAP_PATCH_SIZE, Terrain::MAP_PATCH_HALF_SIZE) / Terrain::MAP_PATCH_SIZE;
                        }
                        else if (i == 3)
                        {
                            // Draw West
                            remainder = vec2(Terrain::MAP_PATCH_HALF_SIZE, Terrain::MAP_PATCH_HALF_SIZE) / Terrain::MAP_PATCH_SIZE;
                        }
                        
                        // NOTE: Order of A, B and C is important, don't swap them around without understanding how it works
                        vec2 a = vec2(Terrain::MAP_PATCH_HALF_SIZE, Terrain::MAP_PATCH_HALF_SIZE);
                        vec2 b = vec2(0, 0);
                        vec2 c = vec2(0, 0);

                        ivec3 vertexIds = GetVertexIDsFromPatchPos(vec2(x, y), remainder, b, c);

                        // X, Y here maps to our Z, X
                        vec2 chunkWorldPos = glm::floor(chunkPos) * Terrain::MAP_CHUNK_SIZE;
                        vec2 cellWorldPos = glm::floor(cellPos) * Terrain::MAP_CELL_SIZE;
                        vec2 patchWorldPos = glm::floor(vec2(x, y)) * Terrain::MAP_PATCH_SIZE;

                        // Below we subtract Terrain::MAP_HALF_SIZE to go from ADT Coordinate back to World Space
                        f32 x = chunkWorldPos.y + cellWorldPos.y + patchWorldPos.y;
                        f32 z = chunkWorldPos.x + cellWorldPos.x + patchWorldPos.x;

                        Triangle& triangle = triangles.emplace_back();

                        // Calculate Vertex A
                        {
                            triangle.vert1.x = Terrain::MAP_HALF_SIZE - (x + a.y);
                            triangle.vert1.y = currentChunk.cells[cellId].heightData[vertexIds.x];
                            triangle.vert1.z = Terrain::MAP_HALF_SIZE - (z + a.x);
                        }

                        // Calculate Vertex B
                        {
                            triangle.vert2.x = Terrain::MAP_HALF_SIZE - (x + b.y);
                            triangle.vert2.y = currentChunk.cells[cellId].heightData[vertexIds.y];
                            triangle.vert2.z = Terrain::MAP_HALF_SIZE - (z + b.x);
                        }

                        // Calculate Vertex C
                        {
                            triangle.vert3.x = Terrain::MAP_HALF_SIZE - (x + c.y);
                            triangle.vert3.y = currentChunk.cells[cellId].heightData[vertexIds.z];
                            triangle.vert3.z = Terrain::MAP_HALF_SIZE - (z + c.x);
                        }
                    }
                }
            }

            return triangles;
        }

        inline f32 GetHeightFromVertexIds(const ivec3& vertexIds, const f32* heightData, const vec2& a, const vec2& b, const vec2& c, const vec2& p)
        {
            // We do standard barycentric triangle interpolation to get the actual height of the position

            f32 det = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
            f32 factorA = (b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y);
            f32 factorB = (c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y);
            f32 alpha = factorA / det;
            f32 beta = factorB / det;
            f32 gamma = 1.0f - alpha - beta;

            f32 aHeight = heightData[vertexIds.x];
            f32 bHeight = heightData[vertexIds.y];
            f32 cHeight = heightData[vertexIds.z];

            return aHeight * alpha + bHeight * beta + cHeight * gamma;
        }
        inline f32 GetHeightFromWorldPosition(const vec3& position)
        {
            entt::registry* registry = ServiceLocator::GetGameRegistry();
            MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

            vec2 adtPos = Terrain::MapUtils::WorldPositionToADTCoordinates(position);

            vec2 chunkPos = Terrain::MapUtils::GetChunkFromAdtPosition(adtPos);
            vec2 chunkRemainder = chunkPos - glm::floor(chunkPos);
            u32 chunkId = GetChunkIdFromChunkPos(chunkPos);

            Terrain::Map& currentMap = mapSingleton.currentMap;
            auto chunkItr = currentMap.chunks.find(chunkId);
            if (chunkItr == currentMap.chunks.end())
                return false;

            Terrain::Chunk& currentChunk = chunkItr->second;

            vec2 cellPos = (chunkRemainder * Terrain::MAP_CHUNK_SIZE) / Terrain::MAP_CELL_SIZE;
            vec2 cellRemainder = cellPos - glm::floor(cellPos);
            u32 cellId = GetCellIdFromCellPos(cellPos);

            vec2 patchPos = (cellRemainder * Terrain::MAP_CELL_SIZE) / Terrain::MAP_PATCH_SIZE;
            vec2 patchRemainder = patchPos - glm::floor(patchPos);

            // NOTE: Order of A, B and C is important, don't swap them around without understanding how it works
            vec2 a = vec2(Terrain::MAP_PATCH_HALF_SIZE, Terrain::MAP_PATCH_HALF_SIZE);
            vec2 b = vec2(0, 0);
            vec2 c = vec2(0, 0);

            ivec3 vertexIds = GetVertexIDsFromPatchPos(patchPos, patchRemainder, b, c);

            return GetHeightFromVertexIds(vertexIds, &currentChunk.cells[cellId].heightData[0], a, b, c, patchRemainder * Terrain::MAP_PATCH_SIZE);
        }

        inline void Project(const vec3& vertex, const vec3& axis, vec2& minMax)
        {
            f32 val = glm::dot(axis, vertex);
            if (val < minMax.x) minMax.x = val;
            if (val > minMax.y) minMax.y = val;
        }
        inline void ProjectTriangle(const Triangle& triangle, const vec3& axis, vec2& minMax)
        {
            minMax.x = 100000.0f;
            minMax.y = -100000.0f;

            Project(triangle.vert1, axis, minMax);
            Project(triangle.vert2, axis, minMax);
            Project(triangle.vert3, axis, minMax);
        }
        inline void ProjectBox(const AABoundingBox& box, const vec3& axis, vec2& minMax)
        {
             minMax.x = 100000.0f;
             minMax.y = -100000.0f;

             Project(box.min, axis, minMax);
             Project({box.min.x, box.min.y, box.max.z}, axis, minMax);
             Project({box.min.x, box.max.y, box.min.z}, axis, minMax);
             Project({box.min.x, box.max.y, box.max.z}, axis, minMax);

             Project(box.max, axis, minMax);
             Project({box.max.x, box.min.y, box.min.z}, axis, minMax);
             Project({box.max.x, box.max.y, box.min.z}, axis, minMax);
             Project({box.max.x, box.min.y, box.max.z}, axis, minMax);
        }

        inline bool Intersect_SPHERE_TRIANGLE(const vec3& spherePos, const f32 sphereRadius, const Triangle& triangle)
        {
            // Translate problem so sphere is centered at origin
            vec3 a = triangle.vert1 - spherePos;
            vec3 b = triangle.vert2 - spherePos;
            vec3 c = triangle.vert3 - spherePos;
            f32 rr = sphereRadius * sphereRadius;

            // Compute a vector normal to triangle plane(V), normalize it(N)
            vec3 v = glm::cross(b - a, c - a);

            // Compute distance d of sphere center to triangle plane
            f32 d = glm::dot(a, v);
            f32 e = glm::dot(v, v);

            bool sep1 = d * d > rr * e;

            f32 aa = glm::dot(a, a);
            f32 ab = glm::dot(a, b);
            f32 ac = glm::dot(a, c);
            f32 bb = glm::dot(b, b);
            f32 bc = glm::dot(b, c);
            f32 cc = glm::dot(c, c);

            bool sep2 = (aa > rr) && (ab > aa) && (ac > aa);
            bool sep3 = (bb > rr) && (ab > bb) && (bc > bb);
            bool sep4 = (cc > rr) && (ac > cc) && (bc > cc);

            vec3 AB = b - a;
            vec3 BC = c - b;
            vec3 CA = a - c;

            f32 d1 = ab - aa;
            f32 d2 = bc - bb;
            f32 d3 = ac - cc;
            f32 e1 = glm::dot(AB, AB);
            f32 e2 = glm::dot(BC, BC);
            f32 e3 = glm::dot(CA, CA);

            vec3 q1 = a * e1 - d1 * AB;
            vec3 q2 = b * e2 - d2 * BC;
            vec3 q3 = c * e3 - d3 * CA;

            vec3 qc = c * e1 - q1;
            vec3 qa = a * e2 - q2;
            vec3 qb = b * e3 - q3;

            bool sep5 = (glm::dot(q1, q1) > rr * e1 * e1) && (glm::dot(q1, qc) > 0);
            bool sep6 = (glm::dot(q2, q2) > rr * e2 * e2) && (glm::dot(q2, qa) > 0);
            bool sep7 = (glm::dot(q3, q3) > rr * e3 * e3) && (glm::dot(q3, qb) > 0);

            bool seperated = sep1 || sep2 || sep3 || sep4 || sep5 || sep6 || sep7;
            return !seperated;
        }
        inline bool Intersect_AABB_TRIANGLE(const AABoundingBox& box, const Triangle& triangle)
        {
            vec2 triangleMinMax;
            vec2 boxMinMax;

            // Test the box normals (x, y and z)
            constexpr vec3 boxNormals[3] = { vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1) };

            for (i32 i = 0; i < 3; i++)
            {
                const vec3& n = boxNormals[i];
                ProjectTriangle(triangle, n, triangleMinMax);

                // If true, there is no intersection possible
                if (triangleMinMax.y < box.min[i] || triangleMinMax.x > box.max[i])
                    return false;
            }

            // Test the triangle normal
            vec3 a = triangle.vert2 - triangle.vert1;
            vec3 b = triangle.vert3 - triangle.vert1;
            vec3 triangleNormal = vec3((a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x));

            f32 triangleOffset = glm::dot(triangleNormal, triangle.vert1);
            ProjectBox(box, triangleNormal, boxMinMax);

            // If true, there is no intersection possible
            if (boxMinMax.y < triangleOffset || boxMinMax.x > triangleOffset)
                return false;

            // Test the nine edge cross-products
            vec3 triangleEdges[3] = { (triangle.vert1 - triangle.vert2), (triangle.vert2 - triangle.vert3), (triangle.vert3 - triangle.vert1) };

            for (i32 i = 0; i < 3; i++)
            {
                for (i32 j = 0; j < 3; j++)
                {
                    // The box normals are the same as it's edge tangents
                    vec3 axis = glm::cross(triangleEdges[i], boxNormals[j]);

                    ProjectBox(box, axis, boxMinMax);
                    ProjectTriangle(triangle, axis, triangleMinMax);

                    // If true, there is no intersection possible
                    if (boxMinMax.y < triangleMinMax.x || boxMinMax.x > triangleMinMax.y)
                        return false; 
                }
            }

            return true;
        }
        inline bool Intersect_AABB_TERRAIN(const vec3& position, const AABoundingBox& box, Triangle& triangle)
        {
            vec3 scale = (box.max - box.min) / 2.0f;

            vec3 offsets[5] =
            {
                {0, 0, 0},
                {-scale.x, 0, -scale.z},
                {scale.x, 0, -scale.z},
                {-scale.x, 0, scale.z},
                {scale.x, 0, scale.z}
            };

            for (i32 i = 0; i < 5; i++)
            {
                vec3 pos = position + offsets[i];

                if (GetVerticesFromWorldPosition(pos, triangle))
                {
                    if (Intersect_AABB_TRIANGLE(box, triangle))
                        return true;
                }
            }

            return false;
        }
    }
}
