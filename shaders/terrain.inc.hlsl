#define NUM_CHUNKS_PER_MAP_SIDE (64)
#define NUM_CELLS_PER_CHUNK_SIDE (16)
#define NUM_CELLS_PER_CHUNK (NUM_CELLS_PER_CHUNK_SIDE * NUM_CELLS_PER_CHUNK_SIDE)

#define NUM_INDICES_PER_CELL (768)
#define NUM_VERTICES_PER_CELL (145)

#define CHUNK_SIDE_SIZE (533.33333f)
#define CELL_SIDE_SIZE (33.33333f)
#define PATCH_SIDE_SIZE (CELL_SIDE_SIZE / 8.0f)

#define MAP_SIZE (CHUNK_SIDE_SIZE * NUM_CHUNKS_PER_MAP_SIDE)
#define MAP_HALF_SIZE (MAP_SIZE / 2.0f)

struct PackedCellData
{
    uint packedDiffuseIDs1;
    uint packedDiffuseIDs2;
    uint packedHoles;
};

struct CellData
{
    uint4 diffuseIDs;
    uint holes;
};

struct ChunkData
{
    uint alphaID;
};

struct CellInstance
{
    uint packedChunkCellID;
    uint instanceID;
};

struct AABB
{
    float3 min;
    float3 max;
};

uint GetGlobalCellID(uint chunkID, uint cellID)
{
    return (chunkID * NUM_CELLS_PER_CHUNK) + cellID;
}

float2 GetChunkPosition(uint chunkID)
{
    const uint chunkX = chunkID % NUM_CHUNKS_PER_MAP_SIDE;
    const uint chunkY = chunkID / NUM_CHUNKS_PER_MAP_SIDE;

    const float2 chunkPos = MAP_HALF_SIZE - (float2(chunkX, chunkY) * CHUNK_SIDE_SIZE);
    return -chunkPos;
}

float2 GetCellPosition(uint chunkID, uint cellID)
{
    const uint cellX = cellID % NUM_CELLS_PER_CHUNK_SIDE;
    const uint cellY = cellID / NUM_CELLS_PER_CHUNK_SIDE;

    const float2 chunkPos = GetChunkPosition(chunkID);
    const float2 cellPos = float2(cellX, cellY) * CELL_SIDE_SIZE;

    float2 pos = chunkPos + cellPos;
    return float2(-pos.y, -pos.x);
}

float2 GetGlobalVertexPosition(uint chunkID, uint cellID, uint vertexID)
{
    const int chunkX = chunkID % NUM_CHUNKS_PER_MAP_SIDE * NUM_CELLS_PER_CHUNK_SIDE;
    const int chunkY = chunkID / NUM_CHUNKS_PER_MAP_SIDE * NUM_CELLS_PER_CHUNK_SIDE;

    const int cellX = ((cellID % NUM_CELLS_PER_CHUNK_SIDE) + chunkX);
    const int cellY = ((cellID / NUM_CELLS_PER_CHUNK_SIDE) + chunkY);

    const int vX = vertexID % 17;
    const int vY = vertexID / 17;

    bool isOddRow = vX > 8.01;

    float2 vertexOffset;
    vertexOffset.x = -(8.5 * isOddRow);
    vertexOffset.y = (0.5 * isOddRow);

    int2 globalVertex = int2(vX + cellX * 8, vY + cellY * 8);

    float2 finalPos = -MAP_HALF_SIZE + (float2(globalVertex)+vertexOffset) * PATCH_SIDE_SIZE;

    return float2(-finalPos.y, -finalPos.x);
}


AABB GetCellAABB(uint chunkID, uint cellID, float2 heightRange)
{
    float2 pos = GetCellPosition(chunkID, cellID);
    float3 aabb_min = float3(pos.x, pos.y, heightRange.x);
    float3 aabb_max = float3(pos.x - CELL_SIDE_SIZE, pos.y - CELL_SIDE_SIZE, heightRange.y);

    AABB boundingBox;
    boundingBox.min = max(aabb_min, aabb_max);
    boundingBox.max = min(aabb_min, aabb_max);

    return boundingBox;
}

float2 GetCellSpaceVertexPosition(uint vertexID)
{
    float vertexX = vertexID % 17.0f;
    float vertexY = floor(vertexID / 17.0f);

    bool isOddRow = vertexX > 8.01f;
    vertexX = vertexX - (8.5f * isOddRow);
    vertexY = vertexY + (0.5f * isOddRow);

    // We go from a 2D coordinate system where x is Positive East & y is Positive South
    // we translate this into 3D where x is Positive North & y is Positive West
    return float2(-vertexY, -vertexX);
}

bool IsHoleVertex(uint vertexId, uint hole)
{
    if (hole == 0)
    {
        return false;
    }

    const uint blockRow = vertexId / 34;
    const uint blockVertexId = vertexId % 34;

    const uint shiftedHole = hole >> (blockRow * 4);

    if (shiftedHole & 0b0001)
    {
        if (blockVertexId == 9 || blockVertexId == 10 || blockVertexId == 26 || blockVertexId == 27)
        {
            return true;
        }
    }

    if (shiftedHole & 0b0010)
    {
        if (blockVertexId == 11 || blockVertexId == 12 || blockVertexId == 28 || blockVertexId == 29)
        {
            return true;
        }
    }

    if (shiftedHole & 0b0100)
    {
        if (blockVertexId == 13 || blockVertexId == 14 || blockVertexId == 30 || blockVertexId == 31)
        {
            return true;
        }
    }

    if (shiftedHole & 0b1000)
    {
        if (blockVertexId == 15 || blockVertexId == 16 || blockVertexId == 32 || blockVertexId == 33)
        {
            return true;
        }
    }

    return false;
}

