#ifndef MMODERN_XEEN_ACTOR_APPROACH_H
#define MMODERN_XEEN_ACTOR_APPROACH_H
#include "games/xeen/XeenActor.h"
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenWorld.h"
#include <array>
#include <functional>

namespace mmodern {
class XeenAssetSource;
struct XeenRegionalActionCandidate;
struct XeenRegionalObservation;
enum class XeenActorPlacement { SameCell, Forward, ForwardLeft, ForwardRight, Other };
struct XeenActorView {
	std::array<XeenMutableOptional<XeenMonsterIdentity>, 26> slots{};
	std::array<bool, 107> activation{};
	std::array<std::optional<XeenActorPlacement>, 107> placements{};
	bool engaged() const { return slots[0] || slots[1] || slots[2]; }
};
enum class XeenMonsterTerrain { Allowed, Blocked, Unsupported };
// MODE_SLEEPING permits an opportunity without permanently activating actors.
struct XeenActorOpportunityContext {
	bool sleeping=false, movementEnabled=true, charactersShooting=false;
};
enum class XeenEncounterAction { Forward, Backward, Left, Right, Wait, Unsupported, StrafeLeft, StrafeRight };
enum class XeenEncounterPhase { Exploring, Engaged, SupportStopped };
enum class XeenEncounterOutcome { Started, Accepted, Blocked, Pulsed, Pending, Engaged, Refused, Stale, Terminal, Stopped };
enum class XeenEncounterStop { None, Envelope, Time, Domain, Preparation, Reporting, Overflow, Ranged, RegionalContact, Defeat };

// Transient coordination value for 26B. Copies are observations; revisions reject replay.
// No clock, scheduler, callbacks or owner instances live here.
class XeenEncounterState {
public:
	unsigned pending() const { return _pending; }
	std::uint64_t revision() const { return _revision; }
	XeenEncounterPhase phase() const { return _phase; }
	XeenEncounterStop reason() const { return _reason; }
private:
	friend class XeenActorApproach;
	friend class XeenCombat;
	friend class XeenEncounterFlow;
	friend struct XeenRestTestAccess;
	const XeenWorld *_world = nullptr;
	const XeenPartyState *_party = nullptr;
	const XeenCamera *_camera = nullptr;
	std::uint64_t _revision = 0;
	unsigned _pending = 0;
	XeenEncounterPhase _phase = XeenEncounterPhase::Exploring;
	XeenEncounterStop _reason = XeenEncounterStop::None;
};
struct XeenEncounterResult {
	XeenEncounterOutcome outcome = XeenEncounterOutcome::Refused;
	XeenEncounterStop reason = XeenEncounterStop::None;
	std::uint64_t revision = 0;
	unsigned movementOpportunities = 0;
	bool automaticEvent = false, needsRest=false;
	XeenMutableOptional<XeenMonsterIdentity> stoppedActor;
	int stoppedX = 0, stoppedY = 0;
	XeenActorView view;
	std::shared_ptr<const XeenRegionalObservation> consequences;
};

class XeenActorApproach {
public:
	// Internal fresh-domain initialization. The caller retains destination/provider guards.
	static XeenEncounterResult initializeJourney(XeenWorld &, XeenPartyState &, XeenCamera &,
		XeenEncounterState &, const std::vector<std::uint8_t> &, const XeenGameplayContext &,
		const std::vector<XeenMonsterRecord> &, const XeenEventFile &, std::uint32_t seed);
	static XeenEncounterResult initializeJourney(XeenWorld &, XeenPartyState &, XeenCamera &,
		XeenEncounterState &, const std::vector<std::uint8_t> &, const XeenGameplayContext &,
		const std::vector<XeenMonsterRecord> &, const XeenEventFile &, std::uint32_t,
		const std::optional<XeenMonsterTreasure> &);
	static XeenEncounterResult initializeJourney(XeenWorld &, XeenPartyState &, XeenCamera &,
		XeenEncounterState &, const std::vector<std::uint8_t> &, const XeenGameplayContext &,
		const std::vector<XeenMonsterRecord> &, const XeenEventFile &, std::uint32_t,
		const std::optional<XeenMonsterTreasure> &, const std::optional<XeenBankBalances> &);
	static constexpr std::size_t kCapacity = 107;
	// Read-only authorization, including terminal states. Never adopts a revision.
	static bool authoritative(const XeenWorld &, const XeenPartyState &, const XeenCamera &,
		const XeenEncounterState &) noexcept;
	using Terrain = std::function<XeenMonsterTerrain(const XeenActor &, int, int)>;
	using BeforeMovement = std::function<void(const std::vector<XeenActor> &, std::size_t)>;
	static std::vector<XeenActor> actorsFromResources(const XeenObjectFile &mob,
		const std::vector<XeenMonsterRecord> &statistics);
	static XeenActorView classify(const std::vector<XeenActor> &actors, const XeenCamera &camera);
	static std::array<unsigned, 1024> occupancy(const std::vector<XeenActor> &actors);
	// Pure preparation. Callback only queries terrain; failure publishes nothing.
	static std::vector<XeenActor> move(const std::vector<XeenActor> &actors,
		const XeenCamera &camera, const Terrain &terrain, bool movementEnabled = true);
	static std::vector<XeenActor> move(const std::vector<XeenActor> &actors,
		const XeenCamera &camera, const Terrain &terrain, bool movementEnabled,
		const BeforeMovement &beforeMovement);
	static std::vector<XeenActor> move(const std::vector<XeenActor> &actors,
		const XeenCamera &camera, const Terrain &terrain, bool movementEnabled,
		const BeforeMovement &beforeMovement, XeenActorOpportunityContext context);
	static void validateDomain(XeenWorld &world, const XeenPartyState &party,
		const XeenGameplayContext &context, const std::vector<XeenActor> &actors,
		const XeenEventFile &events);
	// Immutable environment admission, independent of the party's injury state.
	static void validateEnvironment(XeenWorld &world, const std::vector<XeenActor> &actors,
		const XeenEventFile &events, const std::vector<XeenMonsterRecord> *statistics = nullptr);
	// Explicit startup only; leaves an irreversible marker on preparation failure.
	// Reads context/statistics explicitly; ordinary party loading remains unchanged.
	// Actions do NOT supply a pulse. Caller supplies exactly one post-action pulse,
	// observing intermediate count 3 before pulse 3->2; later pulses finish old work.
	static XeenEncounterResult action(XeenWorld &world, XeenPartyState &party, XeenCamera &camera,
		XeenEncounterState &state, XeenEncounterAction action, const XeenEventFile &events);
	static XeenEncounterResult pulse(XeenWorld &world, XeenPartyState &party, XeenCamera &camera,
		XeenEncounterState &state, const XeenEventFile &events);
	static XeenEncounterResult stop(XeenWorld &world, XeenEncounterState &state,
		XeenEncounterStop reason) noexcept;
private:
	static std::vector<XeenActor> moveWithContext(const std::vector<XeenActor> &,const XeenCamera &,
		const Terrain &,bool,const BeforeMovement &,XeenActorOpportunityContext);
	friend class XeenEncounterFlow;
	// Flow prepares final-owner guard/capture storage from detached values.
	using FreshPublicationPreparation = std::function<void(const XeenPartyState &,
		const std::vector<XeenActor> &, const std::optional<XeenJourneyRandomState> &)>;
	using StartPublicationPreparation = std::function<void(const XeenPartyState &,
		const std::vector<XeenActor> &, const std::optional<XeenJourneyRandomState> &,
		const std::optional<std::vector<XeenActor>> &, const std::vector<XeenMonsterRecord> &)>;
	static XeenEncounterResult initializeStart(XeenWorld &, XeenPartyState &, XeenCamera &,
		XeenEncounterState &, const std::vector<std::uint8_t> &, const XeenGameplayContext &,
		const std::vector<XeenMonsterRecord> &, const XeenEventFile &, const XeenEventFile &,
		std::uint32_t, const std::optional<XeenMonsterTreasure> &, const std::optional<XeenBankBalances> &,
		bool prepared, const std::optional<XeenRegionalRecoveryState> &, const StartPublicationPreparation &);
	static XeenEncounterResult initializeJourney(XeenWorld &, XeenPartyState &, XeenCamera &,
		XeenEncounterState &, const std::vector<std::uint8_t> &, const XeenGameplayContext &,
		const std::vector<XeenMonsterRecord> &, const XeenEventFile &, std::uint32_t,
		const std::optional<XeenMonsterTreasure> &, const std::optional<XeenBankBalances> &,
		const FreshPublicationPreparation &);
	static XeenEncounterResult regionalTransition(XeenWorld &, XeenPartyState &, XeenCamera &, XeenEncounterState &,
		const XeenEventFile &, std::optional<XeenEncounterAction>, std::unique_ptr<XeenRegionalActionCandidate> &);
	static XeenEncounterResult transition(XeenWorld &, XeenPartyState &, XeenCamera &,
		XeenEncounterState &, XeenEncounterAction, const XeenEventFile &, bool pulse);
};
}
#endif
