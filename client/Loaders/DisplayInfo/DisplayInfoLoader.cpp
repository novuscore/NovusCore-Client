#include "DisplayInfoLoader.h"
#include <Utils/ByteBuffer.h>
#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>
#include <filesystem>
#include <entt.hpp>

#include "../DBC/DBC.h"
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

    if (!ExtractCreatureDisplayInfoDBC(itr->second, displayInfoSingleton.creatureDisplayInfoDBCFiles))
    {
        NC_LOG_ERROR("Failed to correctly load CreatureDisplayInfo.ndbc data");
        return false;
    }

    for (DBC::CreatureDisplayInfo& creatureDisplayInfo : displayInfoSingleton.creatureDisplayInfoDBCFiles)
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

    if (!ExtractCreatureModelDataDBC(itr->second, displayInfoSingleton.creatureModelDataDBCFiles))
    {
        NC_LOG_ERROR("Failed to correctly load CreatureModelData.ndbc data");
        return false;
    }

    for (DBC::CreatureModelData& creatureModelData : displayInfoSingleton.creatureModelDataDBCFiles)
    {
        displayInfoSingleton.creatureModelIdToModelData[creatureModelData.id] = &creatureModelData;
    }

    return true;
}

bool DisplayInfoLoader::ExtractCreatureDisplayInfoDBC(DBC::File& file, std::vector<DBC::CreatureDisplayInfo>& creatureDisplayInfos)
{
    u32 numCreatureDisplayInfos = 0;
    file.buffer->GetU32(numCreatureDisplayInfos);

    if (numCreatureDisplayInfos == 0)
        return false;
    
    // Resize our vector and fill it with CreatureDisplayInfo data
    creatureDisplayInfos.resize(numCreatureDisplayInfos);
    file.buffer->GetBytes(reinterpret_cast<u8*>(&creatureDisplayInfos[0]), sizeof(DBC::CreatureDisplayInfo) * numCreatureDisplayInfos);

    return true;
}

bool DisplayInfoLoader::ExtractCreatureModelDataDBC(DBC::File& file, std::vector<DBC::CreatureModelData>& creatureModelDatas)
{
    u32 numcreatureModelDatas = 0;
    file.buffer->GetU32(numcreatureModelDatas);

    if (numcreatureModelDatas == 0)
        return false;

    // Resize our vector and fill it with CreatureDisplayInfo data
    creatureModelDatas.resize(numcreatureModelDatas);
    file.buffer->GetBytes(reinterpret_cast<u8*>(&creatureModelDatas[0]), sizeof(DBC::CreatureModelData) * numcreatureModelDatas);

    return true;
}
