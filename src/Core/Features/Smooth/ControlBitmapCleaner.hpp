#pragma once
// ControlBitmapCleaner: fallback control-state pass for the weapon wheel.
// Locates the control bitmap inside CPed (+0x1000..0x1800) and clears
// bits 37, 157, 158, 24, 25 every 30ms. Re-scans when the ped changes.
// Falls back to CPlayerInfo (ped + m_PlayerInfo) when not found in CPed.
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>

#include <chrono>
#include <cstdint>
#include <vector>

namespace Core {
namespace Features {
namespace Smooth {

class cControlBitmapCleaner {
public:
    void SetEnabled(bool on) {
        if (enabled_ == on)
            return;
        enabled_ = on;
        if (on) {
            cachedPed_ = 0;
            cachedBitmap_ = 0;
            lastTick_ = std::chrono::steady_clock::time_point{};
            OutputDebugStringA("[GHOST] ControlBitmapCleaner started\n");
        } else {
            cachedPed_ = 0;
            cachedBitmap_ = 0;
            OutputDebugStringA("[GHOST] ControlBitmapCleaner stopped\n");
        }
    }

    bool IsEnabled() const { return enabled_; }

    void Tick() {
        CPed* p = SDK::Pointers::pLocalPlayer;
        Tick(reinterpret_cast<uintptr_t>(p));
    }

    // Single clear pass per tick; no write when all goal bits are clear.
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

        if (!cachedBitmap_ || cachedPed_ != ped) {
            cachedBitmap_ = CPed::ScanDisabledControlsBitmap(ped);
            if (!cachedBitmap_ && g_Offsets.m_PlayerInfo) {
                const uintptr_t info = Mem.Read<uintptr_t>(ped + g_Offsets.m_PlayerInfo);
                if (info > 0x10000)
                    cachedBitmap_ = CPed::ScanDisabledControlsBitmap(info);
            }
            cachedPed_ = ped;
            if (!cachedBitmap_)
                return;
            OutputDebugStringA("[GHOST] ControlBitmapCleaner bitmap resolved\n");
        }

        std::vector<uint8_t> buf = Mem.ReadBytes(cachedBitmap_, kBitmapSize);
        if (buf.size() != kBitmapSize) {
            cachedBitmap_ = 0; // force rescan next tick
            return;
        }
        bool changed = false;
        for (uint32_t ctrl : kTargets) {
            const size_t bi = ctrl / 8;
            const uint32_t bp = ctrl % 8;
            if (bi >= buf.size())
                continue;
            if (buf[bi] & (1u << bp)) {
                buf[bi] &= static_cast<uint8_t>(~(1u << bp));
                changed = true;
            }
        }
        if (!changed)
            return; // already at goal: no write
        Mem.WriteBytes(cachedBitmap_, buf); // single write per tick
    }

private:
    static constexpr uintptr_t kScanStart = 0x1000;
    static constexpr uintptr_t kScanEnd = 0x1800;
    static constexpr size_t kBitmapSize = 48;
    static constexpr uint32_t kTargets[] = { 37, 157, 158, 24, 25 };
    static constexpr long long kTickMs = 30;
    static constexpr int kCooldownSec = 2;

    bool enabled_ = false;
    uintptr_t cachedPed_ = 0;
    uintptr_t cachedBitmap_ = 0;
    std::chrono::steady_clock::time_point lastTick_{};
    std::chrono::steady_clock::time_point cooldownUntil_{};
};

inline cControlBitmapCleaner g_ControlBitmapCleaner;

} // namespace Smooth
} // namespace Features
} // namespace Core
