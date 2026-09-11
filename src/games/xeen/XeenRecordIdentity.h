#ifndef MMODERN_GAMES_XEEN_RECORD_IDENTITY_H
#define MMODERN_GAMES_XEEN_RECORD_IDENTITY_H

#include "games/xeen/XeenMapIdentity.h"
#include <cstddef>

namespace mmodern {

struct XeenMonsterIdentity {
	XeenMapIdentity mapId;
	std::size_t recordIndex = 0;
	friend bool operator==(XeenMonsterIdentity a, XeenMonsterIdentity b) {
		return a.mapId == b.mapId && a.recordIndex == b.recordIndex;
	}
	friend bool operator<(XeenMonsterIdentity a, XeenMonsterIdentity b) {
		return std::tie(a.mapId, a.recordIndex) < std::tie(b.mapId, b.recordIndex);
	}
};

// Original zero-based record order, never an active/visible index or resource ID.
struct XeenObjectIdentity {
	XeenMapIdentity mapId;
	std::size_t recordIndex = 0;
	friend bool operator==(XeenObjectIdentity a, XeenObjectIdentity b) {
		return a.mapId == b.mapId && a.recordIndex == b.recordIndex;
	}
	friend bool operator<(XeenObjectIdentity a, XeenObjectIdentity b) {
		return std::tie(a.mapId, a.recordIndex) < std::tie(b.mapId, b.recordIndex);
	}
};

struct XeenEventIdentity {
	XeenMapIdentity mapId;
	std::size_t recordIndex = 0;
	friend bool operator==(XeenEventIdentity a, XeenEventIdentity b) {
		return a.mapId == b.mapId && a.recordIndex == b.recordIndex;
	}
	friend bool operator<(XeenEventIdentity a, XeenEventIdentity b) {
		return std::tie(a.mapId, a.recordIndex) < std::tie(b.mapId, b.recordIndex);
	}
};

} // namespace mmodern
#endif
