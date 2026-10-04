#pragma once
#include <Includes/includes.hpp>
#include <windows.h>
#include <iostream>
#include <thread>
#include <Includes/CustomWidgets/Preview.hpp>
#include <Includes/CustomWidgets/Notify.hpp>

#include <Core/Core.hpp>

using namespace std;

namespace Visuals {


	void Render() {
		int subVi = g_MenuInfo.SubPage[g_MenuInfo.Visuals];
		bool vVi0 = (subVi < 0 || subVi == 0), vVi1 = (subVi < 0 || subVi == 1), vVi2 = (subVi < 0 || subVi == 2);
		const int visibleCount = int(vVi0) + int(vVi1) + int(vVi2);
		const ImVec2 available = ImGui::GetContentRegionAvail();
		const float gap = ImGui::GetStyle().ItemSpacing.x;
		const ImVec2 cardSize(ImMax(1.f, (available.x - gap * (visibleCount - 1)) / ImMax(1, visibleCount)),
			ImMax(100.f, available.y));
		bool firstCard = true;
		auto nextCard = [&]() {
			if (!firstCard) ImGui::SameLine(0.f, gap);
			firstCard = false;
		};
		if (vVi0) {
		nextCard();
		ImGui::BeginGroup();
		{
		ImGui::CustomBeginChild(xorstr("Geral"), xorstr("Ajustar ESP de Jogador"), cardSize, false, 0);
			{
			if (Custom::CheckBox(xorstr("Ativado"), &g_Config.ESP->Enabled))
				NotifyManager::Send(std::string("ESP ") + (g_Config.ESP->Enabled ? "ativado" : "desativado"), 2000);

			if (Custom::CheckBox(xorstr("Esqueleto"), &g_Config.ESP->Skeleton)) {
				Core::SDK::Pointers::pLocalPlayer->RemoveKinematics();
				NotifyManager::Send(std::string("Esqueleto ") + (g_Config.ESP->Skeleton ? "ativado" : "desativado"), 2000);
			}

			if (Custom::CheckBox(xorstr("Caixa"), &g_Config.ESP->Box)) {
				if (!g_Config.ESP->Box) g_Config.ESP->FilledBox = false;
				NotifyManager::Send(std::string("Caixa ") + (g_Config.ESP->Box ? "ativado" : "desativado"), 2000);
			}
			if (Custom::CheckBox(xorstr("Caixa Preenchida"), &g_Config.ESP->FilledBox)) {
				if (!g_Config.ESP->Box) g_Config.ESP->Box = true;
				NotifyManager::Send(std::string("Caixa Preenchida ") + (g_Config.ESP->FilledBox ? "ativado" : "desativado"), 2000);
			}

			if (Custom::CheckBox(xorstr("Nomes"), &g_Config.ESP->UserNames))
				NotifyManager::Send(std::string("Nomes ") + (g_Config.ESP->UserNames ? "ativado" : "desativado"), 2000);
			if (Custom::CheckBox(xorstr("Arma"), &g_Config.ESP->WeaponName))
				NotifyManager::Send(std::string("Arma ") + (g_Config.ESP->WeaponName ? "ativado" : "desativado"), 2000);
			if (Custom::CheckBox(xorstr("Distancia"), &g_Config.ESP->DistanceFromMe))
				NotifyManager::Send(std::string("Distancia ") + (g_Config.ESP->DistanceFromMe ? "ativado" : "desativado"), 2000);
			if (Custom::CheckBox(xorstr("Vida"), &g_Config.ESP->HealthBar))
				NotifyManager::Send(std::string("Barra de Vida ") + (g_Config.ESP->HealthBar ? "ativado" : "desativado"), 2000);
			if (Custom::CheckBox(xorstr("Colete"), &g_Config.ESP->ArmorBar))
				NotifyManager::Send(std::string("Barra de Colete ") + (g_Config.ESP->ArmorBar ? "ativado" : "desativado"), 2000);
			if (Custom::CheckBox(xorstr("Linhas"), &g_Config.ESP->SnapLines))
				NotifyManager::Send(std::string("Linhas ") + (g_Config.ESP->SnapLines ? "ativado" : "desativado"), 2000);

			if (Custom::CheckBox(xorstr("Jogador Local"), &g_Config.ESP->ShowLocalPlayer))
				NotifyManager::Send(std::string("Mostrar Jogador Local ") + (g_Config.ESP->ShowLocalPlayer ? "ativado" : "desativado"), 2000);

				ImGui::SliderIntCustom(xorstr("Distancia de Renderizacao"), &g_Config.ESP->MaxDistance, 0, 1000, xorstr("%dm"), 0);
			}
			ImGui::CustomEndChild();
		}
		ImGui::EndGroup();
		}



		if (vVi1) {
		nextCard();
		ImGui::BeginGroup();
		{
		ImGui::CustomBeginChild(xorstr("Diversos"), xorstr("Ajustar ESP"), cardSize, false, ImGuiWindowFlags_NoScrollbar);
			{
			if (Custom::CheckBox(xorstr("Ignorar NPCs"), &g_Config.ESP->IgnoreNPCs))
				NotifyManager::Send(std::string("Ignorar NPCs ") + (g_Config.ESP->IgnoreNPCs ? "ativado" : "desativado"), 2000);
			if (Custom::CheckBox(xorstr("Ignorar Mortos"), &g_Config.ESP->IgnoreDead))
				NotifyManager::Send(std::string("Ignorar Mortos ") + (g_Config.ESP->IgnoreDead ? "ativado" : "desativado"), 2000);
			if (Custom::CheckBox(xorstr("Filtro de Visibilidade"), &g_Config.ESP->HighlightVisible))
				NotifyManager::Send(std::string("Filtro de Visibilidade ") + (g_Config.ESP->HighlightVisible ? "ativado" : "desativado"), 2000);

			if (Custom::CheckBoxCfg(xorstr("Identificador de Amigo"), &g_Config.ESP->FriendsMarker, [] {
					static int KeyMode = 1;
					ImGui::Keybind(xorstr("Tecla de Ativacao"), &g_Config.ESP->FriendsMarkerBind, &KeyMode);
					}));

			if (Custom::CheckBox(xorstr("ESP Admin"), &g_Config.ESP->AdminESP))
				NotifyManager::Send(std::string("ESP Admin ") + (g_Config.ESP->AdminESP ? "ativado" : "desativado"), 2000);
			}
			ImGui::CustomEndChild();
		}
		ImGui::EndGroup();
		}



		if (vVi2) {
		nextCard();
		ImGui::BeginGroup();
		{
			ImGui::CustomBeginChild(xorstr("Visualizacao"), xorstr("Visualizacao do ESP"), cardSize, false, ImGuiWindowFlags_NoScrollbar);
			{
				Custom::g_EspPreview.DragDropHandler();
				Custom::g_EspPreview.Draw();
			}
			ImGui::CustomEndChild();
		}
		ImGui::EndGroup();
		}


	}
}
