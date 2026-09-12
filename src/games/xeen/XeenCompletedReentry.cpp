#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenCompletedDomain.h"
#include "games/xeen/XeenRestoreGuard.h"
#include <limits>
#include <type_traits>

namespace mmodern {
XeenCompletedReentry XeenWorld::reenterCompletedEncounter(const XeenCompletedEncounterTicket &ticket,
		XeenPartyState &party, XeenCamera &camera, const XeenGameFlags &flags,
		const MonsterLoader &loadMonsters, const EventLoader &loadEvents, const CompletedPreflight &preflight,
		std::optional<XeenCompletedEncounterTicket> *releasedOnFailure, const std::function<void()> &checkBoundary) {
	if (releasedOnFailure) releasedOnFailure->reset();
	const auto monsters = loadMonsters; const auto events = loadEvents; const auto presentation = preflight;
	const auto boundary = checkBoundary;
	if (!monsters || !events || !presentation) throw std::invalid_argument("completed re-entry requires resource/preflight services");
	if (_sessionState._encounterRevision > std::numeric_limits<std::uint64_t>::max() - 3 ||
		_sessionState._completedEntryGeneration == std::numeric_limits<std::uint64_t>::max())
		throw std::overflow_error("completed entry generation exhausted");
	const auto lease = holdCompletedGuard(ticket, XeenCompletedGuard::Operation, party, camera);
	std::optional<XeenRestoreGuard> retainedGuard;
	try {
		retainedGuard.emplace(*this, party, camera, flags, true);
		const auto &retained = *retainedGuard;
		XeenWorld candidate(_loader, _objectLoader);
		candidate._maps = _maps; candidate._maps.erase(20);
		candidate._objects = _objects; candidate._objects.erase(20);
		candidate._sessionState._objects = _sessionState._objects;
		candidate._sessionState._events = _sessionState._events;
		XeenCamera entry = XeenActorApproach::kEntry;
		std::optional<XeenRestoreGuard> prepared;
		const auto adoptPhase = [&] { prepared.emplace(candidate, party, entry, flags); };
		adoptPhase();
		const auto check = [&] {
			retained.check(); prepared->check();
			try { if (boundary) boundary(); }
			catch (...) { retained.check(); prepared->check(); throw; }
			retained.check(); prepared->check();
		};
		const auto callback = [&](auto &&provider) {
			check();
			try { auto value = provider(); check(); const auto owned = value; return owned; }
			catch (...) { check(); throw; }
		};
		const auto maps = candidate._loader; const auto objects = candidate._objectLoader;
		candidate._loader = [&](XeenMapIdentity id) {
			auto value = callback([&] { return maps(id); }); prepared->admitMap(id, value); return value;
		};
		if (objects) candidate._objectLoader = [&](XeenMapIdentity id) {
			auto value = callback([&] { return objects(id); }); prepared->admitObjects(id, value); return value;
		};
		static_cast<void>(candidate.map(20));
		candidate.restoreSessionState(
			{_sessionState._objects.begin(), _sessionState._objects.end()},
			{_sessionState._events.begin(), _sessionState._events.end()},
			[&](XeenMapIdentity id) { return callback([&] { return events(id); }); });
		adoptPhase(); // Only the checked ordinary overlay transition is adopted here.
		const auto statistics = callback(monsters);
		auto actors = XeenActorApproach::actorsFromResources(candidate.objectFile(20), statistics);
		const auto eventFile = callback([&] { return events(20); });
		XeenActorApproach::validateEnvironment(candidate, actors, eventFile);
		xeenApplyCompletedOverlay(actors, _sessionState._completedMonster);
		if (actors.size() != _sessionState._actors.size()) throw std::invalid_argument("completed re-entry actor count changed");
		for (unsigned i = 0; i < actors.size(); ++i)
			if (!xeen_state::sameActor(actors[i], _sessionState._actors[i]))
				throw std::invalid_argument("completed re-entry resources differ from retained actors");
		auto &s = candidate._sessionState;
		s._actors.swap(actors); s._completedMonster = _sessionState._completedMonster;
		s._entry = XeenEncounterEntry::Diagnostic27;
		s._diagnostic27 = s._combatEntered = s._combatAccounted = true;
		s._encounterMarked = s._encounterInitialized = s._encounterTerminal = true;
		s._completion = XeenEncounterCompletion::VictoryQuiescent;
		s._completedAuthority.emplace(xeenCompletedPreimage(candidate, party, entry));
		adoptPhase(); // Retain private completed facts across all nested preflight calls.
		try { presentation(candidate, party, entry, flags); }
		catch (...) { check(); throw; }
		check();
		auto authority = *s._completedAuthority;
		authority.camera = &camera;
		XeenCompletedReentry result{_sessionState._completedEntryGeneration,
			_sessionState._completedEntryGeneration + 1, entry, s._completedMonster};
		static_assert(std::is_nothrow_move_assignable_v<XeenCompletedEncounterAuthority>);
		check();
		_maps.swap(candidate._maps); _objects.swap(candidate._objects);
		_sessionState._actors.swap(s._actors); camera = entry;
		*_sessionState._completedAuthority = std::move(authority);
		_sessionState._completedEntryGeneration = result.newGeneration;
		_sessionState._completedLease = 0; _sessionState._completedLeaseKind.reset();
		++_sessionState._encounterRevision;
		return result;
	} catch (...) {
		if (retainedGuard && !retainedGuard->current())
			escalateCompletedGuard(ticket, XeenCompletedGuard::Operation, lease, XeenCompletedGuard::Integrity);
		else if (releaseCompletedGuard(ticket, XeenCompletedGuard::Operation, lease) && releasedOnFailure)
			*releasedOnFailure = completedTicket(party, camera);
		throw;
	}
}
}
