#include <windows.h>
#include <iostream>
#include <thread>
#include <Includes/includes.hpp>
#include <Includes/CustomWidgets/Notify.hpp>
#include <Core/Features/GodMode.hpp>

using namespace std;

namespace World {

    struct ClothComponent {
        const char* Name;
        int Offset;
        int MaxDrawable;
    };

    static const ClothComponent ClothComponents[] = {
        {"Cabeca", 0x100, 150},
        {"Mascara", 0xF8, 150},
        {"Torso", 0x114, 150},
        {"Pernas", 0x108, 150},
        {"Sapatos", 0xEC, 150},
        {"Acessorios", 0xF4, 150},
        {"Maos", 0xFC, 150},
        {"Colete", 0x10C, 150},
        {"Sub-camisa", 0xAC, 150},
    };

    inline void Render()
    {
        static int SelectedPlayerIndex = -1;
        bool IsPlayerSelected;

        int subWo = g_MenuInfo.SubPage[g_MenuInfo.World];
        bool vWo0 = (subWo < 0 || subWo == 0), vWo1 = (subWo < 0 || subWo == 1), vWo2 = (subWo < 0 || subWo == 2);
        if (vWo0) {
        ImGui::BeginGroup();
        {
            ImGui::CustomBeginChild("Lista de Jogadores", "Procure jogadores!", ImVec2(200, 390), false, 0);
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 8));
                for (int i = 0; i < Core::SDK::Game::EntityList.size(); i++)
                {
                    if (Core::SDK::Game::EntityList[i].Ped == Core::SDK::Pointers::pLocalPlayer)
                        continue;
                    if (Core::SDK::Game::EntityList[i].PedType != 2)
                        continue;
                    if (Core::SDK::Game::EntityList[i].Health <= 0.f)
                        continue;

                    IsPlayerSelected = SelectedPlayerIndex == i;
                    if (ImGui::ListSelectable(Core::SDK::Game::EntityList[i].NetworkInfo.UserName.c_str(), &IsPlayerSelected))
                        SelectedPlayerIndex = i;
                }
                ImGui::PopStyleVar();
            }
            ImGui::CustomEndChild();
        }
        ImGui::EndGroup();
        }

        ImGui::SameLine();

        if (vWo1) {
        ImGui::BeginGroup();
        {
            ImGui::CustomBeginChild("Informacoes", "Dados do Jogador", ImVec2(230, 130), false, 0);
            {
                if (SelectedPlayerIndex == -1)
                {
                    ImGui::TextColored(ImColor(g_Col.SecundaryText), "Selecione um Jogador");
                }
                else
                {
                    auto& ped = Core::SDK::Game::EntityList[SelectedPlayerIndex];
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 4));

                    ImGui::TextColored(ImColor(g_Col.FeaturesText), "Nome:"); ImGui::SameLine();
                    ImGui::TextColored(ImColor(g_Col.SecundaryText), ped.NetworkInfo.UserName.c_str());
                    ImGui::TextColored(ImColor(g_Col.FeaturesText), "ID:"); ImGui::SameLine();
                    ImGui::TextColored(ImColor(g_Col.SecundaryText), std::to_string(ped.Id).c_str());
                    ImGui::TextColored(ImColor(g_Col.FeaturesText), "Distancia:"); ImGui::SameLine();
                    ImGui::TextColored(ImColor(g_Col.SecundaryText), (std::to_string((int)ped.Distance) + "m").c_str());
                    ImGui::TextColored(ImColor(g_Col.FeaturesText), "Arma:"); ImGui::SameLine();
                    ImGui::TextColored(ImColor(g_Col.SecundaryText), ped.WeaponName.c_str());
                    ImGui::TextColored(ImColor(g_Col.FeaturesText), "Vida:"); ImGui::SameLine();
                    ImGui::TextColored(ImColor(g_Col.SecundaryText), (std::to_string((int)ped.Health) + "/" + std::to_string((int)ped.MaxHealth)).c_str());

                    ImGui::PopStyleVar();
                }
            }
            ImGui::CustomEndChild();

            ImGui::CustomBeginChild("Acoes", "Acoes no Jogador", ImVec2(230, 250), false, 0);
            {
                if (SelectedPlayerIndex == -1)
                {
                    ImGui::TextColored(ImColor(g_Col.SecundaryText), "Selecione um Jogador");
                }
                else
                {
                    auto& ped = Core::SDK::Game::EntityList[SelectedPlayerIndex];
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 5));

                    bool Friend = ped.IsFriend;
                    if (Custom::CheckBox("Amigo", &Friend))
                        Core::SDK::Game::FriendMap[ped.Ped] = Friend;

                    ImGui::Separator();
                    ImGui::Spacing();

                    if (Custom::Button("Teleportar", ImVec2(-1, 26), 0)) {
                        if (Core::SDK::Pointers::pLocalPlayer) {
                            Core::Features::g_GodMode.TeleportGuardBegin();
                            Core::SDK::Pointers::pLocalPlayer->SetPos(ped.Pos);
                            std::thread([]() {
                                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                                if (Core::SDK::Pointers::pLocalPlayer)
                                    Core::Features::g_GodMode.TeleportGuardEnd(g_Config.Player->EnableGodMode);
                                NotifyManager::Send(xorstr("Teleportado!"), 2000);
                            }).detach();
                        }
                    }

                    if (Custom::Button("Reviver", ImVec2(-1, 26), 0)) {
                        if (ped.Ped) {
                            ped.Ped->SetHealth(ped.MaxHealth > 0 ? ped.MaxHealth : 200.f);
                            NotifyManager::Send(xorstr("Jogador revivido!"), 2000);
                        }
                    }

                    if (Custom::Button("Curar", ImVec2(-1, 26), 0)) {
                        if (ped.Ped) {
                            ped.Ped->SetHealth(ped.MaxHealth > 0 ? ped.MaxHealth : 200.f);
                            NotifyManager::Send(xorstr("Vida restaurada!"), 2000);
                        }
                    }

                    if (Custom::Button("Adicionar Colete", ImVec2(-1, 26), 0)) {
                        if (ped.Ped) {
                            ped.Ped->SetArmor(100.f);
                            NotifyManager::Send(xorstr("Colete adicionado!"), 2000);
                        }
                    }

                    if (Custom::Button("Copiar Roupas", ImVec2(-1, 26), 0)) {
                        if (!ped.Ped || !Core::SDK::Pointers::pLocalPlayer) return;
                        uintptr_t dh = Mem.Read<uintptr_t>((uintptr_t)ped.Ped + 0x48);
                        uintptr_t ldh = Mem.Read<uintptr_t>((uintptr_t)Core::SDK::Pointers::pLocalPlayer + 0x48);
                        if (!dh || !ldh) return;
                        for (const auto& comp : ClothComponents) {
                            uint16_t val = Mem.Read<uint16_t>(dh + comp.Offset);
                            Mem.Write<uint16_t>(ldh + comp.Offset, val);
                        }
                        NotifyManager::Send(xorstr("Roupas copiadas!"), 2000);
                    }

                    ImGui::Separator();
                    ImGui::Spacing();

                    if (Custom::Button("Matar", ImVec2(-1, 26), 0)) {
                        if (ped.Ped) {
                            ped.Ped->SetHealth(0.f);
                            NotifyManager::Send(xorstr("Jogador morto!"), 2000);
                        }
                    }

                    if (Custom::Button("Explodir Veiculo", ImVec2(-1, 26), 0)) {
                        if (ped.Ped) {
                            CPed* target = ped.Ped;
                            if (target->InVehicle()) {
                                CVehicle* veh = target->GetLastVehicle();
                                if (veh) {
                                    float engineHealth = Mem.Read<float>((uintptr_t)veh + (g_Offsets.m_VehicleEngineHealth ? g_Offsets.m_VehicleEngineHealth : 0x280u));
                                    Mem.Write<float>((uintptr_t)veh + (g_Offsets.m_VehicleEngineHealth ? g_Offsets.m_VehicleEngineHealth : 0x280u), -1000.f);
                                    Mem.Write<uint8_t>((uintptr_t)veh + 0x2E, 2);
                                    NotifyManager::Send(xorstr("Veiculo explodido!"), 2000);
                                }
                            }
                            else {
                                NotifyManager::Send(xorstr("Jogador nao esta em veiculo!"), 4000);
                            }
                        }
                    }

                    if (Custom::Button("Congelar", ImVec2(-1, 26), 0)) {
                        if (ped.Ped) {
                            uintptr_t nav = ped.Ped->GetNavigation();
                            if (nav) {
                                Mem.Write<D3DXVECTOR3>(nav + 0x30, D3DXVECTOR3(0, 0, 0));
                                Mem.Write<D3DXVECTOR3>((uintptr_t)ped.Ped + 0x320, D3DXVECTOR3(0, 0, 0));
                            }
                            NotifyManager::Send(xorstr("Jogador congelado!"), 2000);
                        }
                    }

                    if (Custom::Button("Lancar no Ar", ImVec2(-1, 26), 0)) {
                        if (ped.Ped) {
                            D3DXVECTOR3 pos = ped.Ped->GetPos();
                            Mem.Write<D3DXVECTOR3>((uintptr_t)ped.Ped + 0x320, D3DXVECTOR3(0, 0, 50.f));
                            NotifyManager::Send(xorstr("Jogador lancado!"), 2000);
                        }
                    }

                    ImGui::PopStyleVar();
                }
            }
            ImGui::CustomEndChild();
        }
        ImGui::EndGroup();
        }

        ImGui::SameLine();

        if (vWo2) {
        ImGui::BeginGroup();
        {
            ImGui::CustomBeginChild("Roupas", "Editar Roupas (Local)", ImVec2(220, 190), false, 0);
            {
                if (Core::SDK::Pointers::pLocalPlayer) {
                    uintptr_t ldh = Mem.Read<uintptr_t>((uintptr_t)Core::SDK::Pointers::pLocalPlayer + 0x48);
                    if (ldh) {
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 5));
                        for (const auto& comp : ClothComponents) {
                            uint16_t currentVal = Mem.Read<uint16_t>(ldh + comp.Offset);
                            int drawable = currentVal & 0xFF;
                            int texture = (currentVal >> 8) & 0xFF;

                            char label[32];
                            snprintf(label, sizeof(label), "%s##%d", comp.Name, comp.Offset);
                            if (ImGui::SliderInt(label, &drawable, 0, comp.MaxDrawable, "%d")) {
                                uint16_t newVal = (uint16_t)((texture & 0xFF) << 8 | (drawable & 0xFF));
                                Mem.Write<uint16_t>(ldh + comp.Offset, newVal);
                            }
                        }
                        ImGui::PopStyleVar();
                    }
                    else {
                        ImGui::TextColored(ImColor(g_Col.SecundaryText), "DrawHandler nao encontrado");
                    }
                }
            }
            ImGui::CustomEndChild();

            ImGui::CustomBeginChild("Peds Cidade", "Peds no Mapa", ImVec2(220, 190), false, 0);
            {
                CReplayInterFace* replay = Core::SDK::Pointers::pReplayInterFace;
                if (replay) {
                    CPedInterFace* pedInterface = replay->InterfacePed();
                    if (pedInterface) {
                        CPedList* pedList = pedInterface->PedList();
                        if (pedList) {
                            int maxPed = pedInterface->MaxPed();
                            int pedCount = pedInterface->PedCount();
                            ImGui::TextColored(ImColor(g_Col.FeaturesText), "Total: "); ImGui::SameLine();
                            ImGui::TextColored(ImColor(g_Col.SecundaryText), std::to_string(pedCount).c_str());

                            ImGui::Spacing();

                            ImGui::BeginChild("PedListScroll", ImVec2(190, 140), false);
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 4));
                                int shownCount = 0;
                                for (int i = 0; i < maxPed && shownCount < 30; i++) {
                                    CPed* ped = pedList->Ped(i);
                                    if (!ped) continue;
                                    if (ped == Core::SDK::Pointers::pLocalPlayer) continue;

                                    uint32_t pedType = ped->GetPedType();
                                    uintptr_t modelInfo = Mem.Read<uintptr_t>((uintptr_t)ped + 0x20);
                                    if (!modelInfo) continue;

                                    uint32_t hash = Mem.Read<uint32_t>(modelInfo);
                                    if (hash) {
                                        float dist = ped->GetDistance(Core::SDK::Pointers::pLocalPlayer->GetPos(), ped->GetPos());
                                        if (dist > 200.f) continue;

                                        if (pedType == 2)
                                            ImGui::TextColored(ImColor(g_Col.FeaturesText), "[P] %08X %.0fm", hash, dist);
                                        else
                                            ImGui::TextColored(ImColor(g_Col.SecundaryText), "[N] %08X %.0fm", hash, dist);
                                        shownCount++;
                                    }
                                }
                                if (shownCount == 0) {
                                    ImGui::TextColored(ImColor(g_Col.SecundaryText), "Nenhum ped encontrado");
                                }
                                ImGui::PopStyleVar();
                            }
                            ImGui::EndChild();
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
