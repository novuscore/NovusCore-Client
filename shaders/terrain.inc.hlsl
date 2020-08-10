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
    const float2 cellPos = GetCellPosition(chunkID, cellID);

    const float3x3 rotationMatrix = float3x3(
        0, 0, 1,
        0, 1, 0,
        -1, 0, 0
    );

    const float3 rotatedCellPos = mul(rotationMatrix, float3(cellPos.x, 0.0f, cellPos.y));

    AABB aabb;
    aabb.min = float3(rotatedCellPos.x, -heightRange.y, rotatedCellPos.z) + float3(0.0f, 0.0f, 0.0f);
    aabb.max = float3(rotatedCellPos.x, -heightRange.x, rotatedCellPos.z) + float3(CELL_SIDE_SIZE, 0.0f, -CELL_SIDE_SIZE);

    aabb.min.y = +100000.0f;
    aabb.max.y = -100000.0f;

    return aabb;
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