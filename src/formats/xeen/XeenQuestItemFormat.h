#ifndef MMODERN_FORMATS_XEEN_QUEST_ITEM_FORMAT_H
#define MMODERN_FORMATS_XEEN_QUEST_ITEM_FORMAT_H

#include "games/xeen/XeenParty.h"

namespace mmodern {

class XeenQuestItemFormat {
public:
	// Pinned Party::synchronize: flags begin at 659, followed by 32 + 32 +
	// 16 + 8 packed bytes. Read only the 35-byte Clouds quest-item prefix.
	static constexpr std::size_t kCloudsOffset = 747;
	static constexpr std::size_t kRequiredSize = kCloudsOffset + XeenCloudsQuestItems::kCount;
	static XeenCloudsQuestItems parseClouds(const std::vector<std::uint8_t> &bytes);
};

} // namespace mmodern
#endif
