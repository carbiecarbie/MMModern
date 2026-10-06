#ifndef MMODERN_APP_XEEN_ENCOUNTER_FLOW_H
#define MMODERN_APP_XEEN_ENCOUNTER_FLOW_H

#include "core/PlayerAction.h"
#include "formats/xeen/XeenMonsterAppearance.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenCombat.h"
#include "games/xeen/XeenEventPresenter.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenAntidoteUse.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenArmorRepair.h"
#include "games/xeen/XeenServiceDay.h"
#include "games/xeen/XeenTraining.h"
#include "games/xeen/XeenTempleHeal.h"
#include "games/xeen/XeenEquipmentPurchase.h"
#include "games/xeen/XeenRestRules.h"

namespace mmodern {
struct XeenBarrierCandidate;
class XeenItemCatalog;

// Borrowed original values for one fresh Journey initialization, never restoration.
struct XeenJourneySetup {
	const std::vector<std::uint8_t> &characters;
	XeenGameplayContext context;
	const std::vector<XeenMonsterRecord> &statistics;
	const XeenEventFile &events;
	std::uint32_t seed;
	XeenRegionalManifest regionalManifest;
	std::optional<XeenMonsterTreasure> purse;
	std::optional<XeenRegionalRecoveryState> regionalRecovery;
	std::optional<XeenEventTextFile> regionalText;
	std::optional<XeenLearnedSpellNames> learnedNames;
	std::function<XeenLearnedSpellNames()> learnedNamesProvider;
	std::optional<XeenBankBalances> bank;
	std::function<XeenEventFile()> cityEventsProvider;
};
struct XeenJourneyRestoreTag {};

// Bounded coordinator. World/party/camera and the normalized clock remain borrowed.
class XeenEncounterFlow {
public:
	struct Ticket { XeenEncounterState state; std::uint64_t generation; std::optional<XeenCombat::Ticket> combat;
		std::uint64_t boundaryGeneration = 0; };
	XeenEncounterFlow(XeenWorld &, XeenPartyState &, XeenCamera &, const XeenGameFlags &,
		const XeenEventPresenter::Clock &, const XeenJourneySetup &);
	XeenEncounterFlow(XeenWorld &, XeenPartyState &, XeenCamera &, const XeenGameFlags &,
		const XeenEventPresenter::Clock &, XeenJourneyRestoreTag);
	~XeenEncounterFlow();
	bool journeyQuiet() const noexcept;
	bool journeyMutable() const noexcept;
	std::uint64_t holdJourneyWork(XeenCombatBoundary::Work);
	void releaseJourneyWork(XeenCombatBoundary::Work, std::uint64_t);
	void holdJourneyFrame();
	void journeyRead(const std::function<void()> &);
	std::string journeyInspection() const;
	bool journey() const noexcept { return _journey; }
	Ticket beginJourneySave();
	bool journeySaveCurrent(const Ticket &) const noexcept;
	bool endJourneySave(const Ticket &) noexcept;
	XeenRestoreGuard &journeySavePreimage() { return *_journeyPreimage; }
	std::shared_ptr<XeenRestoreGuard> retainSavePreimage() const { return _journeyPreimage; }
	const std::string &journeyRefusal() const noexcept { return _journeyRefusal; }
	XeenEncounterResult journeyAction(const Ticket &, XeenEncounterAction);
	XeenEncounterResult journeyPulse(const Ticket &);
	XeenEquipmentResult journeyEquipment(const Ticket &, std::size_t, XeenInventoryCategory, std::size_t, XeenEquipmentOperation);
	XeenTransferResult journeyTransfer(const Ticket &, std::size_t, std::size_t, XeenInventoryCategory, std::size_t);
	struct ItemUseSelection {
		std::uint64_t epoch=0;
		std::array<std::uint8_t,XeenParty::kMaximumVisibleMembers> membership{};
		std::size_t membershipSize=0,sourceIndex=0,slot=0;
		std::uint8_t sourceOwner=0;
		XeenInventoryCategory category=XeenInventoryCategory::Weapons;
		XeenItem record{};
	};
	std::optional<std::uint64_t> beginItemUse(const Ticket &, const ItemUseSelection &, std::uint64_t inventoryLease, std::uint64_t certificateLease);
	bool finishItemUse(const Ticket &, std::uint64_t generation, std::uint64_t inventoryEpoch,
		std::optional<std::size_t> targetIndex, std::uint64_t displayedInput,
		const IndexedFrame::Presentation &selectorFrame);
	bool itemUseActive() const noexcept { return bool(_itemUse); }
	bool castingActive() const noexcept { return bool(_casting); }
	bool castingSettlement() const noexcept { return _castingSettlement; }
	bool castingCommitted() const noexcept { return _casting && _casting->committed; }
	const std::string &castingResult() const noexcept { return _castingResult; }
	bool itemUseReady() const noexcept;
	const std::optional<XeenAntidoteResult> &itemUseResult() const noexcept { return _itemUseResult; }
	bool attachJourney(const Ticket &, const std::function<void()> &prepareSprites);
	bool retireJourney(const Ticket &);
	bool prepareJourneyFrame(const Ticket &, const std::function<void()> &compose, bool cosmetic = false);
	bool presentJourney(const Ticket &);
	XeenEncounterFlow(const XeenEncounterFlow &) = delete;
	XeenEncounterFlow &operator=(const XeenEncounterFlow &) = delete;
	Ticket ticket() const noexcept { return {state(), _generation, _combat ? std::optional<XeenCombat::Ticket>{_combat->ticket()} : std::nullopt, _boundary.generation()}; }
	bool canSave() const noexcept { return journeyQuiet(); }
	XeenCombat *combat() noexcept { return _combat.get(); }
	const XeenCombat *combat() const noexcept { return _combat.get(); }
	// Fixed observations for downstream presentation, never continuation authority.
	const XeenCombatResult &combatObservation() const noexcept { return _combatObservation; }
	const XeenCombatResult &combatAward() const noexcept { return _combatAward; }
	XeenCombatResult combatResult() const noexcept { return _combat ? _combat->result() : _retiredCombatResult; }
	XeenCombatBoundary &boundary() noexcept { return _boundary; }
	bool terminal() const noexcept;
	bool combatOperationStale() const noexcept { return _combatOperationStale; }
	void presented(const Ticket &);
	bool current(const Ticket &) const noexcept;
	bool handle(const PlayerAction &, std::optional<std::uint64_t> cycle = {}, std::optional<XeenCombat::Ticket> displayed = {});
	bool idle(std::optional<std::uint64_t> cycle = {});
	bool fail(const Ticket &, XeenEncounterStop = XeenEncounterStop::Reporting) noexcept;
	const XeenEncounterState &state() const noexcept { return _combat ? _combat->approachState() : _state; }
	const XeenEncounterResult &result() const noexcept { return _result; }
	const XeenEncounterResult &actionResult() const noexcept { return _actionResult; }
	unsigned actionPending() const noexcept { return _actionPending; }
	std::uint8_t frame() const noexcept { return _frame; }
	XeenMonsterAppearance appearance() const {
		auto result = _frame < 8 ? XeenMonsterAppearance{_frame} :
			XeenMonsterAppearance{XeenMonsterSpriteKind::Attack, static_cast<std::uint8_t>(_frame - 8)};
		if (_frame >= 8) result.identity = _appearanceIdentity;
        if(_combat && _combat->cast() && _castProjectile && _castProjectile->active)
            result.projectiles.push_back(*_castProjectile);
        for(const auto &p:_projectiles) if(p.active) result.projectiles.push_back(p);
        if(_combat)result.impactSnapshot=_combat->castImpactSnapshot();
        if(_shoot && _shoot->stage==XeenShootCandidate::Stage::PostImpact)result.impactSnapshot=_shoot->bound;
        if(!result.projectiles.empty()) result.projectile=result.projectiles.front();
		return result;
	}
	std::optional<std::uint64_t> deadline() const noexcept { return _deadline; }
	std::uint64_t cosmeticDeadline() const noexcept { return _cosmeticDeadline; }
	std::string notice() const;
private:
	friend class XeenEventFlow;
	friend struct XeenRestTestAccess;
	friend struct XeenTrainingTestAccess;
	friend struct XeenPurchaseTestAccess;
	friend struct XeenCombatPresentationTestAccess;
	bool _trainingEventSettlement=false;
	bool _smithEventSettlement=false;
    std::optional<Ticket> _castFrameTicket;
    IndexedFrame::Presentation _castFrame;
    std::uint64_t _castInput=0;
    std::string _combatCastRefusal;
	bool _needsRestNotice=false;
	struct RestContinuation {
	 enum class Phase { Confirm, Refused, Charges, Remainder, Dream, Recovery, Complete };
	 Phase phase=Phase::Charges;
	 unsigned charges=0, consumed=0, dreamBeat=0;
	 unsigned terrainMinutes=0;
	 std::string refusal;
	 bool starving=false, presented=false;
	 std::uint64_t deadline=0;
	 XeenCombatRandom random;
	 std::optional<XeenConditionTimeCandidate> time;
	 std::optional<XeenRestRecovery> recovery;
	 IndexedFrame dream, background;
	};
	std::unique_ptr<RestContinuation> _rest;
	bool beginRest(const std::function<IndexedFrame()> &);
	bool respondRest(const PlayerAction &);
	bool serviceRest();
	void restPublication();
    void authorizeCombatCastFrame(const Ticket &,std::uint64_t,const IndexedFrame::Presentation &);
    bool respondCombatCast(const PlayerAction &,std::uint64_t,const IndexedFrame::Presentation &,
        const std::function<XeenLearnedSpellNames()> &);

	// Exact immutable value binding of the complete prepared obligation. This
	// never generates stock and cannot adopt later provider-mutated candidates.
	struct SmithDepartureBinding {
		XeenGameplayContext beforeContext,endingContext;
		XeenServiceEconomy beforeEconomy,endingEconomy;
		XeenJourneyRandomState beforeRandom,endingRandom;
		bool triggered;
		explicit SmithDepartureBinding(const XeenServiceDayCandidate &day):
			beforeContext(day.beforeContext()),endingContext(day.context()),beforeEconomy(day.beforeEconomy()),
			endingEconomy(day.economy()),beforeRandom(day.beforeRandom()),endingRandom(day.continuation()),triggered(day.triggered()) {}
		bool matches(const XeenServiceDayCandidate &day) const {
			return day.complete() && beforeContext==day.beforeContext() && endingContext==day.context() &&
				beforeEconomy==day.beforeEconomy() && endingEconomy==day.economy() &&
				beforeRandom==day.beforeRandom() && endingRandom==day.continuation() && triggered==day.triggered();
		}
	};
	struct SmithContinuation {
		std::uint64_t lease=0, input=0, operation=0, reservation=0;
		IndexedFrame::Presentation frame;
		std::uint8_t owner=0, slot=0;
		static constexpr auto category=XeenInventoryCategory::Armor;
		bool quoted=false, departed=false, published=false, buy=false,temple=false,paid=false;
		XeenArmorRepairCandidate result;
		XeenTempleHealResult healResult;
		std::unique_ptr<XeenTempleHealCandidate> healPending;
		std::unique_ptr<XeenServiceDayCandidate> templeUpgrade;
		std::unique_ptr<XeenEquipmentPurchaseCandidate> purchase;
		std::uint64_t quoteOperation=0, quoteReservation=0;
		std::unique_ptr<XeenServiceDayCandidate> departure;
		std::optional<SmithDepartureBinding> binding;
	};
	std::unique_ptr<SmithContinuation> _smith;
	std::unique_ptr<SmithContinuation> _smithPreparation;
	struct TrainingContinuation {
		std::uint64_t lease=0,input=0,operation=0;
		IndexedFrame::Presentation frame;
		std::uint8_t owner=0;
		std::bitset<30> trained;
		bool quoted=false,departed=false,published=false;
		XeenTrainingResult result;
		std::unique_ptr<XeenServiceDayCandidate> departure,nextDeparture;
		std::unique_ptr<XeenTrainingCandidate> pending;
	};
	std::unique_ptr<TrainingContinuation> _training,_trainingPreparation;
	std::function<void(XeenTrainingBoundary)> _trainingBoundary;
	void checkTrainingBoundary(XeenTrainingBoundary);
	bool beginTraining(const std::function<void()> &);
	bool serviceTrainingPreparation();
	void authorizeTrainingFrame(std::uint64_t,const IndexedFrame::Presentation &);
	bool consumeTrainingFrame(std::uint64_t,const IndexedFrame::Presentation &);
	void quoteTraining(std::size_t);
	void confirmTraining();
	bool serviceTrainingLevel();
	void departTraining();
	void advanceTraining() noexcept;
	std::function<void(XeenSmithBoundary)> _smithBoundary;
	void checkSmithBoundary(XeenSmithBoundary);
	bool beginSmith(const std::function<void()> &);
	bool serviceSmithPreparation();
	void authorizeSmithFrame(std::uint64_t,const IndexedFrame::Presentation &);
	bool consumeSmithFrame(std::uint64_t,const IndexedFrame::Presentation &);
	void quoteSmith(std::size_t,std::size_t);
	void quoteSmithBuy(std::size_t,XeenInventoryCategory,std::size_t);
	void confirmSmith();
	void confirmSmithBuy();
	void quoteTempleHeal(std::size_t);
	bool confirmTempleHeal();
	bool serviceTempleHeal();
	void cancelTempleHeal();
	void advanceSmith() noexcept;
	void checkSmithReservation();
	void departSmith();
	bool beginCasting(const Ticket &);
	void authorizeCastingFrame(const Ticket &, std::uint64_t, const IndexedFrame::Presentation &);
	bool castingFrameCurrent(std::uint64_t, const IndexedFrame::Presentation &) const noexcept;
	bool cancelCasting(const Ticket &, std::uint64_t, const IndexedFrame::Presentation &);
	bool confirmCasting(const Ticket &, std::size_t casterIndex, std::size_t slot,
		std::uint64_t, const IndexedFrame::Presentation &);
	bool respondCastingTarget(const Ticket &, std::optional<std::size_t> targetIndex,
		std::uint64_t, const IndexedFrame::Presentation &);
	bool publishAwaken(const Ticket &);
	void adoptJourneyFlowBorrow();
	void beginJourneyEvent(bool barrier=false);
	void publishBarrier(const Ticket &, XeenWorld &, const XeenRestoreGuard &, const XeenBarrierCandidate &, const XeenGameplayContext &, std::uint64_t now, bool dailyReset);
	void endJourneyEvent();
	void publishArrival(const XeenActorView &) noexcept;
	bool journeyEvent() const noexcept { return _journey && _world.sessionState().journeyActivity() == XeenJourneyActivity::Event; }
	std::uint64_t _eventLease = 0;
	bool _journey = false;
	bool _journeyFramePrepared = false, _journeyFrameRetry = false;
	std::string _journeyRefusal;
	std::unique_ptr<XeenRegionalActionCandidate> _regionalWork;
	struct ItemUseContinuation {
		std::uint64_t generation=0,lease=0,epoch=0;
		std::uint64_t selectorInput=0;
		std::optional<Ticket> selectorTicket;
		IndexedFrame::Presentation selectorFrame;
		std::uint8_t sourceOwner=0,spentCharge=0;
		std::size_t slot=0;
		bool exhausted=false,ready=false;
		std::unique_ptr<XeenRegionalActionCandidate> opportunity;
	};
	std::unique_ptr<ItemUseContinuation> _itemUse;
	struct CastingContinuation {
		std::uint64_t lease=0, generation=0, displayedInput=0;
		IndexedFrame::Presentation displayedFrame;
		std::uint8_t casterOwner=0, slot=0;
		XeenLearnedSpell spell=XeenLearnedSpell::Awaken;
		std::int16_t originalSp=0;
		bool committed=false,effectDone=false;
		XeenCombatRandom random;
		std::optional<XeenConditionTimeCandidate> time;
	};
	std::unique_ptr<CastingContinuation> _casting;
	// Input protection outlives spell selection and the time publication.
	// Approach, projectiles and successor presentation retain their existing owners.
	bool _castingSettlement=false;
	void retireCastingFeedback() noexcept { if (!_casting && !_castingSettlement) _castingResult.clear(); }
	std::string _castingResult;
	std::uint64_t _castingGeneration=0;
	bool serviceCasting();
	std::optional<XeenAntidoteResult> _itemUseResult;
	std::uint64_t _itemUseGeneration=0;
	bool authorizeItemUseTarget(const Ticket &, std::uint64_t generation, std::uint64_t inventoryEpoch,
		std::uint64_t displayedInput, const IndexedFrame::Presentation &selectorFrame);
	bool abandonItemUse(const Ticket &, std::uint64_t generation, std::uint64_t inventoryEpoch);
	bool settleItemUse(const Ticket &, std::uint64_t generation, std::uint64_t inventoryEpoch,
		std::optional<std::size_t> targetIndex);
	std::unique_ptr<XeenShootCandidate> _shoot;
	std::vector<XeenProjectileAppearance> _projectiles;
 std::optional<XeenProjectileAppearance> _castProjectile;
 std::optional<std::uint64_t> _castProjectileDeadline;
 void castProjectilePresented() noexcept {
  if(_combat && _combat->cast() && _castProjectile && _castProjectile->active && !_castProjectileDeadline)
   _castProjectileDeadline=_lastTime+100;
 }
	std::uint64_t _projectileDeadline=0;
	std::shared_ptr<const XeenRegionalObservation> _rangedObservation;
	void observeRanged(std::shared_ptr<const XeenRegionalObservation>);
    void projectilesPresented();
	bool animateProjectiles();
	bool projectilesPending() const noexcept { for(const auto &p:_projectiles)if(p.active)return true;return false; }
	std::optional<XeenMonsterDeliveryCandidate> _monsterReceipt;
	std::string _monsterReceiptText;
	std::uint64_t _rewardLease=0;
	bool beginShoot();
	bool serviceShoot();
	bool beginMonsterReward(const XeenItemCatalog &);
	void acknowledgeMonsterReward();
	bool monsterReward() const noexcept { return _monsterReceipt.has_value(); }
	bool _regionalAutomatic = false;
	std::optional<XeenCombatLocation> _regionalAutomaticAddress;
	bool _shootIntent = false;
	std::shared_ptr<XeenRestoreGuard> _journeyPreimage;
	std::shared_ptr<XeenJourneyCapture> _journeyCapture;
	XeenEventFile _journeyEvents;
	std::vector<XeenMonsterRecord> _journeyStatistics;
	std::function<XeenLearnedSpellNames()> _learnedNamesProvider;
	void retainJourney();
	bool journeyCapacity() noexcept;
	bool smithCapacity(unsigned journeySteps,unsigned boundarySteps) const noexcept;
	void closeJourney() noexcept;
	XeenEncounterResult advanceJourney(const Ticket &, std::optional<XeenEncounterAction>);
	bool serviceItemUse();
	bool handleCombat(const PlayerAction &, std::optional<std::uint64_t>);
	bool idleCombat(std::optional<std::uint64_t>);
	bool acceptCombatResult(const XeenCombatResult &);
	void scheduleCombat(std::uint64_t);
	bool handoffCombat();
	std::string combatNotice() const;
	std::string consequenceNotice() const;
	bool observeCombat();
 unsigned _hitRow=0;
 bool _hitAlternatePosition=false;
 std::optional<std::uint64_t> _feedbackGeneration;
	void advanceAppearance() noexcept;
	XeenCombatResult _combatObservation, _combatAward, _retiredCombatResult;
	// Presentation-only binding for the already published finish. Never retirement authority.
	std::optional<std::uint64_t> _disengagementNoticeRevision;
	std::optional<XeenCombat::Ticket> _disengagementNoticeCombat;
	bool _scheduleAfterFrame = false, _combatOperationStale = false;
	bool adopt(const XeenEncounterResult &, std::uint64_t generation) noexcept;
	bool prepareTime(const Ticket &, std::uint64_t &now);
	void guardCallback(const Ticket &, const std::function<void()> &);
	void schedule(std::uint64_t now) noexcept;
	XeenWorld &_world;
	XeenPartyState &_party;
	XeenCamera &_camera;
	const XeenGameFlags &_flags;
	const XeenEventPresenter::Clock &_clock;
	const XeenEventFile &_events;
	XeenCombatBoundary _boundary;
	std::unique_ptr<XeenCombat> _combat;
	XeenEncounterState _state;
	XeenEncounterResult _result, _actionResult;
	unsigned _actionPending = 0;
	std::uint64_t _generation = 0, _lastTime = 0, _cosmeticDeadline = 0;
	std::optional<std::uint64_t> _deadline, _inputCycle;
	std::uint8_t _frame = 0;
	std::uint8_t _appearanceStep = 0;
	std::optional<XeenMonsterIdentity> _appearanceIdentity;
	bool _appearanceAfterFrame = false;
	bool _busy = false, _failure = false;
	std::optional<std::pair<int, int>> _attempted;
};
}
#endif
