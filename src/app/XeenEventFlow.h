#ifndef MMODERN_APP_XEEN_EVENT_FLOW_H
#define MMODERN_APP_XEEN_EVENT_FLOW_H

#include "app/XeenNavigationFlow.h"
#include "games/xeen/XeenEventPresenter.h"
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenGameFlags.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenInventoryView.h"
#include "games/xeen/XeenEquipment.h"
#include "app/XeenEncounterFlow.h"
#include <memory>

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
		const XeenEncounterSetup *encounter = nullptr, EncounterCompose encounterCompose = {},
		const XeenJourneySetup *journey = nullptr, TransitionCompose transitionCompose = {});
	~XeenEventFlow();
	XeenEventFlow(const XeenEventFlow &) = delete;
	XeenEventFlow &operator=(const XeenEventFlow &) = delete;
	IndexedFrame initial();
	IndexedFrame handle(const PlayerAction &action, std::optional<std::uint64_t> displayedInput = {});
	std::optional<std::uint64_t> displayedInput() const noexcept { return _encounter && (journey() || _encounter->combat() || _encounter->completed()) ? std::optional<std::uint64_t>{_inputGeneration} : std::nullopt; }
	// Copies retain the exact immutable published snapshot, including across Flow destruction.
	bool acceptsFrame(const IndexedFrame::Presentation &frame) const noexcept {
		return encounterFrameCurrent() && frame == _frame.presentation() && (!_encounter || frame);
	}
	bool journeyInputCurrent(std::optional<std::uint64_t>) const noexcept;
	std::function<void()> prepareJourneySprites;
	std::function<void(IndexedFrame &)> drawSmithArt;
	std::function<void(XeenSmithBoundary)> smithBoundary;
	bool completed() const noexcept { return _encounter && _encounter->completed(); }
	bool canSave() const noexcept;
	bool serviceSaveBlocked() const noexcept { return _smithUi.has_value() || _dispatching || _handoffPending || _saving || _fatal; }
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
	void framePresented(const IndexedFrame::Presentation &);
	void closeGameplay() noexcept;
	IndexedFrame completedFeedback(std::string);
	static IndexedFrame preflightCompleted(IndexedFrame, const XeenFontFormat &, const XeenItemCatalog *,
		const XeenWorld &, const XeenPartyState &, const XeenCamera &);
	XeenWorld::MonsterLoader completedMonsters;
	XeenWorld::EventLoader completedEvents;
	XeenWorld::CompletedPreflight completedPreflight;
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
	std::optional<std::uint64_t> inventoryConfirmation() const;
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
	struct SmithUi {
		XeenItemCatalog catalog;
		enum class Phase { Lobby, Browse, Quote, Result, Departure };
		Phase phase=Phase::Lobby;
		std::size_t member=0, slot=0;
		std::string title, feedback;
		IndexedFrame art;
	};
	std::optional<SmithUi> _smithUi;
	void prepareSmith();
	IndexedFrame drawSmith(const IndexedFrame &) const;
	IndexedFrame handleSmith(const PlayerAction &,std::uint64_t);
	const XeenEventPublication *_eventPublication = nullptr;
	bool _monsterReceiptPresented=false;
	bool _journeyEventLayers = false;
	struct TransitionCandidate {
		XeenRegionalInteraction kind = XeenRegionalInteraction::None;
		std::unique_ptr<XeenWorld> world;
		XeenPartyState party;
		XeenCamera camera;
		XeenGameFlags flags;
		XeenEventFile destinationEvents;
		std::unique_ptr<XeenRestoreGuard> guard;
		bool preludePublished = false;
		bool refused = false;
	};
	std::unique_ptr<TransitionCandidate> _transition;
	TransitionCompose _transitionCompose;
	bool _arrivalPending = false;
	void checkTransitionCandidate();
	XeenManualEventResult beginVertigoEvent(XeenRegionalInteraction);
	XeenManualEventResult resumeVertigoEvent(XeenEventExecutionState, XeenPresentationResponse);
	void prepareVertigoResult(const XeenManualEventResult &);
	void validateRegionalEvents();
	IndexedFrame journeyEventWork(const std::function<void()> &, bool automatic = false);
	std::uint64_t _saveOperation = 0;
	std::optional<SaveBoundary> _saveBoundary;
	friend class Application;
	XeenRestoreGuard &completedSavePreimage() { return _encounter->completedPreimage(); }
	void requireCurrentOwners() const;
	friend struct XeenRewardTestAccess;
	friend struct XeenInventoryTestAccess;
	// Synchronous dispatch also covers callbacks before a suspension is installed.
	bool _dispatching = false;
	bool _fatal = false;
	bool _saving = false, _handoffPending = false;
	std::unique_ptr<XeenEncounterFlow> _encounter;
	EncounterCompose _encounterCompose;
	std::optional<XeenEncounterFlow::Ticket> _encounterFrame;
	std::optional<std::uint64_t> _cycle;
	std::uint64_t _inputGeneration = 0;
	std::optional<XeenCombat::Ticket> _displayedCombat;
	std::optional<XeenEncounterFlow::Ticket> _displayedCompleted;
	void authorizeCompletedFrame();
	void prepareJourneyTransition();
	std::uint64_t _inventoryLease = 0, _certificateLease = 0;
	bool combatPreparation() const noexcept { return _encounter && _encounter->preparation(); }
	void syncCombatInventory();
	IndexedFrame renderEncounter(bool report = false, bool cosmeticInput = false);
	IndexedFrame frameCopy();
	void sealFrame(IndexedFrame &returned);
	const XeenFontFormat &_inventoryFont;
	const XeenItemCatalog &_catalog;
	XeenInventorySelection _inventory;
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
	std::string castingText() const;
	IndexedFrame handleCasting(const PlayerAction &, std::uint64_t);
	struct InventoryConfirmation {
		std::uint64_t epoch;
		XeenInventorySelection selection;
		std::array<std::uint8_t, XeenParty::kMaximumVisibleMembers> membership{};
		std::size_t size = 0;
	};
	std::optional<InventoryConfirmation> _inventoryConfirmation;
	void advanceInventoryEpoch() noexcept;
	void armEquipmentSelection();
	bool validEquipmentSelection(const EquipmentSelection &) const;
	void handleEquipment();
	bool validInventorySource(bool record) const;
	void invalidateInventorySelection();
	void closeInventory() noexcept;
	void drawInventory();
	void recoverInventory();
	IndexedFrame handleInventory(const PlayerAction &);
	void confirmInventory();
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
	IndexedFrame _frame;
	XeenCamera _renderedCamera;
	std::size_t _disabledObjects = 0;
};
}
#endif
