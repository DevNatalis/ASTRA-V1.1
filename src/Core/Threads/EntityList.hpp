#pragma once
#include <Includes/Includes.hpp>
#include <Core/SDK/Guard.hpp>
#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>

namespace Core {
    namespace Threads {

        class cEntityList {
        public:
            inline std::string getPlayerNameByNetId(int netid) {
                std::string name = "npc";
                if (netid <= 0) return name;

                uintptr_t moduleSize = 0;
                const uintptr_t moduleBase = Mem.GetModuleBaseAddr(
                    g_Variables.ProcIdFiveM,
                    "citizen-playernames-five.dll",
                    &moduleSize
                );

                if (!moduleBase || !moduleSize)
                    return name;

                static uintptr_t cachedOffset = 0;

                if (!cachedOffset) {
                    const size_t chunkSize = 4096;
                    std::vector<uint8_t> buf(chunkSize);

                    for (uintptr_t addr = moduleBase; addr < moduleBase + moduleSize - 7; addr += chunkSize - 7) {
                        SIZE_T bytesRead = 0;
                        if (!ReadProcessMemory(Mem.ProcHandle, (LPCVOID)addr, buf.data(), chunkSize, &bytesRead) || bytesRead < 7)
                            continue;

                        for (size_t i = 0; i < bytesRead - 6; i++) {
                            if (buf[i] != 0x48 || buf[i + 1] != 0x8B || buf[i + 2] != 0x0D)
                                continue;

                            int32_t disp = *(int32_t*)(buf.data() + i + 3);
                            uintptr_t resolved = (addr + i) + 7 + disp;
                            if (resolved < moduleBase || resolved >= moduleBase + moduleSize)
                                continue;

                            uint8_t checkBuf[24];
                            SIZE_T checkRead = 0;
                            if (!ReadProcessMemory(Mem.ProcHandle, (LPCVOID)resolved, checkBuf, 24, &checkRead) || checkRead != 24)
                                continue;

                            uintptr_t ptr = *(uintptr_t*)checkBuf;
                            int32_t count = *(int32_t*)(checkBuf + 8);
                            if (!ptr || count <= 0 || count > 256)
                                continue;

                            uintptr_t listHead = 0;
                            if (!ReadProcessMemory(Mem.ProcHandle, (LPCVOID)(ptr + 0x8), &listHead, sizeof(listHead), &checkRead) || !listHead)
                                continue;

                            uint8_t nodeBuf[32];
                            if (!ReadProcessMemory(Mem.ProcHandle, (LPCVOID)listHead, nodeBuf, 32, &checkRead) || checkRead != 32)
                                continue;

                            int32_t nodeId = *(int32_t*)(nodeBuf + 0x10);
                            if (nodeId >= 0 && nodeId < 10000) {
                                cachedOffset = resolved - moduleBase;
                                break;
                            }
                        }
                        if (cachedOffset) break;
                    }
                }

                if (!cachedOffset)
                    return name;

                const uintptr_t playerNamesArray = Mem.Read<uintptr_t>(moduleBase + cachedOffset);
                if (!playerNamesArray)
                    return name;

                const int lastPlayer = Mem.Read<int>(moduleBase + cachedOffset + 0x8);
                if (lastPlayer <= 0 || lastPlayer > 256)
                    return name;

                uintptr_t list = Mem.Read<uintptr_t>(playerNamesArray + 0x8);
                if (!list)
                    return name;

                for (int i = 0; i < lastPlayer && list; ++i) {
                    const int id = Mem.Read<int>(list + 0x10);
                    if (netid == id)
                        return Mem.ReadString(list + 0x18);
                    list = Mem.Read<uintptr_t>(list + 0x8);
                }

                return name;
            }

            void Update() {
                while (true) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));

                    if (!g_MenuInfo.IsLogged && !g_Variables.g_bPassedByThisVerify)
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

                        entity.NetworkInfo.UserName = getPlayerNameByNetId(entity.Id);
                        if (entity.NetworkInfo.UserName == "npc" || entity.NetworkInfo.UserName.empty()) {
                            CPlayerInfo* pInfo = currentPed->GetPlayerInfo();
                            if (pInfo) {
                                std::string pedName = pInfo->GetName();
                                if (!pedName.empty() && pedName.size() > 1)
                                    entity.NetworkInfo.UserName = pedName;
                            }
                        }
                        if ((entity.NetworkInfo.UserName == "npc" || entity.NetworkInfo.UserName.empty()) && entity.IsPlayer) {
                            uintptr_t netPlayer = Mem.Read<uintptr_t>((uintptr_t)currentPed + 0xD0);
                            if (netPlayer) {
                                uintptr_t cnetGamePlayer = Mem.Read<uintptr_t>(netPlayer + 0xB0);
                                if (cnetGamePlayer) {
                                    std::string netName = Mem.ReadString(cnetGamePlayer + 0x08);
                                    if (!netName.empty() && netName.size() > 1 && netName.size() < 100) {
                                        entity.NetworkInfo.UserName = netName;
                                    }
                                }
                            }
                        }
                        if ((entity.NetworkInfo.UserName == "npc" || entity.NetworkInfo.UserName.empty()) && entity.IsPlayer) {
                            entity.NetworkInfo.UserName = "Jogador_" + std::to_string(entity.Id);
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