#/*
	MIT License

	Copyright (c) 2018-2019 NovusCore

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
#include <NovusTypes.h>
#include <robin_hood.h>
#include "../../../Gameplay/Map/Map.h"
#include "../../../Loaders/NDBC/NDBC.h"

struct MapSingleton
{
    MapSingleton() {}

	Terrain::Map& GetCurrentMap() { return _currentMap; }

	vec3 GetAmbientLight() { return _ambientLight; }
	void SetAmbientLight(vec3 ambientLight) { _ambientLight = ambientLight; }

	vec3 GetDiffuseLight() { return _diffuseLight; }
	void SetDiffuseLight(vec3 diffuseLight) { _diffuseLight = diffuseLight; }

	vec3 GetLightDirection() { return _lightDirection; }
	void SetLightDirection(vec3 lightDirection) { _lightDirection = lightDirection; }
	
	std::vector<const std::string*>& GetMapNames() { return _mapNames; }
	NDBC::Map* GetMapByNameHash(u32 mapNameHash)
	{
		auto& itr = _mapNameHashToDBC.find(mapNameHash);
		if (itr == _mapNameHashToDBC.end())
			return nullptr;

		return itr->second;
	}
	NDBC::Map* GetMapByInternalNameHash(u32 mapInternalNameHash)
	{
		auto& itr = _mapInternalNameHashToDBC.find(mapInternalNameHash);
		if (itr == _mapInternalNameHashToDBC.end())
			return nullptr;

		return itr->second;
	}
	NDBC::AreaTable* GetAreaTableByNameHash(u32 areaTableNameHash)
	{
		auto& itr = _areaNameHashToDBC.find(areaTableNameHash);
		if (itr == _areaNameHashToDBC.end())
			return nullptr;

		return itr->second;
	}
	std::vector<NDBC::Light*>* GetLightsByMapId(u32 mapId)
	{
		auto& itr = _mapIdToLightDBC.find(mapId);
		if (itr == _mapIdToLightDBC.end())
			return nullptr;

		return &itr->second;
	}

// NDBC Helper Functions
public:
	void ClearNDBCs()
	{
		// Map.ndbc
		_mapNameHashToDBC.clear();
		_mapInternalNameHashToDBC.clear();
		_mapNames.clear();

		// AreaTable.ndbc
		_areaNameHashToDBC.clear();

		// Light.ndbc
		_mapIdToLightDBC.clear();
	}

	void AddMapNDBC(NDBC::File* mapsNDBC, NDBC::Map* map)
	{
		const std::string& mapName = mapsNDBC->GetStringTable()->GetString(map->name);
		u32 mapNameHash = mapsNDBC->GetStringTable()->GetStringHash(map->name);
		u32 mapInternalNameHash = mapsNDBC->GetStringTable()->GetStringHash(map->internalName);

		_mapNameHashToDBC[mapNameHash] = map;
		_mapInternalNameHashToDBC[mapInternalNameHash] = map;
		_mapNames.push_back(&mapName);
	}
	void AddAreaTableNDBC(NDBC::File* areaTableNDBC, NDBC::AreaTable* areaTable)
	{
		u32 areaNameHash = areaTableNDBC->GetStringTable()->GetStringHash(areaTable->name);
		_areaNameHashToDBC[areaNameHash] = areaTable;
	}
	void AddLightNDBC(NDBC::Light* light)
	{
		_mapIdToLightDBC[light->mapId].push_back(light);
	}

private:
	Terrain::Map _currentMap;
	vec3 _ambientLight = vec3(0.380392164f, 0.509803891f, 0.635294139f);
	vec3 _diffuseLight = vec3(0.113725491f, 0.235294104f, 0.329411745f);
	vec3 _lightDirection = vec3(-0.595154941f, -0.595155120f, -0.539982319f);

	std::vector<const std::string*> _mapNames;
	robin_hood::unordered_map<u32, NDBC::Map*> _mapNameHashToDBC;
	robin_hood::unordered_map<u32, NDBC::Map*> _mapInternalNameHashToDBC;
	robin_hood::unordered_map<u32, NDBC::AreaTable*> _areaNameHashToDBC;
	robin_hood::unordered_map<u32, std::vector<NDBC::Light*>> _mapIdToLightDBC;
};
