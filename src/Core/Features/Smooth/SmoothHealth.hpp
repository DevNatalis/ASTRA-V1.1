#pragma once
// SmoothHealth: state-stability layer for health restore.
// Interpolates health toward the ceiling in small steps instead of
// writing the final value at once. Used by auto-revive, max-health
// heal and the revive button.
//
// Pattern:
//   - SetEnabled(bool) arms/disarms the pass.
//   - Tick(uintptr_t ped) runs once per frame (called from Gui.cpp).
//   - Step: delta = maxHealth * 0.15 scaled by jitter (+/-10%).
//   - Order: MaxHealth before Health (never health > max in a frame).
//   - Never writes when current == target.
//   - At most one write per tick. Throttle 30ms. Cooldown 2s at ceiling.
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace Core {
namespace Features {
namespace Smooth {

class cSmoothHealth {
public:
    void SetEnabled(bool on) {
        if (enabled_ == on)
            return;
        enabled_ = on;
        SeedOnce();
        if (on) {
            ticksLeft_ = 0;
            current_ = 0.0f;
            target_ = 0.0f;
            lastTick_ = std::chrono::steady_clock::time_point{};
            OutputDebugStringA("[GHOST] SmoothHealth started\n");
        } else {
            ticksLeft_ = 0;
            OutputDebugStringA("[GHOST] SmoothHealth stopped\n");
        }
    }

    bool IsEnabled() const { return enabled_; }

    // Resolve ped internally (main-loop convenience).
    void Tick() {
        CPed* p = SDK::Pointers::pLocalPlayer;
        Tick(reinterpret_cast<uintptr_t>(p));
    }

    // One interpolation step. Called once per frame from Gui.cpp.
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

        // Fixed layout: MaxHealth sits at kHealth + 4 (0x284). No lookup
        // in the SDK offset table, so this pass keeps working when the
        // table has no MaxHealth entry for the running build.
        const uintptr_t maxOff = kMaxHealth;

        float health = Mem.Read<float>(ped + kHealth);
        float maxHealth = Mem.Read<float>(ped + maxOff);
        if (!std::isfinite(health) || !std::isfinite(maxHealth))
            return;
        if (maxHealth <= 0.0f || maxHealth > kSanityMax)
            return;
        if (health < -1.0f || health > kSanityMax)
            return;

        // Already at ceiling: nothing to write, arm cooldown once.
        if (health >= maxHealth) {
            if (ticksLeft_ != 0 || current_ != 0.0f) {
                ticksLeft_ = 0;
                current_ = 0.0f;
                cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
                OutputDebugStringA("[GHOST] SmoothHealth complete\n");
            }
            return;
        }

        // New cycle: snapshot target = ceiling.
        if (ticksLeft_ <= 0) {
            current_ = health;
            target_ = maxHealth;
            ticksLeft_ = EstimateTicks(health, maxHealth);
        } else {
            // Track external changes (damage taken mid-cycle).
            current_ = health;
            target_ = maxHealth;
        }

        if (current_ == target_)
            return;

        // Interpolation: (target - current) / ticks * step, jitter +/-10%.
        float diff = target_ - current_;
        float step = diff / static_cast<float>(ticksLeft_ > 0 ? ticksLeft_ : 1);
        float next = current_ + step * JitterFactor();
        if ((step > 0.0f && next > target_) || (step < 0.0f && next < target_))
            next = target_;
        if (next == current_)
            return;
        if (next > maxHealth)
            next = maxHealth;

        // Order: MaxHealth first so health never exceeds max in a frame.
        const float curMax = Mem.Read<float>(ped + maxOff);
        if (curMax < maxHealth - 0.1f)
            Mem.Write<float>(ped + maxOff, maxHealth);

        Mem.Write<float>(ped + kHealth, next); // single write per tick
        ticksLeft_--;

        if (next >= maxHealth) {
            ticksLeft_ = 0;
            cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
            OutputDebugStringA("[GHOST] SmoothHealth complete\n");
        }
    }

private:
    static constexpr uintptr_t kHealth = 0x280;
    static constexpr uintptr_t kMaxHealth = kHealth + 4; // 0x284 fixed
    static constexpr float kSanityMax = 100000.0f;
    static constexpr long long kTickMs = 30;
    static constexpr int kCooldownSec = 2;

    bool enabled_ = false;
    float current_ = 0.0f;
    float target_ = 0.0f;
    int ticksLeft_ = 0;
    std::chrono::steady_clock::time_point lastTick_{};
    std::chrono::steady_clock::time_point cooldownUntil_{};

    static void SeedOnce() {
        static bool seeded = false;
        if (!seeded) {
            seeded = true;
            srand(static_cast<unsigned>(time(nullptr)));
        }
    }

    // +/-10% scale on the step to avoid repeated patterns.
    static float JitterFactor() {
        SeedOnce();
        const float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); // 0..1
        return 1.0f + (r * 2.0f - 1.0f) * 0.10f;
    }

    static int EstimateTicks(float from, float to) {
        const float span = to - from;
        const float per = (to > 0.0f ? to * 0.15f : 30.0f);
        if (per <= 0.0f)
            return 6;
        int n = static_cast<int>(std::ceil(span / per));
        if (n < 1) n = 1;
        if (n > 20) n = 20;
        return n;
    }
};

inline cSmoothHealth g_SmoothHealth;

} // namespace Smooth
} // namespace Features
} // namespace Core
