#pragma once
#include <Includes/Includes.hpp>
#include <windows.h>
#include <iostream>
#include <thread>
#include <Includes/CustomWidgets/Notify.hpp>

using namespace std;

namespace Combat {

	void Render() {
		int subCb = g_MenuInfo.SubPage[g_MenuInfo.Combat];
		bool vCb0 = (subCb < 0 || subCb == 0), vCb1 = (subCb < 0 || subCb == 1), vCb2 = (subCb < 0 || subCb == 2);
		ImGui::BeginGroup();
		{
			// ===== Aimbot =====
			if (vCb0) {
			ImGui::BeginGroup();
			{
				ImGui::CustomBeginChild(xorstr("Aimbot"), xorstr("Configuracoes"), ImVec2(228, 390), false, 0);
				{
					if (Custom::CheckBox(xorstr("Ativado"), &g_Config.Aimbot->Enabled))
					NotifyManager::Send(std::string("Aimbot ") + (g_Config.Aimbot->Enabled ? "ativado" : "desativado"), 2000);
					Custom::CheckBox(xorstr("Mostrar Fov"), &g_Config.Aimbot->ShowFov);
					Custom::CheckBox(xorstr("Verificacao de Visibilidade"), &g_Config.Aimbot->OnlyVisible);
					Custom::CheckBox(xorstr("Ignorar NPCs"), &g_Config.Aimbot->IgnoreNPCs);
					ImGui::SliderIntCustom(xorstr("Campo de Visao"), &g_Config.Aimbot->FOV, 0, 400, xorstr("%d"), 0);
					ImGui::SliderIntCustom(xorstr("Suavidade"), &g_Config.Aimbot->AimbotSpeed, 0, 100, xorstr("%d"), 0);
					ImGui::SliderIntCustom(xorstr("Distancia Maxima"), &g_Config.Aimbot->MaxDistance, 0, 1000, xorstr("%dm"), 0);
					static int KeyMode = 1;
					ImGui::Keybind(xorstr("Atalho"), &g_Config.Aimbot->KeyBind, &KeyMode);
				}
				ImGui::CustomEndChild();
			}
			ImGui::EndGroup();
			}

			//ImGui::SameLine();

			//ImGui::BeginGroup();
			//{
			//	ImGui::CustomBeginChild(xorstr("AimbotSettings"), xorstr("Adjust Aimbot"), ImVec2(228, 390), false, 0);
			//	{
			//		ImGui::SliderInt(xorstr("Field of View"), &g_Config.Aimbot->FOV, 0, 400, xorstr("%d"), 0);
			//		ImGui::SliderInt(xorstr("Smooth"), &g_Config.Aimbot->AimbotSpeed, 0, 100, xorstr("%d"), 0);
			//		ImGui::SliderInt(xorstr("Max Distance"), &g_Config.Aimbot->MaxDistance, 0, 1000, xorstr("%dm"), 0);
			//		static int KeyModeAimbot = 1;
			//		ImGui::Keybind(xorstr("Bind"), &g_Config.Aimbot->KeyBind, &KeyModeAimbot);
			//	}
			//	ImGui::CustomEndChild();
			//}
			//ImGui::EndGroup();

			ImGui::SameLine();

			// ===== TriggerBot =====
			if (vCb1) {
			ImGui::BeginGroup();
			{
				ImGui::CustomBeginChild(xorstr("Trigger"), xorstr("Configuracoes"), ImVec2(228, 390), false, 0);
				{
					if (Custom::CheckBox(xorstr("Ativado"), &g_Config.TriggerBot->Enabled))
					NotifyManager::Send(std::string("TriggerBot ") + (g_Config.TriggerBot->Enabled ? "ativado" : "desativado"), 2000);
					if (!g_Config.TriggerBot->SmartTrigger) {
						Custom::CheckBox(xorstr("Mostrar Fov"), &g_Config.TriggerBot->ShowFov);
					}
					else {
						g_Config.TriggerBot->ShowFov = false;
					}
					Custom::CheckBox(xorstr("Trigger Inteligente"), &g_Config.TriggerBot->SmartTrigger);
					Custom::CheckBox(xorstr("Verificacao de Visibilidade"), &g_Config.TriggerBot->OnlyVisible);
					Custom::CheckBox(xorstr("Ignorar NPCs"), &g_Config.TriggerBot->IgnoreNPCs);

					if (!g_Config.TriggerBot->SmartTrigger) {
						ImGui::SliderIntCustom(xorstr("Campo de Visao"), &g_Config.TriggerBot->FOV, 0, 400, xorstr("%d"), 0);
						ImGui::SliderIntCustom(xorstr("Distancia Maxima"), &g_Config.TriggerBot->MaxDistance, 0, 1000, xorstr("%dm"), 0);
					}
					ImGui::SliderIntCustom(xorstr("Atraso"), &g_Config.TriggerBot->Delay, 0, 10, xorstr("%d"), 0);
					static int KeyMode = 1;
					ImGui::Keybind(xorstr("Atalho"), &g_Config.TriggerBot->KeyBind, &KeyMode);
				}
				ImGui::CustomEndChild();
			}
			ImGui::EndGroup();
			}

			ImGui::SameLine();

			//ImGui::BeginGroup();
			//{
			//	ImGui::CustomBeginChild(xorstr("TriggerSettings"), xorstr("Adjust Triggerbot"), ImVec2(228, 390), false, 0);
			//	{
			//		if (!g_Config.TriggerBot->SmartTrigger) {
			//			ImGui::SliderInt(xorstr("Field of View"), &g_Config.TriggerBot->FOV, 0, 400, xorstr("%d"), 0);
			//			ImGui::SliderInt(xorstr("Max Distance"), &g_Config.TriggerBot->MaxDistance, 0, 1000, xorstr("%dm"), 0);
			//		}
			//		ImGui::SliderInt(xorstr("Delay"), &g_Config.TriggerBot->Delay, 0, 10, xorstr("%d"), 0);
			//		static int KeyModeTrigger = 1;
			//		ImGui::Keybind(xorstr("Bind"), &g_Config.TriggerBot->KeyBind, &KeyModeTrigger);
			//	}
			//	ImGui::CustomEndChild();
			//}
			//ImGui::EndGroup();


			// ===== SilentAim =====
			if (vCb2) {
			ImGui::BeginGroup();
			{
				ImGui::CustomBeginChild(xorstr("Silent"), xorstr("Configuracoes"), ImVec2(228, 390), false, 0);
				{
					if (Custom::CheckBox(xorstr("Ativado"), &g_Config.SilentAim->Enabled))
					NotifyManager::Send(std::string("SilentAim ") + (g_Config.SilentAim->Enabled ? "ativado" : "desativado"), 2000);
					Custom::CheckBox(xorstr("Mostrar Fov"), &g_Config.SilentAim->ShowFov);
					Custom::CheckBox(xorstr("Balas Magicas"), &g_Config.SilentAim->MagicBullets);
					Custom::CheckBox(xorstr("Verificacao de Visibilidade"), &g_Config.SilentAim->OnlyVisible);
					Custom::CheckBox(xorstr("Ignorar NPCs"), &g_Config.SilentAim->IgnoreNPCs);
					ImGui::SliderIntCustom(xorstr("Campo de Visao"), &g_Config.SilentAim->FOV, 0, 400, xorstr("%d"), 0);
					ImGui::SliderIntCustom(xorstr("Chance de Errar"), &g_Config.SilentAim->MissChance, 0, 100, xorstr("%dx"), 0);
					ImGui::SliderIntCustom(xorstr("Distancia Maxima"), &g_Config.SilentAim->MaxDistance, 0, 1000, xorstr("%dm"), 0);
					static int KeyMode = 1;
					ImGui::Keybind(xorstr("Atalho"), &g_Config.SilentAim->KeyBind, &KeyMode);
				}
				ImGui::CustomEndChild();
			}
			ImGui::EndGroup();
			}

			//ImGui::SameLine();

			//ImGui::BeginGroup();
			//{
			//	ImGui::CustomBeginChild(xorstr("SilentSettings"), xorstr("Adjust Silent Aim"), ImVec2(228, 390), false, 0);
			//	{
			//		ImGui::SliderInt(xorstr("Field of View"), &g_Config.SilentAim->FOV, 0, 400, xorstr("%d"), 0);
			//		ImGui::SliderInt(xorstr("Miss Chance"), &g_Config.SilentAim->MissChance, 0, 100, xorstr("%dx"), 0);
			//		ImGui::SliderInt(xorstr("Max Distance"), &g_Config.SilentAim->MaxDistance, 0, 1000, xorstr("%dm"), 0);
			//		static int KeyModeSilent = 1;
			//		ImGui::Keybind(xorstr("Bind"), &g_Config.SilentAim->KeyBind, &KeyModeSilent);
			//	}
			//	ImGui::CustomEndChild();
			//}
			ImGui::EndGroup();
		}
		ImGui::EndGroup();
	}
}
