#pragma once
#include <Gui/Overlay/Overlay.hpp>
#include <Includes/Includes.hpp>
#include <Core/SDK/Memory.hpp>
#include <Includes/Utils.hpp>
#include <Core/SDK/SDK.hpp>
#include <Core/Core.hpp>
#include <Gui/gui.hpp>
#include <winternl.h>
#include <windows.h>
#include <dwmapi.h>
#include <tchar.h>
#include <vector>
#include <regex>

#include <Security/AntiCrack.hpp>
#include <Includes/CustomWidgets/Custom.hpp>

using namespace Core;

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	if (!Mem.GetMaxPrivileges(GetCurrentProcess()))
	{
        MessageBoxW(nullptr, L"Nao foi possivel inicializar as permissoes do aplicativo.", L"ASTRA - Inicializacao", MB_OK | MB_ICONERROR);
        return 0;
    }

    ULONGLONG waitStarted = GetTickCount64();
	while (!g_Variables.g_hGameWindow)
	{

		g_Variables.g_hGameWindow = FindWindowA(xorstr("grcWindow"), nullptr);
		if (g_Variables.g_hGameWindow)
		{
			auto WindowInfo = Utils::GetWindowPosAndSize(g_Variables.g_hGameWindow);
			g_Variables.g_vGameWindowSize = WindowInfo.second;
			g_Variables.g_vGameWindowPos = WindowInfo.first;
			g_Variables.g_vGameWindowCenter = { g_Variables.g_vGameWindowSize.x / 2, g_Variables.g_vGameWindowSize.y / 2 };
			break;
		}
        // Avoid an invisible busy loop when the required window is unavailable.
        if (GetTickCount64() - waitStarted >= 3000) {
            const int result = MessageBoxW(nullptr,
                L"A janela do jogo nao foi encontrada. O painel precisa dela para aparecer.\n\nAbra o jogo e aguarde a janela carregar. Depois clique em Repetir.\nPara encerrar o aplicativo, clique em Cancelar.",
                L"ASTRA - Aguardando janela", MB_RETRYCANCEL | MB_ICONINFORMATION | MB_SETFOREGROUND);
            if (result != IDRETRY) return 0;
            waitStarted = GetTickCount64();
        }
        Sleep(100);
	}

	GetWindowThreadProcessId(g_Variables.g_hGameWindow, &g_Variables.ProcIdFiveM);
	Core::SetupOffsets();
	Gui::cOverlay.Render();

	return 0;
}

std::string hwid;