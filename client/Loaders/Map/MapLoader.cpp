#include "../LoaderSystem.h"

#include "../NDBC/NDBC.h"
#include "../../Utils/ServiceLocator.h"
#include "../../Utils/MapUtils.h"
#include "../../ECS/Components/Singletons/MapSingleton.h"
#include "../../ECS/Components/Singletons/NDBCSingleton.h"

#include <NovusTypes.h>
#include <entt.hpp>
#include <Utils/FileReader.h>
#include <Utils/ByteBuffer.h>
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <filesystem>

namespace fs = std::filesystem;

class MapLoader : Loader
{
public:
    MapLoader() : Loader("MapLoader", 997) { }

    bool Init()
    {
        fs::path absolutePath = std::filesystem::absolute("Data/extracted/maps");
        if (!fs::is_directory(absolutePath))
        {
            NC_LOG_ERROR("Failed to find maps folder");
            return false;
        }

        entt::registry* registry = ServiceLocator::GetGameRegistry();
        MapSingleton& mapSingleton = registry->set<MapSingleton>();
        NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();

        if (!InitNDBC(mapSingleton, ndbcSingleton))
            return false;

        return true;
    }
    bool InitNDBC(MapSingleton& mapSingleton, NDBCSingleton& ndbcSingleton)
    {
        mapSingleton.ClearNDBCs();

        NDBC::File* mapsNDBC = ndbcSingleton.GetNDBCFile("Maps"_h);
        if (!mapsNDBC)
        {
            NC_LOG_ERROR("Maps.ndbc has not been loaded, please check your data folder.");
            return false;
        }

        NDBC::File* areaTableNDBC = ndbcSingleton.GetNDBCFile("AreaTable"_h);
        if (!areaTableNDBC)
        {
            NC_LOG_ERROR("AreaTable.ndbc has not been loaded, please check your data folder.");
            return false;
        }

        NDBC::File* lightNDBC = ndbcSingleton.GetNDBCFile("Light"_h);
        if (!lightNDBC)
        {
            NC_LOG_ERROR("Light.ndbc has not been loaded, please check your data folder.");
            return false;
        }

        // Add Lookup Table(s) for Maps.ndbc
        {
            for (u32 i = 0; i < mapsNDBC->GetNumRows(); i++)
            {
                NDBC::Map* map = mapsNDBC->GetRowByIndex<NDBC::Map>(i);

                mapSingleton.AddMapNDBC(mapsNDBC, map);
            }
        }

        // Add Lookup Table(s) for AreaTable.ndbc
        {
            for (u32 i = 0; i < areaTableNDBC->GetNumRows(); i++)
            {
                NDBC::AreaTable* areaTable = areaTableNDBC->GetRowByIndex<NDBC::AreaTable>(i);
                mapSingleton.AddAreaTableNDBC(areaTableNDBC, areaTable);
            }
        }

        // Add Lookup Table(s) for Light.ndbc
        {
            for (u32 i = 0; i < lightNDBC->GetNumRows(); i++)
            {
                NDBC::Light* light = lightNDBC->GetRowByIndex<NDBC::Light>(i);
                mapSingleton.AddLightNDBC(light);
            }
        }

        return true;
    }
};

MapLoader mapLoader;