#pragma once
#include <NovusTypes.h>

namespace ASDataStorageUtils
{
    void RegisterNamespace();

    bool PutU8(std::string name, u8 val);
    bool GetU8(std::string name, u8& val);
    bool ClearU8(std::string name);

    bool PutU16(std::string name, u16 val);
    bool GetU16(std::string name, u16& val);
    bool ClearU16(std::string name);

    bool PutU32(std::string name, u32 val);
    bool GetU32(std::string name, u32& val);
    bool ClearU32(std::string name);

    bool PutU64(std::string name, u64 val);
    bool GetU64(std::string name, u64& val);
    bool ClearU64(std::string name);

    bool PutF32(std::string name, f32 val);
    bool GetF32(std::string name, f32& val);
    bool ClearF32(std::string name);

    bool PutF64(std::string name, f64 val);
    bool GetF64(std::string name, f64& val);
    bool ClearF64(std::string name);

    bool PutString(std::string name, std::string val);
    bool GetString(std::string name, std::string& val);
    bool ClearString(std::string name);

    bool PutPointer(std::string name, void* val);
    bool GetPointer(std::string name, void*& val);
    bool ClearPointer(std::string name);
};