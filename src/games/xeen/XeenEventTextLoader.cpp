#include "games/xeen/XeenEventTextLoader.h"

#include "formats/xeen/XeenEventTextFormat.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mmodern {

const std::string *XeenEventTextFile::stringAt(std::size_t index) const {
	if (index >= strings.size())
		return nullptr;
	return &strings[index];
}

XeenEventTextLoader::XeenEventTextLoader(ResourceReader resourceReader) :
	_resourceReader(std::move(resourceReader)) {
	if (!_resourceReader)
		throw std::invalid_argument("leitor de recursos de texto ausente");
}

std::string XeenEventTextLoader::resourceNameForMap(XeenMapIdentity mapId) {
	requireCloudsMap(mapId);
	std::ostringstream name;
	name << "aaze" << (mapId.number >= 100 ? 'x' : '0')
		<< std::setfill('0') << std::setw(3) << mapId.number << ".txt";
	return name.str();
}

XeenEventTextFile XeenEventTextLoader::load(XeenMapIdentity mapId) const {
	XeenEventTextFile result;
	result.mapId = mapId;
	result.resourceName = resourceNameForMap(mapId);
	const std::optional<std::vector<std::uint8_t>> bytes = _resourceReader(result.resourceName);
	if (!bytes)
		return result;
	result.resourcePresent = true;
	result.strings = XeenEventTextFormat::parse(*bytes);
	return result;
}

} // namespace mmodern
