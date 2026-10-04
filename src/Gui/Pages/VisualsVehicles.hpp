#pragma once
#include <Includes/includes.hpp>
#include <windows.h>
#include <iostream>
#include <thread>
#include <Includes/CustomWidgets/Preview.hpp>
#include <Includes/CustomWidgets/Notify.hpp>
#include <Core/Core.hpp>
#include <CustomWidgets/Custom.hpp>
#include <Core/Features/Exploits/HandlingEditor.hpp>

using namespace std;

namespace VisualsVehicles {
	inline void Render() {
		int subVv = g_MenuInfo.SubPage[g_MenuInfo.VisualsVehicles];
		bool vVv0 = (subVv < 0 || subVv == 0), vVv1 = (subVv < 0 || subVv == 1), vVv2 = (subVv < 0 || subVv == 2);
		if (vVv0) {
		ImGui::BeginGroup();
		{
			ImGui::CustomBeginChild(xorstr("Geral"), xorstr("Ajustar ESP de Veiculos"), ImVec2(228, 390), false, 0);
			{
			if (Custom::CheckBox(xorstr("Ativado"), &g_Config.VehicleESP->Enabled))
				NotifyManager::Send(std::string("ESP de Veiculos ") + (g_Config.VehicleESP->Enabled ? "ativado" : "desativado"), 2000);
			if (Custom::CheckBox(xorstr("Nomes dos Veiculos"), &g_Config.VehicleESP->VehName))
				NotifyManager::Send(std::string("Nomes dos Veiculos ") + (g_Config.VehicleESP->VehName ? "ativado" : "desativado"), 2000);
			if (Custom::CheckBox(xorstr("Trancado/Destrancado"), &g_Config.VehicleESP->ShowLockUnlock))
				NotifyManager::Send(std::string("Trancado/Destrancado ") + (g_Config.VehicleESP->ShowLockUnlock ? "ativado" : "desativado"), 2000);
			if (Custom::CheckBox(xorstr("Linhas"), &g_Config.VehicleESP->SnapLines))
				NotifyManager::Send(std::string("Linhas de Veiculos ") + (g_Config.VehicleESP->SnapLines ? "ativado" : "desativado"), 2000);
			if (Custom::CheckBox(xorstr("Distancia"), &g_Config.VehicleESP->DistanceFromMe))
				NotifyManager::Send(std::string("Distancia de Veiculos ") + (g_Config.VehicleESP->DistanceFromMe ? "ativado" : "desativado"), 2000);
				ImGui::SliderIntCustom(xorstr("Distancia de Renderizacao"), &g_Config.VehicleESP->MaxDistance, 0, 1000, xorstr("%dm"), 0);
			}
			ImGui::CustomEndChild();
		}
		ImGui::EndGroup();
		}

		ImGui::SameLine();

		if (vVv1) {
		ImGui::BeginGroup();
		{
			auto pLocal = Core::SDK::Pointers::pLocalPlayer;
			bool InVehicle = pLocal && pLocal->InVehicle();
			auto CurrentVehicle = InVehicle ? pLocal->GetLastVehicle() : nullptr;

			ImGui::CustomBeginChild(xorstr("Diversos"), xorstr("Controles de Veiculos!"), ImVec2(228, 390), false, 0);
			{
				if (Custom::CheckBox(xorstr("GodMode do Veiculo"), &g_Config.Player->VehicleGodMode)) {
					if (!InVehicle || !CurrentVehicle) {
						g_Config.Player->VehicleGodMode = false;
						NotifyManager::Send(xorstr("Voce precisa estar em um veiculo"), 4000);
					}
					else {
						CurrentVehicle->SetGodMode(g_Config.Player->VehicleGodMode);
						NotifyManager::Send(std::string("GodMode do Veiculo ") + (g_Config.Player->VehicleGodMode ? "ativado" : "desativado"), 2000);
					}
				}

				bool Locked = CurrentVehicle ? CurrentVehicle->IsLocked() : false;
				if (Custom::CheckBox(xorstr("Portas Trancadas"), &Locked)) {
					if (CurrentVehicle) {
						CurrentVehicle->DoorState(!Locked);
						NotifyManager::Send(std::string("Portas ") + (Locked ? "trancadas" : "destrancadas"), 2000);
					}
				}

				if (Custom::CheckBox(xorstr("Cinto de Seguranca"), &g_Config.Player->SeatBelt)) {
					if (!InVehicle) {
						g_Config.Player->SeatBelt = false;
						NotifyManager::Send(xorstr("Voce precisa estar em um veiculo"), 4000);
					}
					else {
						std::thread SeatBelt([]() {
							if (Core::SDK::Pointers::pLocalPlayer)
								Core::SDK::Pointers::pLocalPlayer->SeatBealt(g_Config.Player->SeatBelt);
							});
						SeatBelt.detach();
						NotifyManager::Send(std::string("Cinto de Seguranca ") + (g_Config.Player->SeatBelt ? "ativado" : "desativado"), 2000);
					}
				}

				static bool Extras_Restore{ false };
				static bool Extras_Restore2{ false };

				if (Custom::CheckBox(xorstr("Impulso Hornet"), &g_Config.Player->HornetBoost)) {
					g_Config.Player->VehicleJump = false;
					Extras_Restore2 = false;
					NotifyManager::Send(std::string("Impulso Hornet ") + (g_Config.Player->HornetBoost ? "ativado" : "desativado"), 2000);

					if (InVehicle && CurrentVehicle) {
						CVehicle* ModelInfo = reinterpret_cast<CVehicle*>(CurrentVehicle->GetModelInfo());
						if (ModelInfo) {
							ModelInfo->SetExtras(0x40);
							Extras_Restore = true;
						}
					}
				}
				else if (!g_Config.Player->HornetBoost && Extras_Restore && InVehicle && CurrentVehicle) {
					CVehicle* ModelInfo = reinterpret_cast<CVehicle*>(CurrentVehicle->GetModelInfo());
					if (ModelInfo) ModelInfo->SetExtras(0x0);
					Extras_Restore = false;
				}

				if (Custom::CheckBox(xorstr("Pulo do Veiculo"), &g_Config.Player->VehicleJump)) {
					g_Config.Player->HornetBoost = false;
					Extras_Restore = false;
					NotifyManager::Send(std::string("Pulo do Veiculo ") + (g_Config.Player->VehicleJump ? "ativado" : "desativado"), 2000);

					if (InVehicle && CurrentVehicle) {
						CVehicle* ModelInfo = reinterpret_cast<CVehicle*>(CurrentVehicle->GetModelInfo());
						if (ModelInfo) {
							ModelInfo->SetExtras(0x20);
							Extras_Restore2 = true;
						}
					}
				}
				else if (!g_Config.Player->VehicleJump && Extras_Restore2 && InVehicle && CurrentVehicle) {
					CVehicle* ModelInfo = reinterpret_cast<CVehicle*>(CurrentVehicle->GetModelInfo());
					if (ModelInfo) ModelInfo->SetExtras(0x0);
					Extras_Restore2 = false;
				}

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 8));

				if (Custom::Button(xorstr("Reparar Veiculo"), ImVec2(-1, 33), 0)) {
					if (InVehicle && CurrentVehicle) {
						CurrentVehicle->Fix();
						NotifyManager::Send(xorstr("Veiculo reparado!"), 2000);
					}
				}

				static int KeyMode = 1;
				ImGui::Keybind(xorstr("Atalho Reparar"), &g_Config.Player->KeyBindFix, &KeyMode);

				if (g_Config.Player->RepairCar) {
					if (InVehicle && CurrentVehicle)
						CurrentVehicle->Fix();
					g_Config.Player->RepairCar = false;
				}

				if (g_Config.Player->KeyBindFix) {
					if (GetAsyncKeyState(g_Config.Player->KeyBindFix) & 0x8000) {
						NotifyManager::Send(xorstr("Veiculo Foi Reparado!"), 4000);
						g_Config.Player->RepairCar = !g_Config.Player->RepairCar;
					}
				}

				ImGui::PopStyleVar();
			}
			ImGui::CustomEndChild();
		}
		ImGui::EndGroup();
		}

		ImGui::SameLine();

		if (vVv2) {
		ImGui::BeginGroup();
		{
			auto pLocal = Core::SDK::Pointers::pLocalPlayer;
			bool InVehicle = pLocal && pLocal->InVehicle();

			ImGui::CustomBeginChild(xorstr("Editor de Veiculos"), xorstr("Configuracoes"), ImVec2(228, 390), false, 0);
			{
				if (!InVehicle) {
					ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), xorstr("Voce precisa estar em um veiculo!"));
				}
				else {
					if (Custom::CheckBox(xorstr("Ativar Editor"), &g_Config.Player->HandlingEditor)) {
						if (g_Config.Player->HandlingEditor) {
							Features::Exploits::g_HandlingEditor.SaveHandlingValues();
						}
						else {
							Features::Exploits::g_HandlingEditor.RestoreHandlingValues();
						}
					}

					if (g_Config.Player->HandlingEditor) {
						if (ImGui::SliderFloat(xorstr("Aceleracao"), &Features::Exploits::g_HandlingEditor.fAcceleration, 0.0f, 400.f, xorstr("%1.1f"))) {
							Features::Exploits::g_HandlingEditor.ApplyHandlingValues();
						}

						if (ImGui::SliderFloat(xorstr("Forca de Freio"), &Features::Exploits::g_HandlingEditor.fBreakForce, 0.0f, 100.f, xorstr("%1.1f"))) {
							Features::Exploits::g_HandlingEditor.ApplyHandlingValues();
						}

						if (ImGui::SliderFloat(xorstr("Curva de Tracao Min"), &Features::Exploits::g_HandlingEditor.fTractionCurveMin, 0.0f, 100.f, xorstr("%1.1f"))) {
							Features::Exploits::g_HandlingEditor.ApplyHandlingValues();
						}

						ImGui::Spacing();
						ImGui::Separator();
						ImGui::Spacing();

						if (ImGui::SliderFloat(xorstr("Altura Suspensao"), &Features::Exploits::g_HandlingEditor.fSuspensionRaise, -0.3f, 0.3f, xorstr("%1.3f"))) {
							Features::Exploits::g_HandlingEditor.ApplyHandlingValues();
						}

						if (ImGui::SliderFloat(xorstr("Forca Suspensao"), &Features::Exploits::g_HandlingEditor.fSuspensionForce, 0.0f, 10.0f, xorstr("%1.2f"))) {
							Features::Exploits::g_HandlingEditor.ApplyHandlingValues();
						}

						if (ImGui::SliderFloat(xorstr("Damping Subida"), &Features::Exploits::g_HandlingEditor.fSuspensionCompDamp, 0.0f, 5.0f, xorstr("%1.2f"))) {
							Features::Exploits::g_HandlingEditor.ApplyHandlingValues();
						}

						if (ImGui::SliderFloat(xorstr("Damping Descida"), &Features::Exploits::g_HandlingEditor.fSuspensionReboundDamp, 0.0f, 5.0f, xorstr("%1.2f"))) {
							Features::Exploits::g_HandlingEditor.ApplyHandlingValues();
						}

						if (ImGui::SliderFloat(xorstr("Limite Superior"), &Features::Exploits::g_HandlingEditor.fSuspensionUpperLimit, 0.0f, 0.5f, xorstr("%1.3f"))) {
							Features::Exploits::g_HandlingEditor.ApplyHandlingValues();
						}

						if (ImGui::SliderFloat(xorstr("Limite Inferior"), &Features::Exploits::g_HandlingEditor.fSuspensionLowerLimit, -0.5f, 0.0f, xorstr("%1.3f"))) {
							Features::Exploits::g_HandlingEditor.ApplyHandlingValues();
						}
					}
				}
			}
			ImGui::CustomEndChild();
		}
		ImGui::EndGroup();
		}
	}
}
