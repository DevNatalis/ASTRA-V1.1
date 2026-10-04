#pragma once
#include "../SDK.hpp"

namespace SDK2 {
	class CNetwork {
	public:
		void RequestControlOfVehicle2(CPed* pLocalPed, CVehicle* pVehicle, bool bCanMigrate, bool bExistsOnAllMachines);
	};
	inline CNetwork* g_Network;
}