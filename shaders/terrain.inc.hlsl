#define NUM_CHUNKS_PER_MAP_SIDE (64)
#define NUM_CELLS_PER_CHUNK_SIDE (16)
#define NUM_CELLS_PER_CHUNK (NUM_CELLS_PER_CHUNK_SIDE * NUM_CELLS_PER_CHUNK_SIDE)

#define NUM_VERTICES_PER_CELL (145)

#define CHUNK_SIDE_SIZE (533.3333f)
#define CELL_SIDE_SIZE (33.3333f)

uint GetGlobalCellID(uint chunkID, uint cellID)
{
    return (chunkID * NUM_CELLS_PER_CHUNK) + cellID;
}

float2 GetCellPosition(uint chunkID, uint cellID)
{
    const uint chunkX = chunkID % NUM_CHUNKS_PER_MAP_SIDE;
    const uint chunkY = chunkID / NUM_CHUNKS_PER_MAP_SIDE;

    const float2 chunkPos = float2(chunkX, chunkY) * CHUNK_SIDE_SIZE;

    const uint cellX = cellID % NUM_CELLS_PER_CHUNK_SIDE;
    const uint cellY = cellID / NUM_CELLS_PER_CHUNK_SIDE;

    const float2 cellPos = float2(cellX, cellY) * CELL_SIDE_SIZE;

    return chunkPos + cellPos;
}

float2 GetCellVertexPosition(uint vertexID)
{
    const float CELL_PRECISION = CELL_SIDE_SIZE / 8.0f;

    float vertexX = vertexID % 17.0f;
    float vertexY = floor(vertexID / 17.0f);

    bool isOddRow = vertexX > 8.01f;
    vertexX = vertexX - (8.5f * isOddRow);
    vertexY = vertexY + (0.5f * isOddRow);

    float vertexPosX = -vertexX * CELL_PRECISION;
    float vertexPosZ = vertexY * CELL_PRECISION;

    return float2(vertexPosX, vertexPosZ);
}