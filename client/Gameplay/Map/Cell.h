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

// First of all, forget every naming convention wowdev.wiki use, it's extremely confusing.
// A Map (e.g. Eastern Kingdoms) consists of 64x64 Chunks which may or may not be used.
// A Chunk consists of 16x16 Cells which are all being used.
// A Cell consists of various data.

namespace Terrain
{
    constexpr u16 CELL_OUTER_GRID_SIDE = 9;
    constexpr u16 CELL_OUTER_GRID_SIZE = CELL_OUTER_GRID_SIDE * CELL_OUTER_GRID_SIDE;

    constexpr u16 CELL_INNER_GRID_SIDE = 8;
    constexpr u16 CELL_INNER_GRID_SIZE = CELL_INNER_GRID_SIDE * CELL_INNER_GRID_SIDE;

    constexpr u16 CELL_TOTAL_GRID_SIDE = CELL_OUTER_GRID_SIDE + CELL_INNER_GRID_SIDE;
    constexpr u16 CELL_TOTAL_GRID_SIZE = CELL_OUTER_GRID_SIZE + CELL_INNER_GRID_SIZE;

    constexpr f32 CELL_SIZE = 33.3333f; // yards

#pragma pack(push, 1)
    struct LiquidData
    {
        u8 hasMultipleLiquidTypes = 0;
        u8 offsetX = 0;
        u8 offsetY = 0;
        u8 width = 0;
        u8 height = 0;
        u8 liquidFlags = 0;
        u16 liquidEntry = 0;
        f32 level = 0;
        u32 layers;
        f32 liquidHeight = 0;
    };

    struct Cell
    {
        u16 areaId = 0;

        f32 heightData[CELL_TOTAL_GRID_SIZE] = { 0 };

        LiquidData liquidData;

        u16 hole = 0;
    };
#pragma pack(pop)
}