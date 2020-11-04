#pragma once
#include <entity/fwd.hpp>
#include "../Components/Singletons/DayNightSingleton.h"

constexpr u32 DayNight_SecondsPerDay = 24 * 60 * 60;
class DayNightSystem
{
public:
    static void Init(entt::registry& registry);
    static void Update(entt::registry& registry);

    static void SetInitialState(entt::registry& registry, DayNightTimestamp newInitialState);
    static u32 GetSecondsSinceMidnightUTC();
};