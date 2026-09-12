#include "games/xeen/XeenActorApproach.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenOutdoorSceneTables.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>

// Adapted from ScummVM developers' GPL-3.0-or-later Xeen combat.cpp,
// interface_scene.cpp and interface.cpp at 6814ee9ba54582f5b5adcffab49efbbd8f589edd.
namespace mmodern {
namespace {
bool coordinate(int x, int y) { return x >= 0 && x < 32 && y >= 0 && y < 32; }
bool envelope(int x, int y) { return x >= 13 && x <= 14 && y >= 1 && y <= 2; }
void require(bool yes, const char *why) { if (!yes) throw std::invalid_argument(why); }
void bounded(const std::vector<XeenActor> &actors, const XeenCamera &camera) {
	require(actors.size() <= XeenActorApproach::kCapacity, "monster simulation capacity exceeded");
	require(static_cast<unsigned>(camera.direction) < 4 && coordinate(camera.x, camera.y),
		"invalid actor camera");
	for (std::size_t i = 0; i < actors.size(); ++i)
		require(actors[i].id.mapId == camera.mapId && actors[i].id.recordIndex == i,
			"actor identity/order differs from original record order");
}
struct Query { int sample; std::array<int, 3> slots; XeenActorPlacement placement; };
constexpr Query queries[] = {
	{2,{0,1,2},XeenActorPlacement::SameCell}, {7,{3,4,5},XeenActorPlacement::Forward},
	{5,{12,-1,-1},XeenActorPlacement::ForwardLeft}, {9,{13,-1,-1},XeenActorPlacement::ForwardRight},
	{14,{6,7,8},XeenActorPlacement::Other}, {12,{14,20,-1},XeenActorPlacement::Other},
	{16,{15,21,-1},XeenActorPlacement::Other}, {27,{9,10,11},XeenActorPlacement::Other},
	{25,{16,22,24},XeenActorPlacement::Other}, {23,{18,-1,-1},XeenActorPlacement::Other},
	{29,{17,23,25},XeenActorPlacement::Other}, {31,{19,-1,-1},XeenActorPlacement::Other}
};
std::pair<int,int> offset(XeenDirection d, int sample) {
	const int x = xeen_scene_tables::kSouthX[sample], y = xeen_scene_tables::kSouthY[sample];
	switch (d) {
	case XeenDirection::North: return {-x,-y};
	case XeenDirection::East: return {-y,x};
	case XeenDirection::South: return {x,y};
	case XeenDirection::West: return {y,-x};
	}
	throw std::invalid_argument("invalid actor facing");
}
void activate(std::vector<XeenActor> &actors, const XeenActorView &view) noexcept {
	for (std::size_t i = 0; i < actors.size(); ++i)
		actors[i].activated = actors[i].activated || view.activation[i];
}
bool sameEntity(const XeenMapEntity &a, const XeenMapEntity &b) {
	return a.x == b.x && a.y == b.y && a.tableIndex == b.tableIndex &&
		a.direction == b.direction && a.resourceId == b.resourceId;
}
XeenMonsterTerrain terrainAt(XeenWorld &world, const XeenActor &actor, int x, int y) {
	const auto cell = world.sampleCell(actor.id.mapId, x, y);
	if (!cell || !cell->geometry->isOutdoors()) return XeenMonsterTerrain::Unsupported;
	const auto *layers = std::get_if<XeenOutdoorLayers>(&cell->cell->geometry);
	if (!layers || layers->surface >= 16) return XeenMonsterTerrain::Unsupported;
	// Only the verified nonflying Clouds ordinary-ground branch is admitted.
	// Party collision uses a different set of middle values and is not an oracle here.
	switch (layers->middle) {
	case 0: case 2: case 3: case 4: case 5: case 6: case 8: case 11: case 13: case 14:
		if (cell->geometry->surfaceTypes[layers->surface] == 1 && actor.original.resourceId != 59)
			return XeenMonsterTerrain::Allowed;
		return XeenMonsterTerrain::Unsupported;
	default: return XeenMonsterTerrain::Unsupported;
	}
}
}

std::vector<XeenActor> XeenActorApproach::actorsFromResources(const XeenObjectFile &mob,
		const std::vector<XeenMonsterRecord> &statistics) {
	require(mob.resourcePresent && bool(mob.mapId), "missing original monster MOB");
	require(mob.entities.monsters.size() <= kCapacity, "monster simulation capacity exceeded");
	require(!statistics.empty() && statistics.size() <= XeenMonsterFormat::kMaximumBytes / 60,
		"invalid monster statistics count");
	std::vector<XeenActor> actors;
	actors.reserve(mob.entities.monsters.size());
	for (std::size_t i = 0; i < mob.entities.monsters.size(); ++i) {
		const auto &original = mob.entities.monsters[i];
		XeenActor a;
		a.id = {mob.mapId, i}; a.original = original; a.x = original.x; a.y = original.y;
		if (original.hasResource()) {
			require(static_cast<std::size_t>(original.resourceId) < statistics.size(), "missing monster type statistics");
			a.statistics = statistics[original.resourceId];
			a.hp = a.statistics->baseHp();
			a.lifecycle = XeenActorLifecycle::Present;
		}
		if (original.isDisabled()) a.lifecycle = XeenActorLifecycle::Disabled;
		actors.push_back(a);
	}
	return actors;
}

XeenActorView XeenActorApproach::classify(const std::vector<XeenActor> &actors,
		const XeenCamera &camera) {
	bounded(actors, camera);
	XeenActorView view;
	for (std::size_t i = 0; i < actors.size(); ++i) {
		const auto &actor = actors[i];
		for (const auto &q : queries) {
			const auto delta = offset(camera.direction, q.sample);
			if (actor.x != camera.x + delta.first || actor.y != camera.y + delta.second) continue;
			view.activation[i] = true; // Independent of full selection groups and pixel visibility.
			view.placements[i] = q.placement;
			for (int slot : q.slots) {
				if (slot >= 0 && !view.slots[slot]) { view.slots[slot] = actor.id; break; }
			}
		}
	}
	return view;
}

std::array<unsigned, 1024> XeenActorApproach::occupancy(const std::vector<XeenActor> &actors) {
	require(actors.size() <= kCapacity, "monster simulation capacity exceeded");
	std::array<unsigned, 1024> counts{};
	for (const auto &a : actors) if (coordinate(a.x, a.y)) ++counts[a.y * 32 + a.x];
	return counts;
}

std::vector<XeenActor> XeenActorApproach::move(const std::vector<XeenActor> &actors,
		const XeenCamera &camera, const Terrain &terrain, bool movementEnabled) {
	bounded(actors, camera);
	auto result = actors;
	if (!movementEnabled) return result;
	require(bool(terrain), "missing monster terrain predicate");
	auto counts = occupancy(actors);
	std::array<bool, kCapacity> moved{};
	for (int pass = 0; pass < 2; ++pass) {
		for (int dy = 3; dy >= -3; --dy) for (int dx = -3; dx <= 3; ++dx) {
			for (std::size_t i = 0; i < result.size(); ++i) {
				auto &a = result[i];
				if (a.x != camera.x + dx || a.y != camera.y + dy || !a.activated || moved[i]) continue;
				require(coordinate(a.x, a.y) && a.lifecycle == XeenActorLifecycle::Present &&
					a.status == XeenActorStatus::Physical && a.statistics && a.statistics->supportsApproach() &&
					a.original.resourceId != 59, "unsupported relevant actor movement");
				const int sx = dx < 0 ? 1 : dx > 0 ? -1 : 0;
				const int sy = dy < 0 ? 1 : dy > 0 ? -1 : 0;
				// Literal MONSTER_GRID_X/Y and GRID3, including same-column/row zero deltas.
				std::pair<int,int> primary{sx, dx == 0 ? sy : 0};
				std::pair<int,int> fallback{dy == 0 ? sx : 0, sy};
				if (camera.direction == XeenDirection::East || camera.direction == XeenDirection::West)
					std::swap(primary, fallback);
				auto predicate = [&](std::pair<int,int> d) {
					const auto t = terrain(a, a.x + d.first, a.y + d.second);
					require(t != XeenMonsterTerrain::Unsupported, "unsupported monster terrain");
					return t == XeenMonsterTerrain::Allowed;
				};
				std::pair<int,int> delta = primary;
				if (!predicate(primary)) { delta = fallback; if (!predicate(fallback)) continue; }
				const int x = a.x + delta.first, y = a.y + delta.second;
				// Occupancy refusal after allowed terrain never tries the terrain fallback.
				if (!coordinate(x,y) || counts[y * 32 + x] >= 3) continue;
				++counts[y * 32 + x]; --counts[a.y * 32 + a.x];
				a.x = x; a.y = y; moved[i] = true;
			}
		}
	}
	return result;
}

void XeenActorApproach::validateDomain(XeenWorld &world, const XeenPartyState &party,
		const XeenGameplayContext &context, const std::vector<XeenActor> &actors,
		const XeenEventFile &events) {
	bounded(actors, kEntry);
	require(context.profile == XeenBehaviorProfile::WorldOfXeenClouds &&
		context.difficulty == XeenDifficulty::Adventurer && context.day == 1 && context.year == 610 &&
		context.minutes >= 480 && context.minutes < 960 && context.ctr24 < 24 &&
		!context.rested && !context.newDay && context.effects == std::array<std::uint8_t,9>{} &&
		context.lightAndResistances == std::array<std::uint16_t,6>{}, "unsupported encounter context");
	require(party.party.activeRosterIds() == std::vector<std::uint8_t>({0,18,14,11,1,6}) &&
		party.firstSerializedCount == 6 && party.effectiveSerializedCount == 6, "unsupported encounter party");
	for (auto id : party.party.activeRosterIds()) {
		const auto &c = party.roster.at(id);
		require(c.rosterId == id && c.currentHp > 0 && c.conditions == std::array<std::uint8_t,16>{},
			"encounter requires original Good party owners");
	}
	validateEnvironment(world, actors, events);
}

void XeenActorApproach::validateEnvironment(XeenWorld &world,
		const std::vector<XeenActor> &actors, const XeenEventFile &events) {
	bounded(actors, kEntry);
	const auto &geometry = world.map(20).geometry;
	require(geometry.isOutdoors() && geometry.flags == 0, "unsupported encounter map flags");
	require(events.mapId == XeenMapIdentity(20) && events.resourcePresent, "missing encounter event data");
	for (const auto &e : events.records) require(!envelope(e.x,e.y), "event inside encounter envelope");
	for (int y = 1; y <= 2; ++y) for (int x = 13; x <= 14; ++x) {
		const auto &c = geometry.cells[y * 16 + x];
		const auto *l = std::get_if<XeenOutdoorLayers>(&c.geometry);
		require(l && l->surface < 16 && geometry.surfaceTypes[l->surface] == 1 && l->middle == 3 &&
			c.rawAttributes == 0 && c.flags == 0 && c.rawWord == 0x31,
			"unsupported encounter ground or hazard");
	}
	const auto &mob = world.objectFile(20);
	require(mob.resourcePresent && actors.size() == mob.entities.monsters.size() && actors.size() > 5,
		"encounter original record list changed");
	for (std::size_t i = 0; i < actors.size(); ++i) {
		const auto &a = actors[i];
		require(sameEntity(a.original, mob.entities.monsters[i]), "encounter original metadata changed");
		if (i != 5) require(a.x == a.original.x && a.y == a.original.y, "encounter bystander moved");
	}
	const auto &anchor = actors[5];
	require(anchor.original.x == 13 && anchor.original.y == 2 && envelope(anchor.x,anchor.y) &&
		anchor.lifecycle == XeenActorLifecycle::Present && anchor.status == XeenActorStatus::Physical &&
		anchor.statistics && anchor.statistics->supportsApproach() && anchor.original.resourceId != 59,
		"unsupported encounter anchor metadata");
	// Full four-cell/four-facing union, independent of activation and occlusion.
	for (int y = 1; y <= 2; ++y) for (int x = 13; x <= 14; ++x) for (unsigned d = 0; d < 4; ++d) {
		const auto view = classify(actors, {20,x,y,static_cast<XeenDirection>(d)});
		for (std::size_t i = 0; i < actors.size(); ++i) {
			const auto &a = actors[i];
			if (i != 5) require(!view.activation[i] && !(a.x >= x-3 && a.x <= x+3 && a.y >= y-3 && a.y <= y+3),
				"another original actor affects encounter isolation");
		}
	}
}

XeenEncounterResult XeenActorApproach::initialize(XeenWorld &world, XeenPartyState &party,
		XeenCamera &camera, XeenEncounterState &state, const std::vector<XeenMonsterRecord> &statistics,
		const XeenGameplayContext &context, const XeenEventFile &events) {
	auto &session = world._sessionState;
	const auto uninitialized = [&] {
		if (session._entry == XeenEncounterEntry::Diagnostic27 && (!world._combatCheck || session._combatApproachState != &state)) return false;
		return !session._encounterInitialized && !session._encounterTerminal && session._actors.empty() &&
			session._encounterRevision == 0 && !party.encounterContext &&
			!state._world && !state._party && !state._camera && state._revision == 0 && state._pending == 0 &&
			state._phase == XeenEncounterPhase::Exploring && state._reason == XeenEncounterStop::None;
	};
	require(uninitialized(),
		"encounter initialization is one-time only");
	world.markEncounterSession();
	require(camera.mapId == kEntry.mapId && camera.x == 13 && camera.y == 1 &&
		camera.direction == XeenDirection::North, "encounter requires fixed diagnostic entry");
	require(context.minutes == 480 && context.ctr24 == 0, "encounter requires initial PTY time");
	auto actors = actorsFromResources(world.objectFile(20), statistics);
	validateDomain(world, party, context, actors, events);
	XeenEncounterResult result;
	result.outcome = XeenEncounterOutcome::Started; result.revision = 1;
	result.view = classify(actors,camera);
	activate(actors,result.view);
	static_assert(std::is_nothrow_copy_assignable<decltype(party.encounterContext)>::value);
	// A provider may have initialized these same owners reentrantly. Never replace it.
	require(session._encounterMarked && uninitialized(),
		"encounter initialization authority changed during preparation");
	// Preparation ends here. Only nonthrowing stores/swaps until return.
	if (world._combatCheck) world._combatCheck();
	session._actors.swap(actors);
	party.encounterContext = context;
	session._encounterInitialized = true;
	session._encounterRevision = 1;
	state._world = &world; state._party = &party; state._camera = &camera; state._revision = 1;
	return result;
}

XeenEncounterResult XeenActorApproach::initializeFromResources(XeenAssetSource &assets, XeenWorld &world,
		XeenPartyState &party, XeenCamera &camera, XeenEncounterState &state) {
	require(world._sessionState._entry != XeenEncounterEntry::Diagnostic27 || bool(world._combatCheck), "Diagnostic27 initialization requires its coordinator");
	world.markEncounterSession();
	const auto bytes = assets.readCloudsMonsterStatisticsFromDarkArchive();
	require(bool(bytes), "missing DARK.CC/xeen.mon");
	const auto statistics = XeenMonsterFormat::parse(*bytes);
	const auto context = XeenGameplayContextFormat::parse(assets.readInitialResource("maze.pty"));
	XeenEventLoader loader([&assets](const std::string &name) -> std::optional<std::vector<std::uint8_t>> {
		if (!assets.hasInitialResource(name)) return std::nullopt;
		return assets.readInitialResource(name);
	});
	return initialize(world,party,camera,state,statistics,context,loader.load(20));
}

bool XeenActorApproach::authoritative(const XeenWorld &world, const XeenPartyState &party,
		const XeenCamera &camera, const XeenEncounterState &state) noexcept {
	const auto &s = world._sessionState;
	return s._encounterMarked && s._encounterInitialized && state._world == &world &&
		state._party == &party && state._camera == &camera && state._revision == s._encounterRevision &&
		s._encounterTerminal == (state._phase != XeenEncounterPhase::Exploring);
}

XeenEncounterResult XeenActorApproach::stop(XeenWorld &world, XeenEncounterState &state,
		XeenEncounterStop reason) noexcept {
	auto &s = world._sessionState;
	XeenEncounterResult r;
	r.revision = s._encounterRevision;
	if (s._diagnostic27 && (!world._combatCheck || s._combatApproachState != &state)) { r.outcome = XeenEncounterOutcome::Refused; return r; }
	if (s._diagnostic27 && (!world._combatAuthorized || !world._combatAuthorized())) { r.outcome = XeenEncounterOutcome::Stale; return r; }
	if (state._world != &world || state._revision != s._encounterRevision) {
		r.outcome = XeenEncounterOutcome::Stale; return r;
	}
	if (s._encounterTerminal) { r.outcome = XeenEncounterOutcome::Terminal; return r; }
	state._pending = 0; state._phase = XeenEncounterPhase::SupportStopped; state._reason = reason;
	s._encounterTerminal = true;
	if (s._encounterRevision != std::numeric_limits<std::uint64_t>::max()) ++s._encounterRevision;
	state._revision = s._encounterRevision;
	r.revision = state._revision; r.reason = reason; r.outcome = XeenEncounterOutcome::Stopped;
	return r;
}

XeenEncounterResult XeenActorApproach::action(XeenWorld &w, XeenPartyState &p, XeenCamera &c,
		XeenEncounterState &s, XeenEncounterAction a, const XeenEventFile &e) {
	return transition(w,p,c,s,a,e,false);
}
XeenEncounterResult XeenActorApproach::pulse(XeenWorld &w, XeenPartyState &p, XeenCamera &c,
		XeenEncounterState &s, const XeenEventFile &e) {
	return transition(w,p,c,s,XeenEncounterAction::Unsupported,e,true);
}

XeenEncounterResult XeenActorApproach::transition(XeenWorld &world, XeenPartyState &party,
		XeenCamera &camera, XeenEncounterState &state, XeenEncounterAction action,
		const XeenEventFile &events, bool pulse) {
	auto &session = world._sessionState;
	XeenEncounterResult result;
	result.revision = session._encounterRevision;
	if (session._diagnostic27 && (!world._combatCheck || session._combatApproachState != &state)) return result;
	if (state._world != &world || state._party != &party || state._camera != &camera ||
		state._revision != session._encounterRevision) { result.outcome = XeenEncounterOutcome::Stale; return result; }
	if (session._encounterTerminal) { result.outcome = XeenEncounterOutcome::Terminal; return result; }
	if (!pulse && (action == XeenEncounterAction::Unsupported || static_cast<unsigned>(action) > 5)) return result;
	if (state._revision == std::numeric_limits<std::uint64_t>::max()) return stop(world,state,XeenEncounterStop::Overflow);
	const auto entry = state; // Keep authorization facts from before any provider callback.
	const auto combatAuthorized = world._combatAuthorized;
	const auto entryCurrent = [&] {
		return (!session._diagnostic27 || (combatAuthorized && combatAuthorized())) &&
			!session._encounterTerminal && session._encounterRevision == entry._revision &&
			state._revision == entry._revision && state._world == entry._world &&
			state._party == entry._party && state._camera == entry._camera &&
			state._pending == entry._pending && state._phase == entry._phase && state._reason == entry._reason;
	};
	const auto refusal = [&] {
		XeenEncounterResult refused;
		refused.outcome = session._encounterTerminal ? XeenEncounterOutcome::Terminal : XeenEncounterOutcome::Stale;
		refused.revision = session._encounterRevision;
		refused.reason = state._reason;
		return refused;
	};
	const auto stopForEntry = [&](XeenEncounterStop reason) {
		// Failure does not authorize this operation to stop a newer operation's work.
		return entryCurrent() ? stop(world,state,reason) : refusal();
	};
	try {
		require(session._encounterInitialized && party.encounterContext && state._pending <= 3,
			"incomplete encounter owners");
		validateDomain(world,party,*party.encounterContext,session._actors,events);
		require(camera.mapId == XeenMapIdentity(20) && envelope(camera.x,camera.y) &&
			static_cast<unsigned>(camera.direction) < 4, "encounter camera left admitted domain");
		auto candidateCamera = camera;
		auto context = *party.encounterContext;
		auto actors = session._actors;
		auto pending = state._pending;
		bool charge = false, stepTime = false;
		result.outcome = pulse ? XeenEncounterOutcome::Pulsed : XeenEncounterOutcome::Accepted;
		if (!pulse && action == XeenEncounterAction::Wait) charge = stepTime = true;
		else if (!pulse) {
			NavigationAction navigation = NavigationAction::MoveForward;
			if (action == XeenEncounterAction::Backward) navigation = NavigationAction::MoveBackward;
			if (action == XeenEncounterAction::Left) navigation = NavigationAction::TurnLeft;
			if (action == XeenEncounterAction::Right) navigation = NavigationAction::TurnRight;
			const auto movement = XeenMovement().apply(world,candidateCamera,navigation);
			charge = movement == XeenMovementResult::Moved;
			stepTime = charge || movement == XeenMovementResult::Turned;
			if (!stepTime) result.outcome = XeenEncounterOutcome::Blocked;
			if (charge && (candidateCamera.mapId != XeenMapIdentity(20) || !envelope(candidateCamera.x,candidateCamera.y)))
				return stopForEntry(XeenEncounterStop::Envelope);
		}
		// Time-boundary refusal precedes old movement, camera, ctr24 and minute stores.
		if (charge && context.minutes >= 950) return stopForEntry(XeenEncounterStop::Time);
		auto opportunity = [&] {
			actors = move(actors,candidateCamera,[&world](const XeenActor &a, int x, int y) { return terrainAt(world,a,x,y); });
			pending = 0; ++result.movementOpportunities;
		};
		if (charge) {
			context.minutes += 10;
			if (pending) opportunity();
			pending = 3;
			if (action == XeenEncounterAction::Wait) opportunity();
		}
		if (stepTime) context.ctr24 = (context.ctr24 + 1) % 24;
		if (pulse && pending && --pending == 0) opportunity();
		const bool classifyBoundary = pulse || action == XeenEncounterAction::Wait;
		if (classifyBoundary) {
			result.view = classify(actors,candidateCamera);
			activate(actors,result.view);
		}
		validateDomain(world,party,context,actors,events);
		const bool engaged = classifyBoundary && result.view.engaged();
		if (engaged) { pending = 0; result.outcome = XeenEncounterOutcome::Engaged; }
		result.revision = entry._revision + 1;
		// All allocations, terrain/provider calls, validation and result construction are done.
		// No externally applicable prepared result escapes this synchronous single-writer boundary.
		if (world._combatCheck) world._combatCheck();
		static_assert(std::is_nothrow_copy_assignable<XeenEncounterResult>::value);
		if (!entryCurrent() || !session._encounterMarked || !session._encounterInitialized || !party.encounterContext)
			return refusal();
		session._actors.swap(actors);
		camera = candidateCamera; party.encounterContext = context;
		state._pending = pending; state._revision = result.revision;
		session._encounterRevision = result.revision;
		if (engaged) { state._phase = XeenEncounterPhase::Engaged; session._encounterTerminal = true; }
		return result;
	} catch (const std::invalid_argument &) {
		return stopForEntry(XeenEncounterStop::Domain);
	} catch (...) {
		return stopForEntry(XeenEncounterStop::Preparation);
	}
}
} // namespace mmodern
