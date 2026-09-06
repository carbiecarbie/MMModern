#include "games/xeen/XeenGameFlagsLoader.h"

#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenGameFlagsFormat.h"

namespace mmodern {

XeenGameFlags XeenGameFlagsLoader::loadInitialCloudsFlags(
		XeenAssetSource &assets) const {
	return loadFromPartyResource(assets.readInitialResource("maze.pty"));
}

XeenGameFlags XeenGameFlagsLoader::loadFromPartyResource(
		const std::vector<std::uint8_t> &partyBytes) const {
	return XeenGameFlags(XeenGameFlagsFormat::parseClouds(partyBytes));
}

} // namespace mmodern
