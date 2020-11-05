#pragma once
#include <NovusTypes.h>

namespace Renderer
{
    namespace Commands
    {
        struct PushMarker
        {
            static const BackendDispatchFunction DISPATCH_FUNCTION;

            Color color = Color::White;

            static const u32 MARKER_MAX_LENGTH = 32;
            char marker[MARKER_MAX_LENGTH];
        };
    }
}