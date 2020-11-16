#pragma once
#include <NovusTypes.h>

class NDBCEditorBase
{
public:
    virtual void CreateNewEntry() { }
    virtual void DeleteEntry() { }

    virtual void Draw() { }
};