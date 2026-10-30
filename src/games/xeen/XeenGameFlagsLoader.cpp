#include "games/xeen/XeenGameFlagsLoader.h"

#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenGameFlagsFormat.h"
#include "formats/xeen/XeenCharacterFormat.h"

namespace mmodern {

XeenGameFlags XeenGameFlagsLoader::loadInitialFlags(XeenAssetSource &assets) const {
	auto flags = loadFromPartyResource(assets.readInitialResource("maze.pty"));
	const auto dark = assets.readInitialResource("maze.pty", XeenSide::Darkside);
	if (dark.size() != XeenCharacterFormat::kWorldPartySize)
		throw std::runtime_error("Invalid DARK New Game flag layout");
	for (unsigned n = 0; n < XeenGameFlags::kCount; ++n)
		if (dark[XeenGameFlagsFormat::kDarksideFlagsOffset + n / 8] & (0x80u >> (n % 8)))
			flags.set(XeenSide::Darkside, n);
	return flags;
}
XeenGameFlags XeenGameFlagsLoader::loadFromPartyResource(const std::vector<std::uint8_t> &bytes) const {
	return XeenGameFlags(XeenGameFlagsFormat::parseClouds(bytes));
}
} // namespace mmodern
