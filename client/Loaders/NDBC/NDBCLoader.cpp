#include "NDBCLoader.h"
#include <entt.hpp>

#include "../../ECS/Components/Singletons/NDBCSingleton.h"

namespace fs = std::filesystem;

bool NDBCLoader::Load(entt::registry* registry)
{
    fs::path absolutePath = std::filesystem::absolute("Data/extracted/Ndbc");
    if (!fs::is_directory(absolutePath))
    {
        NC_LOG_ERROR("Failed to find Ndbc folder");
        return false;
    }

    NDBCSingleton& ndbcSingleton = registry->set<NDBCSingleton>();

    size_t loadedDBCs = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(absolutePath))
    {
        auto filePath = std::filesystem::path(entry.path());
        if (filePath.extension() != ".ndbc")
            continue;

        FileReader dbcFile(entry.path().string(), filePath.filename().string());
        if (!dbcFile.Open())
        {
            NC_LOG_ERROR("Failed to load %s", filePath.filename().string().c_str());
            return false;
        }

        NDBC::File file;
        file.buffer = new DynamicBytebuffer(dbcFile.Length());
        file.stringTable = new StringTable();
        if (!LoadNDBC(dbcFile, file))
        {
            NC_LOG_ERROR("Failed to read %s", filePath.filename().string().c_str());
            return false;
        }

        std::string dbcName = filePath.filename().replace_extension().string();
        u32 dbcNameHash = StringUtils::fnv1a_32(dbcName.c_str(), dbcName.size());
        ndbcSingleton.nameHashToDBCFile[dbcNameHash] = file;
        ndbcSingleton.nDBCFilename.push_back(filePath.filename().replace_extension("").string());

        loadedDBCs++;
    }

    if (loadedDBCs == 0)
    {
        NC_LOG_ERROR("0 ndbcs found in (%s)", absolutePath.string().c_str());
        return false;
    }

    NC_LOG_SUCCESS("Loaded %u ndbcs", loadedDBCs);
    return true;
}

bool NDBCLoader::LoadNDBC(FileReader& reader, NDBC::File& file)
{
    reader.Read(file.buffer, file.buffer->size);

    bool readHeaderSuccessfully = false;

    readHeaderSuccessfully |= file.buffer->GetU32(file.header.token);
    readHeaderSuccessfully |= file.buffer->GetU32(file.header.version);

    u32 numColumns = 0;
    readHeaderSuccessfully |= file.buffer->GetU32(numColumns);

    if (!readHeaderSuccessfully)
    {
        NC_LOG_FATAL("Attempted to load NDBC file (%s) with no header, try reextracting your data", reader.FileName().c_str());
        return false;
    }

    if (file.header.token != NDBC::NDBC_TOKEN)
    {
        NC_LOG_FATAL("Attempted to load NDBC file (%s) with the wrong token, try reextracting your data", reader.FileName().c_str());
        return false;
    }

    if (file.header.version != NDBC::NDBC_VERSION)
    {
        if (file.header.version < NDBC::NDBC_VERSION)
        {
            NC_LOG_FATAL("Attempted to load NDBC file (%s) with older version of %u instead of expected version of %u, try reextracting your data", reader.FileName().c_str(), file.header.version, NDBC::NDBC_VERSION);
        }
        else
        {
            NC_LOG_FATAL("Attempted to load NDBC file (%s) with newer version of %u instead of expected version of %u, try updating your client", reader.FileName().c_str(), file.header.version, NDBC::NDBC_VERSION);
        }

        return false;
    }

    // Load String Name Indexes
    if (numColumns > 0)
    {
        file.columns.resize(numColumns);

        for (u32 i = 0; i < numColumns; i++)
        {
            NDBC::NDBCColumn& column = file.columns[i];

            file.buffer->GetString(column.name);

            if (!file.buffer->GetU32(column.dataType))
            {
                NC_LOG_FATAL("Attempted to load NDBC file (%s) with corrupt column header data, try reextracting your data", reader.FileName().c_str());
                return false;
            }
        }
    }

    if (!file.buffer->GetU32(file.numRows))
    {
        NC_LOG_FATAL("Attempted to load NDBC file (%s) with corrupt row data, try reextracting your data", reader.FileName().c_str());
        return false;
    }

    file.dataOffsetToRowData = file.buffer->readData;

    return true;
}
