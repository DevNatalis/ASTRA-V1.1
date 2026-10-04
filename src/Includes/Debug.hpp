#pragma once
// STABILITY: tiny throttled diagnostic log. Visual-free, no gameplay logic.
// Uses OutputDebugStringA so it works even when the overlay/console is absent.
#include <string>
#include <unordered_map>
#include <cstdio>
#include <windows.h>

namespace Debug
{
	inline bool ShouldEmit(const char* tag, unsigned throttleMs = 2000)
	{
		static std::unordered_map<std::string, DWORD> last;
		static CRITICAL_SECTION cs;
		static bool init = false;
		if (!init) { InitializeCriticalSection(&cs); init = true; }
		DWORD now = GetTickCount();
		EnterCriticalSection(&cs);
		auto it = last.find(tag ? tag : "?");
		bool emit = (it == last.end()) || (now - it->second >= throttleMs);
		if (emit) last[tag ? tag : "?"] = now;
		LeaveCriticalSection(&cs);
		return emit;
	}

	inline void Log(const char* level, const char* func, const char* detail)
	{
		char buf[512];
		sprintf_s(buf, sizeof(buf), "[GHOST][%s] %s: %s\n",
			level ? level : "?", func ? func : "?", detail ? detail : "");
		OutputDebugStringA(buf);
	}

	inline void Info(const char* func, const char* detail, unsigned throttleMs = 2000)
	{
		char key[256]; sprintf_s(key, sizeof(key), "I|%s|%s", func ? func : "?", detail ? detail : "");
		if (!ShouldEmit(key, throttleMs)) return;
		Log("INFO", func, detail);
	}

	inline void Warning(const char* func, const char* detail, unsigned throttleMs = 2000)
	{
		char key[256]; sprintf_s(key, sizeof(key), "W|%s|%s", func ? func : "?", detail ? detail : "");
		if (!ShouldEmit(key, throttleMs)) return;
		Log("WARN", func, detail);
	}

	inline void Error(const char* func, const char* detail, unsigned throttleMs = 2000)
	{
		char key[256]; sprintf_s(key, sizeof(key), "E|%s|%s", func ? func : "?", detail ? detail : "");
		if (!ShouldEmit(key, throttleMs)) return;
		Log("ERROR", func, detail);
	}
}
