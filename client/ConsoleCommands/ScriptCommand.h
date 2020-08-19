/*
    MIT License

    Copyright (c) 2020 NovusCore

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#pragma once
#include <Utils/Message.h>
#include "../Utils/ServiceLocator.h"
#include "../Utils/MapUtils.h"

#include "../EngineLoop.h"
#include "../Scripting/ScriptHandler.h"
#include "../Rendering/ClientRenderer.h"
#include "../Rendering/TerrainRenderer.h"
#include "../Rendering/Camera.h"
#include <vector>

void ReloadCommand(EngineLoop& engineLoop, std::vector<std::string> subCommands)
{
    Message reloadMessage;
    reloadMessage.code = MSG_IN_RELOAD;

    engineLoop.PassMessage(reloadMessage);
}


void GetChunkIds(EngineLoop& engineLoop, std::vector<std::string> subCommands)
{
    Camera* camera = ServiceLocator::GetCamera();

    f32 height = Terrain::MapUtils::GetHeightFromWorldPosition(camera->GetPosition());

    NC_LOG_MESSAGE("Height(%f), Camera Height(%f)", height, camera->GetPosition().y);
}
void LoadMapCommand(EngineLoop& engineLoop, std::vector<std::string> subCommands)
{
    size_t argSize = subCommands.size();
    if (argSize == 0)
        return;

    std::string& mapInternalName = subCommands[0];
    vec2 pos(0, 0);

    if (argSize > 1)
        pos.x = static_cast<f32>(std::stol(subCommands[1]));

    if (argSize > 2)
        pos.y = static_cast<f32>(std::stol(subCommands[2]));

    pos *= 533.33f;
    pos = 17066.66656f - pos;

    u32 mapInternalNameHash = StringUtils::fnv1a_32(mapInternalName.c_str(), mapInternalName.size());

    LoadMapInfo* loadMapInfo = new LoadMapInfo();
    loadMapInfo->mapInternalNameHash = mapInternalNameHash;
    loadMapInfo->x = pos.x;
    loadMapInfo->y = pos.y;

    Message loadMapMessage;
    loadMapMessage.code = MSG_IN_LOAD_MAP;
    loadMapMessage.object = loadMapInfo;
    engineLoop.PassMessage(loadMapMessage);
}