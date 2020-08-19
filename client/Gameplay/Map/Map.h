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
#include <Containers/StringTable.h>
#include "Chunk.h"

// First of all, forget every naming convention wowdev.wiki uses, it's extremely confusing.
// A Map (e.g. Eastern Kingdoms) consists of 64x64 Chunks which may or may not be used.
// A Chunk consists of 16x16 Cells which are all being used.
// A Cell consists of two interlapping grids. There is the 9*9 OUTER grid and the 8*8 INNER grid.

namespace Terrain
{
    struct Map
    {
        Map() {}

        u16 id = std::numeric_limits<u16>().max(); // Default Map to Invalid ID
        std::string_view name;
        robin_hood::unordered_map<u16, Chunk> chunks;
        robin_hood::unordered_map<u16, StringTable> stringTables;

        /*f32 GetHeight(Vector2& pos);
        bool GetAdtIdFromWorldPosition(Vector2& pos, u16& adtId);*/
        void GetChunkPositionFromChunkId(u16 chunkId, u16& x, u16& y) const;
        bool GetChunkIdFromChunkPosition(u16 x, u16 y, u16& chunkId) const;

        void Clear()
        {
            id = std::numeric_limits<u16>().max();

            chunks.clear();

            for (auto& itr : stringTables)
            {
                itr.second.Clear();
            }
            stringTables.clear();
        }
    };
}