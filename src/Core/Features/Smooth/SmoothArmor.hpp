#pragma once
// SmoothArmor: state-stability layer for armor restore.
// Same interpolation pattern as SmoothHealth. Ceiling = 100 or the
// configured value. Step = 15 * jitter (+/-10%). Cooldown 2s.
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

class cSmoothArmor {
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
            OutputDebugStringA("[GHOST] SmoothArmor started\n");
        } else {
            ticksLeft_ = 0;
            OutputDebugStringA("[GHOST] SmoothArmor stopped\n");
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

        // Validate offset via current SDK table.
        const uintptr_t armorOff = g_Offsets.m_Armor ? g_Offsets.m_Armor : kArmorFallback;
        if (armorOff == 0)
            return;

        float armor = Mem.Read<float>(ped + armorOff);
        if (!std::isfinite(armor))
            return;
        if (armor < 0.0f || armor > kSanityMax)
            return;

        const float target = Ceiling > 0.0f ? Ceiling : 100.0f;
        if (armor >= target) {
            if (ticksLeft_ != 0) {
                ticksLeft_ = 0;
                cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
                OutputDebugStringA("[GHOST] SmoothArmor complete\n");
            }
            return;
        }

        if (ticksLeft_ <= 0) {
            current_ = armor;
            ticksLeft_ = EstimateTicks(armor, target);
        } else {
            current_ = armor;
        }
        if (current_ == target)
            return;

        float diff = target - current_;
        float step = diff / static_cast<float>(ticksLeft_ > 0 ? ticksLeft_ : 1);
        float next = current_ + step * JitterFactor();
        if ((step > 0.0f && next > target) || (step < 0.0f && next < target))
            next = target;
        if (next == current_)
            return;

        Mem.Write<float>(ped + armorOff, next); // single write per tick
        ticksLeft_--;
        if (next >= target) {
            ticksLeft_ = 0;
            cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
            OutputDebugStringA("[GHOST] SmoothArmor complete\n");
        }
    }

private:
    static constexpr uintptr_t kArmorFallback = 0x150C;
    static constexpr float kSanityMax = 100000.0f;
    static constexpr float kBaseDelta = 15.0f;
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
        int n = static_cast<int>(std::ceil(span / kBaseDelta));
        if (n < 1) n = 1;
        if (n > 20) n = 20;
        return n;
    }
};

inline cSmoothArmor g_SmoothArmor;

} // namespace Smooth
} // namespace Features
} // namespace Core
