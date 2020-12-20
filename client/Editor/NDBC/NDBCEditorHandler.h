#pragma once
#include <NovusTypes.h>
#include "NDBCEditorBase.h"
#include "../../Utils/ServiceLocator.h"
#include "TeleportLocationsEditor.h"
#include "../../ECS/Components/Singletons/NDBCSingleton.h"

#include <filesystem>
#include <robin_hood.h>

namespace fs = std::filesystem;

static std::string ndbcColumnTypes[3] = { "I32", "U32", "F32" };
struct TemporaryNDBCColumn
{
    std::string name;
    i32 type;
};

class NDBCEditorHandler
{
public:
    NDBCEditorHandler()
    {
        _editors["TeleportLocations"_h] = new TeleportLocationsEditor();
    }


    void DrawImGuiMenuBar();
    void Draw();

    bool CreateNewNDBC(const std::string name, const u32 numColumns);
    bool DeleteSelectedNDBC();
    bool SaveSelectedNDBC();

    bool GetEditor(NDBCEditorBase*& editor);

private:
    void DrawMenuBar();
    void DrawPopups();
    void DrawDefaultLayout();

private:
    const char* _selectedNDBC = nullptr;

    std::vector<TemporaryNDBCColumn> _newNDBCColumns;
    robin_hood::unordered_map<u32, NDBCEditorBase*> _editors; // Name Hash -> Editor
};