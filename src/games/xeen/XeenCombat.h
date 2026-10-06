#ifndef MMODERN_XEEN_COMBAT_H
#define MMODERN_XEEN_COMBAT_H
#include "core/NavigationAction.h"
#include <stdexcept>
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenEquipment.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenItemTransfer.h"
#include <memory>

namespace mmodern {
// Retained headless boundary authority. A later Flow binds its real pending work
// to these leases; it cannot assert that an unrelated owner graph is quiescent.
class XeenCombatBoundary {
public:
	enum class Work { Inventory, Certificate, Event, Reward, ItemUse, Casting, PresentationFailure, Service };
	XeenCombatBoundary(const XeenWorld &w,const XeenPartyState &p,const XeenCamera &c):world(&w),party(&p),camera(&c) {}
	XeenCombatBoundary(const XeenCombatBoundary &)=delete;
	XeenCombatBoundary &operator=(const XeenCombatBoundary &)=delete;
	std::uint64_t hold(Work);
	void release(Work,std::uint64_t);
	std::uint64_t generation() const noexcept { return epoch; }
	bool holds(Work work, std::uint64_t lease) const noexcept { return lease && leases[static_cast<unsigned>(work)]==lease; }
	bool only(Work, std::uint64_t lease) const noexcept;
	bool quiet() const noexcept;
	bool preparationReady() const noexcept;
private:
	friend class XeenCombat;
	friend struct XeenTrainingTestAccess;
	friend struct XeenPurchaseTestAccess;
	const XeenWorld *world; const XeenPartyState *party; const XeenCamera *camera;
	std::uint64_t epoch=0;
	std::array<std::uint64_t,8> leases{};
};

// A value cursor: cloning never shares mutable position/state. The immutable tape
// is diagnostic input; no history is appended by retained preparation.
class XeenCombatRandom {
public:
	struct Draw { std::uint32_t lo,hi,value; bool raw=false; };
	explicit XeenCombatRandom(std::uint32_t seed=1);
	explicit XeenCombatRandom(std::vector<Draw> tape);
	explicit XeenCombatRandom(XeenJourneyRandomState);
	XeenJourneyRandomState continuation() const;
	// One provider/raw draw, including a rejected conversion. nullopt means reject.
	std::optional<std::uint32_t> draw(std::uint32_t lo,std::uint32_t hi);
	std::uint32_t state() const noexcept { return value; }
	std::uint64_t position() const noexcept { return offset; }
private:
	std::uint32_t value=1;
	std::uint64_t offset=0;
	std::shared_ptr<const std::vector<Draw>> tape;
};

enum class XeenCombatPhase { Engaged, PlayerReady, Casting, PreparingAction,
	PendingEnemy, PendingRound, DisengagementPending, Disengaged, VictoryAwaitingEnd, Victory, Defeat, SupportStopped, Failed };
enum class XeenCombatWork { None, Action, Enemy, Round, End, FinishDisengagement, Cast, Movement };
enum class XeenCombatCommand { Attack, Block, Run };
enum class XeenCombatStatus { Accepted, Pending, Advanced, Refused, Stale, Failed, SupportStopped, Victory, Defeat };
enum class XeenCombatFailure { None, Integrity, Preparation, Time, Observation, Overflow };
enum class XeenCombatOperation { None, BeginCombat, PlayerAttack, Cast, Block, PlayerRun, FinishDisengagement, EnemyAttack, Round, End, Failure, Rotate, Movement };
// Interface::_tillMove, separate from combat's already owed round movement.
// One qualifying normalized draw beat; suspended modes never accumulate debt.
class XeenMovementCountdown {
public:
    void arm(unsigned count=3) {
        if(count>3)throw std::invalid_argument("Invalid movement countdown");
        value=count;beat.reset();
    }
    unsigned remaining() const noexcept {return value;}
    void clear() noexcept {value=0;beat.reset();}
    bool advance(std::uint64_t now,bool enabled,bool monstersAttacking) noexcept {
        if(!value || !enabled || monstersAttacking || (beat && (now<*beat || now-*beat<100)))return false;
        beat=now;return --value==0;
    }
private:
    unsigned value=0;
    std::optional<std::uint64_t> beat;
};
enum class XeenCombatAttackOutcome { NotApplicable, NoParticipants, Pending, Miss, HitZeroDamage, HitPositiveDamage };
enum class XeenCombatExitCause { None, DirectRun, AttritionAfterEscape };
struct XeenCombatLocation { XeenMapIdentity mapId; int x=0,y=0; XeenDirection direction=XeenDirection::North; };
struct XeenCombatDamage {
	std::uint8_t owner=0;
	unsigned attackOrdinal=0;
	int amount=0, beforeHp=0, afterHp=0, beforeAc=0, afterAc=0;
	std::array<std::uint8_t,16> conditions{};
};
struct XeenCombatArmorChange { std::uint8_t owner=0,slot=0; XeenItem before,after; };
struct XeenCombatXp { std::uint8_t owner=0; std::uint32_t before=0,after=0; };
struct XeenCombatResult {
	// Owned observations only. Pending is not a published attack outcome.
	XeenCombatOperation operation=XeenCombatOperation::None;
	XeenCombatAttackOutcome attackOutcome=XeenCombatAttackOutcome::NotApplicable;
	std::optional<std::uint8_t> actingOwner, targetOwner;
	XeenMutableOptional<XeenMonsterIdentity> actingMonster, targetMonster;
	std::optional<XeenEncounterAction> approachAction;
	bool critical=false, needsRest=false;
	unsigned runRoll=0;
	bool runSuccess=false;
	std::uint8_t participantsBefore=0x3f, participantsAfter=0x3f, casualties=0;
	XeenCombatExitCause exitCause=XeenCombatExitCause::None;
	XeenMutableOptional<XeenCombatLocation> origin, destination;
	std::uint32_t forfeitedGold=0, forfeitedMask=0;
	bool originAutomaticSuperseded=false;
	std::uint8_t targetedMembers=0; // Published physical target mask, including misses.
    std::uint8_t blockedMembers=0; // Detached current Block observation.
	std::optional<XeenMonsterDropOutcome> monsterDrop;
	XeenMonsterTreasureItem generatedItem;
	bool generatedArmor=false;
	XeenCombatStatus status=XeenCombatStatus::Refused;
	XeenCombatPhase phase=XeenCombatPhase::Engaged;
	XeenCombatFailure failure=XeenCombatFailure::None;
	XeenCombatWork work=XeenCombatWork::None;
	std::uint64_t oldRevision=0,revision=0,generation=0;
	int participant=-1, actorHpBefore=0,actorHpAfter=0, damage=0;
	XeenMonsterIdentity monster{20,5};
	std::uint16_t minutes=480;
	std::array<XeenCombatDamage,12> injuries{}; unsigned injuryCount=0;
	// Prepared immutable overflow retains noexcept observation publication.
	std::shared_ptr<const std::vector<XeenCombatDamage>> additionalInjuries;
	const XeenCombatDamage &injury(unsigned i) const {
		return i<injuries.size() ? injuries.at(i) : additionalInjuries->at(i-injuries.size());
	}
	std::array<XeenCombatArmorChange,54> armor{}; unsigned armorCount=0;
	std::array<XeenCombatXp,6> xp{}; unsigned xpCount=0;
	std::shared_ptr<const XeenRegionalObservation> ranged;
};

enum class XeenCombatCastPhase { Learned, Enemy, Confirm, PartyTarget, Preparing, Projectile, Impact, PostImpact, Result };
enum class XeenCombatCastInput { Up, Down, Enter, Escape, PartyTarget, EnemyTarget };
struct XeenCombatCastResult {
    unsigned spell=0, count=0;
    int spBefore=0, spAfter=0;
    bool refunded=false, failed=false, resisted=false, noop=true;
    struct Effect {
        std::uint8_t owner=0;
        int beforeHp=0,afterHp=0;
        std::array<std::uint8_t,16> before{},after{};
    };
    std::array<Effect,6> effects{};
};
struct XeenCombatCastView {
    XeenCombatCastPhase phase=XeenCombatCastPhase::Learned;
    unsigned participant=0,owner=0,slot=0;
    std::optional<XeenMonsterIdentity> enemy;
    bool committed=false;
    XeenCombatPhase successorPhase=XeenCombatPhase::Failed;
    XeenCombatWork successorWork=XeenCombatWork::None;
    int successorParticipant=-1;
    std::string refusal;
    XeenCombatCastResult result;
};
class XeenRestoreGuard;
class XeenCombat {
public:
	class Ticket {
		friend class XeenCombat;
		const XeenCombat *owner=nullptr;
		std::uint64_t incarnation=0,generation=0,revision=0,boundary=0;
		XeenCombatPhase phase=XeenCombatPhase::Failed;
		XeenCombatWork work=XeenCombatWork::None;
	};
	~XeenCombat();
	XeenCombat(const XeenCombat &)=delete;
	XeenCombat &operator=(const XeenCombat &)=delete;
	Ticket ticket() const noexcept;
	bool current(const Ticket &) const noexcept;
	bool boundTo(const XeenWorld &, const XeenPartyState &, const XeenCamera &, const XeenCombatBoundary &) const noexcept;
	// Detached observations. No retained selection, cursor or result memory escapes.
	std::optional<XeenCombatCastView> cast() const;
	XeenCombatResult result() const noexcept;
	const XeenEncounterState &approachState() const noexcept;
	XeenCombatPhase phase() const noexcept;
	XeenCombatWork pending() const noexcept;
	int participant() const noexcept;
	std::uint8_t participants() const noexcept;
	XeenCombatExitCause exitCause() const noexcept;
	XeenCombatRandom random() const noexcept;
	XeenCombatResult beginCombat(const Ticket &);
	XeenCombatResult command(const Ticket &,XeenCombatCommand);
	XeenCombatResult selectTarget(const Ticket &, unsigned row);
    XeenCombatResult rotate(const Ticket &,NavigationAction);
    unsigned movementCountdown() const noexcept;
    bool stepped() const noexcept;
	std::array<std::optional<XeenMonsterIdentity>,3> contacts() const noexcept;
	std::optional<XeenMonsterIdentity> selectedTarget() const noexcept;
	XeenCombatResult service(const Ticket &);
	XeenCombatResult fail(const Ticket &,XeenCombatFailure=XeenCombatFailure::Observation) noexcept;
	// Single-writer replacement notification, including byte-identical ABA.
	void invalidate() noexcept;
	// Optional fallible observation/probe seam. Called after each draw or before
	// publication and checked immediately. An observer never receives a capability.
	void setProbe(std::function<void()>);
	void preparePresentation(const Ticket &, const std::function<void()> &);
private:
	friend class XeenEncounterFlow;
    friend struct XeenCombatRotationTestAccess;
    XeenMovementCountdown &countdownForScheduling() noexcept;
    XeenCombatResult drawBeat(const Ticket &,std::uint64_t);
	bool ticketCurrent(const Ticket &) const noexcept;
	void guardCallback(const Ticket &, const std::function<void()> &);
    // Only the coordinator can mint this consumed concrete-frame response.
    class CastResponse {
        friend class XeenEncounterFlow;
        friend class XeenCombat;
        Ticket source;
        bool consumed=false;
        explicit CastResponse(const Ticket &t):source(t) {}
        CastResponse(const CastResponse &)=delete;
    };
    XeenCombatResult beginCast(CastResponse &, const std::function<XeenLearnedSpellNames()> &);
    XeenCombatResult respondCast(CastResponse &, XeenCombatCastInput, unsigned,
        const std::function<XeenLearnedSpellNames()> &);
    XeenCombatResult serviceCast(const Ticket &, std::uint64_t);
    void castPresented(const Ticket &, std::uint64_t);
    std::optional<XeenActor> castImpactSnapshot() const;
    void rangedPresented(const Ticket &, bool travelComplete);
    bool consumeCast(CastResponse &);
    XeenCombatResult settleCast(const Ticket &, std::optional<unsigned>);
	XeenCombat(XeenWorld &, XeenPartyState &, XeenCamera &, XeenCombatBoundary &, const XeenGameFlags &,
		const XeenEncounterState &, const std::vector<XeenMonsterRecord> &, const XeenEventFile &);
	void retireJourney(const Ticket &, XeenEncounterState &);
	void retireDisengagedJourney(const Ticket &, XeenEncounterState &);
	XeenCombatResult finishDisengagement(const Ticket &);
	void retainResources(XeenRestoreGuard &) const;
    void inheritResources(const Ticket &, const XeenRestoreGuard &);
	struct Impl;
	std::unique_ptr<Impl> impl;
	XeenCombatResult serviceConsequences(const Ticket &);
};
}
#endif
