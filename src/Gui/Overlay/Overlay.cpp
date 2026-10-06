#pragma once
#include "Overlay.hpp"

#include <Includes/ImGui/Files/imgui_freetype.h>
#include <Includes/CustomWidgets/Notify.hpp>
#include <Includes/ImGui/Images.hpp>
#include <Includes/ImGui/Fonts.hpp>
#include <Core/Core.hpp>
#include <Gui/Gui.hpp>
#include <dwmapi.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <thread>
#include <ImGui/Awesome/font_awesome.cpp>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Gui
{
	namespace
	{
		constexpr LONG_PTR kOverlayBaseStyle = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW;

		void SetOverlayInteractive(HWND window, bool interactive)
		{
			LONG_PTR style = kOverlayBaseStyle;
			if (!interactive)
				style |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;

			if (GetWindowLongPtrW(window, GWL_EXSTYLE) != style)
			{
				SetWindowLongPtrW(window, GWL_EXSTYLE, style);
				SetWindowPos(window, HWND_TOPMOST, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
			}
		}
	}

	void Overlay::Render()
	{
		WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, Overlay::WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L" ", NULL };
		ATOM RegClass = RegisterClassExW(&wc);

		g_Variables.g_hCheatWindow = CreateWindowExW(
			static_cast<DWORD>(kOverlayBaseStyle | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE),
			wc.lpszClassName,
			L" ",
			WS_POPUP,
			static_cast<int>(g_Variables.g_vGameWindowPos.x),
			static_cast<int>(g_Variables.g_vGameWindowPos.y),
			static_cast<int>(g_Variables.g_vGameWindowSize.x),
			static_cast<int>(g_Variables.g_vGameWindowSize.y),
			NULL, NULL, wc.hInstance, NULL);

		if (!g_Variables.g_hCheatWindow)
		{
			UnregisterClassW(wc.lpszClassName, wc.hInstance);
			return;
		}

		SetLayeredWindowAttributes(g_Variables.g_hCheatWindow, RGB(0, 0, 0), 255, LWA_ALPHA);
		const MARGINS margins = { -1 };
		DwmExtendFrameIntoClientArea(g_Variables.g_hCheatWindow, &margins);

		if (!CreateDeviceD3D(g_Variables.g_hCheatWindow))
		{
			CleanupDeviceD3D();
			UnregisterClassW(wc.lpszClassName, wc.hInstance);
			return;
		}

		ShowWindow(g_Variables.g_hCheatWindow, SW_SHOWDEFAULT);
		UpdateWindow(g_Variables.g_hCheatWindow);

		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui::StyleColorsDark();

		auto& style = ImGui::GetStyle();
		style.FramePadding = ImVec2(1, 0);
		style.FrameRounding = 3;
		style.WindowRounding = 10.0f;
		style.WindowBorderSize = 0;
		style.ScrollbarRounding = 4;
		style.ScrollbarSize = 4.0f;
		style.WindowPadding = ImVec2(0, 0);
		style.ItemSpacing = ImVec2(8, 4);

		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0x12 / 255.f, 0x12 / 255.f, 0x12 / 255.f, 0.96f);
		style.Colors[ImGuiCol_Border] = ImVec4(0x24 / 255.f, 0x24 / 255.f, 0x24 / 255.f, 1.f);
		style.Colors[ImGuiCol_Text] = ImVec4(0xF5 / 255.f, 0xF5 / 255.f, 0xF5 / 255.f, 1.f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0x8A / 255.f, 0x8A / 255.f, 0x8A / 255.f, 1.f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0x0E / 255.f, 0x0E / 255.f, 0x0E / 255.f, 0.80f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0x18 / 255.f, 0x18 / 255.f, 0x18 / 255.f, 0.85f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0x18 / 255.f, 0x18 / 255.f, 0x18 / 255.f, 0.90f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0xF5 / 255.f, 0xF5 / 255.f, 0xF5 / 255.f, 1.f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 212.f / 255.f, 0.f, 1.f);
		style.Colors[ImGuiCol_Button] = ImVec4(0x12 / 255.f, 0x12 / 255.f, 0x12 / 255.f, 0.90f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 224.f / 255.f, 51.f / 255.f, 1.f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 212.f / 255.f, 0.f, 1.f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 212.f / 255.f, 0.f, 1.f);
		style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 212.f / 255.f, 0.f, 0.10f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 212.f / 255.f, 0.f, 0.18f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 212.f / 255.f, 0.f, 0.28f);
		style.Colors[ImGuiCol_Separator] = ImVec4(0x24 / 255.f, 0x24 / 255.f, 0x24 / 255.f, 1.f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0x4A / 255.f, 0x4A / 255.f, 0x4A / 255.f, 1.f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.0f, 212.f / 255.f, 0.f, 1.f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.0f, 212.f / 255.f, 0.f, 1.f);

		ImFontConfig font_config;
		font_config.PixelSnapH = false;
		font_config.OversampleH = 5;
		font_config.OversampleV = 5;
		font_config.RasterizerMultiply = 1.2f;


		Style();
		Fonts();

		ImGui_ImplWin32_Init(g_Variables.g_hCheatWindow);
		ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

		Core::StartThreads();
		if (!Core::ThreadsStarted) return;

		//std::thread([&]() { NotifyManager::Send(xorstr("We are here!"), 4000); }).detach();

		static RECT old_rc;
		ZeroMemory(&Message, sizeof(MSG));

		while (!CloseRequested)
		{
			while (PeekMessage(&Message, nullptr, 0, 0, PM_REMOVE)) {
				if (Message.message == WM_QUIT) { CloseRequested = true; break; }
				TranslateMessage(&Message);
				DispatchMessage(&Message);
			}
			if (CloseRequested) break;

			HWND ActiveWindow = GetForegroundWindow();

			if (GetAsyncKeyState(g_Config.General->MenuKey) & 1)
			{
				if (g_MenuInfo.IsOpen)
					g_MenuInfo.IsOpen = false;
				else if (ActiveWindow == g_Variables.g_hGameWindow)
					g_MenuInfo.IsOpen = true;
			}

			// Make overlay interactive / visible when menu is open OR when user is not logged (show login screen)
			const bool should_show = g_MenuInfo.IsOpen || !g_MenuInfo.IsLogged;
			// If user is not logged, force interactivity so login fields can receive input.
			const bool interactive = should_show && (g_MenuInfo.IsLogged ?
				(ActiveWindow == g_Variables.g_hGameWindow || ActiveWindow == g_Variables.g_hCheatWindow) :
				true);
			if (g_MenuInfo.IsOpen && !interactive)
				g_MenuInfo.IsOpen = false;

			SetOverlayInteractive(g_Variables.g_hCheatWindow, interactive);

			static float BgAlpha = 0.f;
			BgAlpha = ImLerp(BgAlpha, should_show ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 8);

			// Diagnostic logging (throttled): log relevant state changes ~1s or when values change
			{
				static bool last_IsOpen = false;
				static bool last_IsLogged = false;
				static bool last_should_show = false;
				static bool last_interactive = false;
				static float last_BgAlpha = -1.0f;
				static DWORD last_tick = 0;

				DWORD now = GetTickCount();
				bool changed = (last_IsOpen != g_MenuInfo.IsOpen) || (last_IsLogged != g_MenuInfo.IsLogged) || (last_should_show != should_show) || (last_interactive != interactive) || (fabs(last_BgAlpha - BgAlpha) > 0.01f);
				if (changed || now - last_tick > 1000)
				{
					last_tick = now;
					last_IsOpen = g_MenuInfo.IsOpen;
					last_IsLogged = g_MenuInfo.IsLogged;
					last_should_show = should_show;
					last_interactive = interactive;
					last_BgAlpha = BgAlpha;

					char buf[1024];
					sprintf_s(buf, sizeof(buf), "[Overlay] IsOpen=%d IsLogged=%d should_show=%d interactive=%d BgAlpha=%.3f ActiveWindow=0x%p g_hGameWindow=0x%p g_hCheatWindow=0x%p\n",
						g_MenuInfo.IsOpen ? 1 : 0,
						g_MenuInfo.IsLogged ? 1 : 0,
						should_show ? 1 : 0,
						interactive ? 1 : 0,
						BgAlpha,
						(void*)ActiveWindow,
						(void*)g_Variables.g_hGameWindow,
						(void*)g_Variables.g_hCheatWindow);
					OutputDebugStringA(buf);
				}
			}

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			{
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, BgAlpha);

				// Draw fullscreen dim only when the in-game menu is open (post-login).
				if (g_MenuInfo.IsLogged && g_MenuInfo.IsOpen) {
					ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(g_Variables.g_vGameWindowSize.x, g_Variables.g_vGameWindowSize.y), ImColor(0.f, 0.f, 0.f, BgAlpha >= 0.4f ? 0.4f : BgAlpha));
				}
				Gui::Rendering();

				ImGui::PopStyleVar();
			}
			ImGui::EndFrame();

			const float ClearColor[4] = { 0 };
			g_pd3dDeviceContext->OMSetRenderTargets(1U, &g_mainRenderTargetView, NULL);
			g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, ClearColor);

			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

			g_pSwapChain->Present(g_Config.General->VSync, 0U); //VSync
		}


		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		CleanupDeviceD3D();
		DestroyWindow(g_Variables.g_hCheatWindow);
		UnregisterClassW(wc.lpszClassName, wc.hInstance);
	}

	void Overlay::Fonts()
	{
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.IniFilename = nullptr;

		ImFontConfig Cfg;
		Cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint | ImGuiFreeTypeBuilderFlags_LightHinting | ImGuiFreeTypeBuilderFlags_LoadColor;
		Cfg.FontDataOwnedByAtlas = false;

		ImFontConfig DrawCfg;
		DrawCfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint | ImGuiFreeTypeBuilderFlags_Bitmap;
		DrawCfg.FontDataOwnedByAtlas = false;


		g_Variables.lexend_font = io.Fonts->AddFontFromMemoryTTF(lexend, lexend_size, 17.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic());
		g_Variables.lexend_fontbig = io.Fonts->AddFontFromMemoryTTF(lexend, lexend_size, 20.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic());
		g_Variables.lexend_fontsmall = io.Fonts->AddFontFromMemoryTTF(lexend, lexend_size, 14.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic());

		static const ImWchar icons_ranges[] = { 0xe000, 0xf8ff, 0 };
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;

		g_Variables.font_awesome = io.Fonts->AddFontFromMemoryCompressedTTF(font_awesome_data, font_awesome_size, 16.f, &icons_config, icons_ranges);


		g_Variables.m_FontBig = io.Fonts->AddFontFromMemoryTTF(InterSemiBold, sizeof(InterSemiBold), 20.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic());
		g_Variables.m_FontBigSmall = io.Fonts->AddFontFromMemoryTTF(InterSemiBold, sizeof(InterSemiBold), 18.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic());
		g_Variables.m_FontNormal = io.Fonts->AddFontFromMemoryTTF(InterSemiBold, sizeof(InterSemiBold), 18.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic());
		g_Variables.m_FontSecundary = io.Fonts->AddFontFromMemoryTTF(InterSemiBold, sizeof(InterSemiBold), 16.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic());
		g_Variables.m_FontSmaller = io.Fonts->AddFontFromMemoryTTF(InterSemiBold, sizeof(InterSemiBold), 14.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic());

		g_Variables.m_DrawFont = io.Fonts->AddFontFromMemoryTTF(InterSemiBold, sizeof(InterSemiBold), 12.f, &DrawCfg, io.Fonts->GetGlyphRangesCyrillic());

		static const ImWchar FontAwesomeRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		static const ImWchar FontAwesomeRangesBrands[] = { ICON_MIN_FAB, ICON_MAX_FAB, 0 };

		ImFontConfig FontAwesomeConfig;
		FontAwesomeConfig.PixelSnapH = true;
		FontAwesomeConfig.GlyphMinAdvanceX = 19;

		g_Variables.FontAwesomeSolid = io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesome6Solid_compressed_data, sizeof(FontAwesome6Solid_compressed_size), 19, &FontAwesomeConfig, FontAwesomeRanges);
		g_Variables.FontAwesomeSolidSmall = io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesome6Solid_compressed_data, sizeof(FontAwesome6Solid_compressed_size), 16, &FontAwesomeConfig, FontAwesomeRanges);
		g_Variables.FontAwesomeRegular = io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesome6Regular_compressed_data, sizeof(FontAwesome6Regular_compressed_size), 19, &FontAwesomeConfig, FontAwesomeRanges);

		g_Variables.m_Expand = io.Fonts->AddFontFromMemoryTTF(expand_binary, sizeof(expand_binary), 11.f, &Cfg, io.Fonts->GetGlyphRangesCyrillic());
	}

	void Overlay::Style()
	{
		/*	ImGui::StyleColorsDark();

			ImGuiStyle* style = &ImGui::GetStyle();
			{
				style->WindowPadding = ImVec2(0, 0);
				style->WindowBorderSize = 0.f;
				style->ItemSpacing = ImVec2(12, 12);
				style->ScrollbarSize = 4.f;
			}*/

		if (g_Variables.Logo == nullptr)
		{
			D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, logo_binary, sizeof(logo_binary), &Info, Pump, &g_Variables.Logo, 0);
		}
	}

	bool Overlay::CreateDeviceD3D(HWND hWnd)
	{
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 2;
		sd.BufferDesc.Width = 0;
		sd.BufferDesc.Height = 0;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		UINT createDeviceFlags = 0;
		//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		D3D_FEATURE_LEVEL featureLevel;
		const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
		if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
			return false;

		CreateRenderTarget();
		return true;
	}

	void Overlay::CleanupDeviceD3D()
	{
		CleanupRenderTarget();

		if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
		if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
		if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
	}

	void Overlay::CreateRenderTarget()
	{
		ID3D11Texture2D* pBackBuffer;
		g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
		pBackBuffer->Release();
	}

	void Overlay::CleanupRenderTarget()
	{
		if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
	}

	LRESULT CALLBACK Overlay::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		switch (msg) {
		case WM_SIZE:
			if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
				Overlay::CleanupRenderTarget();
				g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				Overlay::CreateRenderTarget();
			}
			return 0;
		case WM_CLOSE:
			CloseRequested = true;
			return 0;
		case WM_DESTROY:
			CloseRequested = true;
			::PostQuitMessage(0);
			return 0;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			return 0;
		}
		case WM_ERASEBKGND:
			return 1;
		case WM_NCHITTEST:
			// Make window click-through when it's not shown. Consider login visible as shown even when IsOpen is false.
			if (!(g_MenuInfo.IsOpen || !g_MenuInfo.IsLogged))
				return HTTRANSPARENT;
			break;
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	DWORD Overlay::GetWinLogonToken(DWORD dwSessionId, DWORD dwDesiredAccess, PHANDLE phToken)
	{
		DWORD dwErr;
		PRIVILEGE_SET ps;

		ps.PrivilegeCount = 1;
		ps.Control = PRIVILEGE_SET_ALL_NECESSARY;

		if (LookupPrivilegeValue(NULL, SE_TCB_NAME, &ps.Privilege[0].Luid)) {
			HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (INVALID_HANDLE_VALUE != hSnapshot) {
				BOOL bCont, bFound = FALSE;
				PROCESSENTRY32 pe;

				pe.dwSize = sizeof(pe);
				dwErr = ERROR_NOT_FOUND;

				for (bCont = Process32First(hSnapshot, &pe); bCont; bCont = Process32Next(hSnapshot, &pe)) {
					HANDLE hProcess;

					if (0 != _tcsicmp(pe.szExeFile, xorstr("winlogon.exe"))) {
						continue;
					}

					hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
					if (hProcess) {
						HANDLE hToken;
						DWORD dwRetLen, sid;

						if (OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &hToken)) {
							BOOL fTcb;

							if (PrivilegeCheck(hToken, &ps, &fTcb) && fTcb) {
								if (GetTokenInformation(hToken, TokenSessionId, &sid, sizeof(sid), &dwRetLen) && sid == dwSessionId) {
									bFound = TRUE;
									if (DuplicateTokenEx(hToken, dwDesiredAccess, NULL, SecurityImpersonation, TokenImpersonation, phToken)) {
										dwErr = ERROR_SUCCESS;
									}
									else {
										dwErr = GetLastError();
									}
								}
							}
							CloseHandle(hToken);
						}
						CloseHandle(hProcess);
					}

					if (bFound) break;
				}

				CloseHandle(hSnapshot);
			}
			else {
				dwErr = GetLastError();
			}
		}
		else {
			dwErr = GetLastError();
		}


		return dwErr;
	}

	DWORD Overlay::CreateUIAccessToken(PHANDLE phToken)
	{
		DWORD dwErr;
		HANDLE hTokenSelf;

		if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &hTokenSelf)) {
			DWORD dwSessionId, dwRetLen;

			if (GetTokenInformation(hTokenSelf, TokenSessionId, &dwSessionId, sizeof(dwSessionId), &dwRetLen)) {
				HANDLE hTokenSystem;

				dwErr = GetWinLogonToken(dwSessionId, TOKEN_IMPERSONATE, &hTokenSystem);
				if (ERROR_SUCCESS == dwErr) {
					if (SetThreadToken(NULL, hTokenSystem)) {
						if (DuplicateTokenEx(hTokenSelf, TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_DEFAULT, NULL, SecurityAnonymous, TokenPrimary, phToken)) {
							BOOL bUIAccess = TRUE;

							if (!SetTokenInformation(*phToken, TokenUIAccess, &bUIAccess, sizeof(bUIAccess))) {
								dwErr = GetLastError();
								CloseHandle(*phToken);
							}
						}
						else {
							dwErr = GetLastError();
						}
						RevertToSelf();
					}
					else {
						dwErr = GetLastError();
					}
					CloseHandle(hTokenSystem);
				}
			}
			else {
				dwErr = GetLastError();
			}

			CloseHandle(hTokenSelf);
		}
		else {
			dwErr = GetLastError();
		}

		return dwErr;
	}

	BOOL Overlay::CheckForUIAccess(DWORD* pdwErr, DWORD* pfUIAccess)
	{
		BOOL result = FALSE;
		HANDLE hToken;

		if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
			DWORD dwRetLen;

			if (GetTokenInformation(hToken, TokenUIAccess, pfUIAccess, sizeof(*pfUIAccess), &dwRetLen)) {
				result = TRUE;
			}
			else {
				*pdwErr = GetLastError();
			}
			CloseHandle(hToken);
		}
		else {
			*pdwErr = GetLastError();
		}

		return result;
	}

	DWORD Overlay::PrepareForUIAccess()
	{
		DWORD dwErr;
		HANDLE hTokenUIAccess;
		DWORD fUIAccess;

		if (CheckForUIAccess(&dwErr, &fUIAccess)) {
			if (fUIAccess) {
				dwErr = ERROR_SUCCESS;
			}
			else {
				dwErr = CreateUIAccessToken(&hTokenUIAccess);
				if (ERROR_SUCCESS == dwErr) {
					std::vector<wchar_t> executable(32768);
					const DWORD length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
					if (!length || length >= executable.size()) {
						const DWORD pathError = length ? ERROR_INSUFFICIENT_BUFFER : GetLastError();
						CloseHandle(hTokenUIAccess);
						return pathError;
					}
					// Explicit application path controls executable selection. Keep
					// original arguments in a writable buffer as required by Win32.
					std::wstring command = GetCommandLineW();
					STARTUPINFOW si{};
					si.cb = sizeof(si);
					PROCESS_INFORMATION pi{};
					if (CreateProcessAsUserW(hTokenUIAccess, executable.data(), command.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
						CloseHandle(pi.hProcess), CloseHandle(pi.hThread);
						ExitProcess(0);
					}
					else {
						dwErr = GetLastError();
					}

					CloseHandle(hTokenUIAccess);
				}
			}
		}

		return dwErr;
	}
}
