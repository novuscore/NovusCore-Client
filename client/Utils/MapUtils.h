#pragma once
#include <NovusTypes.h>
#include <entt.hpp>
#include "../Gameplay/Map/Chunk.h"
#include "../ECS/Components/Singletons/MapSingleton.h"

namespace Terrain
{
    namespace MapUtils
    {
        inline f32 GetHeightFromWorldPosition(vec3& position)
        {
            entt::registry* registry = ServiceLocator::GetGameRegistry();
            MapSingleton& mapSingleton = registry->ctx<MapSingleton>();

            // This is translated to remap positions [-17066 .. 17066] to [0 ..  34132]
            // This is because we want the Chunk Pos to be between [0 .. 64] and not [-32 .. 32]
            
            // We flip X & Z here because X in our coordinate system is north/south and Z is west/east
            // The ADT index is stored as such that the "X" is west/east and "Y" is north/south
            vec2 translatedPos = vec2(Terrain::MAP_HALF_SIZE - position.z, Terrain::MAP_HALF_SIZE - position.x);

            // Convert from World Space to Chunk Space
            vec2 chunkPos = translatedPos / Terrain::MAP_CHUNK_SIZE;
            vec2 chunkRemainder = vec2(fmod(translatedPos.x, Terrain::MAP_CHUNK_SIZE), fmod(translatedPos.y, Terrain::MAP_CHUNK_SIZE));
            u32 chunkId = Math::FloorToInt(chunkPos.x) + (Math::FloorToInt(chunkPos.y) * Terrain::MAP_CHUNKS_PER_MAP_SIDE);
            //NC_LOG_MESSAGE("Chunk Space: ID(%u), Pos(%f, %f), Remainder(%f, %f)", chunkId, chunkPos.x, chunkPos.y, chunkRemainder.x, chunkRemainder.y);

            Terrain::Map& currentMap = mapSingleton.currentMap;
            auto chunkItr = currentMap.chunks.find(chunkId);
            if (chunkItr == currentMap.chunks.end())
                return 0.f;

            Terrain::Chunk& currentChunk = chunkItr->second;
            
            // Convert from Chunk Space to Cell Space
            vec2 cellPos = chunkRemainder / Terrain::CELL_SIZE;
            vec2 cellRemainder = vec2(fmod(chunkRemainder.y, Terrain::CELL_SIZE), fmod(chunkRemainder.x, Terrain::CELL_SIZE));
            u32 cellID = Math::FloorToInt(cellPos.x) + (Math::FloorToInt(cellPos.y) * Terrain::MAP_CELLS_PER_CHUNK_SIDE);
            //NC_LOG_MESSAGE("Cell Space: ID(%u), Pos(%f, %f), Remainder(%f, %f)", cellID, cellPos.x, cellPos.y, cellRemainder.x, cellRemainder.y);
            
            // Convert from Cell Space to Patch Space
            vec2 patchPos = cellRemainder / Terrain::PATCH_SIZE;
            vec2 patchRemainder = vec2(fmod(cellRemainder.x, Terrain::PATCH_SIZE), fmod(cellRemainder.y, Terrain::PATCH_SIZE));
            //NC_LOG_MESSAGE("Patch Space: Pos(%f, %f), Remainder(%f, %f)", patchPos.x, patchPos.y, patchRemainder.x, patchRemainder.y);

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

            // Using CellPos we need to build a square looking something like this depending on what cell we're on
            // TL     TR
            //     C
            // BL     BR
            // TL = TopLeft, TR = TopRight, C = Center, BL = BottomLeft, BR = BottomRight

            // Lets start by finding the Top Left "Vertex"
            u16 topLeftVertex = (Math::FloorToInt(patchPos.x) * Terrain::CELL_TOTAL_GRID_SIDE) + Math::FloorToInt(patchPos.y);

            // Top Right is always +1 from Top Left
            u16 topRightVertex = topLeftVertex + 1;

            // Bottom Left is a full rowStride from the Top Left vertex
            u16 bottomLeftVertex = topLeftVertex + Terrain::CELL_TOTAL_GRID_SIDE;

            // Bottom Right is always +1 from Bottom Left
            u16 bottomRightVertex = bottomLeftVertex + 1;

            // Center is always + cellStride + 1 from Top Left
            u16 centerVertex = topLeftVertex + Terrain::CELL_INNER_GRID_SIDE + 1;

            // The next step is to use the cellRemainder to figure out which of these triangles we are on: https://imgur.com/i9aHwus
            // When we know we set a, b, c, aHeight, bHeight and cHeight accordingly

            // NOTE: Order of A, B and C is important, don't swap them around without understanding how it works
            vec2 a = vec2(Terrain::PATCH_HALF_SIZE, Terrain::PATCH_HALF_SIZE);
            f32 aHeight = currentChunk.cells[cellID].heightData[centerVertex];
            vec2 b = vec2(0, 0);
            f32 bHeight = 0.0f;
            vec2 c = vec2(0, 0);
            f32 cHeight = 0.0f;
            vec2 p = patchRemainder;

            if (Math::Abs(patchRemainder.x - Terrain::PATCH_HALF_SIZE) > Math::Abs(patchRemainder.y - Terrain::PATCH_HALF_SIZE))
            {
                if (patchRemainder.y > Terrain::PATCH_HALF_SIZE)
                {
                    // East triangle consists of Center, TopRight and BottomRight
                    b = vec2(0.0f, Terrain::PATCH_SIZE);
                    bHeight = currentChunk.cells[cellID].heightData[topRightVertex];

                    c = vec2(Terrain::PATCH_SIZE, Terrain::PATCH_SIZE);
                    cHeight = currentChunk.cells[cellID].heightData[bottomRightVertex];
                }
                else
                {
                    // West triangle consists of Center, BottomLeft and TopLeft
                    b = vec2(Terrain::PATCH_SIZE, 0.0f);
                    bHeight = currentChunk.cells[cellID].heightData[bottomLeftVertex];

                    c = vec2(0, 0);
                    cHeight = currentChunk.cells[cellID].heightData[topLeftVertex];
                }
            }
            else
            {
                if (patchRemainder.x < Terrain::PATCH_HALF_SIZE)
                {
                    // North triangle consists of Center, TopLeft and TopRight
                    b = vec2(0, 0);
                    bHeight = currentChunk.cells[cellID].heightData[topLeftVertex];

                    c = vec2(0.0f, Terrain::PATCH_SIZE);
                    cHeight = currentChunk.cells[cellID].heightData[topRightVertex];
                }
                else
                {
                    // South triangle consists of Center, BottomRight and BottomLeft
                    b = vec2(Terrain::PATCH_SIZE, Terrain::PATCH_SIZE);
                    bHeight = currentChunk.cells[cellID].heightData[bottomRightVertex];

                    c = vec2(Terrain::PATCH_SIZE, 0.0f);
                    cHeight = currentChunk.cells[cellID].heightData[bottomLeftVertex];
                }
            }

            // Finally we do standard barycentric triangle interpolation to get the actual height of the position
            f32 det = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
            f32 factorA = (b.y - c.y) * (p.x - c.x) + (c.x - b.x) * (p.y - c.y);
            f32 factorB = (c.y - a.y) * (p.x - c.x) + (a.x - c.x) * (p.y - c.y);
            f32 alpha = factorA / det;
            f32 beta = factorB / det;
            f32 gamma = 1.0f - alpha - beta;

            f32 height = aHeight * alpha + bHeight * beta + cHeight * gamma;

            return height;
        }
    }
}
