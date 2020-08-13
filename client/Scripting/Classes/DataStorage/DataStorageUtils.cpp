#include "DataStorageUtils.h"
#include <entity/registry.hpp>
#include <angelscript.h>
#include "../../ScriptEngine.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/Singletons/DataStorageSingleton.h"

namespace ASDataStorageUtils
{
    void RegisterNamespace()
    {
        i32 r = ScriptEngine::RegisterScriptClass("void_ptr", sizeof(void*), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_PRIMITIVE);
        assert(r >= 0);

        r = ScriptEngine::SetNamespace("DataStorage");
        assert(r >= 0);
        {
            ScriptEngine::RegisterScriptFunction("bool PutU8(string name, uint8 val)", asFUNCTION(PutU8));
            ScriptEngine::RegisterScriptFunction("void EmplaceU8(string name, uint8 val)", asFUNCTION(EmplaceU8));
            ScriptEngine::RegisterScriptFunction("bool GetU8(string name, uint8 &out val)", asFUNCTION(GetU8));
            ScriptEngine::RegisterScriptFunction("bool ClearU8(string name)", asFUNCTION(ClearU8));

            ScriptEngine::RegisterScriptFunction("bool PutU16(string name, uint16 val)", asFUNCTION(PutU16));
            ScriptEngine::RegisterScriptFunction("void EmplaceU16(string name, uint16 val)", asFUNCTION(EmplaceU16));
            ScriptEngine::RegisterScriptFunction("bool GetU16(string name, uint16 &out val)", asFUNCTION(GetU16));
            ScriptEngine::RegisterScriptFunction("bool ClearU16(string name)", asFUNCTION(ClearU16));

            ScriptEngine::RegisterScriptFunction("bool PutU32(string name, uint val)", asFUNCTION(PutU32));
            ScriptEngine::RegisterScriptFunction("void EmplaceU32(string name, uint val)", asFUNCTION(EmplaceU32));
            ScriptEngine::RegisterScriptFunction("bool GetU32(string name, uint &out val)", asFUNCTION(GetU32));
            ScriptEngine::RegisterScriptFunction("bool ClearU32(string name)", asFUNCTION(ClearU32));

            ScriptEngine::RegisterScriptFunction("bool PutU64(string name, uint64 val)", asFUNCTION(PutU64));
            ScriptEngine::RegisterScriptFunction("void EmplaceU64(string name, uint64 val)", asFUNCTION(EmplaceU64));
            ScriptEngine::RegisterScriptFunction("bool GetU64(string name, uint64 &out val)", asFUNCTION(GetU64));
            ScriptEngine::RegisterScriptFunction("bool ClearU64(string name)", asFUNCTION(ClearU64));

            ScriptEngine::RegisterScriptFunction("bool PutF32(string name, float val)", asFUNCTION(PutF32));
            ScriptEngine::RegisterScriptFunction("void EmplaceF32(string name, float val)", asFUNCTION(EmplaceF32));
            ScriptEngine::RegisterScriptFunction("bool GetF32(string name, float &out val)", asFUNCTION(GetF32));
            ScriptEngine::RegisterScriptFunction("bool ClearF32(string name)", asFUNCTION(ClearF32));

            ScriptEngine::RegisterScriptFunction("bool PutF64(string name, double val)", asFUNCTION(PutF64));
            ScriptEngine::RegisterScriptFunction("void EmplaceF64(string name, double val)", asFUNCTION(EmplaceF64));
            ScriptEngine::RegisterScriptFunction("bool GetF64(string name, double &out val)", asFUNCTION(GetF64));
            ScriptEngine::RegisterScriptFunction("bool ClearF64(string name)", asFUNCTION(ClearF64));

            ScriptEngine::RegisterScriptFunction("bool PutString(string name, string val)", asFUNCTION(PutString));
            ScriptEngine::RegisterScriptFunction("void EmplaceString(string name, string val)", asFUNCTION(EmplaceString));
            ScriptEngine::RegisterScriptFunction("bool GetString(string name, string &out val)", asFUNCTION(GetString));
            ScriptEngine::RegisterScriptFunction("bool ClearString(string name)", asFUNCTION(ClearString));

            ScriptEngine::RegisterScriptFunction("bool PutPointer(string name, void_ptr val)", asFUNCTION(PutPointer));
            ScriptEngine::RegisterScriptFunction("void EmplacePointer(string name, void_ptr val)", asFUNCTION(EmplacePointer));
            ScriptEngine::RegisterScriptFunction("bool GetPointer(string name, void_ptr &out val)", asFUNCTION(GetPointer));
            ScriptEngine::RegisterScriptFunction("bool ClearPointer(string name)", asFUNCTION(ClearPointer));

            ScriptEngine::RegisterScriptFunction("bool PutEntity(string name, Entity val)", asFUNCTION(PutEntity));
            ScriptEngine::RegisterScriptFunction("void EmplaceEntity(string name, Entity val)", asFUNCTION(EmplaceEntity));
            ScriptEngine::RegisterScriptFunction("bool GetEntity(string name, Entity &out val)", asFUNCTION(GetEntity));
            ScriptEngine::RegisterScriptFunction("bool ClearEntity(string name)", asFUNCTION(ClearEntity));
        }

        r = ScriptEngine::ResetNamespace();
        assert(r >= 0);
    }

    bool PutU8(std::string name, u8 val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.PutU8(nameHash, val);
    }
    void EmplaceU8(std::string name, u8 val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        dataStorageSingleton.storage.EmplaceU8(nameHash, val);
    }
    bool GetU8(std::string name, u8& val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.GetU8(nameHash, val);
    }
    bool HasU8(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.HasU8(nameHash);
    }
    bool ClearU8(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.ClearU8(nameHash);
    }

    bool PutU16(std::string name, u16 val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.PutU16(nameHash, val);
    }
    void EmplaceU16(std::string name, u16 val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        dataStorageSingleton.storage.EmplaceU16(nameHash, val);
    }
    bool GetU16(std::string name, u16& val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.GetU16(nameHash, val);
    }
    bool HasU16(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.HasU16(nameHash);
    }
    bool ClearU16(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.ClearU16(nameHash);
    }

    bool PutU32(std::string name, u32 val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.PutU32(nameHash, val);
    }
    void EmplaceU32(std::string name, u32 val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        dataStorageSingleton.storage.EmplaceU32(nameHash, val);
    }
    bool GetU32(std::string name, u32& val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.GetU32(nameHash, val);
    }
    bool HasU32(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.HasU32(nameHash);
    }
    bool ClearU32(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.ClearU32(nameHash);
    }

    bool PutU64(std::string name, u64 val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.PutU64(nameHash, val);
    }
    void EmplaceU64(std::string name, u64 val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        dataStorageSingleton.storage.EmplaceU64(nameHash, val);
    }
    bool GetU64(std::string name, u64& val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.GetU64(nameHash, val);
    }
    bool HasU64(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.HasU64(nameHash);
    }
    bool ClearU64(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.ClearU64(nameHash);
    }

    bool PutF32(std::string name, f32 val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.PutF32(nameHash, val);
    }
    void EmplaceF32(std::string name, f32 val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        dataStorageSingleton.storage.EmplaceF32(nameHash, val);
    }
    bool GetF32(std::string name, f32& val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.GetF32(nameHash, val);
    }
    bool HasF32(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.HasF32(nameHash);
    }
    bool ClearF32(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.ClearF32(nameHash);
    }

    bool PutF64(std::string name, f64 val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.PutF64(nameHash, val);
    }
    void EmplaceF64(std::string name, f64 val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        dataStorageSingleton.storage.EmplaceF64(nameHash, val);
    }
    bool GetF64(std::string name, f64& val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.GetF64(nameHash, val);
    }
    bool HasF64(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.HasF64(nameHash);
    }
    bool ClearF64(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.ClearF64(nameHash);
    }

    bool PutString(std::string name, std::string val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.PutString(nameHash, val);
    }
    void EmplaceString(std::string name, std::string val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        dataStorageSingleton.storage.EmplaceString(nameHash, val);
    }
    bool GetString(std::string name, std::string& val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.GetString(nameHash, val);
    }
    bool HasString(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.HasString(nameHash);
    }
    bool ClearString(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.ClearString(nameHash);
    }

    bool PutPointer(std::string name, void* val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.PutPointer(nameHash, val);
    }
    void EmplacePointer(std::string name, void* val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        dataStorageSingleton.storage.EmplacePointer(nameHash, val);
    }
    bool GetPointer(std::string name, void*& val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.GetPointer(nameHash, val);
    }
    bool HasPointer(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.HasPointer(nameHash);
    }
    bool ClearPointer(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.ClearPointer(nameHash);
    }

    bool PutEntity(std::string name, entt::entity val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.PutEntity(nameHash, val);
    }
    void EmplaceEntity(std::string name, entt::entity val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        dataStorageSingleton.storage.EmplaceEntity(nameHash, val);
    }
    bool GetEntity(std::string name, entt::entity& val)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.GetEntity(nameHash, val);
    }
    bool HasEntity(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.HasEntity(nameHash);
    }
    bool ClearEntity(std::string name)
    {
        entt::registry* registry = ServiceLocator::GetGameRegistry();
        DataStorageSingleton& dataStorageSingleton = registry->ctx<DataStorageSingleton>();

        u32 nameHash = StringUtils::fnv1a_32(name.c_str(), name.length());
        return dataStorageSingleton.storage.ClearEntity(nameHash);
    }
}
