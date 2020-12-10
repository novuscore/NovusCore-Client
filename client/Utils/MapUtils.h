#pragma once
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

// Code used from PhysX was modified by us

#include <NovusTypes.h>
#include <Math/Geometry.h>
#include <entt.hpp>
#include "ServiceLocator.h"
#include "../Gameplay/Map/Chunk.h"
#include "../ECS/Components/Singletons/MapSingleton.h"

namespace Terrain
{
    namespace MapUtils
    {
        constexpr f32 f32MaxValue = 3.40282346638528859812e+38F;

        inline vec2 WorldPositionToADTCoordinates(const vec3& position)
        {
            // This is translated to remap positions [-17066 .. 17066] to [0 ..  34132]
            // This is because we want the Chunk Pos to be between [0 .. 64] and not [-32 .. 32]

            // We have to flip "X" and "Y" here due to 3D -> 2D
            return vec2(Terrain::MAP_HALF_SIZE - position.y, Terrain::MAP_HALF_SIZE - position.x);
        }

        inline vec2 GetChunkFromAdtPosition(const vec2& adtPosition)
        {
            return adtPosition / Terrain::MAP_CHUNK_SIZE;
        }

        inline u32 GetChunkIdFromChunkPos(const vec2& chunkPos)
        {
            return Math::FloorToInt(chunkPos.x) + (Math::FloorToInt(chunkPos.y) * Terrain::MAP_CHUNKS_PER_MAP_STRIDE);
        }

        inline u32 GetCellIdFromCellPos(const vec2& cellPos)
        {
            return Math::FloorToInt(cellPos.x) + (Math::FloorToInt(cellPos.y) * Terrain::MAP_CELLS_PER_CHUNK_SIDE);
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

        inline bool GetTriangleFromWorldPosition(const vec3& position, Geometry::Triangle& triangle, f32& height)
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

            vec2 chunkWorldPos = glm::floor(chunkPos) * Terrain::MAP_CHUNK_SIZE;
            vec2 cellWorldPos = glm::floor(cellPos) * Terrain::MAP_CELL_SIZE;
            vec2 patchWorldPos = glm::floor(patchPos) * Terrain::MAP_PATCH_SIZE;

            // Below we subtract Terrain::MAP_HALF_SIZE to go from ADT Coordinate back to World Space
            // X, Y here maps to our Y, X
            f32 y = chunkWorldPos.x + cellWorldPos.x + patchWorldPos.x;
            f32 x = chunkWorldPos.y + cellWorldPos.y + patchWorldPos.y;

            // Calculate Vertex A
            {
                triangle.vert1.x = Terrain::MAP_HALF_SIZE - (x + a.y);
                triangle.vert1.y = Terrain::MAP_HALF_SIZE - (y + a.x);
                triangle.vert1.z = currentChunk.cells[cellId].heightData[vertexIds.x];
            }

            // Calculate Vertex B
            {
                triangle.vert2.x = Terrain::MAP_HALF_SIZE - (x + b.y);
                triangle.vert2.y = Terrain::MAP_HALF_SIZE - (y + b.x);
                triangle.vert2.z = currentChunk.cells[cellId].heightData[vertexIds.y];
            }

            // Calculate Vertex C
            {
                triangle.vert3.x = Terrain::MAP_HALF_SIZE - (x + c.y);
                triangle.vert3.y = Terrain::MAP_HALF_SIZE - (y + c.x);
                triangle.vert3.z = currentChunk.cells[cellId].heightData[vertexIds.z];
            }

            height = GetHeightFromVertexIds(vertexIds, &currentChunk.cells[cellId].heightData[0], a, b, c, patchRemainder * Terrain::MAP_PATCH_SIZE);
            return true;
        }
        inline std::vector<Geometry::Triangle> GetCellTrianglesFromWorldPosition(const vec3& position)
        {
            entt::registry* registry = ServiceLocator::GetGameRegistry();
            MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

            std::vector<Geometry::Triangle> triangles;
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

                        // X, Y here maps to our Y, X
                        vec2 chunkWorldPos = glm::floor(chunkPos) * Terrain::MAP_CHUNK_SIZE;
                        vec2 cellWorldPos = glm::floor(cellPos) * Terrain::MAP_CELL_SIZE;
                        vec2 patchWorldPos = glm::floor(vec2(x, y)) * Terrain::MAP_PATCH_SIZE;

                        // Below we subtract Terrain::MAP_HALF_SIZE to go from ADT Coordinate back to World Space
                        f32 y = chunkWorldPos.x + cellWorldPos.x + patchWorldPos.x;
                        f32 x = chunkWorldPos.y + cellWorldPos.y + patchWorldPos.y;

                        Geometry::Triangle& triangle = triangles.emplace_back();

                        // Calculate Vertex A
                        {
                            triangle.vert1.x = Terrain::MAP_HALF_SIZE - (x + a.y);
                            triangle.vert1.y = Terrain::MAP_HALF_SIZE - (y + a.x);
                            triangle.vert1.z = currentChunk.cells[cellId].heightData[vertexIds.x];
                        }

                        // Calculate Vertex B
                        {
                            triangle.vert2.x = Terrain::MAP_HALF_SIZE - (x + b.y);
                            triangle.vert2.y = Terrain::MAP_HALF_SIZE - (y + b.x);
                            triangle.vert2.z = currentChunk.cells[cellId].heightData[vertexIds.y];
                        }

                        // Calculate Vertex C
                        {
                            triangle.vert3.x = Terrain::MAP_HALF_SIZE - (x + c.y);
                            triangle.vert3.y = Terrain::MAP_HALF_SIZE - (y + c.x);
                            triangle.vert3.z = currentChunk.cells[cellId].heightData[vertexIds.z];
                        }
                    }
                }
            }

            return triangles;
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
        inline void ProjectTriangle(const Geometry::Triangle& triangle, const vec3& axis, vec2& minMax)
        {
            minMax.x = 100000.0f;
            minMax.y = -100000.0f;

            Project(triangle.vert1, axis, minMax);
            Project(triangle.vert2, axis, minMax);
            Project(triangle.vert3, axis, minMax);
        }
        inline void ProjectBox(const Geometry::AABoundingBox& box, const vec3& axis, vec2& minMax)
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

        inline bool Intersect_SPHERE_TRIANGLE(const vec3& spherePos, const f32 sphereRadius, const Geometry::Triangle& triangle)
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

#pragma warning( push )
#pragma warning( disable : 4723 )
        inline bool TestOverlap(const f32& boxExt, bool& validMTD, const f32& triMin, const f32& triMax, f32& d0, f32& d1)
        {
            d0 = -boxExt - triMax;
            d1 = boxExt - triMin;
            const bool intersects = (d0 <= 0.0f && d1 >= 0.0f);
            validMTD &= intersects;

            return intersects;
        }

        inline i32 TestAxis(const vec3& boxScale, const Geometry::Triangle& triangle, const vec3& dir, const vec3& axis, bool& validMTD, f32& tFirst, f32& tLast)
        {
            const f32 d0t = glm::dot(triangle.vert1, axis);
            const f32 d1t = glm::dot(triangle.vert2, axis);
            const f32 d2t = glm::dot(triangle.vert3, axis);

            f32 triMin = glm::min(d0t, d1t);
            f32 triMax = glm::max(d0t, d1t);

            triMin = glm::min(triMin, d2t);
            triMax = glm::max(triMax, d2t);

            const f32 boxExt = glm::abs(axis.x) * boxScale.x + glm::abs(axis.y) * boxScale.y + glm::abs(axis.z) * boxScale.z;

            f32 d0 = 0;
            f32 d1 = 0;
            bool intersected = TestOverlap(boxExt, validMTD, triMin, triMax, d0, d1);

            const f32 v = glm::dot(dir, axis);
            if (glm::abs(v) < 1.0E-6f)
                return intersected;

            const f32 oneOverV = -1.0f / v;
            const f32 t0_ = d0 * oneOverV;
            const f32 t1_ = d1 * oneOverV;

            f32 t0 = glm::min(t0_, t1_);
            f32 t1 = glm::max(t0_, t1_);

            if (t0 > tLast || t1 < tFirst)
                return false;

            tLast = glm::min(t1, tLast);
            tFirst = glm::max(t0, tFirst);

            return true;
        }
        inline i32 TestAxisXYZ(const i32 index, const vec3& boxScale, const Geometry::Triangle& triangle, const vec3& dir, const f32& oneOverDir, bool& validMTD, f32& tFirst, f32& tLast)
        {
            const f32 d0t = triangle.vert1[index];
            const f32 d1t = triangle.vert2[index];
            const f32 d2t = triangle.vert3[index];

            f32 triMin = glm::min(d0t, d1t);
            f32 triMax = glm::max(d0t, d1t);

            triMin = glm::min(triMin, d2t);
            triMax = glm::max(triMax, d2t);

            const f32 boxExt = boxScale[index];

            f32 d0 = 0;
            f32 d1 = 0;
            bool intersected = TestOverlap(boxExt, validMTD, triMin, triMax, d0, d1);

            const f32 v = dir[index];
            if (glm::abs(v) < 1.0E-6f)
                return intersected;

            const f32 oneOverV = -oneOverDir;
            const f32 t0_ = d0 * oneOverV;
            const f32 t1_ = d1 * oneOverV;

            f32 t0 = glm::min(t0_, t1_);
            f32 t1 = glm::max(t0_, t1_);

            if (t0 > tLast || t1 < tFirst)
                return false;

            tLast = glm::min(t1, tLast);
            tFirst = glm::max(t0, tFirst);

            return true;
        }
        inline bool TestSeperationAxes(const vec3& boxScale, const Geometry::Triangle& triangle, const vec3& normal, const vec3& dir, const vec3& oneOverDir, f32 maxDist, f32& outDistToCollision)
        {
            bool validMTD = true;

            f32 tFirst = -f32MaxValue;
            f32 tLast = f32MaxValue;

            // Test Triangle Normal
            if (!TestAxis(boxScale, triangle, dir, normal, validMTD, tFirst, tLast))
                return false;

            // Test Box Normals
            if (!TestAxisXYZ(0, boxScale, triangle, dir, oneOverDir.x, validMTD, tFirst, tLast))
                return false;

            if (!TestAxisXYZ(1, boxScale, triangle, dir, oneOverDir.y, validMTD, tFirst, tLast))
                return false;

            if (!TestAxisXYZ(2, boxScale, triangle, dir, oneOverDir.z, validMTD, tFirst, tLast))
                return false;

            for (u32 i = 0; i < 3; i++)
            {
                i32 j = i + 1;
                if (i == 2)
                    j = 0;

                const vec3& triangleEdge = triangle.GetVert(j) - triangle.GetVert(i);

                {
                    // Cross100
                    const vec3& sep = vec3(0.0f, -triangleEdge.z, triangleEdge.y);
                    if ((glm::dot(sep, sep) >= 1.0E-6f) && !TestAxis(boxScale, triangle, dir, sep, validMTD, tFirst, tLast))
                        return false;
                }

                {
                    // Cross010
                    const vec3& sep = vec3(triangleEdge.z, 0.0f, -triangleEdge.x);
                    if ((glm::dot(sep, sep) >= 1.0E-6f) && !TestAxis(boxScale, triangle, dir, sep, validMTD, tFirst, tLast))
                        return false;
                }

                {
                    // Cross001
                    const vec3& sep = vec3(-triangleEdge.y, triangleEdge.x, 0.0f);
                    if ((glm::dot(sep, sep) >= 1.0E-6f) && !TestAxis(boxScale, triangle, dir, sep, validMTD, tFirst, tLast))
                        return false;
                }
            }

            if (tFirst > maxDist || tLast < 0.0f)
                return false;

            if (tFirst <= 0.0f)
            {
                if (!validMTD)
                    return false;

                outDistToCollision = 0.0f;
            }
            else
            {
                outDistToCollision = tFirst;
            }

            return true;
        }

        // This function assumes the triangle is "translated" as such that its position is relative to the box's center meaning the origin(0,0) is the box's center
        // This function && (TestSeperationAxes, TestAxis and TestAxisXYZ) all come from https://github.com/NVIDIAGameWorks/PhysX/blob/4.1/physx/source/geomutils/src/sweep/GuSweepBoxTriangle_SAT.h
        inline bool Intersect_AABB_TRIANGLE_SWEEP(const vec3& boxScale, const Geometry::Triangle& triangle, const vec3& dir, f32 maxDist, f32& outDistToCollision, bool backFaceCulling)
        {
            const vec3& oneOverDir = 1.0f / dir;
            vec3 triangleNormal = triangle.GetNormal();

            if (backFaceCulling && glm::dot(triangleNormal, dir) >= 0.0f)
                return 0;

            return TestSeperationAxes(boxScale, triangle, triangleNormal, dir, oneOverDir, maxDist, outDistToCollision);
        }
#pragma warning(pop)
        inline bool Intersect_AABB_TRIANGLE(const Geometry::AABoundingBox& box, const Geometry::Triangle& triangle)
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
            vec3 triangleNormal = triangle.GetNormal();
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
        inline bool Intersect_AABB_TERRAIN(const vec3& position, const Geometry::AABoundingBox& box, Geometry::Triangle& triangle, f32& height)
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

            // TODO: Look into if we want to optimize this, the reason we currently
            //       always get the Verticies from pos is because we don't manually
            //       check for chunk/cell borders, but we could speed this part up
            //       by doing that manually instead of calling GetVerticiesFromWorldPosition
            //       5 times.
            for (i32 i = 0; i < 5; i++)
            {
                vec3 pos = position + offsets[i];

                if (GetTriangleFromWorldPosition(pos, triangle, height))
                {
                    if (Intersect_AABB_TRIANGLE(box, triangle))
                        return true;
                }
            }

            return false;
        }
        inline bool Intersect_AABB_TERRAIN_SWEEP(const Geometry::AABoundingBox& box, Geometry::Triangle& triangle, const vec3& direction, f32& height, f32 maxDist, vec3& outDistToCollision)
        {
            vec3 scale = (box.max - box.min) / 2.0f;
            vec3 center = box.max - scale;

            vec3 offsets[5] =
            {
                {0, 0, 0},
                {-scale.x, 0, -scale.z},
                {scale.x, 0, -scale.z},
                {-scale.x, 0, scale.z},
                {scale.x, 0, scale.z}
            };


            // TODO: Look into if we want to optimize this, the reason we currently
            //       always get the Verticies from pos is because we don't manually
            //       check for chunk/cell borders, but we could speed this part up
            //       by doing that manually instead of calling GetVerticiesFromWorldPosition
            //       5 times.

            f32 timeToCollision = f32MaxValue;

            for (i32 i = 0; i < 5; i++)
            {
                vec3 pos = center + offsets[i];
                Geometry::Triangle tri;

                if (GetTriangleFromWorldPosition(pos, tri, height))
                {
                    // First store tri in triangle and then translate tri's position so that center is origin(0,0)
                    triangle = tri;

                    tri.vert1 -= center;
                    tri.vert2 -= center;
                    tri.vert3 -= center;

                    // We need to find the "shortest" collision here and not just "any" collision (Not doing this causes issues when testing against multiple triangles
                    f32 tmpTimeToCollision = 0;
                    if (Intersect_AABB_TRIANGLE_SWEEP(scale, tri, direction, maxDist, tmpTimeToCollision, true))
                    {
                        if (tmpTimeToCollision < timeToCollision)
                            timeToCollision = tmpTimeToCollision;
                    }
                }
            }

            outDistToCollision = timeToCollision * direction;
            return timeToCollision != f32MaxValue;
        }
    }
}
