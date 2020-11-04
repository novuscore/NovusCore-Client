#include "NDBCLoader.h"
#include <entt.hpp>

#include "../../ECS/Components/Singletons/DBCSingleton.h"

namespace fs = std::filesystem;

bool NDBCLoader::Load(entt::registry* registry)
{
    fs::path absolutePath = std::filesystem::absolute("Data/extracted/Ndbc");
    if (!fs::is_directory(absolutePath))
    {
        NC_LOG_ERROR("Failed to find Ndbc folder");
        return false;
    }

    DBCSingleton& dbcSingleton = registry->set<DBCSingleton>();

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
        file.buffer = new Bytebuffer(nullptr, dbcFile.Length());
        file.stringTable = new StringTable();
        if (!LoadNDBC(dbcFile, file))
        {
            NC_LOG_ERROR("Failed to read %s", filePath.filename().string().c_str());
            return false;
        }

        std::string dbcName = filePath.filename().replace_extension().string();
        u32 dbcNameHash = StringUtils::fnv1a_32(dbcName.c_str(), dbcName.size());
        dbcSingleton.nameHashToDBCFile[dbcNameHash] = file;

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

    file.buffer->Get<NDBC::DBCHeader>(file.header);

    if (file.header.token != NDBC::NDBC_TOKEN)
    {
        NC_LOG_FATAL("Attempted to load NDBC file with the wrong token, try reextracting your data");
        return false;
    }

    if (file.header.version != NDBC::NDBC_VERSION)
    {
        if (file.header.version < NDBC::NDBC_VERSION)
        {
            NC_LOG_FATAL("Attempted to load NDBC file with older version of %u instead of expected version of %u, try reextracting your data", file.header.version, NDBC::NDBC_VERSION);
        }
        else
        {
            NC_LOG_FATAL("Attempted to load NDBC file with newer version of %u instead of expected version of %u, try reextracting your data", file.header.version, NDBC::NDBC_VERSION);
        }

        return false;
    }

    return true;
}
