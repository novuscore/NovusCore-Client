#include "DBCLoader.h"
#include <entt.hpp>

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


    fs::path stringTablePath = std::filesystem::absolute("Data/extracted/Ndbc/NDBCStringTable.nst");
    if (!fs::exists(stringTablePath))
    {
        NC_LOG_ERROR("Failed to find NDBCStringTable.nst");
        return false;
    }

    DBCSingleton& dbcSingleton = registry->set<DBCSingleton>();

    // Load StringTable first
    {
        FileReader stringTableFile(stringTablePath.string(), stringTablePath.filename().string());
        if (!stringTableFile.Open())
        {
            NC_LOG_ERROR("Failed to load %s", stringTablePath.filename().string().c_str());
            return false;
        }

        Bytebuffer* buffer = new Bytebuffer(nullptr, stringTableFile.Length());
        stringTableFile.Read(buffer, buffer->size);

        dbcSingleton.stringTable.Deserialize(buffer);
        assert(dbcSingleton.stringTable.GetNumStrings() > 0); // We always expect to have more than 0 strings for our NDBC files
    }

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

        DBC::File file;
        file.buffer = new Bytebuffer(nullptr, dbcFile.Length());
        if (!ExtractFileData(dbcFile, file))
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
        NC_LOG_ERROR("0 dbcs found in (%s)", absolutePath.string().c_str());
        return false;
    }

    NC_LOG_SUCCESS("Loaded %u dbcs", loadedDBCs);
    return true;
}

bool DBCLoader::ExtractFileData(FileReader& reader, DBC::File& file)
{
    reader.Read(file.buffer, file.buffer->size);

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
