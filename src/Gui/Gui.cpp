#include "Gui.hpp"
#include <Gui/UI.hpp>

#include <Includes/CustomWidgets/Custom.hpp>
#include <Includes/CustomWidgets/WaterMarks.hpp>
#include <Includes/CustomWidgets/Notify.hpp>

#include <Gui/Pages/Combat.hpp>
#include <Gui/Pages/Visuals.hpp>
#include <Gui/Pages/VisualsVehicles.hpp>
#include <Gui/Pages/Weapons.hpp>
#include <Gui/Pages/Vehicless.hpp>
#include <Gui/Pages/Local.hpp>
#include <Gui/Pages/Exploits.hpp>
#include <Gui/Pages/World.hpp>
#include <Gui/Pages/Settings.hpp>
#include <Gui/Pages/Login.hpp>
#include <Gui/Pages/Launcher.hpp>

#include <Core/Features/Exploits/Exploits.hpp>
#include <Core/Features/GodMode.hpp>
#include <Core/Features/WeaponWheel.hpp>
#include <Core/Features/Revive.hpp>
#include <Core/Features/StickyToggles.hpp>
#include <Core/Features/Smooth/SmoothHealth.hpp>
#include <Core/Features/Smooth/SmoothArmor.hpp>
#include <Core/Features/Smooth/SmoothVehicleHealth.hpp>
#include <Core/Features/Smooth/SmoothStamina.hpp>
#include <Core/Features/Smooth/SmoothTeleport.hpp>
#include <Core/Features/Smooth/SmoothSpeed.hpp>
#include <Core/Features/Smooth/SmoothNoClip.hpp>
#include <Core/Features/Smooth/SmoothHandling.hpp>
#include <Core/Features/Smooth/ControlActionOverride.hpp>
#include <Core/Features/Smooth/ControlBitmapCleaner.hpp>
#include <Core/Features/Smooth/SmoothRecoil.hpp>
#include <Core/Features/Smooth/SmoothSpread.hpp>
#include <Core/Features/Smooth/SmoothAimAssist.hpp>
#include <Core/Features/Smooth/SmoothTrigger.hpp>
#include <Includes/CustomWidgets/Notify.hpp>
#include <Core/Features/Esp.hpp>

#include <Auth/Get_Hwid.hpp>
#include <Auth/Auth.hpp>
#include "Overlay/Overlay.hpp"
#include <Gui/ReferenceMenu.hpp>
#include <skstr.hpp>

int PauseLoop;
inline std::mutex DrawMtx;
bool Success = false;

namespace
{
	constexpr float kPad = 10.0f;
	constexpr float kSidebarW = 130.0f;
	constexpr float kGap = 12.0f;
	constexpr float kTabsX = 12.0f;
	constexpr float kTabWidth = 100.0f;
	constexpr float kTabHeight = 40.0f;
	constexpr float kContentX = 0.0f;
	constexpr float kLogoX = 40.0f;

	struct CatDef
	{
		int tab;
		const char* title;
		const char* subs[3];
		const char* subIcons[3];
		int subCount;
	};

	const CatDef kCats[] = {
		{ 0, "Assistencia de mira", { "Aimbot", "Trigger", "Silent" }, { ICON_FA_CROSSHAIRS, ICON_FA_BOLT, ICON_FA_GHOST }, 3 },      // Combat
		{ 1, "Visuais",        { "Geral", "Diversos", "Preview" }, { ICON_FA_EYE, ICON_FA_LAYER_GROUP, ICON_FA_IMAGE }, 3 },     // Visuals
		{ 4, "Jogador",         { "Geral", "Diversos", "Teleports" }, { ICON_FA_USER, ICON_FA_SLIDERS, ICON_FA_LOCATION_ARROW }, 3 },   // Local
		{ 2, "Veiculo",        { "Geral", "Diversos", "Handling" }, { ICON_FA_CAR, ICON_FA_WRENCH, ICON_FA_GAUGE_HIGH }, 3 },    // VisualsVehicles
		{ 6, "Armas",        { "Geral", "Spawner", "Resources" }, { ICON_FA_GUN, ICON_FA_PLUS, ICON_FA_BOX_OPEN }, 3 },   // Weapons
		{ 5, "Jogadores",        { "Lista", "Info & Acoes", "Roupas & Peds" }, { ICON_FA_USERS, ICON_FA_CIRCLE_INFO, ICON_FA_SHIRT }, 3 }, // World
		{ 3, "Veiculos",       { "Lista", "Info & Acoes" }, { ICON_FA_CAR_SIDE, ICON_FA_CIRCLE_INFO, nullptr }, 2 },            // Vehicless
		{ 7, "Preferencias",       { "Geral", "Configs", "Cores" }, { ICON_FA_GEAR, ICON_FA_FLOPPY_DISK, ICON_FA_PALETTE }, 3 },        // Settings
	};

	inline const CatDef* FindCat(int tab)
	{
		for (int i = 0; i < IM_ARRAYSIZE(kCats); ++i)
			if (kCats[i].tab == tab) return &kCats[i];
		return &kCats[0];
	}

	inline const char* SubName(const CatDef* c, int tab)
	{
		int s = (tab >= 0 && tab < 8) ? g_MenuInfo.SubPage[tab] : -1;
		if (c && s >= 0 && s < c->subCount) return c->subs[s];
		return "Todas";
	}
}

float accent_color[4] = {
	255 / 255.f,
	212 / 255.f,
	0 / 255.f,
	0.5f
};


#pragma region Animation
struct Particle
{
	ImVec2 position;
	ImVec2 velocity;
	ImVec4 color;
	float radius;
};

void particles2()
{
	if (!g_MenuInfo.particles2) // sï¿½ executa se estiver true
		return;

	ImVec2 screen_size = { (float)GetSystemMetrics(SM_CXSCREEN), (float)GetSystemMetrics(SM_CYSCREEN) };

	static ImVec2 particle_pos[300];
	static ImVec2 particle_target_pos[300];
	static float particle_speed[300];
	static float particle_radius[300];

	for (int i = 0; i < g_MenuInfo.particleCount; i++)
	{
		if (particle_pos[i].x == 0 || particle_pos[i].y == 0)
		{
			particle_pos[i].x = static_cast<float>(rand() % static_cast<int>(screen_size.x) + 1);
			particle_pos[i].y = 15.f;
			particle_speed[i] = g_MenuInfo.minSpeed + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (g_MenuInfo.maxSpeed - g_MenuInfo.minSpeed)));
			particle_radius[i] = g_MenuInfo.minRadius + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (g_MenuInfo.maxRadius - g_MenuInfo.minRadius)));
			particle_target_pos[i].x = static_cast<float>(rand() % static_cast<int>(screen_size.x));
			particle_target_pos[i].y = screen_size.y * 2;
		}

		particle_pos[i] = ImLerp(particle_pos[i], particle_target_pos[i], ImGui::GetIO().DeltaTime * (particle_speed[i] / 60));

		if (particle_pos[i].y > screen_size.y)
		{
			particle_pos[i].x = 0;
			particle_pos[i].y = 0;
		}

		ImColor particlesColorWithAlpha = ImColor(
			g_MenuInfo.particlesColor.Value.x,
			g_MenuInfo.particlesColor.Value.y,
			g_MenuInfo.particlesColor.Value.z,
			g_MenuInfo.particlesColor.Value.w * ImGui::GetStyle().Alpha
		);
		ImGui::GetForegroundDrawList()->AddCircleFilled(particle_pos[i], particle_radius[i], particlesColorWithAlpha);
	}
}


class ParticleSystem
{
public:
	ParticleSystem(int numParticles)
	{
		setupParticles(numParticles);
	}

	void update(float deltaTime, const ImVec2& windowPos)
	{
		this->windowPos = windowPos;

		if (!g_MenuInfo.particles) return;

		for (auto& particle : particles)
		{
			particle.position.x += particle.velocity.x * deltaTime;
			particle.position.y += particle.velocity.y * deltaTime;

			// Check if the particle is out of bounds, reset its position
			if (particle.position.x < 0 || particle.position.x > ImGui::GetWindowWidth() ||
				particle.position.y < 0 || particle.position.y > ImGui::GetWindowHeight())
		{
			resetParticle(particle);
			}
		}
	}

	void render(const ImVec2& windowPos)
	{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImDrawList* drawList = ImGui::GetForegroundDrawList();

		if (!g_MenuInfo.particles) return;

		// Draw particles
		for (const auto& particle : particles)
		{
			ImVec4 particleColor = g_MenuInfo.particlesColor; // usa a cor do menu
			drawList->AddCircleFilled(
				ImVec2(particle.position.x + windowPos.x, particle.position.y + windowPos.y),
				particle.radius,
				ImGui::GetColorU32(particleColor));
		}

		// Draw lines connecting particles within a certain distance
		float maxDistance = 150.0f; // Maximum distance to draw a line
		for (size_t i = 0; i < particles.size(); ++i)
		{
			for (size_t j = i + 1; j < particles.size(); ++j)
			{
				float distance = static_cast<float>(std::sqrt(
					std::pow(particles[i].position.x - particles[j].position.x, 2) +
					std::pow(particles[i].position.y - particles[j].position.y, 2)));

				if (distance < maxDistance)
				{
					// Fade line opacity based on distance
					float alpha = 0.5f - (distance / maxDistance);

					// usa a mesma cor do accent_color para as linhas:
					ImVec4 baseColor = g_MenuInfo.particlesColor; // mesma cor do menu
					ImVec4 lineColorVec = ImVec4(baseColor.x, baseColor.y, baseColor.z, alpha);
					ImU32 lineColor = ImGui::GetColorU32(lineColorVec);

					drawList->AddLine(
						ImVec2(particles[i].position.x + windowPos.x, particles[i].position.y + windowPos.y),
						ImVec2(particles[j].position.x + windowPos.x, particles[j].position.y + windowPos.y),
						lineColor, 1.0f);
				}
			}
		}
	}

private:
	ImVec2 windowPos;
	std::vector<Particle> particles;

	void setupParticles(int numParticles)
	{
		particles.clear();

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> disPos(0.f, 10.f);
		std::uniform_real_distribution<float> disVel(-50.f, 50.f);
		std::uniform_real_distribution<float> disColor(0.f, 1.f);
		std::uniform_real_distribution<float> disRadius(1.f, 3.f);

		for (int i = 0; i < numParticles; ++i)
		{
			Particle particle;
			particle.position = ImVec2(disPos(gen) * ImGui::GetWindowWidth(), disPos(gen) * ImGui::GetWindowHeight());
			particle.velocity = ImVec2(disVel(gen), disVel(gen));
			particle.color = ImVec4(disColor(gen), disColor(gen), disColor(gen), 0.4f);
			particle.radius = disRadius(gen);

			particles.push_back(particle);
		}
	}

	void resetParticle(Particle& particle)
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> disPos(0.f, 1.f);
		std::uniform_real_distribution<float> disVel(-50.f, 50.f);

		particle.position = ImVec2(disPos(gen) * ImGui::GetWindowWidth(), disPos(gen) * ImGui::GetWindowHeight());
		particle.velocity = ImVec2(disVel(gen), disVel(gen));
	}
};



bool Gui::close() {

	DWORD currentPID = GetCurrentProcessId();

	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		return false;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnap, &pe)) {
		CloseHandle(hProcessSnap);
		return false;
	}

	do {
		if (pe.th32ProcessID == currentPID) {
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
			if (hProcess == NULL) {
				CloseHandle(hProcessSnap);
				return false;
			}

			BOOL result = TerminateProcess(hProcess, 0);
			CloseHandle(hProcess);
			CloseHandle(hProcessSnap);
			return result != 0;
		}
	} while (Process32Next(hProcessSnap, &pe));

	CloseHandle(hProcessSnap);
	return false;
}


void Gui::Rendering()
{
	if (!g_MenuInfo.IsLogged)
	{
		g_Auth.ConsumeInitialization();
        const ImVec2 available = ImGui::GetIO().DisplaySize;
        const float desiredHeight = g_Auth.page == AuthPage::Register ? 660.f : 540.f;
        g_MenuInfo.MenuSize = { ImMin(820.f, ImMax(280.f, available.x - 24.f)),
                               ImMin(desiredHeight, ImMax(280.f, available.y - 24.f)) };
	}
	else
	{
		g_MenuInfo.MenuSize = { ReferenceMenu::Width, ReferenceMenu::Height };
	}
	ImGui::SetNextWindowSize(g_MenuInfo.MenuSize);
    if (!g_MenuInfo.IsLogged) {
        // Keep the complete auth window inside the current viewport after resizing.
        ImGui::SetNextWindowPos((ImGui::GetIO().DisplaySize - g_MenuInfo.MenuSize) * .5f);
    }

	if (!PauseLoop) {
		ImGui::SetNextWindowPos(g_Variables.g_vGameWindowSize / 2 - g_MenuInfo.MenuSize / 2);
		PauseLoop++;
	}

	static bool lastLogged = false;
	if (lastLogged != g_MenuInfo.IsLogged) {
		ImGui::SetNextWindowPos(g_Variables.g_vGameWindowSize / 2 - g_MenuInfo.MenuSize / 2);
		lastLogged = g_MenuInfo.IsLogged;
	}

	ImVec2 calculatedPos = g_Variables.g_vGameWindowSize / 2 - g_MenuInfo.MenuSize / 2;
	bool begin_ret = ImGui::Begin(" ", nullptr, ImGuiWindowFlags);
	// Diagnostic logging (throttled): report menu/window state ~1s or when values change
	{
		static bool last_IsOpen = false;
		static bool last_IsLogged = false;
		static ImVec2 last_MenuSize = ImVec2(-1, -1);
		static ImVec2 last_calculatedPos = ImVec2(-1, -1);
		static ImVec2 last_winSize = ImVec2(-1, -1);
		static bool last_begin_ret = false;
		static float last_style_alpha = -1.0f;
		static DWORD last_tick = 0;

		ImVec2 winPos = ImVec2(0,0);
		ImVec2 winSize = ImVec2(0,0);
		if (ImGui::GetCurrentWindow()) {
			winPos = ImGui::GetWindowPos();
			winSize = ImGui::GetWindowSize();
		}

		float styleAlpha = ImGui::GetStyle().Alpha;
		DWORD now = GetTickCount();
		bool changed = (last_IsOpen != g_MenuInfo.IsOpen) || (last_IsLogged != g_MenuInfo.IsLogged) ||
			(last_MenuSize.x != g_MenuInfo.MenuSize.x) || (last_MenuSize.y != g_MenuInfo.MenuSize.y) ||
			(last_calculatedPos.x != calculatedPos.x) || (last_calculatedPos.y != calculatedPos.y) ||
			(last_winSize.x != winSize.x) || (last_winSize.y != winSize.y) ||
			(last_begin_ret != begin_ret) || (fabs(last_style_alpha - styleAlpha) > 0.01f);

		if (changed || now - last_tick > 1000)
		{
			last_tick = now;
			last_IsOpen = g_MenuInfo.IsOpen;
			last_IsLogged = g_MenuInfo.IsLogged;
			last_MenuSize = g_MenuInfo.MenuSize;
			last_calculatedPos = calculatedPos;
			last_winSize = winSize;
			last_begin_ret = begin_ret;
			last_style_alpha = styleAlpha;

			char buf[1024];
			HWND ActiveWindow = GetForegroundWindow();
			sprintf_s(buf, sizeof(buf), "[Gui] IsOpen=%d IsLogged=%d should_show=%d ImGuiStyleAlpha=%.3f BgAlpha(overlay)=? MenuSize=%.0f,%.0f calcPos=%.0f,%.0f winPos=%.0f,%.0f winSize=%.0f,%.0f BeginRet=%d ActiveWindow=0x%p g_hGameWindow=0x%p g_hCheatWindow=0x%p\n",
				g_MenuInfo.IsOpen ? 1 : 0,
				g_MenuInfo.IsLogged ? 1 : 0,
				(g_MenuInfo.IsOpen || !g_MenuInfo.IsLogged) ? 1 : 0,
				styleAlpha,
				g_MenuInfo.MenuSize.x, g_MenuInfo.MenuSize.y,
				calculatedPos.x, calculatedPos.y,
				winPos.x, winPos.y,
				winSize.x, winSize.y,
				begin_ret ? 1 : 0,
				(void*)ActiveWindow,
				(void*)g_Variables.g_hGameWindow,
				(void*)g_Variables.g_hCheatWindow);
			OutputDebugStringA(buf);
		}
	}
	// STABILITY: nunca PushFont(nullptr) — antes caia se as duas fontes fossem nulas.
	ImFont* baseFont = UI::SafeFont(g_Variables.m_FontSmaller ? g_Variables.m_FontSmaller : g_Variables.m_FontNormal);
	ImGui::PushFont(baseFont);
	{

		if (!g_MenuInfo.IsLogged) Custom::DrawBackground(false);

		if (g_MenuInfo.IsLogged && g_MenuInfo.IsOpen && (g_MenuInfo.particles2 || g_MenuInfo.particles))
		{
			particles2();

			// STABILITY: era `new ParticleSystem` a cada primeiro frame com
			// leak (nunca deletado) e recriado sem controle. Instancia estatica.
			static ParticleSystem particleSys(g_MenuInfo.particleCount);
			ImVec2 wPos = ImGui::GetWindowPos();
			particleSys.update(ImGui::GetIO().DeltaTime, wPos);
			particleSys.render(wPos);
		}


		if (g_MenuInfo.IsLogged)
		{
			const float time = static_cast<float>(ImGui::GetTime());
			ImColor rgbColor = ImColor::HSV(fmod(time * g_MenuInfo.rgbSpeed, 1.0f), 0.8f, 0.8f);

			if (g_MenuInfo.enableRGBParticles) {
				g_MenuInfo.particlesColor = rgbColor;
			}
			if (g_MenuInfo.enableRGBRocket) {
				g_MenuInfo.rocketColor = rgbColor;
			}
			if (g_MenuInfo.enableRGBInfos) {
				g_MenuInfo.infosColor = rgbColor;
			}
			if (g_MenuInfo.enableRGBSidebarIcons) {
				g_MenuInfo.sidebarIconsColor = rgbColor;
			}
			if (g_MenuInfo.enableRGBSidebarSelectedIcon) {
				g_MenuInfo.sidebarSelectedIconColor = rgbColor;
			}

			if (Launcher::IsInjected())
				ReferenceMenu::Shell();
			else if (Launcher::Render())
				Gui::CloseRequested = true;
		}

		static bool auth_flag = false;

		char Username[50] = "";
		char DiscordID[50] = "";

		// svchost 2.0 open animation: ~300ms fade blended with tab fade. No input impact.
		float openA = UI::OpenAlpha(ImGui::GetIO().DeltaTime, g_MenuInfo.IsLogged ? g_MenuInfo.IsOpen : true);

		g_MenuInfo.TabAlpha = ImClamp(g_MenuInfo.TabAlpha + (5.f * ImGui::GetIO().DeltaTime * (g_MenuInfo.iTabCount == g_MenuInfo.iCurrentPage ? 1.f : -1.f)), 0.f, 1.f);

		if (g_MenuInfo.TabAlpha == 0.f)
			g_MenuInfo.iCurrentPage = g_MenuInfo.iTabCount;

		ImGuiStyle* style = &ImGui::GetStyle();
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_MenuInfo.TabAlpha * openA * style->Alpha);

		if (g_MenuInfo.IsLogged && Launcher::IsInjected())
		{

            ImGui::SetCursorPos(ImVec2(ReferenceMenu::SidebarWidth + 14, ReferenceMenu::Top));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
            const ImVec2 contentSize(g_MenuInfo.MenuSize.x - ReferenceMenu::SidebarWidth - 28,
                g_MenuInfo.MenuSize.y - ReferenceMenu::Top - 12);
            if (ImGui::BeginChild("##content", contentSize, false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_HorizontalScrollbar))
            {
                if (ReferenceMenu::Group == 0) ReferenceMenu::Aim();
                else if (ReferenceMenu::Group == 1) {
                    if (ReferenceMenu::VisualPage == 0) ReferenceMenu::Players();
                    else ReferenceMenu::Vehicles();
                }
                else {
                    const CatDef* category = FindCat(g_MenuInfo.iCurrentPage);
                    int& section = g_MenuInfo.SubPage[category->tab];
                    if (section < 0) section = 0;
                    if (ReferenceMenu::Group == 2) {
                        for (int i = 0; i < category->subCount; ++i) {
                            if (i) ImGui::SameLine(0, 12);
                            ImGui::PushID(i);
                            if (ImGui::Selectable(category->subs[i], section == i, 0, ImVec2(140, 22))) section = i;
                            ImGui::PopID();
                        }
                        ImGui::Spacing();
                    }
                    switch (g_MenuInfo.iCurrentPage) {
                    case g_MenuInfo.Local: Local::Render(); break;
                    case g_MenuInfo.VisualsVehicles: VisualsVehicles::Render(); break;
                    case g_MenuInfo.Weapons: Weapons::Render(); break;
                    case g_MenuInfo.World: World::Render(); break;
                    case g_MenuInfo.Vehicless: Vehicless::Render(); break;
                    case g_MenuInfo.Settings: Settings::Render(); break;
                    }
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar(2);


			HWND ActiveWindow = GetForegroundWindow();

			{
				std::lock_guard<std::mutex> Lock(DrawMtx);

				NotifyManager::Render();
				if (g_Auth.IsSessionValid())
					Core::Features::g_Revive.Tick();
				else
					Core::Features::g_Revive.SetEnabled(false);

				// GodMode por limiar: o Tick so escreve quando a vida cai
				// abaixo do limiar (cooldown + jitter). Sem IO com vida cheia.
				static bool godModeWasEnabled = false;
				if (g_Auth.IsSessionValid() && Core::SDK::Pointers::pLocalPlayer &&
					(g_Config.Player->EnableGodMode || godModeWasEnabled)) {
					if (g_Config.Player->EnableGodMode && !godModeWasEnabled)
						Core::Features::g_GodMode.SetEnabled(true);
					else if (!g_Config.Player->EnableGodMode && godModeWasEnabled)
						Core::Features::g_GodMode.SetEnabled(false);
					if (g_Config.Player->EnableGodMode)
						Core::Features::g_GodMode.Tick();
					godModeWasEnabled = g_Config.Player->EnableGodMode;
				}

				// Force weapon wheel (unlock wheel): o Tick so escreve se
				// algum bit de controle estiver setado. Sem IO se liberado.
				static bool wheelWasEnabled = false;
				if (g_Auth.IsSessionValid() && Core::SDK::Pointers::pLocalPlayer &&
					(g_Config.Player->ForceWeaponWheel || wheelWasEnabled)) {
					if (g_Config.Player->ForceWeaponWheel && !wheelWasEnabled)
						Core::Features::g_WeaponWheel.SetEnabled(true);
					else if (!g_Config.Player->ForceWeaponWheel && wheelWasEnabled)
						Core::Features::g_WeaponWheel.SetEnabled(false);
					if (g_Config.Player->ForceWeaponWheel)
						Core::Features::g_WeaponWheel.Tick();
					wheelWasEnabled = g_Config.Player->ForceWeaponWheel;
				}

				// Reaplicacao periodica dos toggles (500ms): mantem o estado
				// pedido quando o servidor redefine flags do ped.
				if (g_Auth.IsSessionValid() && Core::SDK::Pointers::pLocalPlayer) {
					Core::Features::g_StickyToggles.Tick();
				}

				// Stamina persistente: o smooth reescreve a cada 30ms com
				// jitter, em vez da aplicacao unica do clique.
				static bool stamWasEnabled = false;
				if (g_Auth.IsSessionValid() && Core::SDK::Pointers::pLocalPlayer &&
					(g_Config.Player->InfiniteStamina || stamWasEnabled)) {
					if (g_Config.Player->InfiniteStamina && !stamWasEnabled)
						Core::Features::Smooth::g_SmoothStamina.SetEnabled(true);
					else if (!g_Config.Player->InfiniteStamina && stamWasEnabled)
						Core::Features::Smooth::g_SmoothStamina.SetEnabled(false);
					stamWasEnabled = g_Config.Player->InfiniteStamina;
				}

				// Smooth state-stability layer: one Tick per feature per
				// frame. Each Tick throttles internally (30ms), writes at
				// most once, skips when current == target, and applies
				// jitter (+/-10%) plus 2s cooldown where applicable.
				if (g_Auth.IsSessionValid() && Core::SDK::Pointers::pLocalPlayer) {
					const uintptr_t smoothPed = reinterpret_cast<uintptr_t>(
						static_cast<CPed*>(Core::SDK::Pointers::pLocalPlayer));
					Core::Features::Smooth::g_SmoothHealth.Tick(smoothPed);
					Core::Features::Smooth::g_SmoothArmor.Tick(smoothPed);
					Core::Features::Smooth::g_SmoothVehicleHealth.Tick(smoothPed);
					Core::Features::Smooth::g_SmoothStamina.Tick(smoothPed);
					Core::Features::Smooth::g_SmoothTeleport.Tick(smoothPed);
					Core::Features::Smooth::g_SmoothSpeed.Tick(smoothPed);
					Core::Features::Smooth::g_SmoothNoClip.Tick(smoothPed);
					Core::Features::Smooth::g_SmoothHandling.Tick(smoothPed);
					Core::Features::Smooth::g_ControlActionOverride.Tick(smoothPed);
					Core::Features::Smooth::g_ControlBitmapCleaner.Tick(smoothPed);
					Core::Features::Smooth::g_SmoothRecoil.Tick(smoothPed);
					Core::Features::Smooth::g_SmoothSpread.Tick(smoothPed);
					Core::Features::Smooth::g_SmoothAimAssist.Tick(smoothPed);
					Core::Features::Smooth::g_SmoothTrigger.Tick(smoothPed);
				}

				if (ActiveWindow == g_Variables.g_hGameWindow)
				{
					if (g_Config.Player->GodModeKey > 0 && (GetAsyncKeyState(g_Config.Player->GodModeKey) & 1))
					{
						g_Config.Player->EnableGodMode = !g_Config.Player->EnableGodMode;

						Core::Features::g_GodMode.SetEnabled(g_Config.Player->EnableGodMode);
						const bool applied = Core::SDK::Pointers::pLocalPlayer != nullptr;
						NotifyManager::Send(applied ? std::string("GodMode ") +
							(g_Config.Player->EnableGodMode ? "ativado" : "desativado") :
							"Nao foi possivel aplicar GodMode ao jogador", 2000);

					}

					// Toggle F10: force weapon wheel (unlock wheel).
					if ((GetAsyncKeyState(VK_F10) & 1))
					{
						g_Config.Player->ForceWeaponWheel = !g_Config.Player->ForceWeaponWheel;

						Core::Features::g_WeaponWheel.SetEnabled(g_Config.Player->ForceWeaponWheel);
						if (g_Config.Player->ForceWeaponWheel)
							Core::Features::g_WeaponWheel.ForceWeaponWheel();
						NotifyManager::Send(std::string("Roda de Armas ") +
							(g_Config.Player->ForceWeaponWheel ? "forcada (F10)" : "normal (F10)"), 2000);
					}

					if (GetAsyncKeyState(g_Config.Player->NoClipKey) & 1)
					{
						g_Config.Player->NoClipEnabled = !g_Config.Player->NoClipEnabled;

						if (Core::SDK::Pointers::pLocalPlayer)
							Core::SDK::Pointers::pLocalPlayer->FreezePed(g_Config.Player->NoClipEnabled);

						std::thread([&]()
							{
								NotifyManager::Send(xorstr("NoClip foi ") + (std::string)(g_Config.Player->NoClipEnabled ? xorstr("ativado!") : xorstr("desativado!")), 2000);
							}
						).detach();
					}

				if (g_Config.Player->NoClipEnabled)
					Features::Exploits::NoClip();

				}

				if (g_Config.Player->FreeCamActivate) {
					Features::Exploits::FreeCam();
					if (Core::g_Config.Player->invisible_while_activate && Core::SDK::Pointers::pLocalPlayer)
						Core::SDK::Pointers::pLocalPlayer->bSetInvisibleLocal(true);
				}

				if (g_Config.Player->Invisibilidade && Core::SDK::Pointers::pLocalPlayer)
					Core::SDK::Pointers::pLocalPlayer->bSetInvisibleLocal(true);

				if (g_Config.Player->CrouchMode && Core::SDK::Pointers::pLocalPlayer)
					Core::SDK::Pointers::pLocalPlayer->SetCrouch(true);

				if (g_MenuInfo.isSpectating && g_MenuInfo.spectateTarget)
				{
					D3DXVECTOR3 vehPos = g_MenuInfo.spectateTarget->GetPos();
					if (vehPos != D3DXVECTOR3(0, 0, 0) && Core::SDK::Pointers::pLocalPlayer) {
						Core::SDK::Pointers::pLocalPlayer->SetPos(vehPos + D3DXVECTOR3(0, 0, 0.5f));
						Core::SDK::Pointers::pLocalPlayer->bSetInvisibleLocal(true);
						Core::SDK::Pointers::pLocalPlayer->FreezePed(true);
					}
				}

				if (ActiveWindow == g_Variables.g_hGameWindow || ActiveWindow == g_Variables.g_hCheatWindow)
				{
					struct FovFuncs_t {
						bool* Enabled;
						int* FovSize;
						ImVec4 FovColor;
					};

					std::vector<FovFuncs_t> FovDrawList = {
						FovFuncs_t(&g_Config.Aimbot->ShowFov, &g_Config.Aimbot->FOV, g_Config.Aimbot->FovColor),
						FovFuncs_t(&g_Config.SilentAim->ShowFov, &g_Config.SilentAim->FOV, g_Config.SilentAim->FovColor),
						FovFuncs_t(&g_Config.TriggerBot->ShowFov, &g_Config.TriggerBot->FOV, g_Config.TriggerBot->FovColor),
					};

					static std::vector<float> Alphas(FovDrawList.size(), 0.0f);
					static std::vector<float> Sizes(FovDrawList.size(), 0.0f);

					for (int i = 0; i < FovDrawList.size(); ++i)
					{
						auto& Fov = FovDrawList[i];

						Alphas[i] = ImClamp(ImLerp(Alphas[i], *Fov.Enabled ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 10.f), 0.f, 1.f);
						Sizes[i] = ImLerp(Sizes[i], (float)*Fov.FovSize, ImGui::GetIO().DeltaTime * 12.f);

						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Alphas[i]);

						ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(g_Variables.g_vGameWindowCenter.x, g_Variables.g_vGameWindowCenter.y), Sizes[i], ImGui::GetColorU32(Fov.FovColor), 999);

						ImGui::PopStyleVar();
					}

					if (g_Config.General->WaterMark)
						Custom::WaterMark::Render();


					ImGui::PushFont(UI::SafeFont(g_Variables.m_DrawFont));

					Features::g_Esp.Draw();
					Features::g_Esp.DrawVehicle();

					ImGui::PopFont();
				}
				else {
					if (ImGui::GetStyle().Alpha >= 0.9f)
						g_MenuInfo.IsOpen = false;
				}

			}
		}
		// Logged but still on the launcher: Shell/launcher already drawn above,
		// nothing else to draw here. Login draws ONLY when logged out — never
		// over the launcher (that overlap froze input and doubled draw cost).
		else if (!g_MenuInfo.IsLogged) {
		{
			// KeyAuth init: executa uma vez por sessao antes de liberar os campos.
			static bool kaInitAttempted = false;
			if (!kaInitAttempted) {
				kaInitAttempted = true;
                g_Auth.InitializeAsync();
			}

			// Notificacao de build (uma vez).
			static bool buildNotified = false;
			if (!buildNotified) {
				if (g_Offsets.CurrentBuild > 0) {
					NotifyManager::Send("Build detected: " + std::to_string(g_Offsets.CurrentBuild) + " - Offsets loaded", 5000);
				}
				else {
					NotifyManager::Send("Aguardando deteccao do build do FiveM...", 5000);
				}
				buildNotified = true;
			}

            const ImVec2 origin = ImGui::GetWindowPos();
            ImDrawList* draw = ImGui::GetWindowDrawList();
            // Raw draw-list colors ignore ImGuiStyleVar_Alpha, so scale them
            // manually — otherwise the login logo pops in at full opacity
            // while the overlay is still fading in/out.
            const float loginAlpha = ImGui::GetStyle().Alpha;
            auto A = [loginAlpha](ImVec4 c) { c.w *= loginAlpha; return c; };
            draw->AddRectFilled(origin, origin + g_MenuInfo.MenuSize, ImGui::GetColorU32(A(UI::Background())), 16.f);
            const bool showBrand = g_MenuInfo.MenuSize.x >= 700.f;
            const float panelHeight = g_MenuInfo.MenuSize.y;
            if (showBrand) {
            draw->AddRectFilled(origin, origin + ImVec2(330, panelHeight), ImGui::GetColorU32(A(UI::Sidebar())), 16.f, ImDrawFlags_RoundCornersLeft);
            draw->AddRect(origin, origin + g_MenuInfo.MenuSize, ImGui::GetColorU32(A(UI::Border())), 16.f);
            draw->AddLine(origin + ImVec2(330, 24), origin + ImVec2(330, panelHeight - 24.f), ImGui::GetColorU32(A(UI::BorderSoft())));
            if (g_Variables.Logo && loginAlpha > 0.01f) {
                draw->AddImageRounded(g_Variables.Logo, origin + ImVec2(65, 165),
                    origin + ImVec2(265, 365), ImVec2(0, 0), ImVec2(1, 1),
                    ImGui::GetColorU32(A(ImVec4(1, 1, 1, 1))), 24.f);
            }
            ImFont* brand = UI::SafeFont(g_Variables.m_FontSecundary);
            draw->AddText(brand, 30.f, origin + ImVec2(32, 32), ImGui::GetColorU32(A(UI::Text())), "svchost");
            draw->AddText(origin + ImVec2(33, 73), ImGui::GetColorU32(A(UI::Accent())), "SEU ESPACO. SEU CONTROLE.");
            }
            // Preserve the two-column card; use a single column on narrow viewports.
            const float cardLeft = showBrand ? 354.f : 20.f;
            const float cardRight = g_MenuInfo.MenuSize.x - 24.f;
            const float left = cardLeft + 20.f;
            draw->AddRectFilled(origin + ImVec2(cardLeft, 28), origin + ImVec2(cardRight, panelHeight - 24.f),
                ImGui::GetColorU32(UI::Surface()), 16.f);
            draw->AddRect(origin + ImVec2(cardLeft, 28), origin + ImVec2(cardRight, panelHeight - 24.f),
                ImGui::GetColorU32(UI::BorderSoft()), 16.f);
            const bool registering = g_Auth.page == AuthPage::Register;
            ImFont* headingFont = UI::SafeFont(g_Variables.m_FontSecundary);
            const char* heading = registering ? "Create Account" : "Welcome back";
            const float headingWidth = headingFont->CalcTextSizeA(28.f, FLT_MAX, 0.f, heading).x;
            const float headingSize = ImMin(28.f, 28.f * ImMax(80.f, cardRight - left - 58.f) / headingWidth);
            draw->AddText(headingFont, headingSize, origin + ImVec2(left, 44.f), ImGui::GetColorU32(UI::Text()), heading);
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), origin + ImVec2(left, 82.f),
                ImGui::GetColorU32(UI::TextDim()), registering ? "A license key is required to register." :
                "Sign in to svchost to continue.", nullptr, cardRight - left - 20.f);

            ImGui::SetCursorPos(ImVec2(cardRight - 40.f, 40.f));
            if (LoginUI::AnimatedButton("X##close_login", ImVec2(28.f, 28.f))) {
                LoginUI::ClearSensitive();
                Gui::CloseRequested = true;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fechar svchost");

            ImGui::SetCursorPos(ImVec2(cardLeft + 12.f, 116.f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
            if (ImGui::BeginChild("##auth_form", ImVec2(cardRight - cardLeft - 24.f, panelHeight - 172.f),
                false, ImGuiWindowFlags_AlwaysUseWindowPadding)) {
                const float width = ImGui::GetContentRegionAvail().x;
                LoginUI::Render(ImGui::GetWindowDrawList(), ImGui::GetWindowPos(), ImGui::GetCursorPosX(), width);
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();

		}

			NotifyManager::Render();
		}
		ImGui::PopStyleVar();

			ImGui::PopFont();
		}

		// STABILITY: todo ImGui::Begin() exige ImGui::End() no mesmo frame,
		// mesmo quando Begin() retorna false. A ausencia desequilibra a
		// pilha de janelas e derruba o processo apos alguns frames.
		ImGui::End();
	}


//void Gui::RenderModeFreeCam() {
//	const char* ModeNames[] = { "Look Around", "Lock Vehicle [E]", "Unlock Vehicle [E]", "Explode Vehicle [E]","Teleport Vehicle To Sky [E]", "Teleport Vehicle To Void [E]" };
//	std::string sDisplayText = skCrypt("FreeCam Mode: ") + std::string(ModeNames[g_Variables->Self.FreeCam.current_mode]);
//	std::string sInstructions = ("Right/Left Arrow = Switch Mode");
//
//	ImVec2 vTextSize = ImGui::CalcTextSize(sDisplayText.c_str());
//	ImVec2 vInstructionSize = ImGui::CalcTextSize(sInstructions.c_str());
//
//	float fWidth = vTextSize.x;
//	if (vInstructionSize.x > vTextSize.x) {
//		fWidth = vInstructionSize.x;
//	}
//
//	float fPadding = 15.0f;
//	float fRounding = 10.0f;
//
//	ImVec2 vPos = ImVec2(ImGui::GetIO().DisplaySize.x / 2 - fWidth / 2 - fPadding, ImGui::GetIO().DisplaySize.y - ImGui::GetTextLineHeightWithSpacing() * 3 - fPadding - 70.0f);
//	ImVec2 vSize = ImVec2(fWidth + fPadding * 2, vTextSize.y + vInstructionSize.y + fPadding * 3);
//
//	ImGui::SetNextWindowPos(vPos);
//	ImGui::SetNextWindowBgAlpha(0.5f);
//
//	if (ImGui::Begin(skCrypt("FreeCam Mode"), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
//		ImVec2 vMin = vPos;
//		ImVec2 vMax = ImVec2(vPos.x + vSize.x, vPos.y + vSize.y);
//
//		ImGui::GetWindowDrawList()->AddRect(vMin, vMax, IM_COL32(0, 128, 244, 255), fRounding, ImDrawFlags_RoundCornersAll, 2.0f);
//
//		ImGui::Text(skCrypt("%s"), sDisplayText.c_str());
//		ImGui::Text(skCrypt("%s"), sInstructions.c_str());
//	}
//	ImGui::End();
//}
