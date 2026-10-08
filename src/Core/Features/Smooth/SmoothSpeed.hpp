#pragma once
// SmoothSpeed: state-stability layer for the speed modifier.
// Clamp: value never exceeds 1.49 (vanilla-safe ceiling). Fades from
// 1.0 to the goal in 10 ticks on enable and back to 1.0 in 10 ticks
// on disable. Used by fast-run.
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

class cSmoothSpeed {
public:
    float Goal = 1.30f;

    void SetEnabled(bool on) {
        if (enabled_ == on)
            return;
        enabled_ = on;
        SeedOnce();
        // Fade length is fixed: 10 interpolation ticks each way.
        ticksLeft_ = kFadeTicks;
        lastTick_ = std::chrono::steady_clock::time_point{};
        if (on)
            OutputDebugStringA("[GHOST] SmoothSpeed started\n");
        else
            OutputDebugStringA("[GHOST] SmoothSpeed fade-out started\n");
    }

    bool IsEnabled() const { return enabled_; }

    void Tick() {
        CPed* p = SDK::Pointers::pLocalPlayer;
        Tick(reinterpret_cast<uintptr_t>(p));
    }

    void Tick(uintptr_t ped) {
        if (ped == 0)
            return;
        if (!Mem.ProcHandle || Mem.ProcHandle == INVALID_HANDLE_VALUE)
            return;
        // Keep fading out after disable until back at 1.0.
        if (!enabled_ && settled_)
            return;
        const auto now = std::chrono::steady_clock::now();
        if (lastTick_ != std::chrono::steady_clock::time_point{} &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick_).count() < kTickMs)
            return;
        if (now < cooldownUntil_)
            return;
        lastTick_ = now;

        if (!g_Offsets.m_PlayerInfo || !g_Offsets.m_Speed)
            return;
        const uintptr_t info = Mem.Read<uintptr_t>(ped + g_Offsets.m_PlayerInfo);
        if (info == 0 || info < 0x10000)
            return;

        float cur = Mem.Read<float>(info + g_Offsets.m_Speed);
        if (!std::isfinite(cur))
            return;
        if (cur < 0.05f || cur > 10.0f)
            cur = 1.0f; // recover from stale reads

        float goal = enabled_ ? Goal : 1.0f;
        if (goal > kCeiling)
            goal = kCeiling; // vanilla-safe clamp
        if (goal < 1.0f)
            goal = 1.0f;
        if (cur == goal) {
            settled_ = !enabled_;
            if (!enabled_) {
                cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
                OutputDebugStringA("[GHOST] SmoothSpeed complete\n");
            }
            return;
        }

        settled_ = false;
        float diff = goal - cur;
        float step = diff / static_cast<float>(ticksLeft_ > 0 ? ticksLeft_ : 1);
        float next = cur + step * JitterFactor();
        if ((step > 0.0f && next > goal) || (step < 0.0f && next < goal))
            next = goal;
        if (next > kCeiling)
            next = kCeiling;
        if (next == cur)
            return;

        Mem.Write<float>(info + g_Offsets.m_Speed, next); // single write per tick
        if (ticksLeft_ > 0)
            ticksLeft_--;
        if (next == goal) {
            ticksLeft_ = kFadeTicks;
            if (!enabled_) {
                settled_ = true;
                cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
                OutputDebugStringA("[GHOST] SmoothSpeed complete\n");
            }
        }
    }

private:
    static constexpr float kCeiling = 1.49f;
    static constexpr int kFadeTicks = 10;
    static constexpr long long kTickMs = 30;
    static constexpr int kCooldownSec = 2;

    bool enabled_ = false;
    bool settled_ = true;
    int ticksLeft_ = kFadeTicks;
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
};

inline cSmoothSpeed g_SmoothSpeed;

} // namespace Smooth
} // namespace Features
} // namespace Core
