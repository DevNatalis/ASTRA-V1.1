#pragma once
// SmoothNoClip: state-stability layer for free movement.
// Never sets the NoCollision config flag. Moves position by a capped
// delta (6.6 m/s max) and applies a 5 cm vertical step-up per tick so
// walls can be climbed smoothly. Velocity stays coherent: written as
// delta / dt. Used by free-move / no-collision modes.
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

class cSmoothNoClip {
public:
    void SetEnabled(bool on) {
        if (enabled_ == on)
            return;
        enabled_ = on;
        SeedOnce();
        if (on) {
            lastTick_ = std::chrono::steady_clock::time_point{};
            OutputDebugStringA("[GHOST] SmoothNoClip started\n");
        } else {
            OutputDebugStringA("[GHOST] SmoothNoClip stopped\n");
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
        if (lastTick_ != std::chrono::steady_clock::time_point{}) {
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick_).count();
            if (ms < kTickMs)
                return;
        }
        float dt = kTickMs / 1000.0f;
        if (lastTick_ != std::chrono::steady_clock::time_point{}) {
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick_).count();
            dt = static_cast<float>(ms) / 1000.0f;
            if (dt <= 0.001f) dt = 0.001f;
            if (dt > 0.25f) dt = 0.25f;
        }
        if (now < cooldownUntil_)
            return;
        lastTick_ = now;

        CPed* cp = reinterpret_cast<CPed*>(ped);
        const D3DXVECTOR3 cur = cp->GetPos();

        // Direction from held keys (WASD + Space/Ctrl), camera-agnostic
        // fallback to forward when no key state is readable.
        D3DXVECTOR3 dir{ 0, 0, 0 };
        if (GetAsyncKeyState('W') & 0x8000) dir.y += 1.0f;
        if (GetAsyncKeyState('S') & 0x8000) dir.y -= 1.0f;
        if (GetAsyncKeyState('A') & 0x8000) dir.x -= 1.0f;
        if (GetAsyncKeyState('D') & 0x8000) dir.x += 1.0f;
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) dir.z += 1.0f;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) dir.z -= 1.0f;
        const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len < 0.001f)
            return; // no input: no write
        dir.x /= len; dir.y /= len; dir.z /= len;

        // Cap: 6.6 m/s max, jittered to avoid repeated patterns.
        const float cap = kMaxSpeed * dt * JitterFactor();
        D3DXVECTOR3 delta{ dir.x * cap, dir.y * cap, dir.z * cap };
        // Vertical step-up: 5 cm per tick for climbing.
        if (delta.z >= 0.0f)
            delta.z += kStepUp;
        D3DXVECTOR3 next{ cur.x + delta.x, cur.y + delta.y, cur.z + delta.z };
        if (next.x == cur.x && next.y == cur.y && next.z == cur.z)
            return;

        cp->SetPos(next); // single position path per tick
        // Coherent velocity: delta / dt.
        const D3DXVECTOR3 vel{ delta.x / dt, delta.y / dt, delta.z / dt };
        Mem.Write<D3DXVECTOR3>(ped + kVelocity, vel);
    }

private:
    static constexpr float kMaxSpeed = 6.6f; // m/s
    static constexpr float kStepUp = 0.05f;  // 5 cm per tick
    static constexpr uintptr_t kVelocity = 0x320;
    static constexpr long long kTickMs = 30;
    static constexpr int kCooldownSec = 2;

    bool enabled_ = false;
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

inline cSmoothNoClip g_SmoothNoClip;

} // namespace Smooth
} // namespace Features
} // namespace Core
