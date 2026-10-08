#pragma once
// SmoothHandling: state-stability layer for the handling editor.
// Clamps every requested value to +20% of the vanilla snapshot and
// applies changes in 5% steps per tick. Used by handling fields such
// as acceleration and brakes.
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <unordered_map>

namespace Core {
namespace Features {
namespace Smooth {

class cSmoothHandling {
public:
    void SetEnabled(bool on) {
        if (enabled_ == on)
            return;
        enabled_ = on;
        SeedOnce();
        if (on) {
            lastTick_ = std::chrono::steady_clock::time_point{};
            OutputDebugStringA("[GHOST] SmoothHandling started\n");
        } else {
            pending_.clear();
            OutputDebugStringA("[GHOST] SmoothHandling stopped\n");
        }
    }

    bool IsEnabled() const { return enabled_; }

    // Queue a goal for a handling field (offset relative to handling).
    void RequestValue(uintptr_t fieldOff, float goal) {
        if (!enabled_)
            return;
        pending_[fieldOff] = goal;
    }

    void Tick() {
        CPed* p = SDK::Pointers::pLocalPlayer;
        Tick(reinterpret_cast<uintptr_t>(p));
    }

    void Tick(uintptr_t ped) {
        if (!enabled_ || ped == 0 || pending_.empty())
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

        if (!g_Offsets.m_LastVehicle || !g_Offsets.m_Handling)
            return;
        const uintptr_t veh = Mem.Read<uintptr_t>(ped + g_Offsets.m_LastVehicle);
        if (veh == 0 || veh < 0x10000)
            return;
        const uintptr_t handling = Mem.Read<uintptr_t>(veh + g_Offsets.m_Handling);
        if (handling == 0 || handling < 0x10000)
            return;

        // One field per tick: keeps at most one write per tick.
        auto it = pending_.begin();
        const uintptr_t fieldOff = it->first;
        float goal = it->second;

        float cur = Mem.Read<float>(handling + fieldOff);
        if (!std::isfinite(cur) || !std::isfinite(goal)) {
            pending_.erase(it);
            return;
        }

        // Vanilla snapshot + clamp to +20%.
        float vanilla = cur;
        auto vit = vanilla_.find(fieldOff);
        if (vit != vanilla_.end())
            vanilla = vit->second;
        else if (cur > 0.0001f)
            vanilla_[fieldOff] = cur;
        if (vanilla > 0.0001f) {
            const float limit = vanilla * 1.20f;
            if (goal > limit)
                goal = limit;
            it->second = goal;
        }

        if (cur == goal) {
            pending_.erase(it);
            if (pending_.empty()) {
                cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
                OutputDebugStringA("[GHOST] SmoothHandling complete\n");
            }
            return;
        }

        // 5% of vanilla per tick toward the goal, jittered +/-10%.
        float stepBase = (vanilla > 0.0001f ? vanilla * 0.05f : std::fabs(goal - cur) / 10.0f);
        float step = stepBase * JitterFactor();
        float next = cur + ((goal > cur) ? step : -step);
        if ((goal > cur && next > goal) || (goal < cur && next < goal))
            next = goal;
        if (next == cur)
            return;

        Mem.Write<float>(handling + fieldOff, next); // single write per tick
        if (next == goal) {
            pending_.erase(it);
            if (pending_.empty()) {
                cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
                OutputDebugStringA("[GHOST] SmoothHandling complete\n");
            }
        }
    }

private:
    static constexpr long long kTickMs = 30;
    static constexpr int kCooldownSec = 2;

    bool enabled_ = false;
    std::unordered_map<uintptr_t, float> pending_;
    std::unordered_map<uintptr_t, float> vanilla_;
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

inline cSmoothHandling g_SmoothHandling;

} // namespace Smooth
} // namespace Features
} // namespace Core
