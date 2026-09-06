#ifndef MMODERN_FORMATS_XEEN_MAP_FORMAT_H
#define MMODERN_FORMATS_XEEN_MAP_FORMAT_H

#include "games/xeen/XeenMap.h"

namespace mmodern {

// Parses decoded bytes only; no archive, engine, or platform access.
class XeenMapFormat {
public:
	static XeenMapGeometry parseDat(const std::vector<std::uint8_t> &bytes);
	static XeenMapEntities parseMob(const std::vector<std::uint8_t> &bytes);
	// Temporary compatibility adapter. XeenEventFormat owns EVT parsing.
	static std::vector<XeenEventInstruction> parseEvt(const std::vector<std::uint8_t> &bytes);
};

} // namespace mmodern

#endif
