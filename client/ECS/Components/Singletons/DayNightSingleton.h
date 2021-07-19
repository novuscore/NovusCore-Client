#pragma once
#include <NovusTypes.h>

struct DayNightTimestamp
{
    u32 secondsSinceMidnightUTC = 0; // This variable defines time since midnight using seconds (0...86400)
    i32 timeOffsetInSeconds = 0; // This variable specifies and offset to the above allowing us to adjust it on
};

struct DayNightSingleton
{
    DayNightTimestamp initialState;
    f32 seconds = 0;
};