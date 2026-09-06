#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_SCRIPT_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_SCRIPT_H

#include "games/xeen/XeenEventFile.h"
#include "games/xeen/XeenNavigation.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mmodern {

constexpr std::uint8_t kXeenEventDirectionAll = 4;

struct XeenEventDuplicateKey {
	std::uint8_t x = 0;
	std::uint8_t y = 0;
	std::uint8_t direction = 0;
	std::uint8_t line = 0;
	std::size_t firstOffset = 0;
	std::size_t duplicateOffset = 0;
};

class XeenEventScript {
public:
	explicit XeenEventScript(XeenEventFile eventFile);

	const XeenEventFile &file() const { return _eventFile; }
	const std::vector<XeenEventRecord> &records() const { return _eventFile.records; }

	// The returned pointer belongs to this script and becomes invalid when the
	// script is destroyed, assigned, or moved.
	const XeenEventRecord *findInstruction(std::uint8_t x, std::uint8_t y,
		XeenDirection direction, std::uint8_t line) const;
	std::vector<XeenEventDuplicateKey> duplicateKeys() const;

private:
	XeenEventFile _eventFile;
};

} // namespace mmodern

#endif
