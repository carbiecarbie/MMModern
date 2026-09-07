#include "games/xeen/XeenWorld.h"

#include <stdexcept>
#include <utility>

namespace mmodern {

XeenWorld::XeenWorld(MapLoader loader) : _loader(std::move(loader)) {
	if (!_loader)
		throw std::invalid_argument("XeenWorld requer um carregador de mapas");
}

const XeenMap &XeenWorld::map(XeenMapIdentity mapId) {
	if (!mapId)
		throw std::invalid_argument("ID zero nao representa um mapa de Xeen");
	const auto cached = _maps.find(mapId);
	if (cached != _maps.end())
		return cached->second;

	XeenMap loaded = _loader(mapId);
	if (loaded.identity() != mapId)
		throw std::runtime_error("ID interno do mapa nao corresponde ao recurso solicitado");
	return _maps.emplace(mapId, std::move(loaded)).first->second;
}

std::optional<XeenCellSample> XeenWorld::sampleCell(
		XeenMapIdentity mapId, int x, int y) {
	// The original outdoor view only resolves the 3x3 map neighborhood.
	if (x < -16 || x >= 32 || y < -16 || y >= 32)
		return std::nullopt;

	const XeenMap *current = &map(mapId);
	if (!current->geometry.isOutdoors()) {
		// Interior exits are event-driven. Declared neighbors deliberately do not
		// extend the coordinate plane until that behavior has its own milestone.
		if (x < 0 || x >= 16 || y < 0 || y >= 16)
			return std::nullopt;
		const auto index = static_cast<std::size_t>(y) * XeenMapGeometry::kWidth +
			static_cast<std::size_t>(x);
		return XeenCellSample{current->identity(), x, y,
			&current->geometry, &current->geometry.cells[index]};
	}
	if (y < 0) {
		const std::uint16_t neighbor = current->geometry.neighbors[2]; // South.
		if (!neighbor)
			return std::nullopt;
		y += 16;
		current = &map({mapId.side, neighbor});
	} else if (y >= 16) {
		const std::uint16_t neighbor = current->geometry.neighbors[0]; // North.
		if (!neighbor)
			return std::nullopt;
		y -= 16;
		current = &map({mapId.side, neighbor});
	}

	// Match Map::getCell(): resolve Y before resolving X, including diagonals.
	if (x < 0) {
		const std::uint16_t neighbor = current->geometry.neighbors[3]; // West.
		if (!neighbor)
			return std::nullopt;
		x += 16;
		current = &map({mapId.side, neighbor});
	} else if (x >= 16) {
		const std::uint16_t neighbor = current->geometry.neighbors[1]; // East.
		if (!neighbor)
			return std::nullopt;
		x -= 16;
		current = &map({mapId.side, neighbor});
	}

	if (x < 0 || x >= 16 || y < 0 || y >= 16)
		return std::nullopt;
	const auto index = static_cast<std::size_t>(y) * XeenMapGeometry::kWidth +
		static_cast<std::size_t>(x);
	return XeenCellSample{current->identity(), x, y,
		&current->geometry, &current->geometry.cells[index]};
}

} // namespace mmodern
