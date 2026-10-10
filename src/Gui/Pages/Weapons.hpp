#pragma once
#include <windows.h>
#include <iostream>
#include <thread>
#include <cstdint>
#include <string>
#include <vector>
#include <d3d11.h>
#include <Includes/includes.hpp>
#include <Core/Core.hpp>
#include <Core/Features/WeaponWheel.hpp>
#include <Core/Features/Exploits/ResourceList.hpp>
#include <Core/Features/Exploits/GiveWeapon.hpp>
#include <Includes/CustomWidgets/Notify.hpp>

using namespace std;

namespace Weapons {

    enum eWeaponTypes {
        Melee, Handguns, SMGs, Shotguns, AssaultRifles, MachineGuns, SniperRifles, HeavyWeapons, Throwables, Misc
    };

    struct WeaponSpawn_t {
        std::string Name;
        uint32_t WeaponHash;
        ID3D11ShaderResourceView* Picture;
        eWeaponTypes WeaponType;

        WeaponSpawn_t(const std::string& name, uint32_t hash, ID3D11ShaderResourceView* pic, eWeaponTypes type)
            : Name(name), WeaponHash(hash), Picture(pic), WeaponType(type) {
        }
    };

    void Render()
    {
        int subWe = g_MenuInfo.SubPage[g_MenuInfo.Weapons];
        bool vWe0 = (subWe < 0 || subWe == 0), vWe1 = (subWe < 0 || subWe == 1), vWe2 = (subWe < 0 || subWe == 2);

        // === CHILD 1: Geral (Recoil, Spread, No Reload, Infinite Ammo, Add Ammo) ===
        if (vWe0) {
        ImGui::BeginGroup();
        {
			ImGui::CustomBeginChild(xorstr("Geral"), xorstr("Armas"), ImVec2(228, 390), false, 0);
            {
                auto pLocal = Core::SDK::Pointers::pLocalPlayer;
                auto pWM = pLocal ? pLocal->GetWeaponManager() : nullptr;
                float recoilBackup = pWM ? pWM->GetRecoil() : 0.f;
                float spreadBackup = pWM ? pWM->GetSpread() : 0.f;

                if (Custom::CheckBox(xorstr("Forcar Roda de Armas"), &g_Config.Player->ForceWeaponWheel))
                {
                    Core::Features::g_WeaponWheel.SetEnabled(g_Config.Player->ForceWeaponWheel);
                    if (g_Config.Player->ForceWeaponWheel)
                        Core::Features::g_WeaponWheel.ForceWeaponWheel();
                    NotifyManager::Send(std::string("Roda de Armas ") + (g_Config.Player->ForceWeaponWheel ? "forcada" : "normal"), 2000);
                }

                ImGui::Spacing();

                if (Custom::CheckBoxCfg(xorstr("Controle de Recoil"), &g_Config.Player->NoRecoilEnabled, [&]() {
                    if (ImGui::SliderFloat(xorstr("Valor"), &g_Config.Player->RecoilValue, 0.0f, 10.f, xorstr("%1.2fx"))) {
                        if (g_Config.Player->NoRecoilEnabled && pWM) {
                            pWM->SetRecoil(g_Config.Player->RecoilValue);
                        }
                    }
                    })) {
                    NotifyManager::Send(std::string("Controle de Recoil ") + (g_Config.Player->NoRecoilEnabled ? "ativado" : "desativado"), 2000);
                }
                else {
                    if (!g_Config.Player->NoRecoilEnabled && pWM) {
                        pWM->SetRecoil(recoilBackup);
                    }
                }

                if (Custom::CheckBoxCfg(xorstr("Controle de Dispersao"), &g_Config.Player->NoSpreadEnabled, [&]() {
                    if (ImGui::SliderFloat(xorstr("Valor"), &g_Config.Player->SpreadValue, 0.0f, 10.f, xorstr("%1.2fx"))) {
                        if (g_Config.Player->NoSpreadEnabled && pWM) {
                            pWM->SetSpread(g_Config.Player->SpreadValue);
                        }
                    }
                    })) {
                    NotifyManager::Send(std::string("Controle de Dispersao ") + (g_Config.Player->NoSpreadEnabled ? "ativado" : "desativado"), 2000);
                }
                else {
                    if (!g_Config.Player->NoSpreadEnabled && pWM) {
                        pWM->SetSpread(spreadBackup);
                    }
                }

                if (Custom::CheckBox(xorstr("Sem Recarga"), &g_Config.Player->NoReloadEnabled)) {
                    if (g_Config.Player->NoReloadEnabled) {
                        Mem.PatchFunc(g_Offsets.m_InfiniteAmmo0, 3);
                    }
                    else {
                        Mem.WriteBytes(g_Offsets.m_InfiniteAmmo0, { 0x41, 0x2B, 0xC9, 0x3B, 0xC8, 0x0F, 0x4D, 0xC8 });
                    }
                    NotifyManager::Send(std::string("Sem Recarga ") + (g_Config.Player->NoReloadEnabled ? "ativado" : "desativado"), 2000);
                }

                if (Custom::CheckBox(xorstr("Municao Infinita"), &g_Config.Player->InfiniteAmmoEnabled)) {
                    if (g_Config.Player->InfiniteAmmoEnabled) {
                        Mem.PatchFunc(g_Offsets.m_InfiniteAmmo0, 3);
                        Mem.PatchFunc(g_Offsets.m_InfiniteAmmo1, 3);
                    }
                    else {
                        if (!g_Config.Player->NoReloadEnabled) {
                            Mem.WriteBytes(g_Offsets.m_InfiniteAmmo0, { 0x41, 0x2B, 0xC9, 0x3B, 0xC8, 0x0F, 0x4D, 0xC8 });
                        }
                        Mem.WriteBytes(g_Offsets.m_InfiniteAmmo1, { 0x41, 0x2B, 0xD1, 0xE8 });
                    }
                    NotifyManager::Send(std::string("Municao Infinita ") + (g_Config.Player->InfiniteAmmoEnabled ? "ativado" : "desativado"), 2000);
                }

                static int AmmoAdd = 0;
                ImGui::SliderInt(xorstr("Quantidade de Municao"), &AmmoAdd, 0, 250);
                ImGui::Spacing();

                if (Custom::Button(xorstr("Adicionar Municao"), ImVec2(-1, 33), 0)) {
                    if (!pLocal) {
                        NotifyManager::Send(xorstr("Voce precisa estar no jogo!"), 4000);
                    }
                    else {
                        CWeaponInfo* pCWeaponInfo = pLocal->GetWeaponManager()->GetWeaponInfo();
                        if (pCWeaponInfo) {
                            uintptr_t Lixo = Mem.FindSignature(
                                { 0x48, 0x8b, 0x43, 0x00, 0x48, 0x83, 0xc4, 0x00, 0x5b, 0xc3, 0xcc, 0x40, 0x53 }
                            );

                            uintptr_t AmmoInfo = Mem.Read<uintptr_t>((uintptr_t)pCWeaponInfo + Mem.Read<BYTE>(Lixo + 3));
                            if (AmmoInfo) {
                                uintptr_t pAmmoCount = Mem.Read<uintptr_t>((uintptr_t)AmmoInfo + 0x8);
                                if (pAmmoCount) {
                                    uintptr_t CurrentAmmoAddr = Mem.Read<uintptr_t>(pAmmoCount + 0x0) + 0x18;
                                    int CurrentAmmo = Mem.Read<int>(CurrentAmmoAddr);
                                    Mem.Write(CurrentAmmoAddr, CurrentAmmo + AmmoAdd);
                                }
                            }
                            else {
                                NotifyManager::Send(xorstr("Pegue uma arma primeiro!"), 4000);
                            }
                        }
                    }
                }
            }
            ImGui::CustomEndChild();
        }
        ImGui::EndGroup();
        }

        ImGui::SameLine();

        // === CHILD 2: Weapons Spawn (lista) ===
        if (vWe1) {
        ImGui::BeginGroup();
        {
            ImGui::CustomBeginChild(xorstr("Arma"), xorstr("Criar"), ImVec2(228, 390), false, 0);
            {
                static bool bMelee = true, bHandguns = true, bSMGs = true, bShotguns = true, bAssaultRifles = true;
                static bool bMachineGuns = true, bSniperRifles = true, bHeavyWeapons = true, bThrowables = true, bMisc = true;
                static bool ShowWeaponFilters = false;
                static int Ammo = 255;

                ImGui::Text(xorstr("Filtros:"));
                ImGui::SameLine();

                Custom::CheckBoxCfg("", &ShowWeaponFilters, [&]() {
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 6));
                    Custom::CheckBox("Corpo a Corpo", &bMelee);
                    Custom::CheckBox("Pistolas", &bHandguns);
                    Custom::CheckBox("SMGs", &bSMGs);
                    Custom::CheckBox("Shotguns", &bShotguns);
                    Custom::CheckBox("Fuzis", &bAssaultRifles);
                    Custom::CheckBox("Metralhadoras", &bMachineGuns);
                    Custom::CheckBox("Rifles de Precisao", &bSniperRifles);
                    Custom::CheckBox("Armas Pesadas", &bHeavyWeapons);
                    Custom::CheckBox("Arremessaveis", &bThrowables);
                    Custom::CheckBox("Diversos", &bMisc);
                    ImGui::PopStyleVar();
                    });

                ImGui::Spacing();

                std::vector<WeaponSpawn_t> WeaponList = {
                    // Melee Weapons
                                                WeaponSpawn_t(xorstr("Dagger"), 0x92A27487, g_Variables.FiveM_WeaponsPic.weapon_dagger, Melee),
                                                WeaponSpawn_t(xorstr("Bat"), 0x958A4A8F, g_Variables.FiveM_WeaponsPic.weapon_bat, Melee),
                                                WeaponSpawn_t(xorstr("Bottle"), 0xF9E6AA4B, g_Variables.FiveM_WeaponsPic.weapon_bottle, Melee),
                                                WeaponSpawn_t(xorstr("Crowbar"), 0x84BD7BFD, g_Variables.FiveM_WeaponsPic.weapon_crowbar, Melee),
                                                // WeaponSpawn_t(xorstr("Unarmed"), 0xA2719263, g_Variables.FiveM_WeaponsPic.weapon_unarmed, Melee),
                                                WeaponSpawn_t(xorstr("Flashlight"), 0x8BB05FD7, g_Variables.FiveM_WeaponsPic.weapon_flashlight, Melee),
                                                WeaponSpawn_t(xorstr("GolfClub"), 0x440E4788, g_Variables.FiveM_WeaponsPic.weapon_golfclub, Melee),
                                                WeaponSpawn_t(xorstr("Hammer"), 0x4E875F73, g_Variables.FiveM_WeaponsPic.weapon_hammer, Melee),
                                                WeaponSpawn_t(xorstr("Hatchet"), 0xF9DCBF2D, g_Variables.FiveM_WeaponsPic.weapon_hatchet, Melee),
                                                WeaponSpawn_t(xorstr("Knuckle"), 0xD8DF3C3C, g_Variables.FiveM_WeaponsPic.weapon_knuckle, Melee),
                                                WeaponSpawn_t(xorstr("Knife"), 0x99B507EA, g_Variables.FiveM_WeaponsPic.weapon_knife, Melee),
                                                WeaponSpawn_t(xorstr("Machete"), 0xDD5DF8D9, g_Variables.FiveM_WeaponsPic.weapon_machete, Melee),
                                                WeaponSpawn_t(xorstr("Switchblade"), 0xDFE37640, g_Variables.FiveM_WeaponsPic.weapon_switchblade, Melee),
                                                WeaponSpawn_t(xorstr("Nightstick"), 0x678B81B1, g_Variables.FiveM_WeaponsPic.weapon_nightstick, Melee),
                                                WeaponSpawn_t(xorstr("Wrench"), 0x19044EE0, g_Variables.FiveM_WeaponsPic.weapon_wrench, Melee),
                                                WeaponSpawn_t(xorstr("BattleAxe"), 0xCD274149, g_Variables.FiveM_WeaponsPic.weapon_battleaxe, Melee),
                                                WeaponSpawn_t(xorstr("PoolCue"), 0x94117305, g_Variables.FiveM_WeaponsPic.weapon_poolcue, Melee),
                                                WeaponSpawn_t(xorstr("StoneHatchet"), 0x3813FC08, g_Variables.FiveM_WeaponsPic.weapon_stone_hatchet, Melee),

                                                // Handguns
                                                WeaponSpawn_t(xorstr("Pistol"), 0x1B06D571, g_Variables.FiveM_WeaponsPic.weapon_pistol, Handguns),
                                                WeaponSpawn_t(xorstr("PistolMk2"), 0xBFE256D4, g_Variables.FiveM_WeaponsPic.weapon_pistol_mk2, Handguns),
                                                WeaponSpawn_t(xorstr("CombatPistol"), 0x5EF9FEC4, g_Variables.FiveM_WeaponsPic.weapon_combatpistol, Handguns),
                                                WeaponSpawn_t(xorstr("APPistol"), 0x22D8FE39, g_Variables.FiveM_WeaponsPic.weapon_appistol, Handguns),
                                                WeaponSpawn_t(xorstr("StunGun"), 0x3656C8C1, g_Variables.FiveM_WeaponsPic.weapon_stungun, Handguns),
                                                WeaponSpawn_t(xorstr("Pistol50"), 0x99AEEB3B, g_Variables.FiveM_WeaponsPic.weapon_pistol50, Handguns),
                                                WeaponSpawn_t(xorstr("SNSPistol"), 0xBFD21232, g_Variables.FiveM_WeaponsPic.weapon_snspistol, Handguns),
                                                WeaponSpawn_t(xorstr("SNSPistolMk2"), 0x88374054, g_Variables.FiveM_WeaponsPic.weapon_snspistol_mk2, Handguns),
                                                WeaponSpawn_t(xorstr("HeavyPistol"), 0xD205520E, g_Variables.FiveM_WeaponsPic.weapon_heavypistol, Handguns),
                                                WeaponSpawn_t(xorstr("VintagePistol"), 0x83839C4, g_Variables.FiveM_WeaponsPic.weapon_vintagepistol, Handguns),
                                                WeaponSpawn_t(xorstr("FlareGun"), 0x47757124, g_Variables.FiveM_WeaponsPic.weapon_flaregun, Handguns),
                                                WeaponSpawn_t(xorstr("MarksmanPistol"), 0xDC4DB296, g_Variables.FiveM_WeaponsPic.weapon_marksmanpistol, Handguns),
                                                WeaponSpawn_t(xorstr("Revolver"), 0xC1B3C3D1, g_Variables.FiveM_WeaponsPic.weapon_revolver, Handguns),
                                                WeaponSpawn_t(xorstr("RevolverMk2"), 0xCB96392F, g_Variables.FiveM_WeaponsPic.weapon_revolver_mk2, Handguns),
                                                WeaponSpawn_t(xorstr("DoubleAction"), 0x97EA20B8, g_Variables.FiveM_WeaponsPic.weapon_doubleaction, Handguns),
                                                WeaponSpawn_t(xorstr("RayPistol"), 0xAF3696A1, g_Variables.FiveM_WeaponsPic.weapon_raypistol, Handguns),
                                                WeaponSpawn_t(xorstr("CeramicPistol"), 0x2B5EF5EC, g_Variables.FiveM_WeaponsPic.weapon_ceramicpistol, Handguns),
                                                WeaponSpawn_t(xorstr("NavyRevolver"), 0x917F6C8C, g_Variables.FiveM_WeaponsPic.weapon_navyrevolver, Handguns),

                                                // SMGs
                                                WeaponSpawn_t(xorstr("MicroSMG"), 0x13532244, g_Variables.FiveM_WeaponsPic.weapon_microsmg, SMGs),
                                                WeaponSpawn_t(xorstr("SMG"), 0x2BE6766B, g_Variables.FiveM_WeaponsPic.weapon_smg, SMGs),
                                                WeaponSpawn_t(xorstr("SMGMk2"), 0x78A97CD0, g_Variables.FiveM_WeaponsPic.weapon_smg_mk2, SMGs),
                                                WeaponSpawn_t(xorstr("AssaultSMG"), 0xEFE7E2DF, g_Variables.FiveM_WeaponsPic.weapon_assaultsmg, SMGs),
                                                WeaponSpawn_t(xorstr("CombatPDW"), 0xA3D4D34, g_Variables.FiveM_WeaponsPic.weapon_combatpdw, SMGs),
                                                WeaponSpawn_t(xorstr("MachinePistol"), 0xDB1AA450, g_Variables.FiveM_WeaponsPic.weapon_machinepistol, SMGs),
                                                WeaponSpawn_t(xorstr("MiniSMG"), 0xBD248B55, g_Variables.FiveM_WeaponsPic.weapon_minismg, SMGs),
                                                WeaponSpawn_t(xorstr("RayCarbine"), 0x476BF155, g_Variables.FiveM_WeaponsPic.weapon_raycarbine, SMGs),

                                                // Shotguns
                                                WeaponSpawn_t(xorstr("PumpShotgun"), 0x1D073A89, g_Variables.FiveM_WeaponsPic.weapon_pumpshotgun, Shotguns),
                                                WeaponSpawn_t(xorstr("PumpShotgunMk2"), 0x555AF99A, g_Variables.FiveM_WeaponsPic.weapon_pumpshotgun_mk2, Shotguns),
                                                WeaponSpawn_t(xorstr("SawnoffShotgun"), 0x7846A318, g_Variables.FiveM_WeaponsPic.weapon_sawnoffshotgun, Shotguns),
                                                WeaponSpawn_t(xorstr("AssaultShotgun"), 0xE284C527, g_Variables.FiveM_WeaponsPic.weapon_assaultshotgun, Shotguns),
                                                WeaponSpawn_t(xorstr("BullpupShotgun"), 0x9D61E50F, g_Variables.FiveM_WeaponsPic.weapon_bullpupshotgun, Shotguns),
                                                WeaponSpawn_t(xorstr("Musket"), 0xA89CB99E, g_Variables.FiveM_WeaponsPic.weapon_musket, Shotguns),
                                                WeaponSpawn_t(xorstr("HeavyShotgun"), 0x3AABBBAA, g_Variables.FiveM_WeaponsPic.weapon_heavyshotgun, Shotguns),

                                                // Assault Rifles
                                                WeaponSpawn_t(xorstr("AssaultRifle"), 0xBFEFFF6D, g_Variables.FiveM_WeaponsPic.weapon_assaultrifle, AssaultRifles),
                                                WeaponSpawn_t(xorstr("AssaultRifleMk2"), 0x394F415C, g_Variables.FiveM_WeaponsPic.weapon_assaultrifle_mk2, AssaultRifles),
                                                WeaponSpawn_t(xorstr("CarbineRifle"), 0x83BF0278, g_Variables.FiveM_WeaponsPic.weapon_carbinerifle, AssaultRifles),
                                                WeaponSpawn_t(xorstr("CarbineRifleMk2"), 0xFAD1F1C9, g_Variables.FiveM_WeaponsPic.weapon_carbinerifle_mk2, AssaultRifles),
                                                WeaponSpawn_t(xorstr("AdvancedRifle"), 0xAF113F99, g_Variables.FiveM_WeaponsPic.weapon_advancedrifle, AssaultRifles),
                                                WeaponSpawn_t(xorstr("SpecialCarbine"), 0xC0A3098D, g_Variables.FiveM_WeaponsPic.weapon_specialcarbine, AssaultRifles),
                                                WeaponSpawn_t(xorstr("SpecialCarbineMk2"), 0x969C3D67, g_Variables.FiveM_WeaponsPic.weapon_specialcarbine_mk2, AssaultRifles),
                                                WeaponSpawn_t(xorstr("BullpupRifle"), 0x7F229F94, g_Variables.FiveM_WeaponsPic.weapon_bullpuprifle, AssaultRifles),
                                                WeaponSpawn_t(xorstr("BullpupRifleMk2"), 0x84D6FAFD, g_Variables.FiveM_WeaponsPic.weapon_bullpuprifle_mk2, AssaultRifles),
                                                WeaponSpawn_t(xorstr("CompactRifle"), 0x624FE830, g_Variables.FiveM_WeaponsPic.weapon_compactrifle, AssaultRifles),

                                                // Machine Guns
                                                WeaponSpawn_t(xorstr("MG"), 0x9D07F764, g_Variables.FiveM_WeaponsPic.weapon_mg, MachineGuns),
                                                WeaponSpawn_t(xorstr("CombatMG"), 0x7FD62962, g_Variables.FiveM_WeaponsPic.weapon_combatmg, MachineGuns),
                                                WeaponSpawn_t(xorstr("CombatMGMk2"), 0xDBBD7280, g_Variables.FiveM_WeaponsPic.weapon_combatmg_mk2, MachineGuns),
                                                WeaponSpawn_t(xorstr("Gusenberg"), 0x61012683, g_Variables.FiveM_WeaponsPic.weapon_gusenberg, MachineGuns),

                                                // Sniper Rifles
                                                WeaponSpawn_t(xorstr("SniperRifle"), 0x5FC3C11, g_Variables.FiveM_WeaponsPic.weapon_sniperrifle, SniperRifles),
                                                WeaponSpawn_t(xorstr("HeavySniper"), 0xC472FE2, g_Variables.FiveM_WeaponsPic.weapon_heavysniper, SniperRifles),
                                                WeaponSpawn_t(xorstr("HeavySniperMk2"), 0xA914799, g_Variables.FiveM_WeaponsPic.weapon_heavysniper_mk2, SniperRifles),
                                                WeaponSpawn_t(xorstr("MarksmanRifle"), 0xC734385A, g_Variables.FiveM_WeaponsPic.weapon_marksmanrifle, SniperRifles),
                                                WeaponSpawn_t(xorstr("MarksmanRifleMk2"), 0x6A6C02E0, g_Variables.FiveM_WeaponsPic.weapon_marksmanrifle_mk2, SniperRifles),

                                                // Heavy Weapons
                                                WeaponSpawn_t(xorstr("RPG"), 0xB1CA77B1, g_Variables.FiveM_WeaponsPic.weapon_rpg, HeavyWeapons),
                                                WeaponSpawn_t(xorstr("GrenadeLauncher"), 0xA284510B, g_Variables.FiveM_WeaponsPic.weapon_grenadelauncher, HeavyWeapons),
                                                WeaponSpawn_t(xorstr("GrenadeLauncherSmoke"), 0x4DD2DC56, g_Variables.FiveM_WeaponsPic.weapon_grenadelauncher_smoke, HeavyWeapons),
                                                WeaponSpawn_t(xorstr("Minigun"), 0x42BF8A85, g_Variables.FiveM_WeaponsPic.weapon_minigun, HeavyWeapons),
                                                WeaponSpawn_t(xorstr("Firework"), 0x7F7497E5, g_Variables.FiveM_WeaponsPic.weapon_firework, HeavyWeapons),
                                                WeaponSpawn_t(xorstr("Railgun"), 0x6D544C99, g_Variables.FiveM_WeaponsPic.weapon_railgun, HeavyWeapons),
                                                WeaponSpawn_t(xorstr("HomingLauncher"), 0x63AB0442, g_Variables.FiveM_WeaponsPic.weapon_hominglauncher, HeavyWeapons),
                                                WeaponSpawn_t(xorstr("CompactGrenadeLauncher"), 0x781FE4A, g_Variables.FiveM_WeaponsPic.weapon_compactlauncher, HeavyWeapons),
                                                WeaponSpawn_t(xorstr("RayMinigun"), 0xB62D1F67, g_Variables.FiveM_WeaponsPic.weapon_rayminigun, HeavyWeapons),

                                                // Throwables
                                                WeaponSpawn_t(xorstr("Grenade"), 0x93E220BD, g_Variables.FiveM_WeaponsPic.weapon_grenade, Throwables),
                                                WeaponSpawn_t(xorstr("BZGas"), 0xA0973D5E, g_Variables.FiveM_WeaponsPic.weapon_bzgas, Throwables),
                                                WeaponSpawn_t(xorstr("Molotov"), 0x24B17070, g_Variables.FiveM_WeaponsPic.weapon_molotov, Throwables),
                                                WeaponSpawn_t(xorstr("StickyBomb"), 0x2C3731D9, g_Variables.FiveM_WeaponsPic.weapon_stickybomb, Throwables),
                                                WeaponSpawn_t(xorstr("ProximityMine"), 0xAB564B93, g_Variables.FiveM_WeaponsPic.weapon_proxmine, Throwables),
                                                WeaponSpawn_t(xorstr("Snowball"), 0x787F0BB, g_Variables.FiveM_WeaponsPic.weapon_snowball, Throwables),
                                                WeaponSpawn_t(xorstr("PipeBomb"), 0xBA45E8B8, g_Variables.FiveM_WeaponsPic.weapon_pipebomb, Throwables),
                                                WeaponSpawn_t(xorstr("Ball"), 0x23C9F95C, g_Variables.FiveM_WeaponsPic.weapon_ball, Throwables),
                                                WeaponSpawn_t(xorstr("SmokeGrenade"), 0xFDBC8A50, g_Variables.FiveM_WeaponsPic.weapon_smokegrenade, Throwables),
                                                WeaponSpawn_t(xorstr("Flare"), 0x497FACC3, g_Variables.FiveM_WeaponsPic.weapon_flare, Throwables),

                                                // Miscellaneous
                                                WeaponSpawn_t(xorstr("PetrolCan"), 0x34A67B97, g_Variables.FiveM_WeaponsPic.weapon_petrolcan, Misc),
                                                WeaponSpawn_t(xorstr("Parachute"), 0xFBAB5776, g_Variables.FiveM_WeaponsPic.gadget_parachute, Misc),
                                                WeaponSpawn_t(xorstr("FireExtinguisher"), 0x60EC506, g_Variables.FiveM_WeaponsPic.weapon_fireextinguisher, Misc)
                };

                for (auto& Weapon : WeaponList) {
                    if (Weapon.Name.empty()) continue;
                    if ((Weapon.WeaponType == Melee && !bMelee) ||
                        (Weapon.WeaponType == Handguns && !bHandguns) ||
                        (Weapon.WeaponType == SMGs && !bSMGs) ||
                        (Weapon.WeaponType == Shotguns && !bShotguns) ||
                        (Weapon.WeaponType == AssaultRifles && !bAssaultRifles) ||
                        (Weapon.WeaponType == MachineGuns && !bMachineGuns) ||
                        (Weapon.WeaponType == SniperRifles && !bSniperRifles) ||
                        (Weapon.WeaponType == HeavyWeapons && !bHeavyWeapons) ||
                        (Weapon.WeaponType == Throwables && !bThrowables) ||
                        (Weapon.WeaponType == Misc && !bMisc))
                        continue;

                    // Usar selectable texto clic�vel (igual seu pedido)
                    if (ImGui::Selectable(Weapon.Name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                        std::thread(&Core::Features::Exploits::cWeaponSpawn::Spawn, Core::Features::Exploits::g_WeaponSpawn, Weapon.WeaponHash, Ammo).detach();
                        std::thread NotifyThread(NotifyManager::Send, Weapon.Name + xorstr(" foi criada."), 4000);
                        NotifyThread.detach();
                    }
                }
            }
            ImGui::CustomEndChild();
        }
        ImGui::EndGroup();
        }

        ImGui::SameLine();

        // === CHILD 3: Resources ===
        if (vWe2) {
        ImGui::BeginGroup();
        {
            ImGui::CustomBeginChild(xorstr("Recursos"), xorstr(""), ImVec2(228, 390), false, 0);
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 8));
                for (int i = 0; i < Features::Exploits::vResources.size(); i++)
                {
                    if (Custom::ResourceListButton(Features::Exploits::vResources[i].Path.c_str(), Features::Exploits::vResources[i].State, ImVec2(-1, 38), 0))
                    {
                        Features::Exploits::vResources[i].State = Features::Exploits::eResourceState::Stopping;
                        std::thread(&Features::Exploits::cResourceList::Stop, &Features::Exploits::g_ResourceList, Features::Exploits::vResources[i].Pointer).detach();
                        Mem.Write<uint32_t>(Features::Exploits::vResources[i].Pointer + 0x118, Features::Exploits::vResources[i].State);
                        std::thread(NotifyManager::Send, Features::Exploits::vResources[i].Path + xorstr(" foi suspenso!"), 4000).detach();
                        std::thread(NotifyManager::Send, xorstr("Parar Recurso esta indisponivel no momento!"), 4000).detach();
                    }
                }
                ImGui::PopStyleVar();
            }
            ImGui::CustomEndChild();
        }
        ImGui::EndGroup();
        }
    }
}
