#pragma once
// SmoothTrigger: state-stability layer for trigger behavior.
// Waits a random 30-120ms delay after the goal enters the crosshair
// before firing once. Used by the trigger pass.
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>

#include <chrono>
#include <cstdlib>
#include <ctime>

namespace Core {
namespace Features {
namespace Smooth {

class cSmoothTrigger {
public:
    void SetEnabled(bool on) {
        if (enabled_ == on)
            return;
        enabled_ = on;
        SeedOnce();
        if (on) {
            armed_ = false;
            hasGoal_ = false;
            OutputDebugStringA("[GHOST] SmoothTrigger started\n");
        } else {
            armed_ = false;
            hasGoal_ = false;
            OutputDebugStringA("[GHOST] SmoothTrigger stopped\n");
        }
    }

    bool IsEnabled() const { return enabled_; }

    // Caller reports whether the goal is under the crosshair now.
    void SetGoalOnCrosshair(bool on) {
        if (on && !hasGoal_) {
            hasGoal_ = true;
            armed_ = true;
            // Random delay 30-120ms, re-rolled with jitter each cycle.
            const int span = kMaxDelayMs - kMinDelayMs;
            delayMs_ = kMinDelayMs + (rand() % (span + 1));
            fireAt_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMs_);
        } else if (!on) {
            hasGoal_ = false;
            armed_ = false;
        }
    }

    void Tick() {
        Tick(reinterpret_cast<uintptr_t>(SDK::Pointers::pLocalPlayer));
    }

    // Fires at most once per armed cycle and never before the delay.
    void Tick(uintptr_t /*ped*/) {
        if (!enabled_ || !armed_ || !hasGoal_)
            return;
        const auto now = std::chrono::steady_clock::now();
        if (now < fireAt_)
            return;
        if (now < cooldownUntil_)
            return;

        INPUT down{};
        down.type = INPUT_MOUSE;
        down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &down, sizeof(down));
        INPUT up{};
        up.type = INPUT_MOUSE;
        up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &up, sizeof(up)); // single shot per cycle

        armed_ = false;
        hasGoal_ = false;
        cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
        OutputDebugStringA("[GHOST] SmoothTrigger complete\n");
    }

private:
    static constexpr int kMinDelayMs = 30;
    static constexpr int kMaxDelayMs = 120;
    static constexpr int kCooldownSec = 2;

    bool enabled_ = false;
    bool armed_ = false;
    bool hasGoal_ = false;
    int delayMs_ = 0;
    std::chrono::steady_clock::time_point fireAt_{};
    std::chrono::steady_clock::time_point cooldownUntil_{};

    static void SeedOnce() {
        static bool seeded = false;
        if (!seeded) { seeded = true; srand(static_cast<unsigned>(time(nullptr))); }
    }
};

inline cSmoothTrigger g_SmoothTrigger;

} // namespace Smooth
} // namespace Features
} // namespace Core
