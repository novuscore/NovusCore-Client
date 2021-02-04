#pragma once
#include <NovusTypes.h>
#include <entity/entity.hpp>
#include <shared_mutex>

#include "../../Scripting/ScriptEngine.h"
#include <Utils/DebugHandler.h>

namespace UIScripting
{
    enum class LockState : u8
    {
        NONE,
        READ,
        WRITE,
        COUNT
    };

    class LockToken
    {
    public:
        LockToken(std::shared_mutex& mutex, LockState state) : _mutex(mutex), _lockState(state)
        {
            if (state == LockState::READ)
            {
                _mutex.lock_shared();
            }
            else if (state == LockState::WRITE)
            {
                _mutex.lock();
            }
            else
            {
                DebugHandler::PrintError("Attempted to request LockToken with invalid LockState(%u)", static_cast<u8>(state));
            }
        }

        // Used for Angelscript
        static void RegisterType()
        {
            i32 r = ScriptEngine::RegisterScriptClass("LockToken", 0, asOBJ_REF | asOBJ_NOCOUNT);
            assert(r >= 0);
            {
                r = ScriptEngine::RegisterScriptClassFunction("void Unlock()", asMETHOD(LockToken, Unlock)); assert(r >= 0);
            }
        }

        void Unlock()
        {
            if (_lockState == LockState::READ)
                _mutex.unlock_shared();
            else if (_lockState == LockState::WRITE)
                _mutex.unlock();

            delete this;
        }

    protected:
        std::shared_mutex& _mutex;
        LockState _lockState = LockState::NONE;
    };
}
