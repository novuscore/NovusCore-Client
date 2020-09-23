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

namespace Terrain
{
    constexpr i32 MAP_OBJECT_TOKEN = 7236975; // UTF8 -> Binary -> Decimal for "nmo"
    constexpr i32 MAP_OBJECT_VERSION = 3;

    struct TriangleData
    {
        struct
        {
            u8 Transparent : 1; // Pursche: I assume this is what this flag means from its usage
            u8 NoCamCollide : 1;
            u8 Detail : 1;
            u8 Collision : 1; // Turns off water ripple effects, and is used for ghost material triangles
            u8 Hint : 1;
            u8 Render : 1;
            u8 Unk_0x40 : 1;
            u8 CollideHit : 1;

            // 0xFF is used for triangles that can only collide, meaning they are not rendered.

            bool IsTransFace() { return Transparent && (Detail || Render); }
            bool IsColor() { return !Collision; }
            bool IsRenderFace() { return Render && !Detail; }
            bool IsCollidable() { return Collision || IsRenderFace(); }
        } flags;

        u8 materialID; // This is an index into MapObjectRoot::materials
    };

#pragma pack(push, 1)
    struct MapObjectHeader
    {
        u32 token = 0;
        u32 version = 0;
    };

    struct MapObjectFlags
    {
        union
        {
            u32 flags;
            struct
            {
                bool hasBspTree : 1;
                bool hasLightMap : 1;
                bool hasVertexColor : 1;
                bool exterior : 1;
                bool unknown0 : 1;
                bool unknown1 : 1;
                bool exteriorLit : 1;
                bool unreachable : 1;
                bool unknown2 : 1;
                bool hasLight : 1;
                bool unknown3 : 1;
                bool hasDoodads : 1;
                bool hasWater : 1;
                bool indoor : 1;
                bool unknown4 : 1;
                bool unknown5 : 1;
                bool alwaysDraw : 1;
                bool hasMoriMorb : 1;
                bool skybox : 1;
                bool ocean : 1;
                bool unknown6 : 1;
                bool mountAllowed : 1;
                bool unknown7 : 1;
                bool unknown8 : 1;
                bool useSecondVertexColor : 1;
                bool hasTwoUVs : 1;
                bool antiportal : 1;
                bool unknown9 : 1;
                bool unused : 4;
            };
        };
    };

    struct RenderBatch
    {
        u32 startIndex;
        u16 indexCount;
        u8 materialID;
    };

    struct VertexColorSet
    {
        std::vector<u32> vertexColors;
    };

    struct MapObjectVertex
    {
        hvec3 position = hvec3(static_cast<f16>(0.0f));
        u8 octNormal[2] = { 0, 0 };
        hvec4 uv = hvec4(static_cast<f16>(0.0f));
    }; // 16 bytes

    struct MapObject
    {
        MapObjectHeader header;
        MapObjectFlags flags;

        std::vector<u16> indices;

        std::vector<VertexColorSet> vertexColorSets;

        std::vector<TriangleData> triangleData;
        std::vector<RenderBatch> renderBatches;
    };
#pragma pack(pop)
}