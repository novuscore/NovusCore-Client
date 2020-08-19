#include "DBCLoader.h"

#include "../../ECS/Components/Singletons/DBCSingleton.h"

namespace fs = std::filesystem;

bool DBCLoader::Load(entt::registry* registry)
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
            NC_LOG_ERROR("Failed to load all maps");
            return false;
        }

        DBC::File file;
        file.buffer = Bytebuffer::Borrow<524288>();
        if (!ExtractFileData(dbcFile, file))
        {
            NC_LOG_ERROR("Failed to load all maps");
            return false;
        }

        std::string dbcName = filePath.filename().replace_extension().string();
        u32 dbcNameHash = StringUtils::fnv1a_32(dbcName.c_str(), dbcName.size());
        dbcSingleton.dbcs[dbcNameHash] = file;

        loadedDBCs++;
    }

    if (loadedDBCs == 0)
    {
        NC_LOG_ERROR("0 dbcs found in (%s)", absolutePath.string().c_str());
        return false;
    }

    NC_LOG_SUCCESS("Loaded %u dbcs", loadedDBCs);
    return true;
}

bool DBCLoader::ExtractFileData(FileReader& reader, DBC::File& file)
{
    file.buffer->size = reader.Length();
    reader.Read(file.buffer.get(), file.buffer->size);

    file.buffer->Get<DBC::DBCHeader>(file.header);

    if (file.header.token != DBC::DBC_TOKEN)
    {
        NC_LOG_FATAL("Tried to load a ndbc file with the wrong token");
    }

    if (file.header.version != DBC::DBC_VERSION)
    {
        NC_LOG_FATAL("Loaded ndbc file has version of %u instead of expected version of %u, try reextracting your data", file.header.version, DBC::DBC_VERSION);
    }

    return true;
}
