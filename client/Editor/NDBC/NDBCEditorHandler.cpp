#include "NDBCEditorHandler.h"

#include <fstream>
#include <CVar/CVarSystem.h>
#include <Utils/StringUtils.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "imgui/misc/cpp/imgui_stdlib.h"

AutoCVar_Int CVAR_NDBCEditorEnabled("editor.editors.ndbc.Enable", "enable the ndbc editor window", 0, CVarFlags::Noedit);
AutoCVar_Int CVAR_NDBCEditorIsCreating("editor.editors.ndbc.IsCreating", "enable the ndbc creator window", 0, CVarFlags::Noedit);
AutoCVar_Int CVAR_NDBCEditorIsEditingHeader("editor.editors.ndbc.IsEditingHeader", "enable the ndbc editing header window", 0, CVarFlags::Noedit);

void NDBCEditorHandler::DrawImGuiMenuBar()
{
    if (ImGui::MenuItem("NDBC Editor"))
    {
        CVAR_NDBCEditorEnabled.Set(1);
    }
}

void NDBCEditorHandler::Draw()
{
    i32* isEditorEnabled = CVAR_NDBCEditorEnabled.GetPtr();
    if (*isEditorEnabled)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();
        
        if (ImGui::Begin("NDBC Editor", reinterpret_cast<bool*>(isEditorEnabled), ImGuiWindowFlags_MenuBar))
        {
            DrawMenuBar();
            DrawPopups();

            const std::vector<std::string>& ndbcFileNames = ndbcSingleton.GetLoadedNDBCFileNames();

            if (_selectedNDBC == nullptr && ndbcFileNames.size() > 0)
                _selectedNDBC = ndbcFileNames[0].c_str();

            {
                if (ImGui::BeginCombo("NDBC File", _selectedNDBC)) // The second parameter is the label previewed before opening the combo.
                {
                    for (const std::string& nDBCFilename : ndbcFileNames)
                    {
                        const char* fileName = nDBCFilename.c_str();
                        bool isSelected = _selectedNDBC == fileName;

                        if (ImGui::Selectable(nDBCFilename.c_str(), &isSelected))
                            _selectedNDBC = fileName;

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();

                }
            }

            ImGui::Separator();

            // Show Selected NDBC
            {
                NDBCEditorBase* editor;
                if (GetEditor(editor))
                {
                    editor->Draw();
                }
                else
                {
                    // Default Layout
                    DrawDefaultLayout();
                }
            }
        }
        
        ImGui::End();
    }
}

bool NDBCEditorHandler::CreateNewNDBC(const std::string name, const u32 numColumns)
{
    if (name.length() == 0)
        return false;

    entt::registry* registry = ServiceLocator::GetGameRegistry();
    NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();

    u32 ndbcNameHash = StringUtils::fnv1a_32(name.data(), name.length());
    NDBC::File* existingFile = ndbcSingleton.GetNDBCFile(ndbcNameHash);
    if (existingFile)
        return false;

    NDBC::File& file = ndbcSingleton.AddNDBCFile(name, 1 * 1024 * 1024); // Default to 1 MB of space for new NDBCs

    // Setup NDBC File
    {
        std::vector<NDBC::NDBCColumn>& columns = file.GetColumns();
        columns.resize(numColumns);

        for (u32 i = 0; i < numColumns; i++)
        {
            const TemporaryNDBCColumn& tempColumn = _newNDBCColumns[i];
            NDBC::NDBCColumn& column = columns[i];

            column.name = _strdup(tempColumn.name.c_str());
            column.dataType = tempColumn.type;
        }

        file.SetNumRows(0);
    }

    _selectedNDBC = ndbcSingleton.GetLoadedNDBCFileNames().back().data();

    SaveSelectedNDBC();

    return true;
}

bool NDBCEditorHandler::DeleteSelectedNDBC()
{
    if (_selectedNDBC == nullptr)
        return false;

    entt::registry* registry = ServiceLocator::GetGameRegistry();
    NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();

    std::string ndbcName = _selectedNDBC;
    u32 ndbcNameHash = StringUtils::fnv1a_32(ndbcName.data(), ndbcName.length());

    fs::path ndbcPath = fs::absolute("Data/extracted/Ndbc");
    fs::path outputPath = (ndbcPath / ndbcName).replace_extension("ndbc");

    if (fs::is_directory(outputPath) || !fs::is_regular_file(outputPath))
        return false;

    if (!fs::remove(outputPath))
        return false;

    const std::vector<std::string>& loadedNDBCFileNames = ndbcSingleton.GetLoadedNDBCFileNames();
    u32 numNDBCNames = static_cast<u32>(loadedNDBCFileNames.size());

    ndbcSingleton.RemoveNDBCFile(ndbcName);

    if (_selectedNDBC == nullptr && numNDBCNames > 1)
        _selectedNDBC = loadedNDBCFileNames.back().data();

    return true;
}

bool NDBCEditorHandler::SaveSelectedNDBC()
{
    if (_selectedNDBC == nullptr)
        return false;

    entt::registry* registry = ServiceLocator::GetGameRegistry();
    NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();

    std::string ndbcName = _selectedNDBC;
    u32 ndbcNameHash = StringUtils::fnv1a_32(ndbcName.data(), ndbcName.length());

    fs::path ndbcPath = fs::absolute("Data/extracted/Ndbc");
    fs::path outputPath = (ndbcPath / ndbcName).replace_extension("ndbc");

    std::ofstream output(outputPath, std::ofstream::out | std::ofstream::binary);
    {
        NDBC::File* file = ndbcSingleton.GetNDBCFile(ndbcNameHash);

        DynamicBytebuffer*& fileBuffer = file->GetBuffer();
        DynamicBytebuffer* buffer = new DynamicBytebuffer(fileBuffer->size);
        std::vector<NDBC::NDBCColumn>& columns = file->GetColumns();

        buffer->Put<NDBC::NDBCHeader>(file->GetHeader());


        u32 numColumns = static_cast<u32>(columns.size());
        buffer->Put<u32>(numColumns);

        if (numColumns > 0)
        {
            for (u32 i = 0; i < columns.size(); i++)
            {
                NDBC::NDBCColumn& column = columns[i];
                buffer->PutString(column.name.c_str());
                buffer->PutU32(column.dataType);
            }
        }

        // Write numRows, Rows and Stringtable
        u32 numRows = file->GetNumRows();
        buffer->Put<u32>(numRows);

        if (numRows > 0)
        {
            size_t dataOffsetInOldBuffer = file->GetBufferOffsetToRowData();
            u32 dataSize = (numColumns * 4) * numRows; // We always 4 byte align the columns which mean we can predict

            file->SetBufferOffsetToRowData(buffer->writtenData);
            buffer->PutBytes(&fileBuffer->GetDataPointer()[dataOffsetInOldBuffer], dataSize);
        }

        // Save StringTable "TODO: When we copy the Dynamic ByteBuffer we want to serialize the stringtable into the buffer and then do a single write into output
        
        if (!file->GetStringTable()->Serialize(buffer))
        {
            DebugHandler::PrintError("Failed to write StringTable during NDBCEditorHandler::SaveSelectedNDBC() for %s", _selectedNDBC);
            return false;
        }

        output.write(reinterpret_cast<char const*>(buffer->GetDataPointer()), buffer->writtenData);
        output.close();

        delete fileBuffer;
        fileBuffer = buffer;
    }

    return true;
}

bool NDBCEditorHandler::GetEditor(NDBCEditorBase*& editor)
{
    u32 editorNameHash = StringUtils::fnv1a_32(_selectedNDBC, strlen(_selectedNDBC));

    auto itr = _editors.find(editorNameHash);
    if (itr == _editors.end())
        return false;

    editor = itr->second;
    return true;
}

void NDBCEditorHandler::DrawMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Create NDBC"))
            {
                CVAR_NDBCEditorIsCreating.Set(1);
            }

            if (ImGui::MenuItem("Delete Selected NDBC"))
            {
                DeleteSelectedNDBC();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Edit Header"))
            {
                CVAR_NDBCEditorIsEditingHeader.Set(1);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void NDBCEditorHandler::DrawPopups()
{
    if (CVAR_NDBCEditorIsCreating.Get())
    {
        ImGui::OpenPopup("Create NDBC");

        ImGui::SetWindowSize("Create NDBC", ImVec2(600, 400));
        if (ImGui::BeginPopupModal("Create NDBC", nullptr, ImGuiWindowFlags_NoResize))
        {
            static std::string ndbcName;
            static i32 ndbcNumColumns;

            ImGui::InputText("Name", &ndbcName);
            if (ImGui::InputInt("Number of Columns", &ndbcNumColumns, 1, 5, ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue))
            {
                if (ndbcNumColumns > _newNDBCColumns.size())
                {
                    _newNDBCColumns.resize(ndbcNumColumns);
                }
            }

            ImGui::Text("Columns");
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            for (i32 i = 0; i < ndbcNumColumns; i++)
            {
                ImGui::PushID(i);
                ImGui::AlignTextToFramePadding();

                TemporaryNDBCColumn& column = _newNDBCColumns[i];
                std::string& columnValue = column.name;

                if (columnValue.length() == 0)
                {
                    columnValue = "Column " + std::to_string(i + 1);
                }

                char* selectedType = ndbcColumnTypes[column.type].data();

                ImGui::InputText("Name", &columnValue); 
                ImGui::NextColumn();
                if (ImGui::BeginCombo("Type", selectedType)) // The second parameter is the label previewed before opening the combo.
                {
                    for (i32 j = 0; j < 3; j++)
                    {
                        char* fileName = ndbcColumnTypes[j].data();
                        bool isSelected = selectedType == fileName;

                        if (ImGui::Selectable(fileName, &isSelected))
                            column.type = j;

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }

                ImGui::NextColumn();
                ImGui::PopID();
            }

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();

            {
                bool shouldClose = false;

                if (ImGui::Button("Create"))
                {
                    CreateNewNDBC(ndbcName, ndbcNumColumns);

                    shouldClose = true;

                    // Reset State (We preserve state if you simply press 'close')
                    ndbcName = "";
                    ndbcNumColumns = 0;
                    _newNDBCColumns.clear();
                }

                ImGui::SameLine();
                shouldClose |= ImGui::Button("Close");

                if (shouldClose)
                {
                    CVAR_NDBCEditorIsCreating.Set(0);
                }
            }


            ImGui::EndPopup();
        }
    }

    if (CVAR_NDBCEditorIsEditingHeader.Get())
    {
        ImGui::OpenPopup("Edit NDBC Header");

        ImGui::SetWindowSize("Edit NDBC Header", ImVec2(600, 400));
        if (ImGui::BeginPopupModal("Edit NDBC Header", nullptr, ImGuiWindowFlags_NoResize))
        {
            entt::registry* registry = ServiceLocator::GetGameRegistry();
            NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();

            u32 dbcNameHash = StringUtils::fnv1a_32(_selectedNDBC, strlen(_selectedNDBC));
            NDBC::File* file = ndbcSingleton.GetNDBCFile(dbcNameHash);
            std::vector<NDBC::NDBCColumn>& columns = file->GetColumns();

            ImGui::Text("Columns");
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            for (i32 i = 0; i < columns.size(); i++)
            {
                ImGui::PushID(i);
                ImGui::AlignTextToFramePadding();

                NDBC::NDBCColumn& column = columns[i];

                if (column.name.length() == 0)
                {
                    column.name = "Column " + std::to_string(i + 1);
                }

                char* selectedType = ndbcColumnTypes[column.dataType].data();

                ImGui::InputText("Name", &column.name);

                ImGui::NextColumn();
                if (ImGui::BeginCombo("Type", selectedType)) // The second parameter is the label previewed before opening the combo.
                {
                    for (i32 j = 0; j < 3; j++)
                    {
                        char* fileName = ndbcColumnTypes[j].data();
                        bool isSelected = selectedType == fileName;

                        if (ImGui::Selectable(fileName, &isSelected))
                            column.dataType = j;

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }

                ImGui::NextColumn();
                ImGui::PopID();
            }

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();

            if (ImGui::Button("Close"))
            {
                CVAR_NDBCEditorIsEditingHeader.Set(0);

                // Save NDBC
                SaveSelectedNDBC();
            }

            ImGui::EndPopup();
        }
    }
}

void NDBCEditorHandler::DrawDefaultLayout()
{
    entt::registry* registry = ServiceLocator::GetGameRegistry();
    NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();

    u32 dbcNameHash = StringUtils::fnv1a_32(_selectedNDBC, strlen(_selectedNDBC));
    NDBC::File* file = ndbcSingleton.GetNDBCFile(dbcNameHash);
    DynamicBytebuffer*& fileBuffer = file->GetBuffer();
    std::vector<NDBC::NDBCColumn>& columns = file->GetColumns();

    u32 numColumns = static_cast<u32>(columns.size());

    //ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
    ImGui::Columns(numColumns);
    ImGui::Separator();

    for (u32 i = 0; i < numColumns; i++)
    {
        //ImGui::AlignTextToFramePadding();
        NDBC::NDBCColumn& column = columns[i];

        if (column.name.length() == 0)
            column.name = "Column " + std::to_string(i + 1);

        ImGui::Text("%s", column.name.c_str());
        ImGui::NextColumn();
    }

    u32 strideInBytes = numColumns * 4;
    for (u32 i = 0; i < file->GetNumRows(); i++)
    {
        u32 rowOffset = i * strideInBytes;

        for (u32 j = 0; j < numColumns; j++)
        {
            u32 finalOffset = rowOffset + (j * 4);

            ImGui::PushID(j + i * numColumns);
            //ImGui::AlignTextToFramePadding();

            NDBC::NDBCColumn& column = columns[j];

            bool isFloat = column.dataType == 2;
            if (isFloat)
            {
                f32* value = reinterpret_cast<f32*>(&fileBuffer->GetDataPointer()[file->GetBufferOffsetToRowData() + finalOffset]);
                ImGui::InputFloat("", value);
            }
            else
            {
                i32* value = reinterpret_cast<i32*>(&fileBuffer->GetDataPointer()[file->GetBufferOffsetToRowData() + finalOffset]);
                ImGui::InputInt("", value);
            }

            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Separator();
    }

    ImGui::Columns(1);
    ImGui::Separator();
    //ImGui::PopStyleVar();
}
