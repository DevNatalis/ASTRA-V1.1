#pragma once
#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <windows.h>
#include <thread>
#include <Includes/CustomWidgets/Notify.hpp>

namespace Core
{
	namespace Threads
	{
		class cScreenResolution
		{
		public:
			auto _stdcall MoveAndSetResolution() -> void
			{
				g_Variables.g_hGameWindow = FindWindowA(xorstr("grcWindow"), nullptr);

				if (!g_Config.General->SecondMonitor) {
					auto WindowInfo = Utils::GetWindowPosAndSize(g_Variables.g_hGameWindow);
					g_Variables.g_vGameWindowSize = WindowInfo.second;
					g_Variables.g_vGameWindowPos = WindowInfo.first;
					g_Variables.g_vGameWindowCenter = { g_Variables.g_vGameWindowSize.x / 2, g_Variables.g_vGameWindowSize.y / 2 };

					// RESPONSIVENESS: MoveWindow(..., repaint=true) every 300ms
					// forces a full repaint/DWM churn even when nothing moved.
					// Only reposition on actual change.
					static ImVec2 lastPos{ -1.0f, -1.0f };
					static ImVec2 lastSize{ -1.0f, -1.0f };
					const ImVec2& pos = g_Variables.g_vGameWindowPos;
					const ImVec2& size = g_Variables.g_vGameWindowSize;
					if (pos.x == lastPos.x && pos.y == lastPos.y &&
						size.x == lastSize.x && size.y == lastSize.y)
						return;
					lastPos = pos;
					lastSize = size;

					MoveWindow(g_Variables.g_hCheatWindow, static_cast<int>(g_Variables.g_vGameWindowPos.x), static_cast<int>(g_Variables.g_vGameWindowPos.y),
						static_cast<int>(g_Variables.g_vGameWindowSize.x), static_cast<int>(g_Variables.g_vGameWindowSize.y) - 1, true);
				}
			}

			auto _stdcall Update() -> void
			{
				static int monitor_check_counter = 0;
				static bool last_monitor_state = true;

				while (true)
				{
					try
					{
						if (monitor_check_counter % 34 == 0) {
							auto monitors = GetAllMonitors();
							bool has_second_monitor = monitors.size() >= 2;

							if (!has_second_monitor && g_Config.General->SecondMonitor) {
								g_Config.General->SecondMonitor = false; //caso esse "false" nao esteja funcionando a func do segundo monitor, voce coloca em "true"

								//MessageBox(NULL, xorstr("Second Monitor not Found!"), xorstr("Warning"), MB_ICONWARNING);
							}

							last_monitor_state = g_Config.General->SecondMonitor;
						}
						monitor_check_counter++;

						MoveAndSetResolution();
					}
					catch (std::exception e)
					{
						continue;
					}

					std::this_thread::sleep_for(std::chrono::milliseconds(300));
				}
			}

			static std::vector<HMONITOR> GetAllMonitors() {
				std::vector<HMONITOR> monitors;
				EnumDisplayMonitors(NULL, NULL, [](HMONITOR hMonitor, HDC, LPRECT, LPARAM lParam) -> BOOL {
					auto* monitors = reinterpret_cast<std::vector<HMONITOR>*>(lParam);
					monitors->push_back(hMonitor);
					return TRUE;
					}, reinterpret_cast<LPARAM>(&monitors));
				return monitors;
			}

			static std::pair<POINT, POINT> GetMonitorDimensions(HMONITOR hMonitor) {
				MONITORINFO mi = { sizeof(MONITORINFO) };
				GetMonitorInfo(hMonitor, &mi);
				return {
					{ mi.rcMonitor.left, mi.rcMonitor.top },
					{ mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top }
				};
			}

			static HMONITOR GetSecondaryMonitor() {
				auto monitors = GetAllMonitors();
				if (monitors.size() > 1) {
					return monitors[1];
				}
				return monitors.empty() ? NULL : monitors[0];
			}

			static HMONITOR GetPrimaryMonitor() {
				auto monitors = GetAllMonitors();
				return monitors.empty() ? NULL : monitors[0];
			}

			static auto _stdcall GetTargetMonitorInfo(bool useSecondMonitor, POINT& pos, POINT& size) -> void {
				HMONITOR targetMonitor = useSecondMonitor ? GetSecondaryMonitor() : GetPrimaryMonitor();

				if (targetMonitor) {
					auto [monitorPos, monitorSize] = GetMonitorDimensions(targetMonitor);
					pos = monitorPos;
					size = monitorSize;
				}
				else {
					pos = { static_cast<LONG>(g_Variables.g_vGameWindowPos.x), static_cast<LONG>(g_Variables.g_vGameWindowPos.y) };
					size = { static_cast<LONG>(g_Variables.g_vGameWindowSize.x), static_cast<LONG>(g_Variables.g_vGameWindowSize.y) };
				}
			}

			static auto _stdcall MapPrimaryToSecondary(ImVec2& primaryPos, ImVec2& mappedPos) -> void {
				HMONITOR primary = GetPrimaryMonitor();
				HMONITOR secondary = GetSecondaryMonitor();

				if (!primary || !secondary || primary == secondary) {
					mappedPos = primaryPos;
					return;
				}

				auto [primaryPosInfo, primarySize] = GetMonitorDimensions(primary);
				auto [secondaryPosInfo, secondarySize] = GetMonitorDimensions(secondary);

				float xRatio = static_cast<float>(secondarySize.x) / primarySize.x;
				float yRatio = static_cast<float>(secondarySize.y) / primarySize.y;

				mappedPos.x = primaryPos.x * xRatio + secondaryPosInfo.x;
				mappedPos.y = primaryPos.y * yRatio + secondaryPosInfo.y;
			}
		};

		inline cScreenResolution g_ScreenResolution;
	}
}