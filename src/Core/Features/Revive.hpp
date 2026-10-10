#pragma once
#include <Core/SDK/SDK.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>

namespace Core::Features {
// Called only from the render thread, including UI commands.
class Revive {
    using Clock = std::chrono::steady_clock;
public:
    // Health lives 4 bytes below MaxHealth on current builds.
    static uintptr_t HealthOff() {
        return g_Offsets.m_MaxHealth ? (g_Offsets.m_MaxHealth - 4u) : 0x280u;
    }
    bool Enabled() const { return enabled_; }
    void SetEnabled(bool on) {
        enabled_ = on;
        if (!on) { restoring_ = false; pending_ = false; }
    }
    void ForceRevive() {
        if (Clock::now() >= cooldownUntil_) pending_ = true;
    }

    void Tick() {
        const auto now = Clock::now();
        if ((!enabled_ && !pending_ && !restoring_) ||
            now < cooldownUntil_ || now < nextTick_) return;
        nextTick_ = now + std::chrono::milliseconds(30);

        uintptr_t factory = 0, ped = 0;
        if (!g_Offsets.m_World || !g_Offsets.m_MaxHealth ||
            !Read(g_Offsets.m_World, factory) || !ValidPointer(factory) ||
            !Read(factory + 0x8, ped) || !ValidPointer(ped)) {
            ResetPed();
            return;
        }
        if (ped != ped_) {
            // Never transfer an in-progress restore to a replacement ped.
            if (ped_) pending_ = false;
            restoring_ = false;
            ped_ = ped;
        }

        float health = 0, maximum = 0;
        const uintptr_t hOff = HealthOff();
        if (!Read(ped + hOff, health) ||
            !Read(ped + g_Offsets.m_MaxHealth, maximum) ||
            !std::isfinite(health) || !std::isfinite(maximum) ||
            maximum <= 0.f || maximum > 100000.f) {
            ResetPed();
            return;
        }
        if (!restoring_) {
            if (!pending_ && health > 0.f) return;
            restoring_ = true;
            pending_ = false;
        }

        // Raise the maximum first; never lower a server-supplied maximum.
        constexpr float minimumMaximum = 200.f;
        if (maximum < minimumMaximum) {
            if (!Write(ped + g_Offsets.m_MaxHealth, minimumMaximum)) return;
            if (!Read(ped + g_Offsets.m_MaxHealth, maximum) ||
                !std::isfinite(maximum) || maximum < minimumMaximum ||
                maximum > 100000.f) return;
        }
        if (health >= maximum) { Complete(now); return; }

        const float delta = maximum * .15f * (1.f + jitter_(random_));
        const float target = (std::min)(maximum, health + delta);
        if (!std::isfinite(target) || target <= health) return;
        if (!Write(ped + hOff, target)) return;
        char message[128];
        sprintf_s(message, "[GHOST] revive: health %.1f -> %.1f\n", health, target);
        OutputDebugStringA(message);

        float observed = 0;
        if (target >= maximum && Read(ped + hOff, observed) &&
            std::isfinite(observed) && observed >= maximum) Complete(now);
    }

private:
    template<class T> static bool Read(uintptr_t address, T& value) {
        if (!Mem.ProcHandle || Mem.ProcHandle == INVALID_HANDLE_VALUE) return false;
        const auto bytes = Mem.ReadBytes(address, sizeof(T));
        if (bytes.size() != sizeof(T)) return false;
        std::memcpy(&value, bytes.data(), sizeof(T));
        return true;
    }
    template<class T> static bool Write(uintptr_t address, const T& value) {
        std::vector<uint8_t> bytes(sizeof(T));
        std::memcpy(bytes.data(), &value, sizeof(T));
        return Mem.WriteBytes(address, bytes) != FALSE;
    }
    static bool ValidPointer(uintptr_t pointer) {
        return pointer > 0x10000 && pointer < 0x7FFFFFFFFFFF;
    }
    void ResetPed() { ped_ = 0; restoring_ = false; pending_ = false; }
    void Complete(Clock::time_point now) {
        restoring_ = false;
        pending_ = false;
        cooldownUntil_ = now + std::chrono::seconds(2);
        OutputDebugStringA("[GHOST] revive completo\n");
    }

    bool enabled_ = false, pending_ = false, restoring_ = false;
    uintptr_t ped_ = 0;
    Clock::time_point nextTick_{}, cooldownUntil_{};
    std::mt19937 random_{std::random_device{}()};
    std::uniform_real_distribution<float> jitter_{-.10f, .10f};
};
inline Revive g_Revive;
}
