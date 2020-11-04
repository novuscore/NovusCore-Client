#include "DisplayInfoLoader.h"
#include <Utils/ByteBuffer.h>
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <filesystem>
#include <entt.hpp>

#include "../NDBC/NDBC.h"
#include "../NDBC/NDBCLoader.h"
#include "../../ECS/Components/Singletons/DisplayInfoSingleton.h"
#include "../../ECS/Components/Singletons/DBCSingleton.h"

namespace fs = std::filesystem;

bool DisplayInfoLoader::Init(entt::registry* registry)
{
    DBCSingleton& dbcSingleton = registry->ctx<DBCSingleton>();
    DisplayInfoSingleton& displayInfoSingleton = registry->set<DisplayInfoSingleton>();

    if (!LoadCreatureModelDataDBC(dbcSingleton, displayInfoSingleton))
        return false;

    if (!LoadCreatureDisplayInfoDBC(dbcSingleton, displayInfoSingleton))
        return false;

    return true;
}

bool DisplayInfoLoader::LoadCreatureDisplayInfoDBC(DBCSingleton& dbcSingleton, DisplayInfoSingleton& displayInfoSingleton)
{
    if (displayInfoSingleton.creatureDisplayInfoDBCFiles.size() != 0)
    {
        NC_LOG_ERROR("DisplayInfoLoader::Init can only be called once");
        return false;
    }

    auto itr = dbcSingleton.nameHashToDBCFile.find("CreatureDisplayInfo"_h);
    if (itr == dbcSingleton.nameHashToDBCFile.end())
    {
        NC_LOG_ERROR("CreatureDisplayInfo.ndbc has not been loaded, please check your data folder.");
        return false;
    }

    if (!NDBCLoader::LoadDataIntoVector<NDBC::CreatureDisplayInfo>(itr->second, displayInfoSingleton.creatureDisplayInfoDBCFiles))
    {
        NC_LOG_ERROR("Failed to correctly load CreatureDisplayInfo.ndbc data");
        return false;
    }

    for (NDBC::CreatureDisplayInfo& creatureDisplayInfo : displayInfoSingleton.creatureDisplayInfoDBCFiles)
    {
        displayInfoSingleton.creatureDisplayIdToDisplayInfo[creatureDisplayInfo.id] = &creatureDisplayInfo;
    }

    return true;
}

bool DisplayInfoLoader::LoadCreatureModelDataDBC(DBCSingleton& dbcSingleton, DisplayInfoSingleton& displayInfoSingleton)
{
    if (displayInfoSingleton.creatureModelDataDBCFiles.size() != 0)
    {
        NC_LOG_ERROR("DisplayInfoLoader::Init can only be called once");
        return false;
    }

    auto itr = dbcSingleton.nameHashToDBCFile.find("CreatureModelData"_h);
    if (itr == dbcSingleton.nameHashToDBCFile.end())
    {
        NC_LOG_ERROR("CreatureModelData.ndbc has not been loaded, please check your data folder.");
        return false;
    }

    if (!NDBCLoader::LoadDataIntoVector<NDBC::CreatureModelData>(itr->second, displayInfoSingleton.creatureModelDataDBCFiles))
    {
        NC_LOG_ERROR("Failed to correctly load CreatureModelData.ndbc data");
        return false;
    }

    for (NDBC::CreatureModelData& creatureModelData : displayInfoSingleton.creatureModelDataDBCFiles)
    {
        displayInfoSingleton.creatureModelIdToModelData[creatureModelData.id] = &creatureModelData;
    }

    return true;
}