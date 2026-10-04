#pragma once
#include <Includes/Includes.hpp>
#include <windows.h>
#include <iostream>
#include <thread>
#include <Bypass/Manipulation/Bypass.hpp>
#include <Bypass/Manipulation/reg.h>
#include <Bypass/Manipulation/task.hpp>
#include <skstr.hpp>

using namespace std;

namespace Settings {

	void CopyToClipboard(const char* text)
	{
		if (OpenClipboard(NULL)) {
			EmptyClipboard();

			size_t len = strlen(text) + 1;
			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
			memcpy(GlobalLock(hMem), text, len);
			GlobalUnlock(hMem);

			SetClipboardData(CF_TEXT, hMem);
			CloseClipboard();
		}
	}

	void Render() {
		int subSe = g_MenuInfo.SubPage[g_MenuInfo.Settings];
		bool vSe0 = (subSe < 0 || subSe == 0), vSe1 = (subSe < 0 || subSe == 1), vSe2 = (subSe < 0 || subSe == 2);
		ImGui::BeginGroup();
		{
			// Linha 1
			ImGui::BeginGroup();
			{
				// Seção: Globals
				if (vSe0) {
				ImGui::BeginGroup();
				{
				ImGui::CustomBeginChild("Geral", "Configuracoes", ImVec2(228, 300), false, 0);
				{
				if (Custom::CheckBox("Modo Stream", &g_Config.General->StreamProof)) {
					SetWindowDisplayAffinity(
						g_Variables.g_hCheatWindow,
						g_Config.General->StreamProof ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE
					);
					NotifyManager::Send(std::string("Modo Stream ") + (g_Config.General->StreamProof ? "ativado" : "desativado"), 2000);
				}
				if (Custom::CheckBox(xorstr("Segundo Monitor"), &g_Config.General->SecondMonitor))
					NotifyManager::Send(std::string("Segundo Monitor ") + (g_Config.General->SecondMonitor ? "ativado" : "desativado"), 2000);

					const char* particleOptions[] = { "Padrao", "Neve" };

					static int currentParticle = g_MenuInfo.particles2 ? 1 : 0;
					bool particlesEnabled = g_MenuInfo.particles || g_MenuInfo.particles2;

					Custom::CheckBox(xorstr("Particulas"), &particlesEnabled);

					if (particlesEnabled)
					{
						ImGui::Combo("Tipo de Particula", &currentParticle, particleOptions, IM_ARRAYSIZE(particleOptions));

						g_MenuInfo.particles = (currentParticle == 0) && particlesEnabled;
						g_MenuInfo.particles2 = (currentParticle == 1) && particlesEnabled;
					}
					else
					{
						g_MenuInfo.particles = false;
						g_MenuInfo.particles2 = false;
					}




					static int KeyMode = 1;
					ImGui::Keybind("Tecla do Menu", &g_Config.General->MenuKey, &KeyMode);

					if (Custom::Button("Descarregar", ImVec2(-1, 30), 0)) {
							exit(0);
						}

						//if (Custom::Button("Bypass SS", ImVec2(-1, 30), 0)) {
						//	bypass::EnabledBypass; // Corrigido: agora chama a função
						//	clearDllRegistryLogs();
						//	exit(1);
						//}
					}
					ImGui::CustomEndChild();
				}
				ImGui::EndGroup();
				}

				ImGui::SameLine();

				// Seção: Configs
				if (vSe1) {
				ImGui::BeginGroup();
				{
				ImGui::CustomBeginChild("Configs", "Salve para depois!", ImVec2(228, 300), false, 0);
				{
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 8));

					if (Custom::Button("Carregar Config", ImVec2(-1, 30), 0)) {
						std::thread cfgLoad([] {
							NotifyManager::Send(g_Config.LoadCfgInternal("..."), 3000);
							});
						cfgLoad.detach();
					}

					if (Custom::Button("Salvar Config", ImVec2(-1, 30), 0)) {
						std::thread cfgSave([] {
							std::string msg = g_Config.SaveCurrentConfigInternal();
							NotifyManager::Send(msg.c_str(), 3000);
							});
						cfgSave.detach();
					}

					if (Custom::Button("Exportar Config", ImVec2(-1, 30), 0)) {
						std::thread cfgExport([] {
							std::string msg = g_Config.SaveCurrentConfig();
							NotifyManager::Send(msg.c_str(), 3000);
							});
						cfgExport.detach();
					}

					if (Custom::Button("Importar Config", ImVec2(-1, 30), 0)) {
						std::thread cfgImport([] {
							std::string clipboard = Utils::GetClipboard();
							NotifyManager::Send(g_Config.LoadCfg("...", clipboard).c_str(), 3000);
							});
						cfgImport.detach();
					}

						ImGui::PopStyleVar();
					}
					ImGui::CustomEndChild();
				}
				ImGui::EndGroup();
				}
			}
			ImGui::EndGroup();

			ImGui::SameLine();

			// Seção: Cores + Partículas
			if (vSe2) {
			ImGui::BeginGroup();
			{
				ImGui::CustomBeginChild("Cores", "Ajustar Cores!", ImVec2(228, 300), false, 0);
				{
					// Função lambda para edição de cores
					auto colorEdit = [](const char* label, float* col, ImVec4& ref) {
						if (ImGui::ColorEdit4(label, col, ImGuiColorEditFlags_AlphaBar)) {
							ref.x = col[0]; ref.y = col[1]; ref.z = col[2]; ref.w = col[3];
						}
						};

					// --- Cores ---
					static float fovCol[4] = { g_Config.Aimbot->FovColor.Value.x, g_Config.Aimbot->FovColor.Value.y, g_Config.Aimbot->FovColor.Value.z, g_Config.Aimbot->FovColor.Value.w };
					colorEdit("Fov do Aimbot", fovCol, g_Config.Aimbot->FovColor.Value);

					static float triggerCol[4] = { g_Config.TriggerBot->FovColor.Value.x, g_Config.TriggerBot->FovColor.Value.y, g_Config.TriggerBot->FovColor.Value.z, g_Config.TriggerBot->FovColor.Value.w };
					colorEdit("Fov do Triggerbot", triggerCol, g_Config.TriggerBot->FovColor.Value);

					static float silentCol[4] = { g_Config.SilentAim->FovColor.Value.x, g_Config.SilentAim->FovColor.Value.y, g_Config.SilentAim->FovColor.Value.z, g_Config.SilentAim->FovColor.Value.w };
					colorEdit("Fov do SilentAim", silentCol, g_Config.SilentAim->FovColor.Value);

					static float skeletonCol[4] = { g_Config.ESP->SkeletonCol.Value.x, g_Config.ESP->SkeletonCol.Value.y, g_Config.ESP->SkeletonCol.Value.z, g_Config.ESP->SkeletonCol.Value.w };
					colorEdit("Esqueleto", skeletonCol, g_Config.ESP->SkeletonCol.Value);

					static float notVisibleCol[4] = { g_Config.ESP->SkeletonNot.Value.x, g_Config.ESP->SkeletonNot.Value.y, g_Config.ESP->SkeletonNot.Value.z, g_Config.ESP->SkeletonNot.Value.w };
					colorEdit("Esqueleto Nao Visivel", notVisibleCol, g_Config.ESP->SkeletonNot.Value);

					static float friendCol[4] = { g_Config.ESP->FriendCol.Value.x, g_Config.ESP->FriendCol.Value.y, g_Config.ESP->FriendCol.Value.z, g_Config.ESP->FriendCol.Value.w };
					colorEdit("Amigo", friendCol, g_Config.ESP->FriendCol.Value);

					static float namesCol[4] = { g_Config.ESP->UserNamesCol.Value.x, g_Config.ESP->UserNamesCol.Value.y, g_Config.ESP->UserNamesCol.Value.z, g_Config.ESP->UserNamesCol.Value.w };
					colorEdit("Nomes", namesCol, g_Config.ESP->UserNamesCol.Value);

					static float weaponCol[4] = { g_Config.ESP->WeaponNameCol.Value.x, g_Config.ESP->WeaponNameCol.Value.y, g_Config.ESP->WeaponNameCol.Value.z, g_Config.ESP->WeaponNameCol.Value.w };
					colorEdit("Nomes das Armas", weaponCol, g_Config.ESP->WeaponNameCol.Value);

					static float distCol[4] = { g_Config.ESP->DistanceCol.Value.x, g_Config.ESP->DistanceCol.Value.y, g_Config.ESP->DistanceCol.Value.z, g_Config.ESP->DistanceCol.Value.w };
					colorEdit("Distancia", distCol, g_Config.ESP->DistanceCol.Value);

					if (g_MenuInfo.particles)
					{
						if (ImGui::ColorButton(xorstr("Cor das Particulas"), g_MenuInfo.particlesColor))
							ImGui::OpenPopup(xorstr("ParticlesColorPicker"));
						if (ImGui::BeginPopup(xorstr("ParticlesColorPicker"))) {
							ImGui::ColorPicker4(xorstr("##picker"), (float*)&g_MenuInfo.particlesColor,
								ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar |
								ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_PickerHueWheel);
							ImGui::EndPopup();
						}
					}

					if (g_MenuInfo.particles2)
					{
						if (Custom::CheckBox2(xorstr("Particulas RGB"), &g_MenuInfo.enableRGBParticles)) {
							if (g_MenuInfo.enableRGBParticles)
								g_MenuInfo.originalParticlesColor = g_MenuInfo.particlesColor;
							else
								g_MenuInfo.particlesColor = g_MenuInfo.originalParticlesColor;
						}
						if (!g_MenuInfo.enableRGBParticles) {
							if (ImGui::ColorButton(xorstr("Cor das Particulas"), g_MenuInfo.particlesColor))
								ImGui::OpenPopup(xorstr("ParticlesColorPicker"));
							if (ImGui::BeginPopup(xorstr("ParticlesColorPicker"))) {
								ImGui::ColorPicker4(xorstr("##picker"), (float*)&g_MenuInfo.particlesColor,
									ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar |
									ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_PickerHueWheel);
								ImGui::EndPopup();
							}

						}

						if (g_MenuInfo.enableRGBParticles || g_MenuInfo.enableRGBRocket ||
							g_MenuInfo.enableRGBInfos || g_MenuInfo.enableRGBSidebarIcons ||
							g_MenuInfo.enableRGBSidebarSelectedIcon)
						{
							ImGui::SliderFloat(xorstr("Velocidade RGB"), &g_MenuInfo.rgbSpeed, 0.01f, 1.0f, "%.2f");
						}

					
						ImGui::SliderInt(xorstr("Quantidade de Particulas"), &g_MenuInfo.particleCount, 1, 300);
						ImGui::SliderFloat(xorstr("Velocidade Minima"), &g_MenuInfo.minSpeed, 0.1f, 10.0f);
						ImGui::SliderFloat(xorstr("Velocidade Maxima"), &g_MenuInfo.maxSpeed, g_MenuInfo.minSpeed, 50.0f);
						ImGui::SliderFloat(xorstr("Raio Minimo"), &g_MenuInfo.minRadius, 0.1f, 5.0f);
						ImGui::SliderFloat(xorstr("Raio Maximo"), &g_MenuInfo.maxRadius, g_MenuInfo.minRadius, 10.0f);
					}
				}
				ImGui::CustomEndChild();
			}
			ImGui::EndGroup();
			}
		}
		ImGui::EndGroup();
	}
}
