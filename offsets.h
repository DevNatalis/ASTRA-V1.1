#pragma once
#include <cstdint>

// FiveM offsets (RAGE) — gerado Oct 05 2026 00:32:15
// FiveM_b3258_GTAProcess.exe base=0x7FF71D7B0000 size=0x647F000 pid=15592

namespace Offsets {
    constexpr uintptr_t ModuleBase = 0x7FF71D7B0000;

    // Globals (RVA = target - base)
    constexpr uintptr_t World = 0x25B14B0;                   // OK RVA=0x25B14B0 target=0x7FF71D8B1464 final=0x2013BBCC940
    constexpr uintptr_t ReplayInterface = 0x1FBD4F0;         // OK RVA=0x1FBD4F0 target=0x7FF71F76D4F0 final=0x2012F464040  via RVA conhecido 0x1FBD4F0 (*target=0x2012F464040)  [32 hit(s) sig sem validacao (hit0 target=0x7FF71F9F4218 *t=0x2017B205E20)]
    constexpr uintptr_t ViewPort = 0x201DBA0;                // OK RVA=0x201DBA0 target=0x7FF71F7CDBA0 final=0x2014970A800  via RVA conhecido 0x201DBA0 (*target=0x2014970A800)
    constexpr uintptr_t Camera = 0x201E7D0;                  // OK RVA=0x201E7D0 target=0x7FF71DA4E0E0 final=0x20146CC8930
    constexpr uintptr_t BlipList = 0x2023400;                // OK RVA=0x2023400 target=0x7FF71DA79408 final=0x7FF71F7D3400
    constexpr uintptr_t CamGameplayDirector = 0x201ED50;     // OK RVA=0x201ED50 target=0x7FF71F7CED50 final=0x2014BA35E60  via RVA conhecido 0x201ED50 (*target=0x2014BA35E60) [RVA-only]
    constexpr uintptr_t PedPool = 0x25B1758;                 // OK RVA=0x25B1758 target=0x7FF71FD61758 final=0x2013BBDB790  via RVA conhecido 0x25B1758 (*target=0x2013BBDB790) [RVA-only]
    constexpr uintptr_t VehiclePool = 0x2F23F78;             // OK RVA=0x2F23F78 target=0x7FF7206D3F78 final=0x20142BAC5E0  via RVA conhecido 0x2F23F78 (*target=0x20142BAC5E0) [RVA-only]
    constexpr uintptr_t NetworkPlayerMgr = 0x2244218;        // OK RVA=0x2244218 target=0x7FF71F9F4218 final=0x2017B205E20  via RVA conhecido 0x2244218 (*target=0x2017B205E20) [RVA-only]
    constexpr uintptr_t NetworkObjectMgr = 0x2982050;        // OK RVA=0x2982050 target=0x7FF720132050 final=0x2013F0CC010  via RVA conhecido 0x2982050 (*target=0x2013F0CC010) [RVA-only]
    constexpr uintptr_t AimCPed = 0x1F73508;                 // OK RVA=0x1F73508 target=0x7FF71F723508 final=0x2013B40E9E0  via RVA conhecido 0x1F73508 (*target=0x2013B40E9E0) [RVA-only]
    constexpr uintptr_t PlayerNames = 0x1E63C68;             // OK RVA=0x1E63C68 target=0x7FF71F613C68 final=0x7FF71F613C68  via RVA conhecido 0x1E63C68 (LEA .data) [RVA-only]
    constexpr uintptr_t SkySettings = 0x2721250;             // OK RVA=0x2721250 target=0x7FF71FED1250 final=0x20143E8ED70  via RVA conhecido 0x2721250 (*target=0x20143E8ED70) [RVA-only]
    constexpr uintptr_t StreamableTextureDict = 0x26671F0;   // OK RVA=0x26671F0 target=0x7FF71FE171F0 final=0x20143E8DED0  via RVA conhecido 0x26671F0 (*target=0x20143E8DED0) [RVA-only]
    constexpr uintptr_t RaycastXWord = 0x2CFA4E0;            // OK RVA=0x2CFA4E0 target=0x7FF7204AA4E0 final=0x0  via RVA conhecido 0x2CFA4E0 (*target=0 — entre no servidor/spawne e rode de novo) [RVA-only]
    constexpr uintptr_t Swapchain = 0x2D2CAA0;               // OK RVA=0x2D2CAA0 target=0x7FF7204DCAA0 final=0x200D71D4370  via RVA conhecido 0x2D2CAA0 (*target=0x200D71D4370) [RVA-only]
    constexpr uintptr_t PlayerAimingAt = 0x200FA10;          // OK RVA=0x200FA10 target=0x7FF71F7BFA10 final=0x0  via RVA conhecido 0x200FA10 (*target=0 — entre no servidor/spawne e rode de novo) [RVA-only]
    constexpr uintptr_t GameBuildString = 0x22295B0;         // OK RVA=0x22295B0 target=0x7FF71F9D95B0 final=0x7FF71F9D95B0  via RVA conhecido 0x22295B0 (LEA .data) [RVA-only]
    constexpr uintptr_t IsSessionStarted = 0x1F74BD8;        // OK RVA=0x1F74BD8 target=0x7FF71F724BD8 final=0x7FF71F724BD8  via RVA conhecido 0x1F74BD8 (LEA .data) [RVA-only]
    constexpr uintptr_t TriggerState = 0x202C8D0;            // OK RVA=0x202C8D0 target=0x7FF71F7DC8D0 final=0x7FF71F7DC8D0  via RVA conhecido 0x202C8D0 (LEA .data) [RVA-only]
    constexpr uintptr_t PointerToHandle = 0x1639CE8;         // OK RVA=0x1639CE8 target=0x7FF71EDE9CE8 final=0x7FF71EDE9CE8  via RVA conhecido 0x1639CE8 (LEA .data) [RVA-only]
    constexpr uintptr_t IsPlayerAiming = 0x2D3839C;          // OK RVA=0x2D3839C target=0x7FF7204E839C final=0x7FF7204E839C  via RVA conhecido 0x2D3839C (LEA .data) [RVA-only]
    constexpr uintptr_t HandleBullet = 0x101A5F4;            // OK RVA=0x101A5F4 target=0x7FF71E7CA5F4 final=0x7FF71E7CA5F4  via RVA conhecido 0x101A5F4 (LEA .data) [RVA-only]

    namespace CPed {
        constexpr uintptr_t Navigation = 0x30;            // u64* - CNavigation (pos real)  [0x201480DE430]
        constexpr uintptr_t VisualCoordinate = 0x90;      // float - Posicao render (X)  [130.2]
        constexpr uintptr_t GodMode = 0x189;               // u8 - Godmode (heuristico)  [1]
        constexpr uintptr_t Health = 0x280;                // float - Vida atual  [400.0]
        constexpr uintptr_t MaxHealth = 0x284;             // float - Vida maxima  [400.0]
        constexpr uintptr_t BoneList = 0x410;              // u64* - Lista de ossos (candidato b3258)  [not validated]
        constexpr uintptr_t BoneManager = 0x430;           // u64* - Matriz de ossos  [not validated]
        constexpr uintptr_t PlayerNetID = 0xE8;           // u16 - NetID FiveM  [31060]
        constexpr uintptr_t Stamina = 0xCF4;               // float - Estamina  [0.0]
        constexpr uintptr_t SpeedModifier = 0xD50;         // float - Mult. velocidade  [0.0]
        constexpr uintptr_t DamageHandler = 0xD70;         // float - Resistencia a dano  [0.0]
        constexpr uintptr_t PlayerInfo = 0x10A8;            // u64* - CPlayerInfo  [0x20146A736A0]
        constexpr uintptr_t WeaponManager = 0x10B8;         // u64* - CWeaponManager  [0x2013B58DC80]
        constexpr uintptr_t VisibleFlag = 0x145C;           // u8 - LOS  [8]
        constexpr uintptr_t Armor = 0x150C;                 // float - Colete  [0.0]
    }

    namespace CNavigation {
        constexpr uintptr_t Position = 0x50;              // float - Pos X (via ped+0x30)  [130.2]
    }

    namespace CPlayerInfo {
        constexpr uintptr_t NetID = 0x7C;                 // u32 - Network ID  [0x0]
        constexpr uintptr_t FrameFlags = 0x270;            // u32 - Bit 28 = super jump  [0x0]
    }

    namespace CVehicle {
        constexpr uintptr_t Handling = 0x960;              // u64* - CHandlingData  [not validated]
        constexpr uintptr_t SteeringAngle = 0x994;         // float - Angulo das rodas  [not validated]
    }

    namespace CWeaponManager {
        constexpr uintptr_t WeaponInfo = 0x20;            // u64* - CWeaponInfo  [0x2014C2DA880]
        constexpr uintptr_t WeaponObject = 0x78;          // u64* - Entidade arma  [not validated]
    }

    namespace CWeaponInfo {
        constexpr uintptr_t Spread = 0x74;                // float - Espalhamento  [0.0]
        constexpr uintptr_t Range = 0x28C;                 // float - Alcance  [30.0]
        constexpr uintptr_t Recoil = 0x2F4;                // float - Recuo  [1.0]
    }

}
