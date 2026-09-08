#ifndef MMODERN_FORMATS_XEEN_QUEST_FLAG_FORMAT_H
#define MMODERN_FORMATS_XEEN_QUEST_FLAG_FORMAT_H

#include "games/xeen/XeenParty.h"

namespace mmodern {
class XeenQuestFlagFormat {
public:
	// Complete packed 60-bit field; only bits 0..29 belong to Clouds.
	static constexpr std::size_t kOffset = 739;
	static constexpr std::size_t kRequiredSize = kOffset + 8;
	static XeenCloudsQuestFlags parseClouds(const std::vector<std::uint8_t> &bytes);
};
}
#endif
