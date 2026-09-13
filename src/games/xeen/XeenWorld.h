#ifndef MMODERN_GAMES_XEEN_WORLD_H
#define MMODERN_GAMES_XEEN_WORLD_H

#include "games/xeen/XeenMap.h"
#include "games/xeen/XeenRecordIdentity.h"
#include "games/xeen/XeenEventFile.h"
#include "games/xeen/XeenActor.h"
#include "games/xeen/XeenEncounterEntry.h"
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenJourneyContent.h"
#include <stdexcept>
#include <set>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <vector>

namespace mmodern {

enum class XeenEncounterCompletion { None, VictoryEnded, VictoryQuiescent };
enum class XeenCompletedGuard { Operation, Presentation, Integrity, Fatal };
enum class XeenJourneyActivity { Unbound, Quiet, Approach, Attachment, Combat, Presentation, Saving, Failed };
class XeenGameFlags;
class XeenJourneyCapture;
struct XeenJourneyRestoration;
struct XeenCompletedReentry {
	std::uint64_t oldGeneration = 0, newGeneration = 0;
	XeenCamera destination;
	XeenMonsterIdentity defeated{{XeenSide::Clouds, 20}, 5};
};

class XeenCompletedEncounterTicket {
public:
	XeenCompletedEncounterTicket() = default;
private:
	friend class XeenWorld;
	const class XeenWorld *world = nullptr;
	std::uint64_t incarnation = 0, revision = 0;
};

// Runtime-only preimage and owner binding. This is an authorization record, not
// another gameplay owner, and none of it is serialized verbatim.
struct XeenCompletedEncounterAuthority {
	const XeenPartyState *party = nullptr;
	const XeenRoster *roster = nullptr;
	const XeenCamera *camera = nullptr;
	std::array<XeenCharacter, XeenRoster::kCharacterCount> characters{};
	std::array<std::optional<XeenCombatInputs>, XeenRoster::kCharacterCount> combatInputs{};
	std::vector<std::uint8_t> activeRosterIds;
	XeenCloudsQuestItems::Counts questItems{};
	XeenCloudsQuestFlags::Values questFlags{};
	std::optional<XeenGameplayContext> context;
	std::uint8_t firstSerializedCount = 0, effectiveSerializedCount = 0;
	std::vector<std::string> diagnostics;
	XeenCamera cameraValue;
	std::vector<XeenActor> actors;
	std::set<XeenObjectIdentity> objects;
	std::set<XeenEventIdentity> events;
	XeenMonsterIdentity monster{{XeenSide::Clouds, 20}, 5};
};

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
	bool journey() const noexcept { return _entry == XeenEncounterEntry::Journey; }
	XeenJourneyActivity journeyActivity() const noexcept { return _journeyActivity; }
	std::uint32_t skeletonSeed() const noexcept { return _skeletonSeed; }
	std::uint16_t journeyContract() const noexcept { return _journeyContract; }
	const std::optional<XeenJourneyRandomState> &journeyRandom() const noexcept { return _journeyRandom; }
	const std::set<XeenMonsterIdentity> &accountedMonsters() const noexcept { return _accountedMonsters; }
	bool isObjectDisabled(XeenObjectIdentity id) const { return _objects.count(id) != 0; }
	bool isEventDisabled(XeenEventIdentity id) const { return _events.count(id) != 0; }
	std::size_t disabledObjectCount() const { return _objects.size(); }
	std::size_t disabledEventCount() const { return _events.size(); }
	const std::set<XeenObjectIdentity> &disabledObjects() const { return _objects; }
	const std::set<XeenEventIdentity> &disabledEvents() const { return _events; }
	bool encounterMarked() const { return _encounterMarked; }
	XeenEncounterEntry encounterEntry() const noexcept { return _entry; }
	bool encounterInitialized() const { return _encounterInitialized; }
	bool encounterTerminal() const { return _encounterTerminal; }
	XeenEncounterCompletion completion() const noexcept { return _completion; }
	bool combatAccounted() const noexcept { return _combatAccounted; }
	XeenMonsterIdentity completedMonster() const noexcept { return _completedMonster; }
	const std::vector<XeenActor> &actors() const { return _actors; }
private:
	friend class XeenEncounterFlow;
	XeenJourneyActivity _journeyActivity = XeenJourneyActivity::Unbound;
	const void *_journeyOwner = nullptr;
	std::uint64_t _journeyGeneration = 0;
	std::uint32_t _skeletonSeed = 0;
	std::uint16_t _journeyContract = 1;
	std::optional<XeenJourneyRandomState> _journeyRandom;
	std::set<XeenMonsterIdentity> _accountedMonsters;
	friend class XeenWorld;
	friend class XeenActorApproach;
	friend class XeenCombat;
	friend class XeenSaveState;
	friend class XeenRestoreGuard;
	const void *_combatOwner = nullptr;
	const void *_combatApproachState = nullptr;
	bool _diagnostic27 = false, _combatEntered = false, _combatAccounted = false;
	XeenEncounterCompletion _completion = XeenEncounterCompletion::None;
	XeenMonsterIdentity _completedMonster{{XeenSide::Clouds, 20}, 5};
	std::optional<XeenCompletedEncounterAuthority> _completedAuthority;
	bool _completedPublished = false;
	std::uint64_t _completedEntryGeneration = 0;
	mutable bool _completedIntegrityUnsafe = false, _completedFatal = false;
	mutable std::uint64_t _completedLease = 0;
	mutable std::optional<XeenCompletedGuard> _completedLeaseKind;
	XeenEncounterEntry _entry = XeenEncounterEntry::Ordinary;
	bool _encounterMarked = false, _encounterInitialized = false, _encounterTerminal = false;
	mutable std::uint64_t _encounterRevision = 0;
	std::vector<XeenActor> _actors;
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
	bool isObjectDisabled(XeenObjectIdentity id);
	bool isEventDisabled(XeenEventIdentity id) const { return _sessionState.isEventDisabled(id); }
	std::optional<XeenObjectIdentity> selectObject(const XeenCamera &camera);
	XeenEventRecord effectiveEvent(XeenEventIdentity id, const XeenEventRecord &base) const;
	void disableObject(XeenObjectIdentity id);
	void disableEventsAtCell(const XeenCamera &physical, const XeenEventFile &events);
	void applyRemove(const XeenCamera &physical, std::optional<XeenObjectIdentity> selected,
		const XeenEventFile &events);
	std::size_t cachedObjectFileCount() const { return _objects.size(); }
	XeenWorld(const XeenWorld &) = delete;
	XeenWorld &operator=(const XeenWorld &) = delete;
	const XeenSessionWorldState &sessionState() const { return _sessionState; }
	// Irreversible safety marker, including failed preparation. No clear/reset API.
	void markEncounterSession() noexcept { _sessionState._encounterMarked = true; }
	void markEncounterSession(XeenEncounterEntry entry) {
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
	bool completedCaptureEligible(const XeenPartyState &, const XeenCamera &) const noexcept;
	bool journeyCaptureEligible(const XeenPartyState &, const XeenCamera &) const noexcept;
	XeenCompletedEncounterTicket completedTicket(const XeenPartyState &, const XeenCamera &) const noexcept;
	bool completedTicketCurrent(const XeenCompletedEncounterTicket &, const XeenPartyState &, const XeenCamera &) const noexcept;
	// Checks an already-held capability; never grants capture or a new ticket.
	bool completedGuardCurrent(const XeenCompletedEncounterTicket &, XeenCompletedGuard, std::uint64_t,
		const XeenPartyState &, const XeenCamera &) const noexcept;
	std::uint64_t holdCompletedGuard(const XeenCompletedEncounterTicket &, XeenCompletedGuard,
		const XeenPartyState &, const XeenCamera &);
	bool releaseCompletedGuard(const XeenCompletedEncounterTicket &, XeenCompletedGuard, std::uint64_t) noexcept;
	bool latchCompletedGuard(const XeenCompletedEncounterTicket &, XeenCompletedGuard,
		const XeenPartyState &, const XeenCamera &) noexcept;
	bool escalateCompletedGuard(const XeenCompletedEncounterTicket &, XeenCompletedGuard,
		std::uint64_t, XeenCompletedGuard) noexcept;
	using MonsterLoader = std::function<std::vector<XeenMonsterRecord>()>;
	using CompletedPreflight = std::function<void(XeenWorld &, const XeenPartyState &,
		const XeenCamera &, const XeenGameFlags &)>;
	std::uint64_t completedEntryGeneration() const noexcept { return _sessionState._completedEntryGeneration; }
	// The optional UI check runs inside the retained graph guard. A failed
	// operation returns renewal authority only after its own checked lease release.
	XeenCompletedReentry reenterCompletedEncounter(const XeenCompletedEncounterTicket &,
		XeenPartyState &, XeenCamera &, const XeenGameFlags &, const MonsterLoader &,
		const EventLoader &, const CompletedPreflight &,
		std::optional<XeenCompletedEncounterTicket> *releasedOnFailure = nullptr,
		const std::function<void()> &checkBoundary = {});
	// For unpublished startup owners only. Validates every original identity
	// before replacing either set; no script execution or cell expansion.
	void restoreSessionState(const std::vector<XeenObjectIdentity> &objects,
		const std::vector<XeenEventIdentity> &events, const EventLoader &eventLoader);
	// Invalidates map/cell/object-file references, not the session state.
	void discardMapCache() { _maps.clear(); _objects.clear(); ++_cacheRevision; }

	const XeenMap &map(XeenMapIdentity mapId);
	std::optional<XeenCellSample> sampleCell(XeenMapIdentity mapId, int x, int y);
	std::size_t cachedMapCount() const { return _maps.size(); }

private:
	friend class XeenSaveState;
	friend class XeenEncounterFlow;
	friend class XeenRestoreGuard;
	friend class XeenCombat;
	friend class XeenActorApproach;
	bool completedFactsCurrent(const XeenPartyState &, const XeenCamera &) const noexcept;
	void swapPreparedState(XeenWorld &candidate) noexcept;
	// Process-lifetime capability identity. It belongs to this object lifetime,
	// not session gameplay state, and is never serialized or swapped.
	const std::uint64_t _incarnation;
	std::uint64_t _ownerRevision = 0;
	std::uint64_t _cacheRevision = 0;
	XeenGameplayBorrowOwner _gameplayBorrow;
	std::weak_ptr<XeenJourneyCapture> _journeyCapture;
	std::shared_ptr<XeenJourneyRestoration> _journeyRestoration;
	// Diagnostic27 checks retained authority after fallible resource providers.
	std::function<void()> _combatCheck;
	// Retained Diagnostic27 authorization, separate from domain validity.
	std::function<bool()> _combatAuthorized;
	XeenSessionWorldState _sessionState;
	MapLoader _loader;
	ObjectLoader _objectLoader;
	std::map<XeenMapIdentity, XeenObjectFile> _objects;
	void validateObject(XeenObjectIdentity id);
	void validateEventCell(const XeenCamera &physical, const XeenEventFile &events);
	std::map<XeenMapIdentity, XeenMap> _maps;
};

} // namespace mmodern

#endif
