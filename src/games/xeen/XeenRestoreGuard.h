#ifndef MMODERN_XEEN_RESTORE_GUARD_H
#define MMODERN_XEEN_RESTORE_GUARD_H
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/XeenGameFlags.h"
namespace mmodern {
// Retained callback preimages, never a gameplay owner or a publication capability.
class XeenRestoreGuard {
public:
	// Watches actual nested map/MOB callbacks and admits only checked detached
	// provider values. Retained cache preimages are never replaced by cache hits.
	class Providers;
	XeenRestoreGuard(const XeenWorld &world, const XeenPartyState &party,
		const XeenCamera &camera, const XeenGameFlags &flags, bool exactCaches = false) :
		w(world), p(party), c(camera), f(flags), worldId(w._incarnation),
		worldRevision(w._ownerRevision),
		partyId(p._incarnation), rosterId(p.roster._incarnation),
		partyReplacement(p._replacement), rosterReplacement(p.roster._replacement),
		s(w._sessionState), characters(p.roster.characters()), inputs(p.roster._combatInputs),
		marked(p.roster.combatMarked()), membership(p.party.activeRosterIds()),
		quests(p.questItems.counts()), questFlags(p.questFlags.values()), context(p.encounterContext),
		first(p.firstSerializedCount), effective(p.effectiveSerializedCount), diagnostics(p.diagnostics),
		cameraValue(c), flagValues(f.values()), combatCheck(bool(w._combatCheck)), combatAuthorized(bool(w._combatAuthorized)),
		maps(w._maps), objects(w._objects), cacheRevision(w._cacheRevision), exactCaches(exactCaches),
		borrowOwners{&w._gameplayBorrow, &p._gameplayBorrow, &p.roster._gameplayBorrow, &c.gameplayBorrow, &f._gameplayBorrow} {
		for (unsigned i = 0; i < borrowOwners.size(); ++i) {
			borrowStates[i] = borrowOwners[i]->retain();
			borrowRevisions[i] = borrowStates[i]->revision;
		}
	}
	bool current() const noexcept {
		using namespace xeen_state;
		if (failed) return false;
		for (const auto &state : borrowStates) if (!state->alive) return false;
		for (unsigned i = 0; i < borrowOwners.size(); ++i)
			if (borrowOwners[i]->state != borrowStates[i] || borrowStates[i]->revision != borrowRevisions[i]) return false;
		if (w._incarnation != worldId || w._ownerRevision != worldRevision ||
			p._incarnation != partyId || p.roster._incarnation != rosterId ||
			p._replacement != partyReplacement || p.roster._replacement != rosterReplacement ||
			p.roster.combatMarked() != marked || p.party.activeRosterIds() != membership ||
			p.questItems.counts() != quests || p.questFlags.values() != questFlags ||
			!(p.encounterContext == context) || p.firstSerializedCount != first ||
			p.effectiveSerializedCount != effective || p.diagnostics != diagnostics ||
			!sameCamera(c, cameraValue) || f.values() != flagValues ||
			bool(w._combatCheck) != combatCheck || bool(w._combatAuthorized) != combatAuthorized) return false;
		const auto &live = w._sessionState;
		if (live._journeyActivity != s._journeyActivity || live._journeyOwner != s._journeyOwner ||
			live._journeyGeneration != s._journeyGeneration || live._skeletonSeed != s._skeletonSeed ||
			live._accountedMonsters != s._accountedMonsters) return false;
		if (live._combatOwner != s._combatOwner || live._combatApproachState != s._combatApproachState ||
			live._diagnostic27 != s._diagnostic27 || live._combatEntered != s._combatEntered ||
			live._combatAccounted != s._combatAccounted || live._completion != s._completion ||
			!(live._completedMonster == s._completedMonster) ||
			bool(live._completedAuthority) != bool(s._completedAuthority) ||
			live._completedPublished != s._completedPublished || live._completedEntryGeneration != s._completedEntryGeneration ||
			live._completedIntegrityUnsafe != s._completedIntegrityUnsafe || live._completedFatal != s._completedFatal ||
			live._completedLease != s._completedLease || live._completedLeaseKind != s._completedLeaseKind ||
			live._entry != s._entry || live._encounterMarked != s._encounterMarked ||
			live._encounterInitialized != s._encounterInitialized || live._encounterTerminal != s._encounterTerminal ||
			live._encounterRevision != s._encounterRevision || live._objects != s._objects || live._events != s._events ||
			live._actors.size() != s._actors.size()) return false;
		if (s._completedAuthority && !sameAuthority(*live._completedAuthority, *s._completedAuthority)) return false;
		for (std::size_t i = 0; i < characters.size(); ++i) {
			if (!sameCharacter(p.roster.characters()[i], characters[i]) ||
				bool(p.roster.combatInputs(i)) != bool(inputs[i])) return false;
			if (inputs[i] && !sameInputs(*p.roster.combatInputs(i), *inputs[i])) return false;
		}
		for (std::size_t i = 0; i < s._actors.size(); ++i)
			if (!sameActor(live._actors[i], s._actors[i])) return false;
		return cachesCurrent();
	}
	// Combat owns its gameplay preimage separately; retain the admitted resource
	// values across its publications, allowing matching cache reconstruction.
	bool cachesCurrent() const noexcept {
		using namespace xeen_state;
		if (failed || !worldAlive()) return false;
		if (exactCaches && (w._cacheRevision != cacheRevision || maps.size() != w._maps.size() || objects.size() != w._objects.size())) return false;
		for (const auto &entry : w._maps) {
			const auto found = maps.find(entry.first);
			if (found == maps.end() || !sameMap(entry.second, found->second)) return false;
		}
		for (const auto &entry : w._objects) {
			const auto found = objects.find(entry.first);
			if (found == objects.end() || !sameObjectFile(entry.second, found->second)) return false;
		}
		return true;
	}
	bool worldAlive() const noexcept { return borrowStates[0]->alive; }
	bool ownersAlive() const noexcept {
		for (const auto &state : borrowStates) if (!state->alive) return false;
		return true;
	}
	void check() const {
		if (!current()) {
			failed = true;
			throw std::logic_error("completed preparation owner preimage changed");
		}
	}
	// Called only with detached provider results after both callback guards pass,
	// before XeenWorld inserts them. Cache hits never establish a new preimage.
	void admitMap(XeenMapIdentity id, const XeenMap &value) {
		check();
		if (value.identity() != id) throw std::invalid_argument("prepared map identity mismatch");
		const auto found = maps.find(id);
		if (found != maps.end() && !xeen_state::sameMap(found->second, value))
			throw std::invalid_argument("prepared map resource changed during reconstruction");
		maps.emplace(id, value);
	}
	void admitObjects(XeenMapIdentity id, const XeenObjectFile &value) {
		check();
		if (value.mapId != id) throw std::invalid_argument("prepared object identity mismatch");
		const auto found = objects.find(id);
		if (found != objects.end() && !xeen_state::sameObjectFile(found->second, value))
			throw std::invalid_argument("prepared object resource changed during reconstruction");
		objects.emplace(id, value);
	}
private:
	friend class XeenEncounterFlow;
	// Only the coordinator's checked, callback-free authority transitions may adopt these fields.
	// Gameplay values, identity controls and cache preimages remain retained.
	void adoptJourneyCoordination() noexcept {
		const auto &live = w._sessionState;
		s._journeyActivity = live._journeyActivity; s._journeyOwner = live._journeyOwner;
		s._journeyGeneration = live._journeyGeneration; s._combatOwner = live._combatOwner;
		s._combatApproachState = live._combatApproachState; s._combatEntered = live._combatEntered;
		s._encounterTerminal = live._encounterTerminal; s._encounterRevision = live._encounterRevision;
		combatCheck = bool(w._combatCheck); combatAuthorized = bool(w._combatAuthorized);
	}
	void adoptJourneyBorrowRelease() noexcept { for (auto &revision : borrowRevisions) ++revision; }
	const XeenWorld &w; const XeenPartyState &p; const XeenCamera &c; const XeenGameFlags &f;
	std::uint64_t worldId, worldRevision;
	std::uint64_t partyId, rosterId, partyReplacement, rosterReplacement;
	XeenSessionWorldState s;
	std::array<XeenCharacter, 30> characters;
	std::array<std::optional<XeenCombatInputs>, 30> inputs;
	bool marked;
	std::vector<std::uint8_t> membership;
	XeenCloudsQuestItems::Counts quests;
	XeenCloudsQuestFlags::Values questFlags;
	std::optional<XeenGameplayContext> context;
	std::uint8_t first, effective;
	std::vector<std::string> diagnostics;
	XeenCamera cameraValue;
	XeenGameFlags::Storage flagValues;
	bool combatCheck, combatAuthorized;
	std::map<XeenMapIdentity, XeenMap> maps;
	std::map<XeenMapIdentity, XeenObjectFile> objects;
	std::uint64_t cacheRevision;
	bool exactCaches;
	std::array<const XeenGameplayBorrowOwner *, 5> borrowOwners;
	std::array<std::shared_ptr<XeenGameplayBorrowOwner::State>, 5> borrowStates;
	std::array<std::uint64_t, 5> borrowRevisions;
	mutable bool failed = false;
};
class XeenRestoreGuard::Providers {
public:
	Providers(XeenRestoreGuard &guard, XeenWorld &world, std::function<void()> authorization = {}) :
		g(guard), w(world), maps(w._loader), objects(w._objectLoader), authorization(std::move(authorization)) {
		check();
		XeenWorld::MapLoader map = [this](XeenMapIdentity id) {
			check();
			try { auto value = maps(id); check(); g.admitMap(id, value); return value; }
			catch (...) { check(); throw; }
		};
		XeenWorld::ObjectLoader object;
		if (objects) object = [this](XeenMapIdentity id) {
			check();
			try { auto value = objects(id); check(); g.admitObjects(id, value); return value; }
			catch (...) { check(); throw; }
		};
		w._loader.swap(map); w._objectLoader.swap(object);
	}
	~Providers() { if (g.worldAlive()) { w._loader.swap(maps); w._objectLoader.swap(objects); } }
	Providers(const Providers &) = delete;
	Providers &operator=(const Providers &) = delete;
private:
	XeenRestoreGuard &g; XeenWorld &w;
	XeenWorld::MapLoader maps; XeenWorld::ObjectLoader objects;
	std::function<void()> authorization;
	void check() const { if (authorization) authorization(); g.check(); }
};
}
#endif
