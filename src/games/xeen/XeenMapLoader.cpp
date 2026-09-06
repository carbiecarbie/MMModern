#include "games/xeen/XeenMapLoader.h"

#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenMapFormat.h"

#include <cstdio>
#include <stdexcept>

namespace mmodern {

XeenMap XeenMapLoader::loadGeometryMap(XeenAssetSource &assets,
		std::uint16_t mapId) const {
	if (!mapId || mapId > 9999)
		throw std::invalid_argument("ID de mapa fora do formato Clouds de quatro digitos");
	char resourceName[13];
	std::snprintf(resourceName, sizeof(resourceName), "maze%04u.dat",
		static_cast<unsigned>(mapId));
	XeenMap map;
	map.geometry = XeenMapFormat::parseDat(assets.readInitialResource(resourceName));
	if (map.geometry.id != mapId)
		throw std::runtime_error(std::string(resourceName) + " possui ID interno inesperado");
	return map;
}

XeenMap XeenMapLoader::loadOutdoorMap(XeenAssetSource &assets,
		std::uint16_t mapId) const {
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
