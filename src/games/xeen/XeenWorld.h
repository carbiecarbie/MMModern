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
#include "games/xeen/XeenBarrierState.h"
#include "games/xeen/XeenObjectState.h"
#include "games/xeen/XeenExploration.h"
#include "games/xeen/XeenMapState.h"
#include "games/xeen/XeenTransitionResult.h"
#include "games/xeen/XeenTerrainArrival.h"
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
struct XeenTerrainArrivalCandidate;
struct XeenRegionalActionCandidate;

enum class XeenDamageType : std::uint8_t;
struct XeenTypedDamageCandidate;
struct XeenConditionTimeCandidate;
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
	std::shared_ptr<const XeenMapCell> effectiveCell;
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
	// Generic per-map access (M69 decision 3). Every reader resolves the
	// entry of the map it observes; absent and empty entries are equivalent.
	const XeenMapTable &mapStates() const noexcept { return _mapStates; }
	const XeenMapState *mapState(XeenMapIdentity id) const noexcept { return _mapStates.find(id); }
	bool isObjectDisabled(XeenObjectIdentity id) const {
		const auto *state=_mapStates.find(id.mapId);return state && state->removedObjects.count(id)!=0;
	}
	bool isEventDisabled(XeenEventIdentity id) const {
		const auto *state=_mapStates.find(id.mapId);return state && state->disabledEvents.count(id)!=0;
	}
	std::size_t disabledObjectCount() const noexcept;
	std::size_t disabledEventCount() const noexcept;
	// Flat projections in identity order (the v12 save layout).
	std::set<XeenObjectIdentity> disabledObjects() const;
	std::set<XeenEventIdentity> disabledEvents() const;
	std::vector<XeenBarrierOverride> barriers() const;
	std::vector<XeenObjectOverride> objectOverrides() const;
	const XeenMutableOptional<XeenHeadCache> &headCache() const noexcept { return _headCache; }
	std::vector<XeenExploredTile> exploredTiles() const;
	bool encounterMarked() const { return _encounterMarked; }
	XeenEncounterEntry encounterEntry() const noexcept { return _entry; }
	bool encounterInitialized() const { return _encounterInitialized; }
	bool encounterTerminal() const { return _encounterTerminal; }
	bool hasRegionalActors(XeenMapIdentity mapId) const noexcept {
		const auto *state=_mapStates.find(mapId);return state && state->monsters;
	}
	XeenReadOnlyVector<XeenActor> regionalActors(XeenMapIdentity mapId) const {
		const auto *state=_mapStates.find(mapId);
		if (!state || !state->monsters) throw std::out_of_range("Regional actor collection is absent");
		return XeenReadOnlyVector<XeenActor>(*state->monsters);
	}
	// The map's monster table, empty when it was never staged.
	XeenReadOnlyVector<XeenActor> monsters(XeenMapIdentity mapId) const noexcept {
		static const std::vector<XeenActor> none;
		const auto *state=_mapStates.find(mapId);
		return XeenReadOnlyVector<XeenActor>(state && state->monsters ? *state->monsters : none);
	}
	// Fallible: creates the (empty, unobservable) entries a later
	// nonthrowing exploration publication needs.
	void reserveTiles(const std::vector<XeenExploredTile> &values);
	bool anyMonsters() const noexcept {
		for (const auto &entry:_mapStates) if (entry.second.monsters && !entry.second.monsters->empty()) return true;
		return false;
	}
	// Any staged monster table, including an empty one.
	bool anyStagedMonsters() const noexcept {
		for (const auto &entry:_mapStates) if (entry.second.monsters) return true;
		return false;
	}
private:
	friend class XeenEncounterFlow;
	friend struct XeenTrainingTestAccess;
    friend struct XeenInnTestAccess;
    friend struct XeenEntranceTestAccess;
	friend struct XeenPurchaseTestAccess;
	friend struct XeenMonsterTestAccess;
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
	// Mutable monster table of a staged map (friends only).
	std::vector<XeenActor> &mutableMonsters(XeenMapIdentity mapId) {
		auto *state=_mapStates.find(mapId);
		if (!state || !state->monsters) throw std::out_of_range("Regional actor collection is absent");
		return *state->monsters;
	}
	// Stages a map's complete monster table (MOB records then script slots).
	void stageMonsters(XeenMapIdentity mapId, std::vector<XeenActor> actors, std::size_t originalCount, bool outdoors);
	// Complete flat explored values onto reserved entries; never allocates.
	void publishExploration(const std::vector<XeenExploredTile> &values) noexcept;
	void assignExploration(const std::vector<XeenExploredTile> &values) { reserveTiles(values); publishExploration(values); }
	// Fallible complete replacement of the cell overrides.
	void assignCells(const std::vector<XeenBarrierOverride> &values);
	XeenMapTable _mapStates;
	XeenMutableOptional<XeenHeadCache> _headCache;
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
	XeenMapEntity effectiveObject(XeenObjectIdentity id);
	bool effectiveObjectFlip(XeenObjectIdentity id) const;
	void applyMoveObject(XeenObjectIdentity id,int x,int y);
	void applyMoveWallObject(XeenWallItemIdentity id,int x,int y);
	void applyMakeNothingHere(const XeenCamera &physical,const XeenEventFile &events);
	void applyAlterMap(XeenMapIdentity logical,int x,int y,unsigned direction,unsigned wall);
	void applyAlterCellFlag(XeenMapIdentity logical,int x,int y,unsigned surface);
	void restoreObjectOverrides(const std::vector<XeenObjectOverride> &);
	XeenMapEntity effectiveWallItem(XeenWallItemIdentity);
	// Complete startup-only replacement, bound to the original MOB/EVT records.
	void restoreWallItemOverrides(const std::vector<XeenWallItemOverride> &);
	void restoreEventOverrides(const std::vector<XeenEventOverride> &,const EventLoader &);
	// drawScene resets flip from directional animation metadata on every draw.
	void objectFlipDrawn(XeenObjectIdentity id,bool flipped) {_scenePresentation.objectFlips[id]=flipped;}
	void objectFrameDrawn(XeenObjectIdentity id,unsigned frame) {_scenePresentation.objectFrames[id]=frame;}
	std::optional<unsigned> objectFrame(XeenObjectIdentity id) const {
		const auto found=_scenePresentation.objectFrames.find(id);
		return found==_scenePresentation.objectFrames.end()?std::nullopt:std::optional<unsigned>(found->second);
	}
	XeenHeadIndicators headIndicators(const XeenCamera &);
	void alterHeads(const XeenCamera &,XeenHeadIndicators);
	void reloadHeads(XeenMapIdentity);
	void restoreHeads(const std::optional<XeenHeadCache> &,const XeenCamera &);
    XeenTypedDamageCandidate prepareEventDamage(const XeenPartyState &,unsigned,XeenDamageType,std::optional<std::size_t>,bool) const;
    void publishEventDamagePrefix(XeenPartyState &,const XeenTypedDamageCandidate &,const XeenJourneyRandomState &,bool feedback=false);
	void applyEventDamage(XeenPartyState &,unsigned,XeenDamageType,std::optional<std::size_t>,bool,bool feedback=true);
	void applyEventYear(XeenPartyState &,XeenGameFlags &,unsigned);
	// Script addTime results (characters, inputs, calendar, economy), detached only.
	void publishEventTime(XeenPartyState &,const XeenConditionTimeCandidate &);
	void retainEventRandom(const XeenJourneyRandomState &);
	void publishEventParty(XeenPartyState &,const XeenPartyState &) noexcept;
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
	// The running program's pending treasure, borrowed by each gameplay session
	// (never swapped, saved or copied with session state). A world outside a
	// session, such as a scratch, tool or Event candidate world, uses its own.
	XeenTreasureStagingState &treasureStaging() noexcept { return *_treasureStaging; }
	const XeenTreasureStagingState &treasureStaging() const noexcept { return *_treasureStaging; }
	void borrowTreasureStaging(XeenTreasureStagingState &);
	// A transition candidate stages on a copy taken at a live revision. Callers
	// check that revision before their nonthrowing stores; publishTransition
	// returns the copy only when the detached work changed it.
	void checkTransitionTreasure(const XeenWorld &candidate) const { treasureStaging().check(candidate._transitionTreasureBase); }
	// The staging value the candidate copied: an Event's entry snapshot.
	const XeenTreasureStaging &transitionTreasureSnapshot() const noexcept { return _transitionTreasureSnapshot; }
	bool transitionTreasureChanged() const noexcept {
		return _detachedEventCandidate && _ownTreasureStaging.revision() != _transitionTreasureCopy;
	}
	// Transient developer admission; never serialized or offered by the title.
	std::optional<XeenMapIdentity> developerEventMap() const noexcept { return _developerEventMap; }
	bool eventCameraAdmitted(const XeenCamera &) const noexcept;
	XeenTransitionResult prepareEventTransition(const XeenCamera &,bool runLocation=false);
	XeenTerrainArrival prepareTerrainArrival(const XeenCamera &,const XeenPartyLanes<XeenCharacter> &,const XeenGameplayContext &);
	void publishTerrainArrival(XeenPartyState &,const XeenTerrainArrivalCandidate &,const XeenJourneyRandomState &);
	// Event interpreter reports its own clear before publication; other loads
    // publish a transient input signal with the destination, including reloads.
    void loadEventDestination(XeenMapIdentity from,XeenMapIdentity to,bool clearKeyboard=true);
    std::uint64_t inputMapLoadGeneration() const noexcept {return _inputMapLoadGeneration;}
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
			_sessionState.anyMonsters();
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
	// Detached work/startup only. Live publication belongs to Encounter/Save.
	void setBarrier(const XeenCamera &, std::uint8_t wall, bool unlock);
	void restoreBarriers(const std::vector<XeenBarrierOverride> &);
	// Prepare an occupied-cell update at a stepped boundary, never while drawing.
	std::vector<XeenExploredTile> explorationAfterStep(const XeenPartyState &,const XeenCamera &);
	void recordExploration(const XeenPartyState &,const XeenCamera &);
	void restoreExploredTiles(const std::vector<XeenExploredTile> &);
	std::uint8_t tavernTipIndex(XeenMapIdentity);
	void setTavernTipIndex(XeenMapIdentity,std::uint8_t);
	// Original Map::getCell neighbor queries, for scene observation only.
	// Movement and mechanic admission continue to use sampleCell.
	std::optional<XeenCellSample> sceneCell(XeenMapIdentity mapId, int x, int y);
 // Read-only lighting preparation. The existing gameplay owner publishes the
 // returned counter through its guard before calling the scene composer.
 std::uint16_t lightAfterScenePreparation(const XeenPartyState &,const XeenCamera &);
 bool sceneIsBlack(const XeenPartyState &,const XeenCamera &);
	std::size_t cachedMapCount() const { return _maps.size(); }
	// Unpublished Vertigo Event candidate; callers retain a live owner guard.
	std::unique_ptr<XeenWorld> transitionCandidate() const;
	void copyEventParty(const XeenPartyState &source, XeenPartyState &candidate) const;
	// Stages an indoor map's MOB monster table on first entry.
	void stageIndoorMonsters(const XeenObjectFile &, const std::vector<XeenMonsterRecord> &);
	void stageMapMonsters(const XeenObjectFile &, const std::vector<XeenMonsterRecord> &);
	void applySpawn(XeenMapIdentity mapId, std::uint8_t slot, int x, int y, std::uint8_t unused);
    void generateArena(const std::array<unsigned,5> &,unsigned count);
	void applyAlterEvent(const XeenCamera &physical, std::uint8_t line, std::uint8_t replacement,
		const XeenEventFile &);
	void publishTransition(XeenWorld &candidate) noexcept;
	XeenScenePresentation prepareSpawnPresentation(const XeenWorld &candidate) const;
	XeenActorView prepareTransitionArrival(const XeenCamera &);
	// Map load (decision 8): the destination's live monsters return to MON HP,
	// asleep, before the arrival view wakes those it sees. Detached work only.
	void reloadMapMonsters(XeenMapIdentity);
	// Retain a manual Space suffix in its detached Event owner, before publication.
	void retainEventStep(XeenPartyState &,const XeenRegionalActionCandidate &,const std::vector<XeenActor> &);
	// Map exit (decision 2): a map that is never retained, the Warzone, loses
	// its state and accounting, so re-entry starts fresh. Detached work only.
	void leaveMap(XeenMapIdentity);

private:
	friend class XeenSaveState;
	friend class XeenEventPublication;
	friend class XeenEncounterFlow;
	friend struct XeenTrainingTestAccess;
    friend struct XeenInnTestAccess;
	friend struct XeenMonsterTestAccess;
    friend struct XeenEntranceTestAccess;
	friend class XeenRestoreGuard;
	friend class XeenCombat;
	friend class XeenActorApproach;
	friend void xeenValidateIndoorActors(XeenWorld &, XeenMapIdentity, const std::vector<XeenActor> &);
	void swapPreparedState(XeenWorld &candidate) noexcept;
	// Process-lifetime capability identity. It belongs to this object lifetime,
	// not session gameplay state, and is never serialized or swapped.
	const std::uint64_t _incarnation;
	std::uint64_t _ownerRevision = 0;
	std::uint64_t _cacheRevision = 0;
    std::uint64_t _inputMapLoadGeneration = 0; // Platform input only; never saved.
	XeenGameplayBorrowOwner _gameplayBorrow;
	std::weak_ptr<XeenJourneyCapture> _journeyCapture;
	std::shared_ptr<XeenJourneyRestoration> _journeyRestoration;
	// Combat checks retained authority after fallible resource providers.
	std::function<void()> _combatCheck;
	// Retained combat authorization, separate from domain validity.
	std::function<bool()> _combatAuthorized;
	XeenSessionWorldState _sessionState;
	XeenTreasureStagingState _ownTreasureStaging;
	XeenTreasureStagingState *_treasureStaging = &_ownTreasureStaging;
	std::uint64_t _transitionTreasureBase = 0, _transitionTreasureCopy = 0;
	XeenTreasureStaging _transitionTreasureSnapshot;
	// Immutable loaded MON catalog for mainland, original city and script-created city slots.
	std::vector<XeenMonsterRecord> _cityStatistics;
	// Derived from checked immutable city resources; never gameplay authority.
	bool _detachedEventCandidate = false;
	std::optional<XeenMapIdentity> _developerEventMap;
	// Detached Event presentation only; no additional gameplay slot ownership.
	std::set<XeenMonsterIdentity> _spawnedPresentationSlots;
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
