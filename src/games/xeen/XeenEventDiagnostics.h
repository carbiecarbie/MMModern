#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_DIAGNOSTICS_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_DIAGNOSTICS_H

#include "games/xeen/XeenEventFile.h"
#include "games/xeen/XeenNavigation.h"

#include <cstdint>
#include <optional>
#include <string>

namespace mmodern {

class XeenEventScript;

enum class XeenEventDirectionFilter {
	Any,
	Physical,
	AllOnly
};

struct XeenEventDiagnosticFilter {
	std::uint8_t x = 0;
	std::uint8_t y = 0;
	XeenEventDirectionFilter directionFilter = XeenEventDirectionFilter::Any;
	XeenDirection direction = XeenDirection::North;
	std::optional<bool> automatic;
};

class XeenEventDiagnostics {
public:
	static std::string opcodeName(std::uint8_t opcode);
	static std::string directionName(std::uint8_t direction);
	static std::string format(const XeenEventFile &eventFile);
	static std::string format(const XeenEventScript &script,
		const std::optional<XeenEventDiagnosticFilter> &filter = std::nullopt);
};

} // namespace mmodern

#endif
