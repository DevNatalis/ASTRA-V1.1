#pragma once
#include <Includes/Includes.hpp>
#include <Core/SDK/Guard.hpp>
#include <Core/SDK/PlayerNames.hpp>
#include <algorithm>
#include <Auth/auth_manager.hpp>
#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>

namespace Core {
    namespace Threads {

        class cEntityList {
        public:
            inline std::string getPlayerNameByNetId(int netid) {
                if (netid <= 0 || netid > 16777215) return {};
                static DWORD processId = 0;
                static uintptr_t listEntry = 0;
                static uint64_t lastRefresh = 0, lastScan = 0;
                static std::unordered_map<int, std::string> names;
                if (processId != g_Variables.ProcIdFiveM) {
                    processId = g_Variables.ProcIdFiveM;
                    listEntry = 0;
                    lastRefresh = lastScan = 0;
                    names.clear();
                }
                const auto read = [](uintptr_t address, void* buffer, size_t size) {
                    SIZE_T received = 0;
                    return ReadProcessMemory(Mem.ProcHandle, reinterpret_cast<LPCVOID>(address),
                        buffer, size, &received) && received == size;
                };
                const uint64_t now = GetTickCount64();
                if (!lastRefresh || now - lastRefresh >= 1000) {
                    lastRefresh = now;
                    if (!listEntry || !PlayerNames::ReadList(read, listEntry, names)) {
                        names.clear();
                        listEntry = 0;
                        // Atalho barato antes do scan: RVA direto do offdet.h
                        // (PlayerNames b3258 = 0x1E63C68, LEA .data). Se o modulo
                        // trocar a global de lugar, o scan abaixo assume.
                        if (g_Offsets.CurrentBuild == 3258 && Mem.ModBase) {
                            const uintptr_t direct = Mem.ModBase + 0x1E63C68;
                            if (PlayerNames::ReadList(read, direct, names))
                                listEntry = direct;
                        }
                        if (!lastScan || now - lastScan >= 5000) {
                            lastScan = now;
                            uintptr_t size = 0;
                            const uintptr_t base = Mem.GetModuleBaseAddr(processId,
                                "citizen-playernames-five.dll", &size);
                            std::unordered_set<uintptr_t> checked;
                            // Scan RIP-relative references and validate the complete list.
                            for (uintptr_t offset = 0; base && offset + 7 <= size && !listEntry; offset += 4089) {
                                unsigned char bytes[4096]{};
                                const size_t length = (std::min)(size_t(4096), size_t(size - offset));
                                if (!read(base + offset, bytes, length)) continue;
                                for (size_t i = 0; i + 7 <= length && !listEntry; ++i) {
                                    if ((bytes[i] & 0xF8) != 0x48 ||
                                        (bytes[i+1] != 0x8B && bytes[i+1] != 0x8D) ||
                                        (bytes[i+2] & 0xC7) != 0x05) continue;
                                    int32_t displacement = 0;
                                    std::memcpy(&displacement, bytes+i+3, 4);
                                    const uintptr_t target = base + offset + i + 7 + displacement;
                                    for (int adjustment : {0, 8, -8}) {
                                        const uintptr_t candidate = target + adjustment;
                                        if (candidate < base || candidate + 16 > base + size ||
                                            !checked.insert(candidate).second) continue;
                                        if (PlayerNames::ReadList(read, candidate, names)) {
                                            listEntry = candidate;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (!listEntry)
                                Debug::Warning("ESP Name", "Valid player-name list not found", 10000);
                        }
                    }
                }
                const auto found = names.find(netid);
                return found == names.end() ? std::string{} : found->second;
            }

            void Update() {
                while (true) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                    // Gate de sessao (VULN 1): sem recibo valido do servidor,
                    // nenhum dado de entidade e coletado — flipar IsLogged
                    // em memoria nao libera ESP/aimbot.
                    if (!g_Auth.IsSessionValid())
                        continue;

                    CReplayInterFace* replayInterface = Core::SDK::Pointers::pReplayInterFace;
                    if (!replayInterface)
                        continue;

                    CPedInterFace* pedInterface = replayInterface->InterfacePed();
                    if (!pedInterface)
                        continue;

                    CPedList* pedList = pedInterface->PedList();
                    if (!pedList)
                        continue;

                    CPed* localPlayer = Core::SDK::Pointers::pLocalPlayer;
                    if (!localPlayer)
                        continue;

                    const int maxPed = pedInterface->MaxPed();
                    // STABILITY: MaxPed vem da memoria do jogo — sem clamp, um valor
                    // corrompido (ex: 200000) vira loop gigante + reserve enorme.
                    // Causa concreta: reserve(maxPed) sem limite + Ped(i) sem validar.
                    const int safeMax = Guard::ClampCount(maxPed, 512);
                    if (safeMax <= 0)
                        continue;
                    std::vector<Core::SDK::Game::EntityStruct> freshEntities;
                    freshEntities.reserve(static_cast<size_t>(safeMax));

                    const D3DXVECTOR3 localPos = localPlayer->GetPos();
                    for (int i = 0; i < safeMax; ++i) {
                        CPed* currentPed = pedList->Ped(i);
                        if (!Guard::IsRemotePtr((uintptr_t)currentPed))
                            continue;

                        Core::SDK::Game::EntityStruct entity{};
                        entity.Ped = currentPed;
                        entity.Id = currentPed->GetID();
                        entity.Index = i;
                        entity.Pos = currentPed->GetPos();
                        entity.IsFriend = Core::SDK::Game::GetFriend(currentPed);
                        entity.MaxHealth = currentPed->GetMaxHealth();
                        entity.Health = currentPed->GetHealth();
                        entity.Armor = currentPed->GetArmor();
                        entity.PedType = currentPed->GetPedType();
                        entity.IsPlayer = currentPed->GetPlayerInfo() != nullptr;
                        entity.Distance = currentPed->GetDistance(localPos, entity.Pos);

                        CWeaponManager* weaponManager = currentPed->GetWeaponManager();
                        if (weaponManager) {
                            CWeaponInfo* weaponInfo = weaponManager->GetWeaponInfo();
                            if (weaponInfo)
                                entity.WeaponName = weaponInfo->GetName();
                        }

                        // Match exact server IDs. Never guess names from arbitrary fields.
                        if (entity.IsPlayer && entity.Id > 0) {
                            entity.NetworkInfo.UserName = getPlayerNameByNetId(entity.Id);
                            if (entity.NetworkInfo.UserName.empty()) {
                                Core::SDK::Game::NetworkInfo known{};
                                if (g_UpdateNames.GetById(entity.Id, known))
                                    entity.NetworkInfo = std::move(known);
                            }
                        }

                        freshEntities.push_back(std::move(entity));
                    }

                    Core::SDK::Game::PublishEntityList(freshEntities);
                }
            }
        };

        inline cEntityList g_EntityList;
    }
}
