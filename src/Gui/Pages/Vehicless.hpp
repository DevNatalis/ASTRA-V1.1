#include <Includes/includes.hpp>
#include <CustomWidgets/Custom.hpp>
#include <Includes/CustomWidgets/Notify.hpp>
#include <Core/SDK/SDK.hpp>
#include <Core/Threads/VehicleList.hpp>
#include <Core/SDK/Network/CNetwork.h>

using namespace std;
namespace Vehicless {

    inline void Render()
    {
        static int SelectedVehicleIndex = -1;  // ✅ adicione isto no topo da função
        bool IsVehicleSelected;                // ✅ esta também deve vir logo abaixo

        int subVs = g_MenuInfo.SubPage[g_MenuInfo.Vehicless];
        bool vVs0 = (subVs < 0 || subVs == 0), vVs1 = (subVs < 0 || subVs == 1);
        if (vVs0) {
        ImGui::BeginGroup();
        {
            ImGui::CustomBeginChild("Lista de Veiculos", "Procure veiculos!", ImVec2(330, 390), false, 0);
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 8));
                std::lock_guard<std::mutex> lock(Core::Threads::g_VehicleList.vehicleListMutex);

                for (int i = 0; i < Core::SDK::Game::VehicleList.size(); i++)
                {
                    IsVehicleSelected = SelectedVehicleIndex == i;
                    std::string name = Core::SDK::Game::VehicleList[i].Name + " (" + std::to_string((int)Core::SDK::Game::VehicleList[i].Dist) + "m)";
                    if (ImGui::ListSelectable(name.c_str(), &IsVehicleSelected))
                        SelectedVehicleIndex = i;
                }

                ImGui::PopStyleVar();
            }
            ImGui::CustomEndChild();
        }
        ImGui::EndGroup();
        }

        ImGui::SameLine();

        if (vVs1) {
        ImGui::BeginGroup();
        {
            ImGui::CustomBeginChild("Veiculo", "Informacoes", ImVec2(262, 130), false, 0);
            {
                if (SelectedVehicleIndex == -1 || SelectedVehicleIndex >= (int)Core::SDK::Game::VehicleList.size())
                {
                    ImGui::TextColored(ImColor(g_Col.SecundaryText), "Selecione um Veiculo");
                }
                else
                {
                    auto& veh = Core::SDK::Game::VehicleList[SelectedVehicleIndex];
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 4));
                    ImGui::TextColored(ImColor(g_Col.FeaturesText), "Nome:"); ImGui::SameLine();
                    ImGui::TextColored(ImColor(g_Col.SecundaryText), veh.Name.c_str());
                    ImGui::TextColored(ImColor(g_Col.FeaturesText), "Distancia:"); ImGui::SameLine();
                    ImGui::TextColored(ImColor(g_Col.SecundaryText), (std::to_string((int)veh.Dist) + "m").c_str());
                    ImGui::TextColored(ImColor(g_Col.FeaturesText), "Motorista:"); ImGui::SameLine();
                    ImGui::TextColored(ImColor(g_Col.SecundaryText), veh.Driver == 0 ? "Desconhecido" : veh.Driver->GetPedType() != 2 ? "NPC" : Core::SDK::Game::GetPedName(veh.Driver).c_str());
                    ImGui::TextColored(ImColor(g_Col.FeaturesText), "Portas Trancadas:"); ImGui::SameLine();
                    ImGui::TextColored(ImColor(g_Col.SecundaryText), veh.IsLocked ? "Sim" : "Nao");
                    ImGui::PopStyleVar();
                }
            }
            ImGui::CustomEndChild();

            ImGui::CustomBeginChild("Geral", "Acoes no Veiculo", ImVec2(262, 250), false, 0);
            {
                if (SelectedVehicleIndex == -1 || SelectedVehicleIndex >= (int)Core::SDK::Game::VehicleList.size())
                {
                    ImGui::TextColored(ImColor(g_Col.SecundaryText), "Selecione um Veiculo");
                }
                else
                {
                    auto* Veh = Core::SDK::Game::VehicleList[SelectedVehicleIndex].Pointer;
                    if (!Veh)
                    {
                        ImGui::TextColored(ImColor(g_Col.SecundaryText), "Veiculo nao existe mais");
                    }
                    else
                    {
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 8));

                    if (Veh->IsLocked())
                    {
                        if (Custom::Button("Destrancar", ImVec2(-1, 30), 0)) {
                            Veh->DoorState(true);
                            NotifyManager::Send(xorstr("Veiculo destrancado!"), 2000);
                        }
                    }
                    else
                    {
                        if (Custom::Button("Trancar", ImVec2(-1, 30), 0)) {
                            Veh->DoorState(false);
                            NotifyManager::Send(xorstr("Veiculo trancado!"), 2000);
                        }
                    }

                    if (Custom::Button("Teleportar", ImVec2(-1, 30), 0)) {
                        if (Core::SDK::Pointers::pLocalPlayer)
                            Core::SDK::Pointers::pLocalPlayer->SetPos(Veh->GetPos());
                        NotifyManager::Send(xorstr("Teleportado para o veiculo!"), 2000);
                    }

                    if (Custom::Button("Reparar Veiculo", ImVec2(-1, 30), 0))
                    {
                        Veh->Fix();
                        NotifyManager::Send("Veiculo reparado!", 4000);
                    }

                    if (!g_MenuInfo.isSpectating)
                    {
                        if (Custom::Button("Spectar Veiculo", ImVec2(-1, 30), 0))
                        {
                            if (Core::SDK::Pointers::pLocalPlayer) {
                                g_MenuInfo.spectateOldPos = Core::SDK::Pointers::pLocalPlayer->GetPos();
                                g_MenuInfo.isSpectating = true;
                                g_MenuInfo.spectateTarget = Veh;
                                NotifyManager::Send(xorstr("Spectando veiculo..."), 4000);
                            }
                        }
                    }
                    else
                    {
                        if (Custom::Button("Parar Spectate", ImVec2(-1, 30), 0))
                        {
                            if (Core::SDK::Pointers::pLocalPlayer) {
                                Core::SDK::Pointers::pLocalPlayer->SetPos(g_MenuInfo.spectateOldPos);
                                Core::SDK::Pointers::pLocalPlayer->bSetInvisibleLocal(false);
                                Core::SDK::Pointers::pLocalPlayer->FreezePed(false);
                            }
                            g_MenuInfo.isSpectating = false;
                            g_MenuInfo.spectateTarget = nullptr;
                            NotifyManager::Send(xorstr("Spectate interrompido"), 4000);
                        }
                    }

                    if (Custom::Button("Pedir Controle (beta)", ImVec2(-1, 30), 0))
                    {
                        if (Core::SDK::Pointers::pLocalPlayer != nullptr)
                            SDK2::g_Network->RequestControlOfVehicle2(Core::SDK::Pointers::pLocalPlayer, Veh, true, true);
                    }

                    ImGui::PopStyleVar();
                    }
                }
            }
            ImGui::CustomEndChild();
        }
        ImGui::EndGroup();
        }
    }

}
