/*
    MIT License

    Copyright (c) 2018-2020 NovusCore

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

// First of all, forget every naming convention wowdev.wiki use, it's extremely confusing.
// A Map (e.g. Eastern Kingdoms) consists of 64x64 Chunks which may or may not be used.
// A Chunk consists of 16x16 Cells which are all being used.
// A Cell consists of various data.

namespace Terrain
{
    constexpr u16 MAP_CELLS_PER_CHUNK_SIDE = 16;
    constexpr u16 MAP_CELLS_PER_CHUNK = MAP_CELLS_PER_CHUNK_SIDE * MAP_CELLS_PER_CHUNK_SIDE;

    constexpr u16 MAP_CELL_OUTER_GRID_STRIDE = 9;
    constexpr u16 MAP_CELL_OUTER_GRID_SIZE = MAP_CELL_OUTER_GRID_STRIDE * MAP_CELL_OUTER_GRID_STRIDE;

    constexpr u16 MAP_CELL_INNER_GRID_STRIDE = 8;
    constexpr u16 MAP_CELL_INNER_GRID_SIZE = MAP_CELL_INNER_GRID_STRIDE * MAP_CELL_INNER_GRID_STRIDE;

    constexpr u16 MAP_CELL_TOTAL_GRID_STRIDE = MAP_CELL_OUTER_GRID_STRIDE + MAP_CELL_INNER_GRID_STRIDE;
    constexpr u16 MAP_CELL_TOTAL_GRID_SIZE = MAP_CELL_OUTER_GRID_SIZE + MAP_CELL_INNER_GRID_SIZE;

    constexpr f32 MAP_CELL_SIZE = 33.33333f; // yards
    constexpr f32 MAP_CELL_HALF_SIZE = MAP_CELL_SIZE / 2.0f; // yards
    constexpr f32 MAP_PATCH_SIZE = MAP_CELL_SIZE / 8.0f; // yards
    constexpr f32 MAP_PATCH_HALF_SIZE = MAP_PATCH_SIZE / 2.0f; // yards

#pragma pack(push, 1)
    struct LayerData
    {
        static const u32 TextureIdInvalid = std::numeric_limits<u32>().max();

        u32 textureId = TextureIdInvalid;
    };

    struct Cell
    {
        u16 areaId = 0;

        f32 heightData[MAP_CELL_TOTAL_GRID_SIZE] = { 0 };
        u8 normalData[MAP_CELL_TOTAL_GRID_SIZE][3] = { {127}, {255}, {127} }; // This is ugly but lets us pack this data into 25% of the original size
        u8 colorData[MAP_CELL_TOTAL_GRID_SIZE][3] = { {127}, {127}, {127} }; // This is ugly but lets us pack this data into 25% of the original size

        u16 hole = 0;

        LayerData layers[4];
    };
#pragma pack(pop)
}