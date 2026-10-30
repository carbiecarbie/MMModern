#include "games/xeen/XeenEventLoader.h"

#include "formats/xeen/XeenEventFormat.h"

#include <exception>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mmodern {
XeenEventLoader::XeenEventLoader(SideReader reader):_sideReader(std::move(reader)) {if(!_sideReader)throw std::invalid_argument("Missing side resource reader");}


XeenEventLoader::XeenEventLoader(ResourceReader resourceReader) :
	_resourceReader(std::move(resourceReader)) {
	if (!_resourceReader)
		throw std::invalid_argument("Missing EVT resource reader");
}

std::string XeenEventLoader::resourceNameForMap(XeenMapIdentity mapId) {
	requireXeenMap(mapId);
	std::ostringstream name;
	name << (mapId.number>=100?"mazex":"maze") << std::setfill('0') << std::setw(mapId.number>=100?3:4) << mapId.number << ".evt";
	return name.str();
}

XeenEventFile XeenEventLoader::load(XeenMapIdentity mapId) const {
	XeenEventFile result;
	result.mapId = mapId;
	result.resourceName = resourceNameForMap(mapId);

	const std::optional<std::vector<std::uint8_t>> bytes =
		(_sideReader?_sideReader(result.resourceName,mapId.side):_resourceReader(result.resourceName));
	if (!bytes)
		return result;

	result.resourcePresent = true;
	try {
		result.records = XeenEventFormat::parse(*bytes);
	} catch (const std::exception &error) {
		throw std::runtime_error(result.resourceName + ": " + error.what());
	}
	return result;
}

} // namespace mmodern
