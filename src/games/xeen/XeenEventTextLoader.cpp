#include "games/xeen/XeenEventTextLoader.h"

#include "formats/xeen/XeenEventTextFormat.h"

#include <iomanip>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mmodern {

const std::string *XeenEventTextFile::stringAt(std::size_t index) const {
	if (index >= strings.size())
		return nullptr;
	return &strings[index];
}

XeenEventTextLoader::XeenEventTextLoader(ResourceReader resourceReader,ResourceReader mirrorReader) :
	_resourceReader(std::move(resourceReader)),_mirrorReader(std::move(mirrorReader)) {
	if (!_resourceReader)
		throw std::invalid_argument("Event text resource reader is absent");
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
    if (_mirrorReader) if (const auto mirrors=_mirrorReader(mapId.side==XeenSide::Clouds?"xeenmirr.txt":"darkmirr.txt"))
        result.mirrors=parseMirrors(*mirrors);
	return result;
}

std::vector<XeenMirrorDestination> XeenEventTextLoader::parseMirrors(const std::vector<std::uint8_t> &bytes) {
    if(bytes.size()%32)throw std::invalid_argument("Malformed mirror destination table");
    std::vector<XeenMirrorDestination> result;
    for(std::size_t at=0;at<bytes.size();at+=32) {
        const auto end=std::find(bytes.begin()+at,bytes.begin()+at+28,0);
        if(end==bytes.begin()+at+28)throw std::invalid_argument("Unterminated mirror destination name");
        result.push_back({std::string(bytes.begin()+at,end),bytes[at+28],
            static_cast<std::int8_t>(bytes[at+29]),static_cast<std::int8_t>(bytes[at+30]),static_cast<std::int8_t>(bytes[at+31])});
    }
    return result;
}

} // namespace mmodern
