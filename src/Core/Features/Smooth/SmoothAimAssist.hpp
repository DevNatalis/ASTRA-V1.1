#pragma once
// SmoothAimAssist: state-stability layer for aim assistance.
// Moves the cursor by delta / smoothing factor with jitter (+/-10%)
// and only while the goal is inside the FOV window (pixels). Used by
// the aim pass.
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

class cSmoothAimAssist {
public:
    float Smoothing = 4.0f; // divisor applied to the delta
    float FovPx = 150.0f;   // goal must be within this radius

    void SetEnabled(bool on) {
        if (enabled_ == on)
            return;
        enabled_ = on;
        SeedOnce();
        if (on) {
            lastTick_ = std::chrono::steady_clock::time_point{};
            OutputDebugStringA("[GHOST] SmoothAimAssist started\n");
        } else {
            OutputDebugStringA("[GHOST] SmoothAimAssist stopped\n");
        }
    }

    bool IsEnabled() const { return enabled_; }

    // Goal in screen pixels (e.g. resolved by the caller).
    void SetGoal(int x, int y) {
        goalX_ = x;
        goalY_ = y;
        hasGoal_ = true;
    }

    void ClearGoal() { hasGoal_ = false; }

    void Tick() {
        Tick(reinterpret_cast<uintptr_t>(SDK::Pointers::pLocalPlayer));
    }

    // One interpolated mouse step per tick toward the goal.
    void Tick(uintptr_t /*ped*/) {
        if (!enabled_ || !hasGoal_)
            return;
        const auto now = std::chrono::steady_clock::now();
        if (lastTick_ != std::chrono::steady_clock::time_point{} &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick_).count() < kTickMs)
            return;
        if (now < cooldownUntil_)
            return;
        lastTick_ = now;

        POINT cur{};
        if (!GetCursorPos(&cur))
            return;
        const float dx = static_cast<float>(goalX_ - cur.x);
        const float dy = static_cast<float>(goalY_ - cur.y);
        const float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > FovPx)
            return; // outside FOV window: no write
        if (dist < 0.5f) {
            hasGoal_ = false;
            cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
            OutputDebugStringA("[GHOST] SmoothAimAssist complete\n");
            return;
        }

        const float div = Smoothing > 0.5f ? Smoothing : 4.0f;
        const float j = JitterFactor();
        const int mx = static_cast<int>((dx / div) * j);
        const int my = static_cast<int>((dy / div) * j);
        if (mx == 0 && my == 0)
            return; // already at goal: no write

        INPUT in{};
        in.type = INPUT_MOUSE;
        in.mi.dwFlags = MOUSEEVENTF_MOVE;
        in.mi.dx = mx;
        in.mi.dy = my;
        SendInput(1, &in, sizeof(in)); // single input per tick
    }

private:
    static constexpr long long kTickMs = 30;
    static constexpr int kCooldownSec = 2;

    bool enabled_ = false;
    bool hasGoal_ = false;
    int goalX_ = 0;
    int goalY_ = 0;
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

inline cSmoothAimAssist g_SmoothAimAssist;

} // namespace Smooth
} // namespace Features
} // namespace Core
