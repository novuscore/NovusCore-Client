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
#include "../../../Loaders/NDBC/NDBC.h"

struct NDBCSingleton
{
	NDBCSingleton() {}

	NDBC::File& AddNDBCFile(std::string name, size_t size)
	{
		StringUtils::StringHash stringHash = name;
		_loadedNDBCFileNames.push_back(name);

		NDBC::File& file = _nameHashToDBCFile[stringHash];
		file.GetBuffer() = new DynamicBytebuffer(size);
		file.GetStringTable() = new StringTable();

		return file;
	}

	bool RemoveNDBCFile(std::string name)
	{
		StringUtils::StringHash stringHash = name;

		auto& dbcFileItr = _nameHashToDBCFile.find(stringHash);
		if (dbcFileItr == _nameHashToDBCFile.end())
			return false;

		NDBC::File& file = dbcFileItr->second;

		delete file.GetBuffer();
		delete file.GetStringTable();

		for (std::vector<std::string>::iterator fileNameItr = _loadedNDBCFileNames.begin(); fileNameItr != _loadedNDBCFileNames.end(); fileNameItr++)
		{
			if (name == *fileNameItr)
			{
				_loadedNDBCFileNames.erase(fileNameItr);
				break;
			}
		}

		return true;
	}

	NDBC::File* GetNDBCFile(StringUtils::StringHash nameHash)
	{
		auto& itr = _nameHashToDBCFile.find(nameHash);
		if (itr == _nameHashToDBCFile.end())
			return nullptr;

		return &itr->second;
	}

	const std::vector<std::string>& GetLoadedNDBCFileNames() { return _loadedNDBCFileNames; }

private:
    robin_hood::unordered_map<u32, NDBC::File> _nameHashToDBCFile;
	std::vector<std::string> _loadedNDBCFileNames;
};
