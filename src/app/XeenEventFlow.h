#ifndef MMODERN_APP_XEEN_EVENT_FLOW_H
#define MMODERN_APP_XEEN_EVENT_FLOW_H

#include "app/XeenNavigationFlow.h"
#include "games/xeen/XeenEventPresenter.h"
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenInventoryView.h"

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
	XeenEventFlow(XeenWorld &world, XeenEventSystem &events,
		XeenPartyState &party, XeenCamera &camera, XeenGameFlags &flags,
		const XeenFontFormat &font, Compose compose,
		XeenEventPresenter::NpcDraw npcDraw = {}, XeenEventPresenter::Clock clock = {},
		XeenEventPresenter::RandomFrame randomFrame = {}, const XeenItemCatalog *catalog = nullptr);
	~XeenEventFlow();
	XeenEventFlow(const XeenEventFlow &) = delete;
	XeenEventFlow &operator=(const XeenEventFlow &) = delete;
	IndexedFrame initial();
	IndexedFrame handle(const PlayerAction &action);
	IndexedFrame refresh(bool reconstruct = false);
	IndexedFrame acceptManual(XeenManualEventResult result);
	IndexedFrame acceptAutomatic(XeenAutomaticEventResult result);
	const IndexedFrame &frame() const { return _frame; }
	bool blocksGameplay() const { return _pending.has_value() || _dispatching || inventoryOpen() || _fatal; }
	bool inventoryOpen() const { return _inventory.mode != XeenInventoryMode::Closed; }
	const XeenInventorySelection &inventorySelection() const { return _inventory; }
	const XeenTransferResult &transferResult() const { return _transferResult; }
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
	std::function<void(XeenMovementResult)> reportMovement;
private:
	friend struct XeenRewardTestAccess;
	friend struct XeenInventoryTestAccess;
	// Synchronous dispatch also covers callbacks before a suspension is installed.
	bool _dispatching = false;
	bool _fatal = false;
	const XeenFontFormat &_inventoryFont;
	const XeenItemCatalog &_catalog;
	XeenInventorySelection _inventory;
	IndexedFrame _inventoryUnderlay;
	const char *_inventoryFeedback = "";
	XeenTransferResult _transferResult;
	std::uint64_t _inventoryEpoch = 0;
	struct InventoryConfirmation {
		std::uint64_t epoch;
		XeenInventorySelection selection;
		std::array<std::uint8_t, XeenParty::kMaximumVisibleMembers> membership{};
		std::size_t size = 0;
	};
	std::optional<InventoryConfirmation> _inventoryConfirmation;
	void advanceInventoryEpoch() noexcept;
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
	bool refreshScene(bool reconstruct, OrdinaryCause cause, bool committedTransition = false);
	template<class Result> IndexedFrame drive(Result result, bool automatic, bool reconstruct = false,
		OrdinaryCause cause = OrdinaryCause::None, bool committedTransition = false);
	IndexedFrame presentationFailed(const std::exception &exception);
	bool pendingNpc() const;
	struct Pending { XeenEventExecutionState state; bool automatic; std::uint64_t generation; };
	std::uint64_t _generation = 0;
	XeenWorld &_world;
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
