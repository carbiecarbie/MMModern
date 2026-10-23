#ifndef MMODERN_APP_XEEN_EVENT_FLOW_H
#define MMODERN_APP_XEEN_EVENT_FLOW_H
#include "games/xeen/XeenProgramState.h"

#include "app/XeenNavigationFlow.h"
#include "formats/xeen/XeenSpriteDrawOptions.h"
#include "games/xeen/XeenEventPresenter.h"
#include "games/xeen/XeenEventContinuation.h"
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenGameFlags.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenBarrierRules.h"
#include "games/xeen/XeenInventoryState.h"
#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenServiceAnimation.h"
#include "games/xeen/XeenEntranceScene.h"
#include "games/xeen/XeenEquipment.h"
#include "app/XeenEncounterFlow.h"
#include <memory>
#include "core/InputContext.h"
#include "games/xeen/XeenTextInput.h"
#include "core/PresentationClock.h"
#include "formats/xeen/XeenSpriteDrawOptions.h"

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
	XeenEventFlow(XeenProgramState &program, XeenWorld &world, XeenEventSystem &events,
		XeenPartyState &party, XeenCamera &camera, XeenGameFlags &flags,
		const XeenFontFormat &font, Compose compose,
		XeenEventPresenter::NpcDraw npcDraw = {}, XeenEventPresenter::Clock clock = {},
		XeenEventPresenter::RandomFrame randomFrame = {}, const XeenItemCatalog *catalog = nullptr,
		EncounterCompose encounterCompose = {},
		const XeenJourneySetup *journey = nullptr, TransitionCompose transitionCompose = {},
        const XeenDosText *text = nullptr, PresentationClock::Source presentationClock = {});
	~XeenEventFlow();
	XeenEventFlow(const XeenEventFlow &) = delete;
	XeenEventFlow &operator=(const XeenEventFlow &) = delete;
	IndexedFrame initial();
	// Publication callers capture first, then complete their exploration input.
	IndexedFrame completeExplorationInput();
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
    std::uint64_t presentationNow();
    std::optional<std::uint64_t> nextPresentationDeadline();
    std::optional<unsigned> audioLocation() const;
	bool journeyInputCurrent(std::optional<std::uint64_t>) const noexcept;
	std::function<void()> prepareJourneySprites;
	std::function<void(IndexedFrame &)> drawSmithArt;
	std::function<void(XeenSmithBoundary)> smithBoundary;
	std::function<void(IndexedFrame &)> drawTrainingArt;
	std::function<void(IndexedFrame &)> drawTempleArt;
	std::function<void(IndexedFrame &)> drawTavernArt,drawBankArt,drawGuildArt;
    std::function<std::vector<std::uint8_t>()> loadSpellDescriptions;
    std::function<std::vector<std::uint8_t>(bool awards)> loadJournalText;
	std::function<std::string(unsigned)> tavernFoodOffer;
    std::function<std::string(std::size_t)> tavernString;
	std::function<void(IndexedFrame &)> drawCombatButtons;
	XeenDialogSpriteDraw drawDialogSprite;
    std::function<void(IndexedFrame &,const char *,unsigned,int,int,int)> drawEntranceSprite;
    std::function<void(std::uint8_t)> loadArenaSprites;
	std::function<std::string(XeenMapIdentity)> mapName;
 // Borrowed from the existing asset owner, before any dialog is presented.
 const XeenDosText *dialogText = nullptr;
 const XeenDosText &dosText() const {
  if(!dialogText)throw std::logic_error("DOS dialog text provider is missing");
  return *dialogText;
 }
	std::function<IndexedFrame()> loadRestDream;
    std::function<std::uint64_t(const char*,XeenSceneArchive)> loadSampleDuration;
    std::function<std::string(unsigned,bool)> loadEntranceCaption;
    std::function<void(IndexedFrame &,const std::string &)> drawEntranceCaption;
	std::function<void(XeenTrainingBoundary)> trainingBoundary;
	// Test-only observation/counterfactual seam; production applies every DOS clear.
    std::function<bool(unsigned)> eventInputClearForAudit;
    std::function<void(unsigned,const std::optional<PlayerAction> &)> observeEventInputClear;
    std::function<void(const PlayerAction &)> observeManualResponse;
    std::function<std::optional<IndexedFrame>(const PlayerAction &)> continueExplorationPanel;
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
	// Control panel Mr Wizard; empty when it is refused with nothing published.
	std::optional<IndexedFrame> mrWizard();
 bool configureSound(XeenSoundOptions);
	const IndexedFrame &frame() const { return _frame; }
	bool blocksGameplay() const { return _explorationPass || (_encounter && (!journey() || !_encounter->journeyQuiet())) ||
		_pending.has_value() || _dispatching || _saving || _handoffPending || inventoryOpen() || _summary || _fatal; }
	const XeenEncounterFlow *encounter() const noexcept { return _encounter.get(); }
	void beginCycle(std::uint64_t cycle);
	bool encounterFrameCurrent() const noexcept;
	void failEncounterHandoff(const XeenEncounterFlow::Ticket &) noexcept;
	// Fault/observer seam immediately before the actual fallible frame copy.
	std::function<void()> beforeEncounterFrameCopy;
	// Fault seam after an admitted reward producer is validated, before staging.
	std::function<void()> beforeRewardStaging;
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
    bool partyDeathComplete() const noexcept;
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
    IndexedFrame handleExplorationAction(const PlayerAction &, std::optional<std::uint64_t>, const IndexedFrame::Presentation &);
    struct EntranceUi {
        XeenEntranceAnimation animation;
        std::string caption;
        IndexedFrame saved,zoom,art;
        explicit EntranceUi(XeenEntranceAnimation value,const IndexedFrame &frame):animation(std::move(value)),saved(frame),art(frame) {}
    };
    std::optional<EntranceUi> _entranceUi;
    void prepareEntrance();
    bool advanceEntrance();
    std::optional<IndexedFrame> updateEntrance();
	friend struct XeenRestTestAccess;
	friend struct XeenPartyInterfaceTestAccess;
	IndexedFrame drawRest(const IndexedFrame &);
	// DOS location wait 0x1c520 polls input before each independent three-tick reset.
	struct ServiceArtState {
		XeenServiceArt kind = XeenServiceArt::Smith;
		unsigned index = 0;
		std::uint64_t deadline = 0;
        XeenSilentSampleInterval sample;
        std::optional<unsigned> pendingIndex,pendingSpeaking;
        std::optional<XeenJourneyRandomState> randomBefore,randomAfter;
        bool awaitingFrame=false,randomPublished=false,resetAfterFrame=false;
	};
    std::string serviceNotEnoughGold();
	bool pulseServiceArt(ServiceArtState &, IndexedFrame &art);
    void startServiceSample(ServiceArtState &,const char *,bool resetIndex=false);
    bool serviceSampleBusy(const ServiceArtState &);
    void serviceInputPolled(ServiceArtState &);
	struct TrainingUi {
		enum class Phase { Preparation, Menu, Candidate, Departure };
		ServiceArtState animation;
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
		enum class Mode { Repair, Buy, Heal, Food, Bank, Guild, Sell, Identify };
		Mode mode=Mode::Repair;
		XeenItemService itemAction() const noexcept {
			return mode==Mode::Buy?XeenItemService::Buy:mode==Mode::Sell?XeenItemService::Sell:
				mode==Mode::Identify?XeenItemService::Identify:XeenItemService::Fix;
		}
		enum class Phase { Preparation, Lobby, Browse, Confirm, Upgrade, Departure, Currency, Amount, Detail };
		Phase phase=Phase::Lobby;
		std::size_t member=0, slot=0;
		XeenInventoryCategory category=XeenInventoryCategory::Weapons;
		bool selected=false;
		std::uint64_t revision=0;
		std::string feedback;
		std::string itemDetails;
		std::string foodOffer,amount;
        std::string tavernStatus,tavernPopup;
		XeenTextInput amountCursor;
		bool withdrawal=false,gems=false,bankRefreshed=false;
        bool goodbye=false;
        XeenTempleSurcharges templePrices;
        std::uint64_t templePriceRevision=0;
        bool spellInfo=false;
        ServiceArtState animation;
        XeenSpellListUi spells;
        std::vector<std::string> descriptions;
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
	IndexedFrame handleFoodBank(const PlayerAction &,std::uint64_t,const IndexedFrame::Presentation &);
	std::shared_ptr<const DialogInput> serviceDialogInput() const;
	const XeenEventPublication *_eventPublication = nullptr;
	bool _monsterReceiptPresented=false;
	struct TreasureWait {
		std::uint64_t generation=0,deadline=0;
		unsigned ticks=0;
		bool monster=false,acquired=false;
		std::size_t monsterCursor=0;
		XeenRewardPhase monsterPhase=XeenRewardPhase::Receipt;
	};
	std::optional<TreasureWait> _treasureWait;
	void prepareTreasureWait(std::uint64_t,unsigned,bool monster=false);
	std::optional<IndexedFrame> advanceTreasureWait();
	void publishEventSequencePrefix(const XeenEventExecutionState &);
	bool _journeyEventLayers = false;
    bool _passEvent=false;
	// One obligation across the outer input's dialogs and asynchronous work.
	bool _explorationPass = false;
	struct ManualInput {
        std::optional<PlayerAction> action;
    };
    std::optional<ManualInput> _manualInput;
    bool _continuingManual=false;
    std::uint64_t _keyboardClear=0, _mouseClear=0, _mapLoadInputGeneration=0;
    unsigned _eventInputClears=0;
    void clearManualInput(unsigned site);
    void replaceManualInput(const PlayerAction &);
    XeenPresentationUpdate handleEventResponse(const PlayerAction &);
    void clearManualPresentationInput();
    std::optional<IndexedFrame> continueManualInput();
    bool explorationInputPending() const;
    std::optional<std::pair<std::uint64_t,std::size_t>> _manualPageClear;
	void armExplorationPass();
	bool explorationEffectComplete() const;
	bool prepareExplorationPass();
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
		std::optional<XeenManualEventCompleted> completion;
		bool refused = false;
		bool cancelled = false;
		bool presentationRetry = false;
		bool automatic=false, terrainDone=false, manualStepDone=false, entrance=false;
		std::optional<XeenTerrainArrivalCandidate> terrain;
		std::optional<XeenRegionalActionCandidate> manualStep;
        // The old observable party/RNG prefix, retained until the final fall
        // check. Its World remains detached; only the old prefix projection
        // can be published on ordinary cancellation or an unsupported tail.
        struct ObservedPrefix {
            std::unique_ptr<XeenWorld> world;XeenPartyState party;XeenCamera camera;XeenGameFlags flags;
            std::unique_ptr<XeenRestoreGuard> guard;
        };
        std::unique_ptr<ObservedPrefix> observed;
	};
	std::unique_ptr<TransitionCandidate> _transition;
	void prepareTerrainTransition();
	bool serviceManualStep();
	void retainManualPrefix(bool party);
	void publishManualPrefix();
	std::optional<XeenEventContinuation> _serviceEventContinuation;
	const void *_serviceEventOwner=nullptr;
	bool _candidateResultRefused=false;
	TransitionCompose _transitionCompose;
	bool _arrivalPending = false;
	struct BarrierWork {
        bool fmOpened=false;
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
		bool trapWaiting=false, trapPresented=false, dailyReset=false, needsRest=false;
		std::optional<XeenServiceEconomy> economy;
		std::uint64_t deadline=0;
	};
	std::unique_ptr<BarrierWork> _barrier;
	bool beginBarrier(bool bash);
	bool serviceBarrier();
	IndexedFrame handleBarrier(const PlayerAction &);
	void checkTransitionCandidate();
	XeenPresentationUpdate presentPendingEvent();
    struct PortraitSequence {
        XeenEventPortraitEffect effect;
        unsigned frame=0,countdown=0;
        std::uint64_t generation=0,deadline=0;
        bool event=false,acquired=false,window11=false,ranged=false;
    };
    std::optional<PortraitSequence> _portraitSequence;
    void preparePortraitSequence(const XeenEventPortraitEffect &,std::uint64_t generation=0);
    void prepareStandalonePortrait();
    std::optional<IndexedFrame> advancePortraitSequence();
    bool portraitSequenceActive() const noexcept {return _portraitSequence.has_value();}

    struct NpcSequence {
        XeenEventPresenter::NpcTiming timing;
        std::uint64_t speechEnd=0, deadline=0, generation=0;
        std::optional<unsigned> choice;
        bool acquired=false, composed=false;
        bool bodyHidden=false,textOffLoop=false;
        std::string speechNotice;
        std::size_t page=0;
    };
    std::optional<NpcSequence> _npcSequence;
    XeenEventPresenter::RandomFrame _npcRandomFrame;
    bool advanceNpcSequence(std::optional<std::uint64_t> sampledNow = {});
    struct NpcResponse {PlayerAction action;std::uint64_t generation;};
    std::optional<NpcResponse> _npcResponse;
    IndexedFrame consumeNpcResponse();
 bool drawPendingHeads(IndexedFrame &);
	XeenManualEventResult beginDetachedEvent(XeenRegionalInteraction, bool automatic=false);
	XeenEventSystem detachedEventResources();
	XeenManualEventResult resumeDetachedEvent(XeenEventExecutionState, XeenPresentationResponse);
	void prepareDetachedResult(const XeenManualEventResult &);
	void claimServiceContinuation(const void *owner);
	void requireServiceSettlementOwner(const void *owner) const;
	void requireServiceSettlementEntry(const void *owner) const;
	XeenManualEventResult resumeOwnedServiceEvent(const void *owner);
	void validateRegionalEvents();
	IndexedFrame journeyEventWork(const std::function<void()> &, bool automatic = false);
	std::uint64_t _saveOperation = 0;
	std::optional<SaveBoundary> _saveBoundary;
	friend class Application;
	friend struct XeenTemplePriceStateTestAccess;
	friend struct XeenTrainingTestAccess;
    friend struct XeenEntranceTestAccess;
    friend struct XeenArenaTestAccess;
	friend struct XeenMonsterTestAccess;
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
	IndexedFrame renderEncounter(bool report = false, bool cosmeticInput = false, bool reconstruct = false);
	struct DeathContinuation {
        enum class Phase { Fire, Final, Laugh, Restore, Scroll, Closed, FadeOut, Done };
        Phase phase=Phase::Fire;
        unsigned fire=0,laugh=0,restarts=0,scroll=22,fadeLevel=128;
        bool acquired=false;
        std::uint64_t deadline=0,laughEnd=0,laughDuration=0;
        IndexedFrame gameplay,final;
    };
    std::optional<DeathContinuation> _death;
    std::optional<std::uint64_t> _fmPresentationGeneration;
    bool partyDeathRequired() const noexcept;
    void beginDeath(const IndexedFrame &);
    IndexedFrame drawDeath(const IndexedFrame &);
    bool advanceDeath();
    void deathPresented();
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
	enum class SummaryDialog { QuickReference, Info, Map, Quests, Awards };
	std::optional<SummaryDialog> _summary;
	std::uint64_t _mapPhase=0,_mapDeadline=0;
	std::string _mapName;
	bool _exchange=false;
	std::optional<std::size_t> _quickFightMember;
	std::size_t _quickFightHighlight=0;
	IndexedFrame drawQuickFightOptions(const IndexedFrame &) const;
	IndexedFrame handleQuickFightOptions(const PlayerAction &);
	IndexedFrame _summaryUnderlay;
	XeenJournalUi _journalUi;
	std::vector<std::string> _journalStrings;
	void openJournal(bool awards);
	std::vector<std::string> journalLines() const;
	IndexedFrame drawSummary(const IndexedFrame &) const;
	bool _itemsVisible=false, _combatItems=false;
	std::optional<XeenDialogPopup> _statPopup;
	std::optional<std::string> _dialogError;
	std::optional<unsigned> _itemOption;
	bool _discardConfirm=false;
	std::string _discardPrompt;
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
    struct SpellItemsUi {
        XeenLearnedSpell spell;
        XeenInventorySelection selection;
        bool choosing=false,waiting=false;
        std::string error;
    };
    std::optional<SpellItemsUi> _spellItemsUi;
    void openSpellItems(XeenLearnedSpell,std::size_t);
    void syncSpellItems();
    IndexedFrame handleSpellItems(const PlayerAction &,std::uint64_t);
    IndexedFrame drawSpellItems(const IndexedFrame &) const;
    std::shared_ptr<const DialogInput> spellItemsInput() const;
    struct CastingUi {
        enum class Phase { Cast, New, ChooseTarget, ChooseElement, Settling, Items };
        Phase phase=Phase::Cast;
        std::size_t caster=0;
        XeenSpellListUi list;
        std::string refusal;
    };
    std::optional<CastingUi> _castingUi;
    struct TravelUi {bool item=false;XeenLearnedSpell spell=XeenLearnedSpell::Teleport;std::uint8_t owner=0;
        XeenTextInput digits;std::string refusal;std::array<std::string,5> names;};
    bool beginTravelDialog(XeenLearnedSpell,std::uint8_t,bool);
    std::optional<TravelUi> _travelUi;
    std::optional<unsigned> _travelChoice;
    IndexedFrame drawTravelDialog(const IndexedFrame &) const;
    IndexedFrame handleTravelDialog(const PlayerAction &,std::uint64_t);
    std::optional<std::size_t> _lastCaster;
    void initializeCaster();
    std::shared_ptr<const DialogInput> castingDialogInput() const;
    IndexedFrame drawCasting(const IndexedFrame &) const;
    IndexedFrame handleCombatCasting(const PlayerAction &,std::uint64_t);
    IndexedFrame handleCasting(const PlayerAction &,std::uint64_t);
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
	Composition composeJourneyScene(bool reconstruct);
	// DOS 0x8c52 (Direction Sense): a word, amount or travel-digit prompt is open.
	bool textPromptOpen() const;
	template<class Result> IndexedFrame drive(Result result, bool automatic, bool reconstruct = false,
		OrdinaryCause cause = OrdinaryCause::None, bool committedTransition = false);
	IndexedFrame presentationFailed(const std::exception &exception);
	bool pendingNpc() const;
	struct Pending { XeenEventExecutionState state; bool automatic; std::uint64_t generation; };
	std::uint64_t _generation = 0;
	XeenProgramState &_program;
	XeenWorld &_world;
	std::unique_ptr<XeenWorld::GameplayBorrow> _gameplayBorrow;
	XeenEventSystem &_events;
	XeenPartyState &_party;
	XeenCamera &_camera;
	XeenGameFlags &_flags;
	XeenNavigationFlow _navigation;
	XeenEventPresenter::Clock _clock;
    PresentationClock::Source _presentationClock;
    bool _sceneAdvancePending=false;
    void finishOrdinaryDraw();
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
	std::vector<XeenEventDispatchRecord> _regionalExecutedRecords;
	std::optional<XeenEventContinuation> _regionalEventContinuation;
	IndexedFrame _frame;
	std::string _mainScreenNotice;
    std::string _whoWillRefusal;
	IndexedFrame drawMainScreenNotice(const IndexedFrame &, const std::string &) const;
    IndexedFrame drawTerminalEncounterNotice(const IndexedFrame &, const std::string &) const;
 void drawPartyPresentation(IndexedFrame &) const;
	XeenCamera _renderedCamera;
	std::size_t _disabledObjects = 0;
};
}
#endif
