#pragma once
// STABILITY: minimal remote-memory validation helpers.
// Goal: never touch the game when a dependency is invalid — skip safely,
// log once (throttled), retry only when state becomes valid again.
// No try/catch, no invented offsets, no blind writes.
#include <Includes/Debug.hpp>
#include <Includes/Includes.hpp>
#include <cstdint>

namespace Guard
{
	// Remote (game-side) pointer sanity. Rejects null/low/high garbage.
	inline bool IsRemotePtr(uintptr_t addr)
	{
		if (addr == 0) return false;
		if (addr < 0x10000) return false;
#if defined(_WIN64)
		if (addr > 0x7FFFFFFFFFFFULL) return false;
#else
		if (addr > 0xFFFFFFF0ULL) return false;
#endif
		return true;
	}

	// Offset sanity: 0 means "not resolved yet" — never Read(0+off).
	inline bool IsOffset(uintptr_t off) { return off != 0; }

	// Clamp list counts coming from game memory to a safe range.
	inline int ClampCount(int v, int hi)
	{
		if (v < 0) return 0;
		if (v > hi) return hi;
		return v;
	}

	// Process/memory readiness. Mem/ProcId declared in Globals/Memory.
	inline bool ProcReady()
	{
		if (!::Core::Mem.ProcHandle || ::Core::Mem.ProcHandle == INVALID_HANDLE_VALUE)
			return false;
		if (::g_Variables.ProcIdFiveM == 0) return false;
		if (::Core::Mem.ModBase == 0) return false;
		return true;
	}
}
