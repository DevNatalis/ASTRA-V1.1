#pragma once
// SmoothRecoil: state-stability layer for recoil reduction.
// Applies locally via WeaponManager -> WeaponInfo -> Recoil offset and
// fades from the vanilla value to the reduced value in 5 ticks with
// jitter (+/-10%). Used by recoil control.
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

class cSmoothRecoil {
public:
    float Factor = 0.0f; // 0 = no recoil, 1 = vanilla

    void SetEnabled(bool on) {
        if (enabled_ == on)
            return;
        enabled_ = on;
        SeedOnce();
        if (on) {
            CaptureVanilla();
            ticksLeft_ = kFadeTicks;
            lastTick_ = std::chrono::steady_clock::time_point{};
            OutputDebugStringA("[GHOST] SmoothRecoil started\n");
        } else {
            RestoreVanilla();
            ticksLeft_ = 0;
            OutputDebugStringA("[GHOST] SmoothRecoil stopped\n");
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

        const uintptr_t info = ResolveWeaponInfo(ped);
        if (!info)
            return;
        const uintptr_t off = g_Offsets.m_Recoil;
        if (off == 0)
            return;

        float cur = Mem.Read<float>(info + off);
        if (!std::isfinite(cur))
            return;
        const float goal = vanilla_ * Factor;
        if (cur == goal) {
            if (ticksLeft_ != 0) {
                ticksLeft_ = 0;
                cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
                OutputDebugStringA("[GHOST] SmoothRecoil complete\n");
            }
            return;
        }

        float diff = goal - cur;
        float step = diff / static_cast<float>(ticksLeft_ > 0 ? ticksLeft_ : 1);
        float next = cur + step * JitterFactor();
        if ((step > 0.0f && next > goal) || (step < 0.0f && next < goal))
            next = goal;
        if (next == cur)
            return;

        Mem.Write<float>(info + off, next); // single write per tick
        if (ticksLeft_ > 0)
            ticksLeft_--;
        if (next == goal) {
            ticksLeft_ = 0;
            cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
            OutputDebugStringA("[GHOST] SmoothRecoil complete\n");
        }
    }

private:
    static constexpr int kFadeTicks = 5;
    static constexpr long long kTickMs = 30;
    static constexpr int kCooldownSec = 2;

    bool enabled_ = false;
    float vanilla_ = 1.0f;
    bool hasVanilla_ = false;
    int ticksLeft_ = 0;
    std::chrono::steady_clock::time_point lastTick_{};
    std::chrono::steady_clock::time_point cooldownUntil_{};

    static uintptr_t ResolveWeaponInfo(uintptr_t ped) {
        if (!g_Offsets.m_WeaponManager)
            return 0;
        const uintptr_t mgr = Mem.Read<uintptr_t>(ped + g_Offsets.m_WeaponManager);
        if (mgr == 0 || mgr < 0x10000)
            return 0;
        const uintptr_t info = Mem.Read<uintptr_t>(mgr + 0x20);
        if (info == 0 || info < 0x10000)
            return 0;
        return info;
    }

    void CaptureVanilla() {
        CPed* p = SDK::Pointers::pLocalPlayer;
        if (!p || !g_Offsets.m_Recoil) {
            hasVanilla_ = false;
            return;
        }
        const uintptr_t info = ResolveWeaponInfo(reinterpret_cast<uintptr_t>(p));
        if (!info)
            return;
        const float v = Mem.Read<float>(info + g_Offsets.m_Recoil);
        if (std::isfinite(v) && v > 0.0f) {
            vanilla_ = v;
            hasVanilla_ = true;
        }
    }

    void RestoreVanilla() {
        if (!hasVanilla_)
            return;
        CPed* p = SDK::Pointers::pLocalPlayer;
        if (!p)
            return;
        const uintptr_t info = ResolveWeaponInfo(reinterpret_cast<uintptr_t>(p));
        if (info && g_Offsets.m_Recoil)
            Mem.Write<float>(info + g_Offsets.m_Recoil, vanilla_);
        hasVanilla_ = false;
    }

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

inline cSmoothRecoil g_SmoothRecoil;

} // namespace Smooth
} // namespace Features
} // namespace Core
