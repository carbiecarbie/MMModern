#ifndef MMODERN_GAMES_XEEN_XEEN_PARTY_LOADER_H
#define MMODERN_GAMES_XEEN_XEEN_PARTY_LOADER_H

#include "games/xeen/XeenParty.h"

#include <cstdint>
#include <vector>

namespace mmodern {

class XeenAssetSource;

// Calendar year used by the original Clouds of Xeen new-game state.
inline constexpr std::uint32_t kCloudsInitialYear = 610;

class XeenPartyLoader {
public:
	XeenPartyState loadInitialCloudsParty(XeenAssetSource &assets) const;
	XeenPartyState loadFromResources(const std::vector<std::uint8_t> &rosterBytes,
		const std::vector<std::uint8_t> &partyBytes) const;
};

} // namespace mmodern

#endif
