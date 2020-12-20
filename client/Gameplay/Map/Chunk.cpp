#include "Chunk.h"

#include <Utils/ByteBuffer.h>
#include <Utils/FileReader.h>

bool Terrain::Chunk::Read(FileReader& reader, Terrain::Chunk& chunk, StringTable& stringTable)
{
    Bytebuffer buffer(nullptr, reader.Length());
    reader.Read(&buffer, buffer.size);

    buffer.Get<Terrain::ChunkHeader>(chunk.chunkHeader);

    if (chunk.chunkHeader.token != Terrain::MAP_CHUNK_TOKEN)
    {
        NC_LOG_FATAL("Tried to load a map chunk file with the wrong token");
    }

    if (chunk.chunkHeader.version != Terrain::MAP_CHUNK_VERSION)
    {
        if (chunk.chunkHeader.version < Terrain::MAP_CHUNK_VERSION)
        {
            NC_LOG_FATAL("Loaded map chunk with too old version %u instead of expected version of %u, rerun dataextractor", chunk.chunkHeader.version, Terrain::MAP_CHUNK_VERSION);
        }
        else
        {
            NC_LOG_FATAL("Loaded map chunk with too new version %u instead of expected version of %u, update your client", chunk.chunkHeader.version, Terrain::MAP_CHUNK_VERSION);
        }
    }

    buffer.Get<Terrain::HeightHeader>(chunk.heightHeader);
    buffer.Get<Terrain::HeightBox>(chunk.heightBox);

    buffer.GetBytes(reinterpret_cast<u8*>(&chunk.cells[0]), sizeof(Terrain::Cell) * Terrain::MAP_CELLS_PER_CHUNK);

    buffer.Get<u32>(chunk.alphaMapStringID);

    u32 numMapObjectPlacements;
    buffer.Get<u32>(numMapObjectPlacements);

    if (numMapObjectPlacements > 0)
    {
        chunk.mapObjectPlacements.resize(numMapObjectPlacements);
        buffer.GetBytes(reinterpret_cast<u8*>(&chunk.mapObjectPlacements[0]), sizeof(Terrain::Placement) * numMapObjectPlacements);
    }

    u32 numComplexModelPlacements;
    buffer.Get<u32>(numComplexModelPlacements);

    if (numComplexModelPlacements > 0)
    {
        chunk.complexModelPlacements.resize(numComplexModelPlacements);
        buffer.GetBytes(reinterpret_cast<u8*>(&chunk.complexModelPlacements[0]), sizeof(Terrain::Placement) * numComplexModelPlacements);
    }

    // Read Liquid Bytes
    {
        u32 numLiquidBytes;
        buffer.Get<u32>(numLiquidBytes);

        if (numLiquidBytes > 0)
        {
            chunk.liquidBytes.resize(numLiquidBytes);
            buffer.GetBytes(chunk.liquidBytes.data(), sizeof(u8) * numLiquidBytes);

            chunk.liquidHeaders.resize(Terrain::MAP_CELLS_PER_CHUNK);
            std::memcpy(chunk.liquidHeaders.data(), chunk.liquidBytes.data(), Terrain::MAP_CELLS_PER_CHUNK * sizeof(Terrain::CellLiquidHeader));

            u32 numInstances = 0;
            u32 firstInstanceOffset = std::numeric_limits<u32>().max();

            // We reserver memory for at least 1 layer for each liquid header
            for (u32 i = 0; i < Terrain::MAP_CELLS_PER_CHUNK; i++)
            {
                const Terrain::CellLiquidHeader& header = chunk.liquidHeaders[i];

                if (header.layerCount > 0)
                {
                    if (header.instancesOffset < firstInstanceOffset)
                        firstInstanceOffset = header.instancesOffset;

                    numInstances += header.layerCount;
                }
            }

            if (numInstances > 0)
            {
                chunk.liquidInstances.resize(numInstances);
                std::memcpy(chunk.liquidInstances.data(), &chunk.liquidBytes.data()[firstInstanceOffset], sizeof(Terrain::CellLiquidInstance) * numInstances);
            }
        }
    }

    stringTable.Deserialize(&buffer);
    assert(stringTable.GetNumStrings() > 0); // We always expect to have at least 1 string in our stringtable, a path for the base texture
    return true;
}
