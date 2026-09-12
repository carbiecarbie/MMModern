#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/XeenGameFlags.h"

#include <atomic>
#include <stdexcept>
#include <limits>
#include <utility>

namespace mmodern {
XeenWorld::GameplayBorrow::GameplayBorrow(XeenWorld &w, XeenPartyState &p,
		XeenCamera &c, XeenGameFlags &f) : owners{w._gameplayBorrow.retain(),
		p._gameplayBorrow.retain(), p.roster._gameplayBorrow.retain(),
		c.gameplayBorrow.retain(), f._gameplayBorrow.retain()} {
	for (const auto &state : owners) { ++state->references; ++state->revision; }
}
namespace {
std::uint64_t nextWorldIncarnation() {
	static std::atomic<std::uint64_t> next{1};
	auto value = next.load(std::memory_order_relaxed);
	for (;;) {
		if (value == std::numeric_limits<std::uint64_t>::max())
			throw std::overflow_error("world incarnation exhausted");
		if (next.compare_exchange_weak(value, value + 1,
				std::memory_order_relaxed, std::memory_order_relaxed)) return value;
	}
}
using namespace xeen_state;
}

XeenWorld::XeenWorld(MapLoader loader, ObjectLoader objectLoader) :
	_incarnation(nextWorldIncarnation()), _loader(std::move(loader)), _objectLoader(std::move(objectLoader)) {
	if (!_loader)
		throw std::invalid_argument("XeenWorld requer um carregador de mapas");
}

bool XeenWorld::completedCaptureEligible(const XeenPartyState &party, const XeenCamera &camera) const noexcept {
	return !_sessionState._completedLease && !_sessionState._completedLeaseKind && completedFactsCurrent(party, camera);
}

bool XeenWorld::completedFactsCurrent(const XeenPartyState &party, const XeenCamera &camera) const noexcept {
	const auto &s = _sessionState;
	if (!s._completedPublished || s._completion != XeenEncounterCompletion::VictoryQuiescent || !s._completedAuthority ||
		!s._combatAccounted || !s._diagnostic27 || !s._combatEntered || !s._encounterMarked ||
		!s._encounterInitialized || !s._encounterTerminal || s._entry != XeenEncounterEntry::Diagnostic27 ||
		s._combatOwner || s._combatApproachState || _combatCheck || _combatAuthorized ||
		s._completedIntegrityUnsafe || s._completedFatal) return false;
	const auto &a = *s._completedAuthority;
	// A detached or copied caller owns no capability and cannot poison the bound graph.
	if (a.party != &party || a.roster != &party.roster || a.camera != &camera) return false;
	bool exact = party.roster.combatMarked() && party.party.activeRosterIds() == a.activeRosterIds &&
		party.questItems.counts() == a.questItems && party.questFlags.values() == a.questFlags &&
		party.encounterContext == a.context && party.firstSerializedCount == a.firstSerializedCount &&
		party.effectiveSerializedCount == a.effectiveSerializedCount && party.diagnostics == a.diagnostics &&
		sameCamera(camera, a.cameraValue) && s._actors.size() == a.actors.size() &&
		s._objects == a.objects && s._events == a.events && s._completedMonster == a.monster &&
		s._completedMonster == XeenMonsterIdentity{{XeenSide::Clouds, 20}, 5};
	for (std::size_t i = 0; exact && i < a.characters.size(); ++i) {
		exact = sameCharacter(party.roster.characters()[i], a.characters[i]) &&
			bool(party.roster.combatInputs(i)) == bool(a.combatInputs[i]);
		if (exact && a.combatInputs[i]) exact = sameInputs(*party.roster.combatInputs(i), *a.combatInputs[i]);
	}
	for (std::size_t i = 0; exact && i < a.actors.size(); ++i) exact = sameActor(s._actors[i], a.actors[i]);
	if (!exact) {
		s._completedIntegrityUnsafe = true;
		if (s._encounterRevision != std::numeric_limits<std::uint64_t>::max()) ++s._encounterRevision;
	}
	return exact;
}

bool XeenWorld::completedTicketCurrent(const XeenCompletedEncounterTicket &t,
		const XeenPartyState &p, const XeenCamera &c) const noexcept {
	return t.world == this && t.incarnation == _incarnation && t.revision == _sessionState._encounterRevision &&
		completedCaptureEligible(p, c);
}

bool XeenWorld::completedGuardCurrent(const XeenCompletedEncounterTicket &t, XeenCompletedGuard kind,
		std::uint64_t lease, const XeenPartyState &p, const XeenCamera &c) const noexcept {
	const auto &s = _sessionState;
	return (kind == XeenCompletedGuard::Operation || kind == XeenCompletedGuard::Presentation) &&
		t.world == this && t.incarnation == _incarnation && t.revision != std::numeric_limits<std::uint64_t>::max() &&
		t.revision + 1 == lease && s._completedLease == lease && s._completedLeaseKind == kind &&
		s._encounterRevision == lease && completedFactsCurrent(p, c);
}

XeenCompletedEncounterTicket XeenWorld::completedTicket(const XeenPartyState &party,
		const XeenCamera &camera) const noexcept {
	XeenCompletedEncounterTicket ticket;
	if (completedCaptureEligible(party, camera)) {
		ticket.world = this;
		ticket.incarnation = _incarnation;
		ticket.revision = _sessionState._encounterRevision;
	}
	return ticket;
}

std::uint64_t XeenWorld::holdCompletedGuard(const XeenCompletedEncounterTicket &ticket,
		XeenCompletedGuard guard, const XeenPartyState &party, const XeenCamera &camera) {
	auto &s = _sessionState;
	if ((guard != XeenCompletedGuard::Operation && guard != XeenCompletedGuard::Presentation) ||
		ticket.world != this || ticket.incarnation != _incarnation ||
		ticket.revision != s._encounterRevision || s._completedLease ||
		s._completedLeaseKind ||
		s._completion != XeenEncounterCompletion::VictoryQuiescent || !completedCaptureEligible(party, camera))
		throw std::logic_error("stale completed encounter ticket");
	if (s._encounterRevision == std::numeric_limits<std::uint64_t>::max())
		throw std::overflow_error("completed encounter revision exhausted");
	++s._encounterRevision;
	s._completedLease = s._encounterRevision;
	s._completedLeaseKind = guard;
	return s._encounterRevision;
}

bool XeenWorld::releaseCompletedGuard(const XeenCompletedEncounterTicket &ticket,
		XeenCompletedGuard guard, std::uint64_t lease) noexcept {
	auto &s = _sessionState;
	if ((guard != XeenCompletedGuard::Operation && guard != XeenCompletedGuard::Presentation) ||
		ticket.world != this || ticket.incarnation != _incarnation ||
		ticket.revision == std::numeric_limits<std::uint64_t>::max() ||
		ticket.revision + 1 != lease || s._completedLease != lease || s._completedLeaseKind != guard ||
		s._encounterRevision != lease || s._completedIntegrityUnsafe || s._completedFatal ||
		s._encounterRevision == std::numeric_limits<std::uint64_t>::max()) return false;
	++s._encounterRevision;
	s._completedLease = 0;
	s._completedLeaseKind.reset();
	return true;
}

bool XeenWorld::latchCompletedGuard(const XeenCompletedEncounterTicket &ticket,
		XeenCompletedGuard guard, const XeenPartyState &party, const XeenCamera &camera) noexcept {
	auto &s = _sessionState;
	if ((guard != XeenCompletedGuard::Integrity && guard != XeenCompletedGuard::Fatal) ||
		ticket.world != this || ticket.incarnation != _incarnation ||
		ticket.revision != s._encounterRevision || s._completedLease ||
		s._completedLeaseKind || s._encounterRevision == std::numeric_limits<std::uint64_t>::max() ||
		!completedCaptureEligible(party, camera)) return false;
	++s._encounterRevision;
	if (guard == XeenCompletedGuard::Integrity) s._completedIntegrityUnsafe = true;
	else s._completedFatal = true;
	return true;
}

bool XeenWorld::escalateCompletedGuard(const XeenCompletedEncounterTicket &ticket,
		XeenCompletedGuard heldGuard, std::uint64_t lease, XeenCompletedGuard escalation) noexcept {
	auto &s = _sessionState;
	if ((heldGuard != XeenCompletedGuard::Operation && heldGuard != XeenCompletedGuard::Presentation) ||
		(escalation != XeenCompletedGuard::Integrity && escalation != XeenCompletedGuard::Fatal) ||
		ticket.world != this || ticket.incarnation != _incarnation ||
		ticket.revision == std::numeric_limits<std::uint64_t>::max() ||
		ticket.revision + 1 != lease || s._completedLease != lease || s._completedLeaseKind != heldGuard ||
		s._encounterRevision != lease || s._completion != XeenEncounterCompletion::VictoryQuiescent ||
		s._completedIntegrityUnsafe || s._completedFatal ||
		s._encounterRevision == std::numeric_limits<std::uint64_t>::max()) return false;
	++s._encounterRevision;
	s._completedLease = 0;
	s._completedLeaseKind.reset();
	if (escalation == XeenCompletedGuard::Integrity) s._completedIntegrityUnsafe = true;
	else s._completedFatal = true;
	return true;
}

void XeenWorld::restoreSessionState(const std::vector<XeenObjectIdentity> &objects,
		const std::vector<XeenEventIdentity> &events, const EventLoader &eventLoader) {
	if (hasEncounterState()) throw std::logic_error("cannot restore an encounter world");
	XeenSessionWorldState prepared;
	for (const auto id : objects) {
		static_cast<void>(map(id.mapId));
		if (hasEncounterState()) throw std::logic_error("encounter appeared during overlay map loading");
		validateObject(id);
		if (hasEncounterState()) throw std::logic_error("encounter appeared during overlay resource loading");
		if (!prepared._objects.insert(id).second)
			throw std::invalid_argument("duplicate restored object identity");
	}
	std::map<XeenMapIdentity, XeenEventFile> files;
	for (const auto id : events) {
		static_cast<void>(map(id.mapId));
		if (hasEncounterState()) throw std::logic_error("encounter appeared during overlay map loading");
		if (!eventLoader) throw std::invalid_argument("restoration requires an event loader");
		auto found = files.find(id.mapId);
		if (found == files.end()) found = files.emplace(id.mapId, eventLoader(id.mapId)).first;
		if (hasEncounterState()) throw std::logic_error("encounter appeared during overlay event loading");
		const auto &file = found->second;
		if (file.mapId != id.mapId || !file.resourcePresent || id.recordIndex >= file.records.size())
			throw std::invalid_argument("restored original event identity does not exist");
		if (!prepared._events.insert(id).second)
			throw std::invalid_argument("duplicate restored event identity");
	}
	if (hasEncounterState()) throw std::logic_error("encounter appeared before overlay publication");
	_sessionState._objects.swap(prepared._objects);
	_sessionState._events.swap(prepared._events);
	++_ownerRevision;
}

void XeenWorld::swapPreparedState(XeenWorld &candidate) noexcept {
	// Private to guarded save publication. Never swaps/clears encounter authority.
	_sessionState._objects.swap(candidate._sessionState._objects);
	_sessionState._events.swap(candidate._sessionState._events);
	_maps.swap(candidate._maps);
	_objects.swap(candidate._objects);
	++_ownerRevision; ++candidate._ownerRevision;
}

const XeenObjectFile &XeenWorld::objectFile(XeenMapIdentity mapId) {
	if (!mapId) throw std::invalid_argument("invalid object map identity");
	const auto found = _objects.find(mapId);
	if (found != _objects.end()) return found->second;
	// Geometry-only clients may omit the provider; this represents no MOB source.
	XeenObjectFile loaded = _objectLoader ? _objectLoader(mapId) :
		XeenObjectFile{mapId, {}, false, {}};
	if (_combatCheck) _combatCheck();
	if (loaded.mapId != mapId)
		throw std::runtime_error("object file identity differs from requested map");
	const auto result = _objects.emplace(mapId, std::move(loaded));
	++_cacheRevision;
	return result.first->second;
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
	if (_combatCheck) _combatCheck();
	if (loaded.identity() != mapId)
		throw std::runtime_error("ID interno do mapa nao corresponde ao recurso solicitado");
	const auto result = _maps.emplace(mapId, std::move(loaded));
	++_cacheRevision;
	return result.first->second;
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
