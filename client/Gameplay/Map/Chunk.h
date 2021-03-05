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

class FileReader;
namespace Terrain
{
    constexpr i32 MAP_CHUNK_TOKEN = 1128812107; // UTF8 -> Binary -> Decimal for "chnk"
    constexpr i32 MAP_CHUNK_VERSION = 7;
    constexpr u16 MAP_CHUNK_ID_INVALID = std::numeric_limits<u16>().max();

    constexpr u32 MAP_CHUNKS_PER_MAP_STRIDE = 64;
    constexpr u32 MAP_CHUNKS_PER_MAP = MAP_CHUNKS_PER_MAP_STRIDE * MAP_CHUNKS_PER_MAP_STRIDE;

    constexpr f32 MAP_CHUNK_SIZE = 533.33333f; // yards
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

    struct Placement
    {
        u32 uniqueID = 0;
        u32 nameID = 0;
        vec3 position = vec3(0, 0, 0);
        vec3 rotation = vec3(0, 0, 0);
        u16 scale = 1024;
    };

    struct CellLiquidHeader
    {
        u32 instancesOffset = 0;
        u32 layerCount = 0;
        u32 attributesOffset = 0;

        /*// Packed Format
        // Bit 1-7 (numInstances)
        // Bit 8 (hasAttributes)
        u8 packedData = 0;

        u8 cellID = 0;*/
    };

    /*
        struct LiquidInstance
        {
            u16 liquidType = 0; // Foreign Key (Referencing LiquidType.dbc)
            u16 liquidVertexFormat = 0; // Classic, TBC and WOTLK Only (Cata+ Foreign Key)

            f32 minHeightLevel = 0;
            f32 maxHeightLevel = 0;

            u8 xOffset = 0;
            u8 yOffset = 0;
            u8 width = 0;
            u8 height = 0;

            u32 bitmapExistsOffset = 0; // Will contain bytes equals to height (For every "Row" we have cells on, we need at least 8 bits to display which is 1 byte)
            u32 vertexDataOffset = 0; // Vertex Data, can be in 2 formats for WOTLK and will always contain vertexCount entries for both arrays (f32* heightMap, char* depthMap), (f32* heightMap, UVEntry* uvMap)
        };
    */

    struct CellLiquidInstance
    {
        u16 liquidType = 0; // Foreign Key (Referencing LiquidType.dbc)
        u16 liquidVertexFormat = 0; // Classic, TBC and WOTLK Only (Cata+ Foreign Key)

        f32 minHeightLevel = 0;
        f32 maxHeightLevel = 0;

        u8 xOffset = 0;
        u8 yOffset = 0;
        u8 width = 0;
        u8 height = 0;

        u32 bitmapExistsOffset = 0; // Will contain bytes equals to height (For every "Row" we have cells on, we need at least 8 bits to display which is 1 byte)
        u32 vertexDataOffset = 0; // Vertex Data, can be in 2 formats for WOTLK and will always contain vertexCount entries for both arrays (f32* heightMap, char* depthMap), (f32* heightMap, UVEntry* uvMap)
    };

    struct CellLiquidAttributes
    {
        u64 fishable = 0; // seems to be usable as visibility information.
        u64 deep = 0; // Might be related to fatigue area if bit set.

        // Note that these are bitmasks.
    };

    // The following 4 Structs only exists for the purpose of being able to sizeof() inside for Mh2o::Read
    // This makes it easier to read the code (The actual) structs in memory are arrays one after another
    struct LiquidVertexFormat_Height_Depth
    {
        f32 heightMap;
        u8 depthMap;
    };

    struct LiquidVertexFormat_Height_UV
    {
        f32 heightMap;

        u16 uvX;
        u16 uvY;
    };

    struct LiquidVertexFormat_Depth
    {
        u8 depthMap;
    };

    struct LiquidVertexFormat_Height_UV_Depth
    {
        f32 heightMap;

        u16 uvX;
        u16 uvY;

        u8 depthMap;
    };

    struct LiquidUVMapEntry
    {
        u16 x;
        u16 y;
    };

    struct Chunk
    {
        ChunkHeader chunkHeader;

        HeightHeader heightHeader;
        HeightBox heightBox;

        Cell cells[MAP_CELLS_PER_CHUNK];
        u32 alphaMapStringID;

        std::vector<Placement> mapObjectPlacements;
        std::vector<Placement> complexModelPlacements;

        std::vector<u8> liquidBytes;

        std::vector<CellLiquidHeader> liquidHeaders;
        std::vector<CellLiquidInstance> liquidInstances;
        std::vector<CellLiquidAttributes> liquidAttributes;
        std::vector<u8> liquidBitMaskForPatchesData;
        std::vector<u8> liquidvertexData;

        static bool Read(FileReader& reader, Terrain::Chunk& chunk, StringTable& stringTable);
    };
#pragma pack(pop)
}