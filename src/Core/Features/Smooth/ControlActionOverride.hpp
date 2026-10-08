#pragma once
// ControlActionOverride: control-action layer for the weapon wheel.
// Resolves the DISABLE_CONTROL_ACTION native handler through its
// joaat hash in the CFX table, enumerates modules of the game PID
// (not the host) via Core::Mem, and installs a RET stub on the
// handler. Used by the force weapon-wheel pass.
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>

#include <TlHelp32.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

namespace Core {
namespace Features {
namespace Smooth {

class cControlActionOverride {
public:
    void SetEnabled(bool on) {
        if (enabled_ == on)
            return;
        enabled_ = on;
        SeedOnce();
        if (on) {
            lastTick_ = std::chrono::steady_clock::time_point{};
            OutputDebugStringA("[GHOST] ControlActionOverride started\n");
        } else {
            Restore();
            OutputDebugStringA("[GHOST] ControlActionOverride stopped\n");
        }
    }

    bool IsEnabled() const { return enabled_; }

    void Tick() {
        CPed* p = SDK::Pointers::pLocalPlayer;
        Tick(reinterpret_cast<uintptr_t>(p));
    }

    // Keeps the RET stub installed; re-applies if the byte changed.
    // At most one write per tick and never when already patched.
    void Tick(uintptr_t /*ped*/) {
        if (!enabled_)
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

        if (!handler_) {
            handler_ = ResolveHandler();
            if (!handler_)
                return;
            original_ = Mem.Read<uint8_t>(handler_);
            OutputDebugStringA("[GHOST] ControlActionOverride handler resolved\n");
        }
        const uint8_t cur = Mem.Read<uint8_t>(handler_);
        if (cur == kRet)
            return; // already in the goal state: no write
        Mem.Write<uint8_t>(handler_, kRet); // single write per tick
    }

private:
    static constexpr uint8_t kRet = 0xC3;
    static constexpr long long kTickMs = 30;
    static constexpr int kCooldownSec = 2;
    // joaat("DISABLE_CONTROL_ACTION").
    static constexpr uint32_t kHash = 0xFE99B66D;

    bool enabled_ = false;
    uintptr_t handler_ = 0;
    uint8_t original_ = 0;
    std::chrono::steady_clock::time_point lastTick_{};
    std::chrono::steady_clock::time_point cooldownUntil_{};

    static void SeedOnce() {
        static bool seeded = false;
        if (!seeded) { seeded = true; srand(static_cast<unsigned>(time(nullptr))); }
    }

    void Restore() {
        if (handler_ && original_ && original_ != kRet)
            Mem.Write<uint8_t>(handler_, original_);
        handler_ = 0;
        original_ = 0;
    }

    static uint32_t Joaat(const char* s) {
        uint32_t h = 0;
        while (*s) {
            h += static_cast<uint8_t>(*s++);
            h += (h << 10);
            h ^= (h >> 6);
        }
        h += (h << 3);
        h ^= (h >> 11);
        h += (h << 15);
        return h;
    }

    // Enumerate modules of the game PID (Toolhelp snapshot on the game
    // PID only, never the host) and scan the CFX native table for the
    // handler matching kHash. Falls back to 0 when absent.
    uintptr_t ResolveHandler() {
        (void)Joaat("DISABLE_CONTROL_ACTION");
        // Game PID only (never the host): prefer the PID behind the
        // Core::Mem handle so the snapshot always matches Mem reads.
        const DWORD pid = Mem.ProcId ? Mem.ProcId : g_Variables.ProcIdFiveM;
        if (!pid)
            return 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap == INVALID_HANDLE_VALUE)
            return 0;
        MODULEENTRY32W me{};
        me.dwSize = sizeof(me);
        uintptr_t found = 0;
        if (Module32FirstW(snap, &me)) {
            do {
                const uintptr_t base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                const uintptr_t size = static_cast<uintptr_t>(me.modBaseSize);
                found = ScanModuleForHash(base, size);
                if (found)
                    break;
            } while (Module32NextW(snap, &me));
        }
        CloseHandle(snap);
        return found;
    }

    static uintptr_t ScanModuleForHash(uintptr_t base, uintptr_t size) {
        if (!base || !size || size > 0x40000000)
            return 0;
        // Small-window scan: look for the joaat slot then read the
        // handler pointer stored next to it.
        const size_t window = 0x1000;
        std::vector<uint8_t> buf = Mem.ReadBytes(base, window);
        if (buf.size() != window)
            return 0;
        for (size_t i = 0; i + 16 <= buf.size(); ++i) {
            uint32_t v = 0;
            memcpy(&v, buf.data() + i, sizeof(v));
            if (v == kHash) {
                uintptr_t handlerPtr = 0;
                memcpy(&handlerPtr, buf.data() + i + 8, sizeof(uintptr_t) > 8 ? 8 : sizeof(uintptr_t));
                if (handlerPtr > 0x10000 && handlerPtr < 0x7FFFFFFFFFFF)
                    return handlerPtr;
            }
        }
        return 0;
    }
};

inline cControlActionOverride g_ControlActionOverride;

} // namespace Smooth
} // namespace Features
} // namespace Core
