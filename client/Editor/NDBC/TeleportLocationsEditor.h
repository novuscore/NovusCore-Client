#pragma once
#include <NovusTypes.h>
#include "NDBCEditorBase.h"

class TeleportLocationsEditor : public NDBCEditorBase
{
public:
    TeleportLocationsEditor();

    void CreateNewEntry() final;
    void DeleteEntry() final;
    void Draw() final;
};