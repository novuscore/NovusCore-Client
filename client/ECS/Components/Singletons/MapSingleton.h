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

    Terrain::Map currentMap;

	vec3 ambientLight = vec3(0.380392164f, 0.509803891f, 0.635294139f);
	vec3 diffuseLight = vec3(0.113725491f, 0.235294104f, 0.329411745f);
	vec3 lightDirection = vec3(-0.595154941f, -0.595155120f, -0.539982319f);

	robin_hood::unordered_map<u32, NDBC::Map*> mapIdToDBC;
	robin_hood::unordered_map<u32, NDBC::Map*> mapNameToDBC;
	robin_hood::unordered_map<u32, NDBC::Map*> mapInternalNameToDBC;
	std::vector<const std::string*> mapNames;
	std::vector<NDBC::Map> mapDBCEntries;

	robin_hood::unordered_map<u32, NDBC::AreaTable*> areaIdToDBC;
	robin_hood::unordered_map<u32, NDBC::AreaTable*> areaNameToDBC;
	std::vector<NDBC::AreaTable> areaTableDBCEntries;

	robin_hood::unordered_map<u32, NDBC::Light*> lightIdToDBC;
	robin_hood::unordered_map<u32, std::vector<NDBC::Light*>> mapIdToLightDBC;
	std::vector<NDBC::Light> lightDBCEntries;

	robin_hood::unordered_map<u32, NDBC::LightParams*> lightParamsIdToDBC;
	std::vector<NDBC::LightParams> lightParamsDBCEntries;

	robin_hood::unordered_map<u32, NDBC::LightIntBand*> lightIntBandIdToDBC;
	std::vector<NDBC::LightIntBand> lightIntBandDBCEntries;

	robin_hood::unordered_map<u32, NDBC::LightFloatBand*> lightFloatBandIdToDBC;
	std::vector<NDBC::LightFloatBand> lightFloatBandDBCEntries;

	robin_hood::unordered_map<u32, NDBC::LightSkybox*> lightSkyboxIdToDBC;
	std::vector<NDBC::LightSkybox> lightSkyboxDBCEntries;
};
