#define NUM_CHUNKS_PER_MAP_SIDE (64)
#define NUM_CELLS_PER_CHUNK_SIDE (16)
#define NUM_CELLS_PER_CHUNK (NUM_CELLS_PER_CHUNK_SIDE * NUM_CELLS_PER_CHUNK_SIDE)

#define NUM_INDICES_PER_CELL (768)
#define NUM_VERTICES_PER_CELL (145)

#define CHUNK_SIDE_SIZE (533.3333f)
#define CELL_SIDE_SIZE (33.3333f)

#define HALF_WORLD_SIZE (17066.66656f)

struct AABB
{
    float3 min;
    float3 max;
};

uint GetGlobalCellID(uint chunkID, uint cellID)
{
    return (chunkID * NUM_CELLS_PER_CHUNK) + cellID;
}

float2 GetCellPosition(uint chunkID, uint cellID)
{
    const uint chunkX = chunkID % NUM_CHUNKS_PER_MAP_SIDE;
    const uint chunkY = chunkID / NUM_CHUNKS_PER_MAP_SIDE;

    const float2 chunkPos = (float2(chunkX, chunkY) * CHUNK_SIDE_SIZE) - HALF_WORLD_SIZE;

    const uint cellX = cellID % NUM_CELLS_PER_CHUNK_SIDE;
    const uint cellY = cellID / NUM_CELLS_PER_CHUNK_SIDE;

    const float2 cellPos = float2(cellX, cellY) * CELL_SIDE_SIZE;

    return -(chunkPos + cellPos);
}

AABB GetCellAABB(uint chunkID, uint cellID, float2 heightRange)
{
    const uint chunkPosX = chunkID % NUM_CHUNKS_PER_MAP_SIDE;
    const uint chunkPosY = chunkID / NUM_CHUNKS_PER_MAP_SIDE;

    float2 chunkOrigin;
    chunkOrigin.x = -((chunkPosY) * CHUNK_SIDE_SIZE - HALF_WORLD_SIZE);
    chunkOrigin.y = ((NUM_CHUNKS_PER_MAP_SIDE - chunkPosX) * CHUNK_SIDE_SIZE - HALF_WORLD_SIZE);

    const uint cellX = cellID % NUM_CELLS_PER_CHUNK_SIDE;
    const uint cellY = cellID / NUM_CELLS_PER_CHUNK_SIDE;

    float3 aabb_min;
    float3 aabb_max;

    aabb_min.x = chunkOrigin.x - (cellY * CELL_SIDE_SIZE);
    aabb_min.y = heightRange.x;
    aabb_min.z = chunkOrigin.y - (cellX * CELL_SIDE_SIZE);

    aabb_max.x = chunkOrigin.x - ((cellY + 1) * CELL_SIDE_SIZE);
    aabb_max.y = heightRange.y;
    aabb_max.z = chunkOrigin.y - ((cellX + 1) * CELL_SIDE_SIZE);

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

    return float2(vertexX, vertexY);
}