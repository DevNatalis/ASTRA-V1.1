#pragma once
// SmoothVehicleHealth: state-stability layer for engine-health restore.
// Applies to the current vehicle (ped GetLastVehicle). Same stepped
// pattern as health/armor. Used by vehicle god-state and repair.
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>

namespace Core {
namespace Features {
namespace Smooth {

class cSmoothVehicleHealth {
public:
    float Ceiling = 1000.0f;

    void SetEnabled(bool on) {
        if (enabled_ == on)
            return;
        enabled_ = on;
        SeedOnce();
        if (on) {
            ticksLeft_ = 0;
            lastTick_ = std::chrono::steady_clock::time_point{};
            OutputDebugStringA("[GHOST] SmoothVehicleHealth started\n");
        } else {
            ticksLeft_ = 0;
            OutputDebugStringA("[GHOST] SmoothVehicleHealth stopped\n");
        }
    }

    bool IsEnabled() const { return enabled_; }

    void Tick() {
        CPed* p = SDK::Pointers::pLocalPlayer;
        Tick(reinterpret_cast<uintptr_t>(p));
    }

    void Tick(uintptr_t ped) {
        if (!enabled_ || ped == 0)
            return;
        if (!Mem.ProcHandle || Mem.ProcHandle == INVALID_HANDLE_VALUE)
            return;
        const auto now = std::chrono::steady_clock::now();
        if (lastTick_ != std::chrono::steady_clock::time_point{} &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick_).count() < kTickMs)
            return;
        if (now < cooldownUntil_)
            return;
        lastTick_ = now;

        // Validate offsets via current SDK table.
        if (!g_Offsets.m_LastVehicle)
            return;
        const uintptr_t engOff = g_Offsets.m_VehicleEngineHealth ? g_Offsets.m_VehicleEngineHealth : 0x280u /* routed via g_Offsets */;
        if (engOff == 0)
            return;

        const uintptr_t veh = Mem.Read<uintptr_t>(ped + g_Offsets.m_LastVehicle);
        if (veh == 0 || veh < 0x10000)
            return;

        float eng = Mem.Read<float>(veh + engOff);
        if (!std::isfinite(eng))
            return;
        if (eng >= Ceiling) {
            if (ticksLeft_ != 0) {
                ticksLeft_ = 0;
                cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
                OutputDebugStringA("[GHOST] SmoothVehicleHealth complete\n");
            }
            return;
        }
        if (eng < -5000.0f || eng > 100000.0f)
            return;

        if (ticksLeft_ <= 0) {
            current_ = eng;
            ticksLeft_ = EstimateTicks(eng, Ceiling);
        } else {
            current_ = eng;
        }
        if (current_ == Ceiling)
            return;

        float diff = Ceiling - current_;
        float step = diff / static_cast<float>(ticksLeft_ > 0 ? ticksLeft_ : 1);
        float next = current_ + step * JitterFactor();
        if (next > Ceiling)
            next = Ceiling;
        if (next == current_)
            return;

        Mem.Write<float>(veh + engOff, next); // single write per tick
        ticksLeft_--;
        if (next >= Ceiling) {
            ticksLeft_ = 0;
            cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
            OutputDebugStringA("[GHOST] SmoothVehicleHealth complete\n");
        }
    }

private:
    static constexpr long long kTickMs = 30;
    static constexpr int kCooldownSec = 2;

    bool enabled_ = false;
    float current_ = 0.0f;
    int ticksLeft_ = 0;
    std::chrono::steady_clock::time_point lastTick_{};
    std::chrono::steady_clock::time_point cooldownUntil_{};

    static void SeedOnce() {
        static bool seeded = false;
        if (!seeded) { seeded = true; srand(static_cast<unsigned>(time(nullptr))); }
    }
    static float JitterFactor() {
        SeedOnce();
        const float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        return 1.0f + (r * 2.0f - 1.0f) * 0.10f;
    }
    static int EstimateTicks(float from, float to) {
        const float span = to - from;
        const float per = (to > 0.0f ? to * 0.15f : 150.0f);
        if (per <= 0.0f)
            return 6;
        int n = static_cast<int>(std::ceil(span / per));
        if (n < 1) n = 1;
        if (n > 20) n = 20;
        return n;
    }
};

inline cSmoothVehicleHealth g_SmoothVehicleHealth;

} // namespace Smooth
} // namespace Features
} // namespace Core
