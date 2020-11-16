#pragma once
#include <NovusTypes.h>
#include <Utils/FileReader.h>
#include <entity/fwd.hpp>

class StringTable;
namespace NDBC
{
    struct File;
}

class NDBCLoader
{
public:
    NDBCLoader() { }
    static bool Load(entt::registry* registry);

    template <typename T>
    static bool LoadDataIntoVector(NDBC::File& file, std::vector<T>& data)
    {
        u32 numElements = file.numRows;
        if (numElements == 0)
            return false;

        // Resize our vector and fill it with data
        data.resize(numElements);
        if (!file.buffer->GetBytes(reinterpret_cast<u8*>(&data[0]), sizeof(T) * numElements))
            return false;

        if (!file.stringTable->Deserialize(file.buffer))
            return false;

        return true;
    }
private:
    static bool LoadNDBC(FileReader& reader, NDBC::File& file);
};