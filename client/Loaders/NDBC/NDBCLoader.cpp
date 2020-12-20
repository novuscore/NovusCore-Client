#include "../LoaderSystem.h"
#include "../../Utils/ServiceLocator.h"
#include "../../ECS/Components/Singletons/NDBCSingleton.h"

#include <NovusTypes.h>
#include <entt.hpp>
#include <Utils/FileReader.h>
#include <filesystem>
namespace fs = std::filesystem;

class NDBCLoader : Loader
{
public:
    NDBCLoader() : Loader("NDBCLoader", 998) { }

    bool Init()
    {
        fs::path absolutePath = std::filesystem::absolute("Data/extracted/Ndbc");
        if (!fs::is_directory(absolutePath))
        {
            NC_LOG_ERROR("Failed to find Ndbc folder");
            return false;
        }

        entt::registry* registry = ServiceLocator::GetGameRegistry();
        NDBCSingleton& ndbcSingleton = registry->set<NDBCSingleton>();

        size_t loadedDBCs = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(absolutePath))
        {
            auto filePath = std::filesystem::path(entry.path());
            if (filePath.extension() != ".ndbc")
                continue;

            std::string filePathStr = filePath.filename().string();

            FileReader dbcFile(entry.path().string(), filePathStr);
            if (!dbcFile.Open())
            {
                NC_LOG_ERROR("Failed to load %s", filePathStr.c_str());
                return false;
            }

            std::string dbcName = filePath.filename().replace_extension("").string();
            NDBC::File& file = ndbcSingleton.AddNDBCFile(dbcName, dbcFile.Length());

            if (!LoadNDBC(dbcFile, file))
            {
                NC_LOG_ERROR("Failed to read %s", filePathStr.c_str());
                return false;
            }

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
    bool LoadNDBC(FileReader& reader, NDBC::File& file)
    {
        NDBC::NDBCHeader& header = file.GetHeader();
        std::vector<NDBC::NDBCColumn>& columns = file.GetColumns();
        DynamicBytebuffer*& fileBuffer = file.GetBuffer();

        reader.Read(fileBuffer, fileBuffer->size);

        bool readHeaderSuccessfully = false;

        readHeaderSuccessfully |= fileBuffer->GetU32(header.token);
        readHeaderSuccessfully |= fileBuffer->GetU32(header.version);

        u32 numColumns = 0;
        readHeaderSuccessfully |= fileBuffer->GetU32(numColumns);

        if (!readHeaderSuccessfully)
        {
            NC_LOG_FATAL("Attempted to load NDBC file (%s) with no header, try reextracting your data", reader.FileName().c_str());
            return false;
        }

        if (header.token != NDBC::NDBC_TOKEN)
        {
            NC_LOG_FATAL("Attempted to load NDBC file (%s) with the wrong token, try reextracting your data", reader.FileName().c_str());
            return false;
        }

        if (header.version != NDBC::NDBC_VERSION)
        {
            if (header.version < NDBC::NDBC_VERSION)
            {
                NC_LOG_FATAL("Attempted to load NDBC file (%s) with older version of %u instead of expected version of %u, try reextracting your data", reader.FileName().c_str(), header.version, NDBC::NDBC_VERSION);
            }
            else
            {
                NC_LOG_FATAL("Attempted to load NDBC file (%s) with newer version of %u instead of expected version of %u, try updating your client", reader.FileName().c_str(), header.version, NDBC::NDBC_VERSION);
            }

            return false;
        }

        // Load String Name Indexes
        if (numColumns > 0)
        {
            columns.resize(numColumns);

            for (u32 i = 0; i < numColumns; i++)
            {
                NDBC::NDBCColumn& column = columns[i];

                fileBuffer->GetString(column.name);

                if (!fileBuffer->GetU32(column.dataType))
                {
                    NC_LOG_FATAL("Attempted to load NDBC file (%s) with corrupt column header data, try reextracting your data", reader.FileName().c_str());
                    return false;
                }
            }
        }

        // All Columns are 4 bytes
        u32 rowSize = numColumns * sizeof(u32);
        file.SetRowSize(rowSize);

        u32 numRows = 0;
        if (!fileBuffer->GetU32(numRows))
        {
            NC_LOG_FATAL("Attempted to load NDBC file (%s) with corrupt row data, try reextracting your data", reader.FileName().c_str());
            return false;
        }

        file.SetNumRows(numRows);
        file.SetBufferOffsetToRowData(fileBuffer->readData);

        // Setup Row Id to Row unordered map
        robin_hood::unordered_map<u32, void*>& rowIdToRowMap = file.GetRowIdToRowMap();

        for (u32 i = 0; i < numRows; i++)
        {
            void* rowPtr = &fileBuffer->GetReadPointer()[i * rowSize];
            u32 id = *reinterpret_cast<u32*>(rowPtr);

            rowIdToRowMap[id] = rowPtr;
        }

        u32 rowDataBytes = numRows * rowSize;
        fileBuffer->SkipRead(rowDataBytes);

        StringTable*& stringTable = file.GetStringTable();
        if (!stringTable->Deserialize(fileBuffer))
        {
            NC_LOG_FATAL("Attempted to load NDBC file (%s) with corrupt StringTable data, try reextracting your data", reader.FileName().c_str());
            return false;
        }

        return true;
    }
};

NDBCLoader ndbcLoader;