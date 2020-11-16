#include "DayNightSystem.h"
#include <time.h>
#include <Utils/ByteBuffer.h>
#include <Utils/StringUtils.h>
#include "../../Utils/ServiceLocator.h"
#include "../../Utils/MapUtils.h"
#include "../../Rendering/ClientRenderer.h"
#include "../Components/Singletons/TimeSingleton.h"
#include <imgui/imgui.h>
#include <CVar/CVarSystem.h>

AutoCVar_Float CVAR_DayNightTimeMultiplier("DayNight.timeMultiplier", "Controls how fast the time ticks", 1.0f);

void DayNightSystem::Init(entt::registry& registry)
{
    DayNightSingleton& dayNightSingleton = registry.set<DayNightSingleton>();

    SetInitialState(registry, dayNightSingleton.initialState);
}

void DayNightSystem::Update(entt::registry& registry)
{
    TimeSingleton& timeSingleton = registry.ctx<TimeSingleton>();
    DayNightSingleton& dayNightSingleton = registry.ctx<DayNightSingleton>();
    
    dayNightSingleton.seconds += (1.0f * CVAR_DayNightTimeMultiplier.GetFloat()) * timeSingleton.deltaTime;

    while (dayNightSingleton.seconds > DayNight_SecondsPerDay)
        dayNightSingleton.seconds -= DayNight_SecondsPerDay;

    if (ImGui::Begin("Clock"))
    {
        // Direct Time Manipulations
        {
            if (ImGui::Button("Reset Time"))
            {
                SetInitialState(registry, dayNightSingleton.initialState);
            }

            ImGui::SameLine();

            if (ImGui::Button("Set Time to Noon"))
            {
                dayNightSingleton.seconds = static_cast<f32>(DayNight_SecondsPerDay) / 2.0f;
            }
        }

        // Time Speed Manipulation
        {
            if (ImGui::Button("Reset Speed"))
            {
                CVAR_DayNightTimeMultiplier.Set(1.0);
            }

            ImGui::SameLine();

            ImGui::InputDouble("", CVAR_DayNightTimeMultiplier.GetPtr(), 1.0f, 10.f, "%.2f");
        }

        u32 totalSeconds = static_cast<u32>(dayNightSingleton.seconds);

        u32 hours = totalSeconds / 60 / 60;
        u32 mins = (totalSeconds / 60) - hours * 60;
        u32 seconds = totalSeconds - (hours * 60 * 60) - (mins * 60);

        std::string hoursStr = hours < 10 ? std::string("0").append(std::to_string(hours)) : std::to_string(hours);
        std::string minsStr = mins < 10 ? std::string("0").append(std::to_string(mins)) : std::to_string(mins);
        std::string secondsStr = seconds < 10 ? std::string("0").append(std::to_string(seconds)) : std::to_string(seconds);

        ImGui::Text("%s:%s:%s", hoursStr.c_str(), minsStr.c_str(), secondsStr.c_str());
    }

    ImGui::End();
}

void DayNightSystem::SetInitialState(entt::registry& registry, DayNightTimestamp newInitialState)
{
    DayNightSingleton& dayNightSingleton = registry.ctx<DayNightSingleton>();
    DayNightTimestamp& timeStamp = dayNightSingleton.initialState;

    timeStamp.secondsSinceMidnightUTC = newInitialState.secondsSinceMidnightUTC;
    timeStamp.timeOffsetInSeconds = newInitialState.timeOffsetInSeconds;

    i32 timeOffset = timeStamp.secondsSinceMidnightUTC + timeStamp.timeOffsetInSeconds;
    i32 secondsSinceDawnWithOffset = GetSecondsSinceMidnightUTC() + timeOffset;

    while (secondsSinceDawnWithOffset < 0 ||
           secondsSinceDawnWithOffset > DayNight_SecondsPerDay)
    {
        if (secondsSinceDawnWithOffset > DayNight_SecondsPerDay)
            secondsSinceDawnWithOffset -= DayNight_SecondsPerDay;

        if (secondsSinceDawnWithOffset < 0)
            secondsSinceDawnWithOffset += DayNight_SecondsPerDay;
    }

    dayNightSingleton.seconds = static_cast<f32>(secondsSinceDawnWithOffset);
}

u32 DayNightSystem::GetSecondsSinceMidnightUTC()
{
    time_t timeNow = std::time(nullptr);

#pragma warning(push)
#pragma warning(disable: 4996)
    tm* timestampUTC = std::gmtime(&timeNow);
#pragma warning(pop)

    u32 seconds = timestampUTC->tm_sec + (((timestampUTC->tm_hour * 60) + timestampUTC->tm_min) * 60);
    return seconds;
}
