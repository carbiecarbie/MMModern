#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_FILE_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_FILE_H

#include "formats/xeen/XeenEventFormat.h"

#include <cstdint>
#include <string>
#include <vector>

namespace mmodern {

struct XeenEventFile {
	std::uint16_t mapId = 0;
	std::string resourceName;
	bool resourcePresent = false;
	std::vector<XeenEventRecord> records;
};

} // namespace mmodern

#endif
