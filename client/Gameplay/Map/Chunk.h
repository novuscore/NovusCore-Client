/*
    MIT License

    Copyright (c) 2018-2019 NovusCore

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#pragma once
#include <NovusTypes.h>
#include <robin_hood.h>
#include <limits>

#include "Cell.h"
#include <Containers/StringTable.h>

// First of all, forget every naming convention wowdev.wiki use, it's extremely confusing.
// A Map (e.g. Eastern Kingdoms) consists of 64x64 Chunks which may or may not be used.
// A Chunk consists of 16x16 Cells which are all being used.
// A Cell consists of two interlapping grids. There is the 9*9 OUTER grid and the 8*8 INNER grid.

namespace Terrain
{
    constexpr i32 MAP_CHUNK_TOKEN = 1313685840;
    constexpr i32 MAP_CHUNK_VERSION = 2;
    constexpr u16 MAP_CHUNK_ID_INVALID = std::numeric_limits<u16>().max();

    constexpr u32 MAP_CHUNKS_PER_MAP_STRIDE = 64;
    constexpr u32 MAP_CHUNKS_PER_MAP = MAP_CHUNKS_PER_MAP_STRIDE * MAP_CHUNKS_PER_MAP_STRIDE;

    constexpr f32 MAP_CHUNK_SIZE = 533.3333f; // yards
    constexpr f32 MAP_CHUNK_HALF_SIZE = MAP_CHUNK_SIZE / 2.0f; // yards

#pragma pack(push, 1)
    struct ChunkHeader
    {
        u32 token = 0;
        u32 version = 0;
    };

    struct HeightHeader
    {
        u8 hasHeightBox = 0;
        f32 gridMinHeight = 0;
        f32 gridMaxHeight = 0;
    };

    struct HeightPlane
    {
        // For future implementation: https://www.ownedcore.com/forums/world-of-warcraft/world-of-warcraft-bots-programs/wow-memory-editing/351404-traceline-intersection-collision-detection-height-limit.html
        i16 heightPoints[3 * 3] = { 0 };
    };

    struct HeightBox
    {
        HeightPlane minHeight;
        HeightPlane maxHeight;
    };

    struct MapObjectPlacement
    {
        u32 nameID;
        vec3 position;
        vec3 rotation;
        u16 scale;
    };

    struct Chunk
    {
        ChunkHeader chunkHeader;

        HeightHeader heightHeader;
        HeightBox heightBox;

        Cell cells[MAP_CELLS_PER_CHUNK];
        u32 alphaMapStringID;

        std::vector<MapObjectPlacement> mapObjectPlacements;
    };
#pragma pack(pop)
}