#pragma once

#include <cstdint>

namespace SDK {
	class CNetObject {
	public:
		uint8_t GetOwnerId() const;
		void SetOwnerId(uint8_t ownerId);
		void SetInvisible(bool state);

	private:
	};
}
#pragma once
