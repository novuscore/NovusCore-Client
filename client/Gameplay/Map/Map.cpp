#include "Map.h"

#include <Utils/ByteBuffer.h>
#include <Utils/FileReader.h>

namespace Terrain
{
    void Map::GetChunkPositionFromChunkId(u16 chunkId, u16& x, u16& y) const
    {
        x = chunkId % MAP_CHUNKS_PER_MAP_STRIDE;
        y = chunkId / MAP_CHUNKS_PER_MAP_STRIDE;
    }

    bool Map::GetChunkIdFromChunkPosition(u16 x, u16 y, u16& chunkId) const
    {
        chunkId = Math::FloorToInt(x) + (Math::FloorToInt(y) * MAP_CHUNKS_PER_MAP_STRIDE);

        return chunks.find(chunkId) != chunks.end();
    }

    bool MapHeader::Read(FileReader& reader, Terrain::MapHeader& header)
    {
        Bytebuffer buffer(nullptr, reader.Length());
        reader.Read(&buffer, buffer.size);

        if (!buffer.GetU32(header.token))
            return false;

        if (!buffer.GetU32(header.version))
            return false;

        if (!buffer.Get(header.flags))
            return false;

        if (header.flags.UseMapObjectInsteadOfTerrain)
        {
            buffer.GetString(header.mapObjectName);

            if (header.mapObjectName.length() == 0)
                return false;

            if (!buffer.Get(header.mapObjectPlacement))
                return false;
        }

        return true;
    }
}
