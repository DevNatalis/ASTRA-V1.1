#pragma once
// SmoothTeleport: state-stability layer for position changes.
// Splits the route into steps of at most 5 meters per tick (30ms) and
// interpolates linearly from the start to the goal. Applies via the
// SDK SetPos path. Used by waypoint/fixed-point/world teleports.
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

class cSmoothTeleport {
public:
    void SetEnabled(bool on) {
        if (enabled_ == on)
            return;
        enabled_ = on;
        if (on) {
            SeedOnce();
            lastTick_ = std::chrono::steady_clock::time_point{};
            OutputDebugStringA("[GHOST] SmoothTeleport started\n");
        } else {
            active_ = false;
            stepsLeft_ = 0;
            OutputDebugStringA("[GHOST] SmoothTeleport stopped\n");
        }
    }

    bool IsEnabled() const { return enabled_; }
    bool IsActive() const { return active_; }

    void RequestTeleport(D3DXVECTOR3 goal) {
        CPed* p = SDK::Pointers::pLocalPlayer;
        if (!p)
            return;
        const D3DXVECTOR3 from = p->GetPos();
        const float dist = Length(goal - from);
        if (dist < 0.01f)
            return;
        start_ = from;
        goal_ = goal;
        stepsTotal_ = static_cast<int>(std::ceil(dist / kMaxStep));
        if (stepsTotal_ < 1) stepsTotal_ = 1;
        if (stepsTotal_ > 200) stepsTotal_ = 200;
        stepsLeft_ = stepsTotal_;
        active_ = true;
        lastTick_ = std::chrono::steady_clock::time_point{};
        OutputDebugStringA("[GHOST] SmoothTeleport route started\n");
    }

    void Tick() {
        CPed* p = SDK::Pointers::pLocalPlayer;
        Tick(reinterpret_cast<uintptr_t>(p));
    }

    void Tick(uintptr_t ped) {
        if (!enabled_ || !active_ || ped == 0)
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

        CPed* cp = reinterpret_cast<CPed*>(ped);
        const D3DXVECTOR3 cur = cp->GetPos();
        // Auto-disable on exact arrival: current == target means the
        // route is done — never keep pushing position after that.
        if (cur.x == goal_.x && cur.y == goal_.y && cur.z == goal_.z) {
            active_ = false;
            stepsLeft_ = 0;
            cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
            OutputDebugStringA("[GHOST] SmoothTeleport complete\n");
            return;
        }
        const D3DXVECTOR3 diff{ goal_.x - cur.x, goal_.y - cur.y, goal_.z - cur.z };
        const float dist = Length(diff);
        if (dist < 0.05f) {
            active_ = false;
            stepsLeft_ = 0;
            cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
            OutputDebugStringA("[GHOST] SmoothTeleport complete\n");
            return;
        }

        // Interpolation: linear blend start -> goal by progress, plus
        // clamp so no single tick covers more than kMaxStep meters.
        const int done = stepsTotal_ - stepsLeft_;
        float t = static_cast<float>(done + 1) / static_cast<float>(stepsTotal_ > 0 ? stepsTotal_ : 1);
        if (t > 1.0f) t = 1.0f;
        D3DXVECTOR3 blend{
            start_.x + (goal_.x - start_.x) * t,
            start_.y + (goal_.y - start_.y) * t,
            start_.z + (goal_.z - start_.z) * t,
        };
        // Clamp per-tick travel with jitter on the cap (+/-10%).
        const float cap = kMaxStep * JitterFactor();
        const D3DXVECTOR3 hop{ blend.x - cur.x, blend.y - cur.y, blend.z - cur.z };
        const float hopLen = Length(hop);
        D3DXVECTOR3 next = blend;
        if (hopLen > cap && hopLen > 0.0001f) {
            const float k = cap / hopLen;
            next = D3DXVECTOR3{ cur.x + hop.x * k, cur.y + hop.y * k, cur.z + hop.z * k };
        }
        if (next.x == cur.x && next.y == cur.y && next.z == cur.z)
            return;

        cp->SetPos(next); // single write path per tick via SDK
        stepsLeft_--;
        if (stepsLeft_ <= 0 || Length(goal_ - next) < 0.1f) {
            active_ = false;
            stepsLeft_ = 0;
            cooldownUntil_ = now + std::chrono::seconds(kCooldownSec);
            OutputDebugStringA("[GHOST] SmoothTeleport complete\n");
        }
    }

private:
    static constexpr float kMaxStep = 5.0f; // meters per tick
    static constexpr long long kTickMs = 30;
    static constexpr int kCooldownSec = 2;

    bool enabled_ = false;
    bool active_ = false;
    D3DXVECTOR3 start_{ 0, 0, 0 };
    D3DXVECTOR3 goal_{ 0, 0, 0 };
    int stepsTotal_ = 0;
    int stepsLeft_ = 0;
    std::chrono::steady_clock::time_point lastTick_{};
    std::chrono::steady_clock::time_point cooldownUntil_{};

    static float Length(const D3DXVECTOR3& v) {
        return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
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

inline cSmoothTeleport g_SmoothTeleport;

} // namespace Smooth
} // namespace Features
} // namespace Core
