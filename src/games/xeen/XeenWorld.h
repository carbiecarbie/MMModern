#ifndef MMODERN_GAMES_XEEN_WORLD_H
#define MMODERN_GAMES_XEEN_WORLD_H

#include "games/xeen/XeenMap.h"
#include "games/xeen/XeenRecordIdentity.h"
#include "games/xeen/XeenEventFile.h"
#include "games/xeen/XeenActor.h"
#include "games/xeen/XeenEncounterEntry.h"
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenJourneyContent.h"
#include "games/xeen/XeenScenePresentation.h"
#include <stdexcept>
#include <set>
#include <array>
#include <bitset>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <memory>
#include <vector>

namespace mmodern {

class XeenEventPublication;
enum class XeenJourneyActivity { Unbound, Quiet, Event, Approach, Attachment, Combat, Presentation, Saving, Shoot, Reward, ItemUse, Casting, Failed, SupportStopped, Service };
class XeenGameFlags;
class XeenJourneyCapture;
struct XeenActorView;
struct XeenJourneyRestoration;
struct XeenCellSample {
	XeenMapIdentity mapId = 0;
	int x = 0;
	int y = 0;
	const XeenMapGeometry *geometry = nullptr;
	const XeenMapCell *cell = nullptr;
};

// Session-owned overlays and explicit encounter authority. Original records remain untouched.
class XeenSessionWorldState {
public:
	XeenSessionWorldState()=default;
	XeenSessionWorldState(const XeenSessionWorldState &)=default;
	XeenSessionWorldState &operator=(const XeenSessionWorldState &)=default;
	bool journey() const noexcept { return _entry == XeenEncounterEntry::Journey; }
	XeenJourneyActivity journeyActivity() const noexcept { return _journeyActivity; }
	std::uint32_t skeletonSeed() const noexcept { return _skeletonSeed; }
	const XeenMutableOptional<XeenJourneyRandomState> &journeyRandom() const noexcept { return _journeyRandom; }
	XeenReadOnlySet<XeenMonsterIdentity> accountedMonsters() const noexcept { return XeenReadOnlySet<XeenMonsterIdentity>(_accountedMonsters); }
	bool isObjectDisabled(XeenObjectIdentity id) const { return _objects.count(id) != 0; }
	bool isEventDisabled(XeenEventIdentity id) const { return _events.count(id) != 0; }
	std::size_t disabledObjectCount() const { return _objects.size(); }
	std::size_t disabledEventCount() const { return _events.size(); }
	XeenReadOnlySet<XeenObjectIdentity> disabledObjects() const noexcept { return XeenReadOnlySet<XeenObjectIdentity>(_objects); }
	XeenReadOnlySet<XeenEventIdentity> disabledEvents() const noexcept { return XeenReadOnlySet<XeenEventIdentity>(_events); }
	bool encounterMarked() const { return _encounterMarked; }
	XeenEncounterEntry encounterEntry() const noexcept { return _entry; }
	bool encounterInitialized() const { return _encounterInitialized; }
	bool encounterTerminal() const { return _encounterTerminal; }
	XeenReadOnlyVector<XeenActor> actors() const noexcept { return XeenReadOnlyVector<XeenActor>(_actors); }
	bool hasRegionalActors(XeenMapIdentity mapId) const noexcept {
		return (!_actors.empty() && _actors.front().id.mapId==mapId) ||
			(mapId == XeenMapIdentity(28) && _vertigoActors.has_value());
	}
	XeenReadOnlyVector<XeenActor> regionalActors(XeenMapIdentity mapId) const {
		if (!_actors.empty() && _actors.front().id.mapId==mapId) return XeenReadOnlyVector<XeenActor>(_actors);
		if (mapId == XeenMapIdentity(28) && _vertigoActors) return XeenReadOnlyVector<XeenActor>(*_vertigoActors);
		throw std::out_of_range("Regional actor collection is absent");
	}
private:
	friend class XeenEncounterFlow;
	friend struct XeenTrainingTestAccess;
	friend struct XeenPurchaseTestAccess;
	XeenMutationMarker _mutation;
	XeenJourneyActivity _journeyActivity = XeenJourneyActivity::Unbound;
	const void *_journeyOwner = nullptr;
	std::uint64_t _journeyGeneration = 0;
	std::uint32_t _skeletonSeed = 0;
	XeenMutableOptional<XeenJourneyRandomState> _journeyRandom;
	std::set<XeenMonsterIdentity> _accountedMonsters;
	friend class XeenWorld;
	friend class XeenActorApproach;
	friend class XeenCombat;
	friend class XeenSaveState;
	friend class XeenRestoreGuard;
	friend class XeenEventPublication;
	const void *_combatOwner = nullptr;
	const void *_combatApproachState = nullptr;
	bool _combatEntered = false;
	XeenEncounterEntry _entry = XeenEncounterEntry::Ordinary;
	bool _encounterMarked = false, _encounterInitialized = false, _encounterTerminal = false;
	mutable std::uint64_t _encounterRevision = 0;
	std::vector<XeenActor> _actors;
	// Optional Vertigo actor collection. Absence is distinct from an empty city.
	std::optional<std::vector<XeenActor>> _vertigoActors;
	std::set<XeenObjectIdentity> _objects;
	std::set<XeenEventIdentity> _events;
};

class XeenWorld {
public:
	class GameplayBorrow {
	public:
		~GameplayBorrow() { for (const auto &state : owners) { --state->references; ++state->revision; } }
		bool current() const noexcept {
			for (const auto &state : owners) if (!state->alive) return false;
			return true;
		}
		GameplayBorrow(const GameplayBorrow &) = delete;
		GameplayBorrow &operator=(const GameplayBorrow &) = delete;
	private:
		friend class XeenEventFlow;
		friend class XeenCombat;
		GameplayBorrow(XeenWorld &, XeenPartyState &, XeenCamera &, const XeenGameFlags &);
		std::array<std::shared_ptr<XeenGameplayBorrowOwner::State>, 5> owners;
	};
	using MapLoader = std::function<XeenMap(XeenMapIdentity)>;

	using ObjectLoader = std::function<XeenObjectFile(XeenMapIdentity)>;
	using EventLoader = std::function<XeenEventFile(XeenMapIdentity)>;
	explicit XeenWorld(MapLoader loader, ObjectLoader objectLoader = {});
	const XeenObjectFile &objectFile(XeenMapIdentity mapId);
	// Resource observations for presentation. Live session actors take precedence;
	// fallback collections never enter combat/session state or simulation.
	std::vector<XeenActor> sceneActors(XeenMapIdentity,
		const std::function<std::vector<XeenMonsterRecord>()> &loadStatistics = {});
	XeenScenePresentation &scenePresentation() { return _scenePresentation; }
	const XeenScenePresentation &scenePresentation() const { return _scenePresentation; }
	bool isObjectDisabled(XeenObjectIdentity id);
	bool isEventDisabled(XeenEventIdentity id) const { return _sessionState.isEventDisabled(id); }
	std::optional<XeenObjectIdentity> selectObject(const XeenCamera &camera);
	XeenEventRecord effectiveEvent(XeenEventIdentity id, const XeenEventRecord &base) const;
	void disableObject(XeenObjectIdentity id);
	void disableEventsAtCell(const XeenCamera &physical, const XeenEventFile &events);
	void applyRemove(const XeenCamera &physical, std::optional<XeenObjectIdentity> selected,
		const XeenEventFile &events, const XeenEventPublication *publication = nullptr);
	std::size_t cachedObjectFileCount() const { return _objects.size(); }
	XeenWorld(const XeenWorld &) = delete;
	XeenWorld &operator=(const XeenWorld &) = delete;
	const XeenSessionWorldState &sessionState() const { return _sessionState; }
	bool regionalJourney() const noexcept {
		return (_sessionState.journey() || _detachedEventCandidate);
	}
	bool detachedEventCandidate() const noexcept {
		return _detachedEventCandidate && _sessionState._entry==XeenEncounterEntry::Ordinary;
	}
	// Irreversible safety marker, including failed preparation. No clear/reset API.
	void markEncounterSession() noexcept { XeenMutationWatch::write(this);_sessionState._encounterMarked = true; }
	void markEncounterSession(XeenEncounterEntry entry) {
		XeenMutationWatch::write(this);
		if (entry == XeenEncounterEntry::Ordinary ||
			(_sessionState._entry != XeenEncounterEntry::Ordinary && _sessionState._entry != entry) ||
			(_sessionState._encounterMarked && _sessionState._entry == XeenEncounterEntry::Ordinary))
			throw std::logic_error("Encounter entry cannot be replaced");
		_sessionState._entry = entry;
		_sessionState._encounterMarked = true;
	}
	bool hasEncounterState() const {
		return _sessionState._encounterMarked || _sessionState._encounterInitialized ||
			!_sessionState._actors.empty();
	}
	bool journeyCaptureEligible(const XeenPartyState &, const XeenCamera &) const noexcept;
	// For unpublished startup owners only. Validates every original identity
	// before replacing either set; no script execution or cell expansion.
	void restoreSessionState(const std::vector<XeenObjectIdentity> &objects,
		const std::vector<XeenEventIdentity> &events, const EventLoader &eventLoader);
	// Invalidates map/cell/object-file references, not the session state.
	void discardMapCache();

	const XeenMap &map(XeenMapIdentity mapId);
	std::optional<XeenCellSample> sampleCell(XeenMapIdentity mapId, int x, int y);
	// Original Map::getCell neighbor queries, for scene observation only.
	// Movement and mechanic admission continue to use sampleCell.
	std::optional<XeenCellSample> sceneCell(XeenMapIdentity mapId, int x, int y);
	std::size_t cachedMapCount() const { return _maps.size(); }
	// Unpublished Vertigo Event candidate; callers retain a live owner guard.
	std::unique_ptr<XeenWorld> transitionCandidate() const;
	void copyEventParty(const XeenPartyState &source, XeenPartyState &candidate) const;
	void stageVertigoActors(const XeenObjectFile &, const std::vector<XeenMonsterRecord> &);
	void applySpawn(std::uint8_t slot, int x, int y, std::uint8_t unused);
	void applyAlterEvent(const XeenCamera &physical, std::uint8_t line, std::uint8_t replacement,
		const XeenEventFile &);
	void publishTransition(XeenWorld &candidate) noexcept;
	XeenScenePresentation prepareSpawnPresentation(const XeenWorld &candidate) const;
	XeenActorView prepareTransitionArrival(const XeenCamera &);

private:
	friend class XeenSaveState;
	friend class XeenEventPublication;
	friend class XeenEncounterFlow;
	friend struct XeenTrainingTestAccess;
	friend class XeenRestoreGuard;
	friend class XeenCombat;
	friend class XeenActorApproach;
	friend void xeenValidateVertigoActors(XeenWorld &, const std::vector<XeenActor> &);
	void swapPreparedState(XeenWorld &candidate) noexcept;
	// Process-lifetime capability identity. It belongs to this object lifetime,
	// not session gameplay state, and is never serialized or swapped.
	const std::uint64_t _incarnation;
	std::uint64_t _ownerRevision = 0;
	std::uint64_t _cacheRevision = 0;
	XeenGameplayBorrowOwner _gameplayBorrow;
	std::weak_ptr<XeenJourneyCapture> _journeyCapture;
	std::shared_ptr<XeenJourneyRestoration> _journeyRestoration;
	// Combat checks retained authority after fallible resource providers.
	std::function<void()> _combatCheck;
	// Retained combat authorization, separate from domain validity.
	std::function<bool()> _combatAuthorized;
	XeenSessionWorldState _sessionState;
	// Immutable loaded catalog for original and script-created city slots.
	std::vector<XeenMonsterRecord> _cityStatistics;
	XeenMutable<std::uint16_t> _cityOriginalActorCount=0;
	// Derived from checked immutable city resources; never gameplay authority.
	bool _detachedEventCandidate = false;
	// Detached Event presentation only; no additional gameplay slot ownership.
	std::set<std::size_t> _spawnedPresentationSlots;
	// Stable dependencies never contain a scoped RestoreGuard::Providers wrapper.
	const MapLoader _baseLoader;
	const ObjectLoader _baseObjectLoader;
	MapLoader _loader;
	ObjectLoader _objectLoader;
	std::map<XeenMapIdentity, XeenObjectFile> _objects;
	std::map<XeenMapIdentity, std::vector<XeenActor>> _sceneActors;
	std::optional<std::vector<XeenMonsterRecord>> _sceneStatistics;
	XeenScenePresentation _scenePresentation;
	void validateObject(XeenObjectIdentity id);
	void validateEventCell(const XeenCamera &physical, const XeenEventFile &events);
	std::map<XeenMapIdentity, XeenMap> _maps;
};

} // namespace mmodern

#endif
