#include "games/xeen/XeenWorld.h"

#include <stdexcept>
#include <utility>

namespace mmodern {

XeenWorld::XeenWorld(MapLoader loader, ObjectLoader objectLoader) :
	_loader(std::move(loader)), _objectLoader(std::move(objectLoader)) {
	if (!_loader)
		throw std::invalid_argument("XeenWorld requer um carregador de mapas");
}

const XeenObjectFile &XeenWorld::objectFile(XeenMapIdentity mapId) {
	if (!mapId) throw std::invalid_argument("invalid object map identity");
	const auto found = _objects.find(mapId);
	if (found != _objects.end()) return found->second;
	// Geometry-only clients may omit the provider; this represents no MOB source.
	XeenObjectFile loaded = _objectLoader ? _objectLoader(mapId) :
		XeenObjectFile{mapId, {}, false, {}};
	if (loaded.mapId != mapId)
		throw std::runtime_error("object file identity differs from requested map");
	return _objects.emplace(mapId, std::move(loaded)).first->second;
}

void XeenWorld::validateObject(XeenObjectIdentity id) {
	const auto &file = objectFile(id.mapId);
	if (!file.resourcePresent || id.recordIndex >= file.entities.objects.size())
		throw std::invalid_argument("selected original object index does not exist");
}

bool XeenWorld::isObjectDisabled(XeenObjectIdentity id) {
	validateObject(id);
	return objectFile(id.mapId).entities.objects[id.recordIndex].isDisabled() ||
		_sessionState.isObjectDisabled(id);
}

std::optional<XeenObjectIdentity> XeenWorld::selectObject(const XeenCamera &camera) {
	if (!camera.mapId || camera.x < 0 || camera.x > 15 || camera.y < 0 || camera.y > 15)
		throw std::invalid_argument("invalid physical object-selection cell");
	const auto &file = objectFile(camera.mapId);
	if (!file.resourcePresent) return std::nullopt;
	for (std::size_t i = 0; i < file.entities.objects.size(); ++i) {
		const auto &object = file.entities.objects[i];
		XeenObjectIdentity id{camera.mapId, i};
		if (object.x == camera.x && object.y == camera.y && object.isActive() &&
				!_sessionState.isObjectDisabled(id)) return id;
	}
	return std::nullopt;
}

XeenEventRecord XeenWorld::effectiveEvent(XeenEventIdentity id, const XeenEventRecord &base) const {
	XeenEventRecord result = base;
	if (isEventDisabled(id)) result.opcode = 0;
	return result;
}

void XeenWorld::disableObject(XeenObjectIdentity id) {
	validateObject(id);
	_sessionState._objects.insert(id);
}

void XeenWorld::validateEventCell(const XeenCamera &physical, const XeenEventFile &events) {
	if (!physical.mapId || physical.x < 0 || physical.x > 15 || physical.y < 0 || physical.y > 15 ||
			events.mapId != physical.mapId)
		throw std::invalid_argument("event mutation requires the physical map/cell");
	static_cast<void>(map(physical.mapId));
}

void XeenWorld::disableEventsAtCell(const XeenCamera &physical, const XeenEventFile &events) {
	validateEventCell(physical, events);
	for (std::size_t i = 0; i < events.records.size(); ++i) {
		const auto &record = events.records[i];
		if (record.x == physical.x && record.y == physical.y)
			_sessionState._events.insert({physical.mapId, i});
	}
}

void XeenWorld::applyRemove(const XeenCamera &physical,
		std::optional<XeenObjectIdentity> selected, const XeenEventFile &events) {
	// Resolve every predictable failure before changing this operation's state.
	validateEventCell(physical, events);
	if (selected) {
		if (selected->mapId != physical.mapId)
			throw std::invalid_argument("selected object belongs to another physical map/side");
		validateObject(*selected);
	}
	if (selected) disableObject(*selected);
	disableEventsAtCell(physical, events);
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
