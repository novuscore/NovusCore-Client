#include "Map.h"

namespace Terrain
{
    void Map::GetChunkPositionFromChunkId(u16 chunkId, u16& x, u16& y) const
    {
        x = chunkId % MAP_CHUNKS_PER_MAP_SIDE;
        y = chunkId / MAP_CHUNKS_PER_MAP_SIDE;
    }

    bool Map::GetChunkIdFromChunkPosition(u16 x, u16 y, u16& chunkId) const
    {
        chunkId = Math::FloorToInt(x) + (Math::FloorToInt(y) * MAP_CHUNKS_PER_MAP_SIDE);

        return chunks.find(chunkId) != chunks.end();
    }

}
