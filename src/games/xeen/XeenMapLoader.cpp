#include "games/xeen/XeenMapLoader.h"

#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenMapFormat.h"

#include <cstdio>
#include <stdexcept>

namespace mmodern {

XeenObjectFile XeenMapLoader::loadObjects(XeenAssetSource &assets,
		XeenMapIdentity mapId) const {
	return loadObjects([&assets](const std::string &name)
			-> std::optional<std::vector<std::uint8_t>> {
		if (!assets.hasInitialResource(name)) return std::nullopt;
		return assets.readInitialResource(name);
	}, mapId);
}

XeenObjectFile XeenMapLoader::loadObjects(const ResourceReader &reader,
		XeenMapIdentity mapId) const {
	requireCloudsMap(mapId);
	if (!mapId || mapId.number > 9999 || !reader)
		throw std::invalid_argument("invalid Clouds object loader context");
	char name[13];
	std::snprintf(name, sizeof(name), "maze%04u.mob", static_cast<unsigned>(mapId.number));
	XeenObjectFile file{mapId, name, false, {}};
	const auto bytes = reader(file.resourceName);
	if (!bytes) return file;
	file.resourcePresent = true;
	try {
		file.entities = XeenMapFormat::parseMob(*bytes);
	} catch (const std::exception &error) {
		throw std::runtime_error(file.resourceName + ": " + error.what());
	}
	return file;
}

XeenMap XeenMapLoader::loadGeometryMap(XeenAssetSource &assets,
		XeenMapIdentity mapId) const {
	requireCloudsMap(mapId);
	if (!mapId || mapId.number > 9999)
		throw std::invalid_argument("ID de mapa fora do formato Clouds de quatro digitos");
	char resourceName[13];
	std::snprintf(resourceName, sizeof(resourceName), "maze%04u.dat",
		static_cast<unsigned>(mapId.number));
	XeenMap map;
	map.geometry = XeenMapFormat::parseDat(assets.readInitialResource(resourceName));
	map.side = mapId.side;
	if (map.identity() != mapId)
		throw std::runtime_error(std::string(resourceName) + " possui ID interno inesperado");
	return map;
}

XeenMap XeenMapLoader::loadOutdoorMap(XeenAssetSource &assets,
		XeenMapIdentity mapId) const {
	XeenMap map = loadGeometryMap(assets, mapId);
	if (!map.geometry.isOutdoors())
		throw std::runtime_error("mapa solicitado nao e exterior");
	return map;
}

XeenMap XeenMapLoader::loadAreaA1(XeenAssetSource &assets) const {
	XeenMap map = loadOutdoorMap(assets, 1);
	map.entities = XeenMapFormat::parseMob(assets.readInitialResource("maze0001.mob"));
	map.instructions = XeenMapFormat::parseEvt(assets.readInitialResource("maze0001.evt"));
	return map;
}

} // namespace mmodern
