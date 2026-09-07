#include "games/xeen/XeenEventLoader.h"

#include "formats/xeen/XeenEventFormat.h"

#include <exception>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mmodern {

XeenEventLoader::XeenEventLoader(ResourceReader resourceReader) :
	_resourceReader(std::move(resourceReader)) {
	if (!_resourceReader)
		throw std::invalid_argument("leitor de recursos EVT ausente");
}

std::string XeenEventLoader::resourceNameForMap(XeenMapIdentity mapId) {
	requireCloudsMap(mapId);
	std::ostringstream name;
	name << "maze" << std::setfill('0') << std::setw(4) << mapId.number << ".evt";
	return name.str();
}

XeenEventFile XeenEventLoader::load(XeenMapIdentity mapId) const {
	XeenEventFile result;
	result.mapId = mapId;
	result.resourceName = resourceNameForMap(mapId);

	const std::optional<std::vector<std::uint8_t>> bytes =
		_resourceReader(result.resourceName);
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
