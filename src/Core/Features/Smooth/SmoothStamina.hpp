#pragma once
// SmoothStamina: state-stability layer for stamina restore.
// Step = 20 * jitter (+/-10%). No cooldown: upkeep pass only writes
// while stamina is below the ceiling and never when equal.
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

class cSmoothStamina {
public:
    float Ceiling = 100.0f;

    void SetEnabled(bool on) {
        if (enabled_ == on)
            return;
        enabled_ = on;
        SeedOnce();
        if (on) {
            ticksLeft_ = 0;
            lastTick_ = std::chrono::steady_clock::time_point{};
            OutputDebugStringA("[GHOST] SmoothStamina started\n");
        } else {
            ticksLeft_ = 0;
            OutputDebugStringA("[GHOST] SmoothStamina stopped\n");
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
        lastTick_ = now;

        if (!g_Offsets.m_PlayerInfo)
            return;
        const uintptr_t info = Mem.Read<uintptr_t>(ped + g_Offsets.m_PlayerInfo);
        if (info == 0 || info < 0x10000)
            return;

        float stam = Mem.Read<float>(info + kStamina);
        if (!std::isfinite(stam))
            return;
        if (stam < 0.0f || stam > 100000.0f)
            return;
        if (stam >= Ceiling) {
            if (ticksLeft_ != 0) {
                ticksLeft_ = 0;
                OutputDebugStringA("[GHOST] SmoothStamina complete\n");
            }
            return;
        }

        if (ticksLeft_ <= 0) {
            current_ = stam;
            ticksLeft_ = EstimateTicks(stam, Ceiling);
        } else {
            current_ = stam;
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

        Mem.Write<float>(info + kStamina, next); // single write per tick
        ticksLeft_--;
        if (next >= Ceiling) {
            ticksLeft_ = 0;
            OutputDebugStringA("[GHOST] SmoothStamina complete\n");
        }
    }

private:
    static constexpr uintptr_t kStamina = 0xCF4; // relative to CPlayerInfo
    static constexpr float kBaseDelta = 20.0f;
    static constexpr long long kTickMs = 30;

    bool enabled_ = false;
    float current_ = 0.0f;
    int ticksLeft_ = 0;
    std::chrono::steady_clock::time_point lastTick_{};

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
        int n = static_cast<int>(std::ceil(span / kBaseDelta));
        if (n < 1) n = 1;
        if (n > 20) n = 20;
        return n;
    }
};

inline cSmoothStamina g_SmoothStamina;

} // namespace Smooth
} // namespace Features
} // namespace Core
