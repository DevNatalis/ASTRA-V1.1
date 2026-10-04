#include "CNetwork.h"

void SDK2::CNetwork::RequestControlOfVehicle2(CPed* pLocalPed, CVehicle* pVehicle, bool bCanMigrate, bool bExistsOnAllMachines) {
    auto word_004E = Mem.Read<uint16_t>((uintptr_t)pVehicle->pNetObject() + 0x4E);
    if (bCanMigrate)
        Mem.Write<uint16_t>((uintptr_t)pVehicle->pNetObject() + 0x4E, word_004E &= ~1);
    else
        Mem.Write<uint16_t>((uintptr_t)pVehicle->pNetObject() + 0x4E, word_004E |= 1);

    word_004E = Mem.Read<uint16_t>((uintptr_t)pVehicle->pNetObject() + 0x4E);
    if (bExistsOnAllMachines)
        Mem.Write<uint16_t>((uintptr_t)pVehicle->pNetObject() + 0x4E, word_004E |= 8);
    else
        Mem.Write<uint16_t>((uintptr_t)pVehicle->pNetObject() + 0x4E, word_004E &= ~8);

    Mem.Write<uint8_t>((uintptr_t)pVehicle->pNetObject() + 0x49, (uintptr_t)pLocalPed->pNetObject()->mOwnerId());
    Mem.Write<uint8_t>((uintptr_t)pVehicle->pNetObject() + 0x4A, 0xFF);
    Mem.Write<uint8_t>((uintptr_t)pVehicle->pNetObject() + 0x4B, 0);
}