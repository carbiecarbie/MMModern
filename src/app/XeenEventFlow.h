#ifndef MMODERN_APP_XEEN_EVENT_FLOW_H
#define MMODERN_APP_XEEN_EVENT_FLOW_H

#include "app/XeenNavigationFlow.h"
#include "games/xeen/XeenEventPresenter.h"
#include "games/xeen/XeenEventContinuation.h"
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenGameFlags.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenBarrierRules.h"
#include "games/xeen/XeenInventoryState.h"
#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenEquipment.h"
#include "app/XeenEncounterFlow.h"
#include <memory>
#include "core/InputContext.h"

namespace mmodern {

// Presentation/composition boundary shared by Application and integration tests.
// All gameplay state remains owned by the caller's existing session graph.
// The caller's mutable party must outlive this flow and pending presentations.
class XeenEventFlow {
public:
	struct Composition {
		IndexedFrame frame;
		bool containsOrdinaryAnimation = false;
	};
	using Compose = std::function<Composition(std::uint64_t ordinaryPhase)>;
	using EncounterCompose = std::function<Composition(std::uint64_t ordinaryPhase, XeenMonsterAppearance actorFrame)>;
	using TransitionCompose = std::function<Composition(XeenWorld &, const XeenPartyState &,
		const XeenCamera &, std::uint64_t, XeenMonsterAppearance)>;
	XeenEventFlow(XeenWorld &world, XeenEventSystem &events,
		XeenPartyState &party, XeenCamera &camera, XeenGameFlags &flags,
		const XeenFontFormat &font, Compose compose,
		XeenEventPresenter::NpcDraw npcDraw = {}, XeenEventPresenter::Clock clock = {},
		XeenEventPresenter::RandomFrame randomFrame = {}, const XeenItemCatalog *catalog = nullptr,
		EncounterCompose encounterCompose = {},
		const XeenJourneySetup *journey = nullptr, TransitionCompose transitionCompose = {});
	~XeenEventFlow();
	XeenEventFlow(const XeenEventFlow &) = delete;
	XeenEventFlow &operator=(const XeenEventFlow &) = delete;
	IndexedFrame initial();
	IndexedFrame handle(const PlayerAction &action, std::optional<std::uint64_t> displayedInput = {},
		const IndexedFrame::Presentation &inputFrame = {});
	std::optional<std::uint64_t> displayedInput() const noexcept { return _encounter && (journey() || _encounter->combat()) ? std::optional<std::uint64_t>{_inputGeneration} : std::nullopt; }
	// Copies retain the exact immutable published snapshot, including across Flow destruction.
	bool acceptsFrame(const IndexedFrame::Presentation &frame) const noexcept {
		return encounterFrameCurrent() && frame == _frame.presentation() && (!_encounter || frame);
	}
	// Upload candidates and acquired input origins are distinct during cosmetics.
	bool acceptsInputFrame(const IndexedFrame::Presentation &frame) const noexcept {
		return !_fatal && !_handoffPending && encounterFrameCurrent() &&
			frame == _actionableFrame && (!_encounter || frame);
	}
	InputContext inputContext(const IndexedFrame::Presentation &);
	bool journeyInputCurrent(std::optional<std::uint64_t>) const noexcept;
	std::function<void()> prepareJourneySprites;
	std::function<void(IndexedFrame &)> drawSmithArt;
	std::function<void(XeenSmithBoundary)> smithBoundary;
	std::function<void(IndexedFrame &)> drawTrainingArt;
	std::function<void(IndexedFrame &)> drawTempleArt;
	std::function<void(IndexedFrame &)> drawCombatButtons;
	XeenDialogSpriteDraw drawDialogSprite;
 // Borrowed from the existing asset owner, before any dialog is presented.
 const XeenDosText *dialogText = nullptr;
 const XeenDosText &dosText() const {
  if(!dialogText)throw std::logic_error("DOS dialog text provider is missing");
  return *dialogText;
 }
	std::function<IndexedFrame()> loadRestDream;
	std::function<void(XeenTrainingBoundary)> trainingBoundary;
	bool canSave() const noexcept;
	bool serviceSaveBlocked() const noexcept { return _smithUi.has_value() || _trainingUi.has_value() || _dispatching || _handoffPending || _saving || _fatal; }
	class SaveBoundary {
		friend class XeenEventFlow;
		const XeenEventFlow *owner = nullptr;
		std::uint64_t generation = 0, inventory = 0, input = 0;
		std::uint64_t operation = 0;
		std::optional<XeenEncounterFlow::Ticket> journey;
	};
	SaveBoundary beginSave();
	bool saveCurrent(const SaveBoundary &) const noexcept;
	void endSave();
	void endSave(const SaveBoundary &);
	bool journey() const noexcept { return _encounter && _encounter->journey(); }
	void framePresented(const IndexedFrame::Presentation &, bool deferCosmeticInput = false);
	void completeInputHandoff(const IndexedFrame::Presentation &);
	void closeGameplay() noexcept;
	IndexedFrame refresh(bool reconstruct = false);
	IndexedFrame acceptManual(XeenManualEventResult result);
	IndexedFrame acceptAutomatic(XeenAutomaticEventResult result);
	const IndexedFrame &frame() const { return _frame; }
	bool blocksGameplay() const { return (_encounter && (!journey() || !_encounter->journeyQuiet())) ||
		_pending.has_value() || _dispatching || _saving || _handoffPending || inventoryOpen() || _fatal; }
	const XeenEncounterFlow *encounter() const noexcept { return _encounter.get(); }
	void beginCycle(std::uint64_t cycle);
	bool encounterFrameCurrent() const noexcept;
	void failEncounterHandoff(const XeenEncounterFlow::Ticket &) noexcept;
	// Fault/observer seam immediately before the actual fallible frame copy.
	std::function<void()> beforeEncounterFrameCopy;
	// Fault seam after an admitted reward producer is validated, before enqueue.
	std::function<void()> beforeRewardEnqueue;
	std::function<void()> rebuildEncounterPresentation;
	bool inventoryOpen() const { return _inventory.mode != XeenInventoryMode::Closed; }
	const XeenInventorySelection &inventorySelection() const { return _inventory; }
	const XeenTransferResult &transferResult() const { return _transferResult; }
	const XeenMutableOptional<XeenEquipmentResult> &equipmentResult() const { return _equipmentResult; }
	// Explicit single-writer notification, including byte-identical owner replacement.
	void invalidateInventory();
	IndexedFrame refuseInventorySave();
	bool canCancelInteraction() const;
	bool handlesEscape() const;
	std::optional<IndexedFrame> updatePresentation();
	void abandonPresentation();
	const XeenEventPresenter &presenter() const { return _presenter; }
	std::optional<std::uint64_t> presentationGeneration() const;
	// Returns false for an obsolete/already consumed presentation. No resume occurs.
	bool respond(std::uint64_t generation, XeenPresentationResponse response);
	std::function<void(const XeenManualEventResult &)> reportManual;
	std::function<void(const XeenAutomaticEventResult &)> reportAutomatic;
	std::function<void(const std::string &)> reportText;
	std::function<void(const XeenTransferResult &)> reportInventory;
	std::function<void(const XeenEquipmentResult &)> reportEquipment;
	std::function<void(XeenMovementResult)> reportMovement;
private:
	friend struct XeenRestTestAccess;
	IndexedFrame drawRest(const IndexedFrame &);
	struct TrainingUi {
		enum class Phase { Preparation, Menu, Candidate, Departure };
		Phase phase=Phase::Preparation;
		std::size_t member=0;
		std::uint64_t revision=0;
		std::string feedback;
		IndexedFrame art;
	};
	std::optional<TrainingUi> _trainingUi;
	std::optional<std::uint64_t> _trainingRenderedRevision;
	bool _trainingSettlement=false,_trainingReported=false;
	std::optional<XeenManualEventResult> _trainingTerminalResult;
	void prepareTraining();
	IndexedFrame drawTraining(const IndexedFrame &) const;
	std::string trainingText() const;
	IndexedFrame handleTraining(const PlayerAction &,std::uint64_t,const IndexedFrame::Presentation &);
	IndexedFrame settleTrainingEvent();
	std::optional<IndexedFrame> updateTraining();
	struct SmithUi {
		XeenItemCatalog catalog;
		enum class Mode { Repair, Buy, Heal };
		Mode mode=Mode::Repair;
		enum class Phase { Preparation, Lobby, Browse, Confirm, Upgrade, Departure };
		Phase phase=Phase::Lobby;
		std::size_t member=0, slot=0;
		XeenInventoryCategory category=XeenInventoryCategory::Weapons;
		bool selected=false;
		std::uint64_t revision=0;
		std::string feedback;
		IndexedFrame art;
	};
	std::optional<SmithUi> _smithUi;
	std::optional<std::uint64_t> _smithRenderedRevision;
	bool _smithSettlement=false, _smithReported=false;
	std::optional<XeenManualEventResult> _smithTerminalResult;
	void prepareSmith();
	IndexedFrame settleSmithEvent();
	IndexedFrame drawSmith(const IndexedFrame &) const;
	std::string smithText() const;
	IndexedFrame handleSmith(const PlayerAction &,std::uint64_t,const IndexedFrame::Presentation &);
	IndexedFrame handleTemple(const PlayerAction &,std::uint64_t,const IndexedFrame::Presentation &);
	std::shared_ptr<const DialogInput> serviceDialogInput() const;
	const XeenEventPublication *_eventPublication = nullptr;
	bool _monsterReceiptPresented=false;
	bool _journeyEventLayers = false;
	struct TransitionCandidate {
		XeenRegionalInteraction kind = XeenRegionalInteraction::None;
		std::unique_ptr<XeenWorld> world;
		XeenPartyState party;
		XeenCamera camera;
		XeenGameFlags flags;
		XeenCamera initialCamera;
		XeenGameFlags initialFlags;
		XeenEventFile destinationEvents;
		std::unique_ptr<XeenRestoreGuard> guard;
		std::optional<XeenEventContinuation> continuation;
		bool preludePublished = false;
		bool refused = false;
		bool preludeRequired = false;
	};
	std::unique_ptr<TransitionCandidate> _transition;
	std::optional<XeenEventContinuation> _serviceEventContinuation;
	const void *_serviceEventOwner=nullptr;
	bool _candidateResultRefused=false;
	TransitionCompose _transitionCompose;
	bool _arrivalPending = false;
	struct BarrierWork {
		std::unique_ptr<XeenWorld> world;
		XeenPartyState party;
		XeenCamera camera;
		XeenGameFlags flags;
		std::unique_ptr<XeenRestoreGuard> guard;
		std::optional<XeenBarrierCandidate> rule;
		std::optional<XeenConditionTimeCandidate> time;
		XeenConsequenceCharacters characters;
		XeenConsequenceInputs inputs;
		XeenGameplayContext context;
		XeenCombatRandom random;
		bool bash=false, secondCharge=false, published=false, portraitWaiting=false, portraitPresented=false;
		bool trapWaiting=false, dailyReset=false, needsRest=false;
		std::optional<XeenServiceEconomy> economy;
		std::uint64_t deadline=0;
	};
	std::unique_ptr<BarrierWork> _barrier;
	bool beginBarrier(bool bash);
	bool serviceBarrier();
	IndexedFrame handleBarrier(const PlayerAction &);
	void checkTransitionCandidate();
	XeenManualEventResult beginVertigoEvent(XeenRegionalInteraction, bool automatic=false);
	XeenManualEventResult resumeVertigoEvent(XeenEventExecutionState, XeenPresentationResponse);
	void prepareVertigoResult(const XeenManualEventResult &);
	void claimServiceContinuation(const void *owner);
	void requireServiceSettlementOwner(const void *owner) const;
	void requireServiceSettlementEntry(const void *owner) const;
	XeenManualEventResult resumeOwnedServiceEvent(const void *owner);
	void validateRegionalEvents();
	IndexedFrame journeyEventWork(const std::function<void()> &, bool automatic = false);
	std::uint64_t _saveOperation = 0;
	std::optional<SaveBoundary> _saveBoundary;
	friend class Application;
	friend struct XeenTrainingTestAccess;
	friend struct XeenCityEventTestAccess;
	friend struct XeenPurchaseTestAccess;
	void requireCurrentOwners() const;
	friend struct XeenRewardTestAccess;
	friend struct XeenInventoryTestAccess;
	// Synchronous dispatch also covers callbacks before a suspension is installed.
	bool _dispatching = false;
	bool _fatal = false;
	bool _saving = false, _handoffPending = false;
	bool _cosmeticPending = false;
	IndexedFrame::Presentation _actionableFrame, _acquiredCosmeticFrame;
	const IndexedFrame::Presentation &responseFrame() const noexcept {
		return _actionableFrame ? _actionableFrame : _frame.presentation();
	}
	void authorizeInputFrame(const IndexedFrame::Presentation &);
	std::unique_ptr<XeenEncounterFlow> _encounter;
	EncounterCompose _encounterCompose;
	std::optional<XeenEncounterFlow::Ticket> _encounterFrame;
	std::optional<std::uint64_t> _cycle;
	std::uint64_t _inputGeneration = 0;
    // Separate from the per-step input generation. Busy frames stay in context.
    struct QueueContext {
        XeenMapIdentity map;
        unsigned panel = 0;
        const XeenCombat *combat = nullptr;
        std::uint64_t dialog = 0;
        bool operator==(const QueueContext &b) const {
            return map == b.map && panel == b.panel && combat == b.combat && dialog == b.dialog;
        }
    };
    std::optional<QueueContext> _queueContext;
    std::uint64_t _queueContextId = 0;

	std::optional<XeenCombat::Ticket> _displayedCombat;
	void prepareJourneyTransition();
	std::uint64_t _inventoryLease = 0, _certificateLease = 0;
	void syncCombatInventory();
	IndexedFrame renderEncounter(bool report = false, bool cosmeticInput = false);
	IndexedFrame frameCopy();
	void sealFrame(IndexedFrame &returned);
	const XeenFontFormat &_inventoryFont;
	const XeenItemCatalog &_catalog;
	XeenInventorySelection _inventory;
	struct CharacterSheetUi {
		unsigned cursor=0;
		bool blink=false;
		std::uint64_t deadline=0;
	};
	std::optional<CharacterSheetUi> _sheet;
	bool _itemsVisible=false, _combatItems=false;
	std::optional<XeenDialogPopup> _statPopup;
	std::optional<std::string> _dialogError;
	std::optional<unsigned> _itemOption;
	IndexedFrame drawCharacterDialog(const IndexedFrame &) const;
	IndexedFrame handleCharacterDialog(const PlayerAction &);
	std::shared_ptr<const DialogInput> characterDialogInput() const;
	std::optional<std::size_t> dialogMember(std::size_t) const;
	void dialogError(std::string);
	void performItemOption(unsigned);
	IndexedFrame _inventoryUnderlay;
	const char *_inventoryFeedback = "";
	XeenTransferResult _transferResult;
	XeenMutableOptional<XeenEquipmentResult> _equipmentResult;
	std::uint64_t _inventoryEpoch = 0;
	struct EquipmentSelection {
		std::uint64_t epoch;
		std::array<std::uint8_t, XeenParty::kMaximumVisibleMembers> membership{};
		std::size_t membershipSize = 0;
		std::size_t sourceActiveIndex = 0;
		std::uint8_t resolvedOwner = 0;
		XeenInventoryCategory category = XeenInventoryCategory::Weapons;
		std::size_t physicalSlot = 0;
		XeenItem selectedRecord{};
	};
	std::optional<EquipmentSelection> _equipmentSelection;
	std::optional<std::uint64_t> _itemUseGeneration;
	struct CastingUi {
		enum class Phase { ChooseCaster, BrowseLearned, ConfirmCast, ChooseTarget, Settling };
		Phase phase=Phase::ChooseCaster;
		std::size_t caster=0, slot=0, scroll=0;
		std::string refusal;
	};
	std::optional<CastingUi> _castingUi;
	bool castingCasterEligible(std::size_t) const;
	std::string combatCastingText() const;
	IndexedFrame handleCombatCasting(const PlayerAction &,std::uint64_t);
	std::string castingText() const;
	IndexedFrame handleCasting(const PlayerAction &, std::uint64_t);
	void advanceInventoryEpoch() noexcept;
	void armEquipmentSelection();
	bool validEquipmentSelection(const EquipmentSelection &) const;
	void handleEquipment(XeenEquipmentOperation);
	bool validInventorySource(bool record) const;
	void invalidateInventorySelection();
	void closeInventory() noexcept;
	void drawInventory();
	void recoverInventory();
	IndexedFrame handleInventory(const PlayerAction &);
	void transferInventory(std::size_t);
	XeenRewardReceipt cleanup(XeenRewardDiscard reason) noexcept;
	bool resumePending(std::uint64_t generation, XeenPresentationResponse response);
	enum class OrdinaryCause { None, Action, Idle };
	bool updateOrdinaryPhase(OrdinaryCause cause, bool reset, std::uint64_t now);
	bool advanceEncounterOrdinary(OrdinaryCause cause = OrdinaryCause::Idle);
	bool refreshScene(bool reconstruct, OrdinaryCause cause, bool committedTransition = false);
	template<class Result> IndexedFrame drive(Result result, bool automatic, bool reconstruct = false,
		OrdinaryCause cause = OrdinaryCause::None, bool committedTransition = false);
	IndexedFrame presentationFailed(const std::exception &exception);
	bool pendingNpc() const;
	struct Pending { XeenEventExecutionState state; bool automatic; std::uint64_t generation; };
	std::uint64_t _generation = 0;
	XeenWorld &_world;
	std::unique_ptr<XeenWorld::GameplayBorrow> _gameplayBorrow;
	XeenEventSystem &_events;
	XeenPartyState &_party;
	XeenCamera &_camera;
	XeenGameFlags &_flags;
	XeenNavigationFlow _navigation;
	XeenEventPresenter::Clock _clock;
	XeenEventPresenter _presenter;
	struct OrdinaryAnimationState {
		std::uint64_t phase = 0;
		std::uint64_t deadline = 0;
		XeenMapIdentity mapId;
		XeenDirection direction;
		bool containsOrdinaryAnimation = false;
	};
	OrdinaryAnimationState _ordinary;
	Compose _compose;
	std::optional<Pending> _pending;
	// Interpreter-dispatched resource identities retained independently of callback state.
	std::vector<std::size_t> _regionalExecutedRecords;
	std::optional<XeenEventContinuation> _regionalEventContinuation;
	IndexedFrame _frame;
	std::string _mainScreenNotice;
	IndexedFrame drawMainScreenNotice(const IndexedFrame &, const std::string &) const;
 void drawPartyPresentation(IndexedFrame &) const;
	XeenCamera _renderedCamera;
	std::size_t _disabledObjects = 0;
};
}
#endif
