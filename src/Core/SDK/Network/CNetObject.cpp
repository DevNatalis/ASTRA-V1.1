#include "CNetObject.h"
#include <Core/SDK/Memory.hpp>

uint8_t SDK::CNetObject::GetOwnerId() const {
	return Core::Mem.Read<uint8_t>((uintptr_t)this + 0x49);
}

void SDK::CNetObject::SetOwnerId(uint8_t ownerId) {
	Core::Mem.Write<uint8_t>((uintptr_t)this + 0x49, ownerId);
}

void SDK::CNetObject::SetInvisible(bool state) {
	Core::Mem.Write<uint8_t>((uintptr_t)this + 0xD1, state ? 1 : 0);
}