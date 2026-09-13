#include "app/XeenEventFlow.h"
#include <type_traits>
#include <utility>
#include <iostream>
#include <chrono>
#include <limits>

namespace mmodern {
namespace {
const XeenItemCatalog &fallbackCatalog() {
	static const auto catalog = XeenItemCatalog::unavailable();
	return catalog;
}
static_assert(std::is_nothrow_move_constructible<XeenEventExecutionState>::value,
	"Flow must adopt execution ownership without allocation");
static_assert(std::is_nothrow_copy_assignable<std::optional<XeenEquipmentResult>>::value,
	"Flow must adopt a fixed equipment result without allocation");
struct DispatchScope {
	bool &value;
	bool previous;
	explicit DispatchScope(bool &v) : value(v), previous(v) { value = true; }
	~DispatchScope() { value = previous; }
};
bool sameCamera(const XeenCamera &a, const XeenCamera &b) {
	return a.mapId == b.mapId && a.x == b.x && a.y == b.y && a.direction == b.direction;
}

IndexedFrame noticeFrame(const IndexedFrame &base, const XeenFontFormat &font, const std::string &notice, bool roster) {
	XeenTextRenderOptions options;
	options.bounds = {9, 9, 222, 50}; options.x = 10; options.y = 10;
	options.size = XeenFontSize::Reduced;
	options.paginate = true;
	options.drawWindow = true; options.windowBounds = {8,8,223,51};
	if (roster) {
		options.bounds = {3,137,229,200}; options.windowBounds = {1,135,231,200};
		options.x = 3; options.y = 137;
	}
	const auto split = roster ? notice.find("\n\n") : std::string::npos;
	auto rendered = XeenTextRenderer(font).render(base, notice.substr(0,split), options);
	if (rendered.pages.size() != 1) throw std::runtime_error("Encounter notice did not fit");
	if (split != std::string::npos) {
		options.bounds = {235,3,318,198}; options.windowBounds = {233,1,320,200};
		options.x=235; options.y=3;
		rendered = XeenTextRenderer(font).render(rendered.pages.front(),notice.substr(split+2),options);
		if (rendered.pages.size()!=1) throw std::runtime_error("Combat roster notice did not fit");
	}
	return std::move(rendered.pages.front());
}
}

IndexedFrame XeenEventFlow::preflightCompleted(IndexedFrame base, const XeenFontFormat &font, const XeenItemCatalog *catalog,
		const XeenWorld &world, const XeenPartyState &party, const XeenCamera &camera) {
	if (!base.isValid()) throw std::runtime_error("Invalid completed scene");
	auto frame = noticeFrame(base, font, XeenEncounterFlow::completedNotice(world, party, camera), true);
	static_cast<void>(XeenEncounterFlow::completedInspection(world, party, camera));
	for (unsigned member = 0; member < party.party.size(); ++member) {
		XeenInventorySelection selection;
		selection.mode = XeenInventoryMode::Browse; selection.source = member;
		selection.sourceOwner = party.party.activeRosterIds()[member];
		for (unsigned category = 0; category < 4; ++category) {
			selection.category = static_cast<XeenInventoryCategory>(category);
			for (unsigned slot = 0; slot < 9; ++slot) {
				selection.slot = slot;
				selection.record = (*xeenInventoryItems(party.roster.at(*selection.sourceOwner), selection.category))[slot];
				static_cast<void>(xeenInventoryLayout(font, catalog ? *catalog : fallbackCatalog(), party, selection, "", nullptr, false, true));
			}
			static_cast<void>(drawXeenInventory(frame, font, catalog ? *catalog : fallbackCatalog(), party, selection, "", nullptr, false, true));
		}
	}
	return frame;
}

void XeenEventFlow::requireCurrentOwners() const {
	if (!_gameplayBorrow->current()) throw std::runtime_error("Stale Flow borrowed owner lifetime");
}

bool XeenEventFlow::canSave() const noexcept {
	return _gameplayBorrow->current() && !_fatal && !_dispatching && !_saving && !_handoffPending &&
		!inventoryOpen() && !_pending && !_equipmentSelection && !_inventoryConfirmation &&
		(!_encounter || (_encounter->canSave() && encounterFrameCurrent()));
}

void XeenEventFlow::authorizeCompletedFrame() {
	if (!completed()) return;
	if (_inputGeneration == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Completed input generation exhausted");
	++_inputGeneration;
	_displayedCombat.reset();
	_displayedCompleted = _encounter->ticket();
	_encounterFrame = _displayedCompleted;
}

XeenEventFlow::SaveBoundary XeenEventFlow::beginSave() {
	if (!canSave()) throw std::logic_error("Save boundary is unavailable");
	if (_saveOperation == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Save generation exhausted");
	if (completed()) _encounter->holdCompleted();
	SaveBoundary boundary;
	if (journey()) boundary.journey = _encounter->beginJourneySave();
	_saving = true;
	boundary.operation = ++_saveOperation;
	boundary.owner = this; boundary.generation = _generation;
	boundary.inventory = _inventoryEpoch; boundary.input = _inputGeneration;
	_saveBoundary = boundary;
	return boundary;
}
bool XeenEventFlow::journeyInputCurrent(std::optional<std::uint64_t> input) const noexcept {
	return journey() && input && *input == _inputGeneration && !_handoffPending && !_fatal && !_dispatching && !_saving && encounterFrameCurrent();
}
void XeenEventFlow::prepareJourneyTransition() {
	if (!journey()) return;
	if (!_encounter->combat() && _world.sessionState().journeyActivity() == XeenJourneyActivity::Attachment) {
		closeInventory();
		if (!_encounter->attachJourney(_encounter->ticket(),prepareJourneySprites)) throw std::runtime_error("Journey attachment failed");
	}
	if (_encounter->combat() && _encounter->combat()->phase() == XeenCombatPhase::Victory) {
		if (!_encounter->retireJourney(_encounter->ticket())) throw std::runtime_error("Journey retirement failed");
		_displayedCombat.reset();
	}
}

bool XeenEventFlow::saveCurrent(const SaveBoundary &b) const noexcept {
	return b.owner == this && _saving && !_fatal && !_dispatching && _gameplayBorrow->current() &&
		b.operation == _saveOperation && (!b.journey || _encounter->journeySaveCurrent(*b.journey)) &&
		b.generation == _generation && b.inventory == _inventoryEpoch && b.input == _inputGeneration &&
		!inventoryOpen() && !_pending && !_equipmentSelection && !_inventoryConfirmation;
}

void XeenEventFlow::endSave() {
	if (_saveBoundary) endSave(*_saveBoundary);
}
void XeenEventFlow::endSave(const SaveBoundary &boundary) {
	if (boundary.owner != this || boundary.operation != _saveOperation) return;
	if (!_saving) return;
	if (boundary.journey) {
		if (!_encounter->endJourneySave(*boundary.journey)) { _fatal = true; return; }
		_encounterFrame = _encounter->ticket();
	}
	if (completed()) { _encounter->releaseCompleted(); authorizeCompletedFrame(); }
	_saving = false;
	_saveBoundary.reset();
}

void XeenEventFlow::framePresented() {
	requireCurrentOwners();
	if (_dispatching || _saving) throw std::logic_error("Frame handoff during dispatch");
	if (!encounterFrameCurrent()) throw std::logic_error("Stale successful frame handoff");
	if (journey() && _handoffPending) {
		if (_encounter->combat()) _encounter->presented(*_encounterFrame);
		else if (!_encounter->presentJourney(*_encounterFrame)) throw std::logic_error("Stale Journey frame handoff");
		_handoffPending = false; _encounterFrame = _encounter->ticket();
	}
	if (completed() && _handoffPending) {
		if (!inventoryOpen()) _encounter->releaseCompleted();
		_handoffPending = false;
		authorizeCompletedFrame();
	}
}

void XeenEventFlow::closeGameplay() noexcept {
	if (_gameplayBorrow->current() && completed()) _encounter->closeCompleted();
	if (_gameplayBorrow->current() && journey()) _encounter->fail(_encounter->ticket());
	_fatal = true;
}

IndexedFrame XeenEventFlow::completedFeedback(std::string message) {
	if (!completed() || _dispatching || _saving || _fatal) return frameCopy();
	DispatchScope dispatch(_dispatching);
	_encounter->feedback(std::move(message));
	return renderEncounter();
}

void XeenEventFlow::beginCycle(std::uint64_t cycle) {
	requireCurrentOwners();
	if (!_encounter) return;
	if (_dispatching || _fatal || cycle == 0 || (_cycle && cycle <= *_cycle))
		throw std::runtime_error("Obsolete encounter loop cycle");
	_cycle = cycle;
}

bool XeenEventFlow::encounterFrameCurrent() const noexcept {
	return _gameplayBorrow->current() && (!_encounter || (!_fatal && _encounterFrame && _encounter->current(*_encounterFrame)));
}

void XeenEventFlow::failEncounterHandoff(const XeenEncounterFlow::Ticket &entry) noexcept {
	if (_gameplayBorrow->current() && _encounter) {
		if (completed()) { if (_encounter->current(entry)) _encounter->closeCompleted(); }
		else _encounter->fail(entry);
	}
	_fatal = true;
}

IndexedFrame XeenEventFlow::frameCopy() {
	requireCurrentOwners();
	if (!_encounter) return _frame;
	const auto entry = _encounter->ticket();
	try { return _frame; }
	catch (...) { failEncounterHandoff(entry); throw; }
}
bool XeenEventFlow::updateOrdinaryPhase(OrdinaryCause cause, bool reset, std::uint64_t now) {
	requireCurrentOwners();
	if (!reset && cause != OrdinaryCause::Action &&
		(cause != OrdinaryCause::Idle || now < _ordinary.deadline)) return false;
	if (now > std::numeric_limits<std::uint64_t>::max() - 100 ||
		(!reset && _ordinary.phase == std::numeric_limits<std::uint64_t>::max()))
		throw std::overflow_error("Ordinary animation overflow");
	// One shared M22 policy: committed facing/map reset wins over the action step.
	_ordinary.phase = reset ? 0 : _ordinary.phase + 1;
	_ordinary.deadline = now + 100;
	if (reset) {
		_ordinary.mapId = _camera.mapId;
		_ordinary.direction = _camera.direction;
	}
	return true;
}

bool XeenEventFlow::advanceEncounterOrdinary(OrdinaryCause cause) {
	if (cause == OrdinaryCause::Idle && (_encounter->terminal() || _encounter->preparation())) return false;
	const auto entry = _encounter->ticket();
	try {
		const auto now = _clock();
		if (!_encounter->current(entry)) throw std::runtime_error("Stale ordinary animation callback");
		const bool reset = _ordinary.mapId != _camera.mapId || _ordinary.direction != _camera.direction;
		return updateOrdinaryPhase(cause, reset, now) && _ordinary.containsOrdinaryAnimation;
	} catch (...) {
		if (!_encounter->fail(entry, XeenEncounterStop::Preparation)) { _fatal = true; throw; }
		return true;
	}
}

IndexedFrame XeenEventFlow::renderEncounter(bool report) {
	if (journey() && !_encounter->combat()) {
		_encounter->holdJourneyFrame();
		for (unsigned attempt=0;attempt<2;++attempt) {
			IndexedFrame returned;
			const auto t = _encounter->ticket();
			if (_encounter->prepareJourneyFrame(t,[&] {
				if (attempt) { _world.discardMapCache(); if (rebuildEncounterPresentation) rebuildEncounterPresentation(); }
				auto composed = _encounterCompose(_ordinary.phase,_encounter->appearance());
				if (!composed.frame.isValid()) throw std::runtime_error("Invalid Journey frame");
				auto rendered = noticeFrame(composed.frame,_inventoryFont,_encounter->notice(),true);
				_inventoryUnderlay = rendered;
				if (inventoryOpen()) rendered = drawXeenInventory(rendered,_inventoryFont,_catalog,_party,_inventory,_inventoryFeedback,
					_equipmentResult ? &*_equipmentResult : nullptr);
				if (report && !attempt) {
					if (reportText) reportText(_encounter->notice());
					std::cout << _encounter->journeyInspection();
				}
				if (!attempt && beforeEncounterFrameCopy) beforeEncounterFrameCopy();
				returned = rendered; _frame = std::move(rendered);
				_ordinary.containsOrdinaryAnimation = composed.containsOrdinaryAnimation;
			})) {
				if (_inputGeneration == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Journey input generation exhausted");
				++_inputGeneration;
				_encounterFrame = _encounter->ticket(); _handoffPending = true;
				return returned;
			}
			if (!_encounter->current(t)) break;
		}
		_fatal = true; throw std::runtime_error("Journey presentation recovery failed");
	}
	const bool hadFrame = _frame.isValid();
	for (unsigned attempt = 0; attempt < 2; ++attempt) {
		if (completed()) _encounter->holdCompleted();
		const auto entry = _encounter->ticket();
		try {
			if (!_encounter->current(entry)) throw std::runtime_error("Stale encounter composition");
			std::optional<XeenRestoreGuard::Providers> providers;
			if (completed()) providers.emplace(_encounter->completedPreimage(), _world);
			if (attempt) {
				_world.discardMapCache();
				if (rebuildEncounterPresentation) rebuildEncounterPresentation();
				if (!_encounter->current(entry)) throw std::runtime_error("Stale encounter rebuild");
			}
			auto composed = _encounterCompose(_ordinary.phase, _encounter->appearance());
			if (!_encounter->current(entry)) throw std::runtime_error("Stale encounter frame");
			if (!composed.frame.isValid()) throw std::runtime_error("Invalid encounter frame");
			const auto notice = _encounter->notice();
			auto rendered = noticeFrame(composed.frame, _inventoryFont, notice, _encounter->combat() || completed());
			if (report && !attempt && reportText) reportText(notice);
			if (!_encounter->current(entry)) throw std::runtime_error("Stale encounter report");
			// Complete the fallible return copy before installing the frame.
			if (!attempt && beforeEncounterFrameCopy) beforeEncounterFrameCopy();
			if (!_encounter->current(entry)) throw std::runtime_error("Stale encounter frame copy");
			IndexedFrame returned = rendered;
			_frame = std::move(rendered);
			if (_encounter->combat() || completed()) {
				_inventoryUnderlay = _frame;
				if (inventoryOpen()) {
					_frame = drawXeenInventory(_inventoryUnderlay,_inventoryFont,_catalog,_party,_inventory,_inventoryFeedback,
						_equipmentResult ? &*_equipmentResult : nullptr, !completed(), completed());
					returned = _frame;
				}
				if (_encounter->combat() && (journey() || !_displayedCombat || !_encounter->combat()->current(*_displayedCombat))) {
					if (_inputGeneration == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Combat input generation exhausted");
					++_inputGeneration;
					_displayedCombat = _encounter->combat()->ticket();
				}
				if (!journey()) _encounter->presented(entry);
			}
			_ordinary.containsOrdinaryAnimation = composed.containsOrdinaryAnimation;
			_encounterFrame = entry;
			if (journey()) _handoffPending = true;
			if (completed()) { _handoffPending = true; authorizeCompletedFrame(); }
			return returned;
		} catch (...) {
			if (journey() && !attempt && hadFrame && _encounter->current(entry)) continue;
			if (!_encounter->fail(entry) || attempt || !hadFrame) { _encounter->closeCompleted(); _fatal = true; throw; }
			if (_encounter->combat()) closeInventory();
		}
	}
	throw std::logic_error("Unreachable encounter recovery");
}

XeenEventFlow::XeenEventFlow(XeenWorld &world, XeenEventSystem &events,
		XeenPartyState &party, XeenCamera &camera, XeenGameFlags &flags,
		const XeenFontFormat &font, Compose compose, XeenEventPresenter::NpcDraw npcDraw,
		XeenEventPresenter::Clock clock, XeenEventPresenter::RandomFrame randomFrame, const XeenItemCatalog *catalog,
		const XeenEncounterSetup *encounter, EncounterCompose encounterCompose, const XeenJourneySetup *journey) :
	_inventoryFont(font), _catalog(catalog ? *catalog : fallbackCatalog()),
	_world(world), _gameplayBorrow(world.sessionState().journey() ? nullptr : new XeenWorld::GameplayBorrow(world,party,camera,flags)),
	_events(events), _party(party), _camera(camera), _flags(flags),
	_navigation(events), _clock(clock ? std::move(clock) : XeenEventPresenter::Clock{[] {
		return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count());
	}}), _presenter(font, std::move(npcDraw), [this] { return _clock(); }, std::move(randomFrame)),
	_ordinary{0, 0, camera.mapId, camera.direction, false},
	_compose(std::move(compose)) {
	if (journey || world.sessionState().journey()) {
		if (encounter || !encounterCompose) throw std::invalid_argument("Journey requires exclusive presentation setup");
		_encounterCompose = std::move(encounterCompose);
		if (journey) _encounter = std::make_unique<XeenEncounterFlow>(world,party,camera,flags,_clock,*journey);
		else _encounter = std::make_unique<XeenEncounterFlow>(world,party,camera,flags,_clock,XeenJourneyRestoreTag{});
	} else if (encounter) {
		if (!_encounterCompose && !encounterCompose) throw std::invalid_argument("Missing encounter composer");
		_encounterCompose = std::move(encounterCompose);
		_encounter = std::make_unique<XeenEncounterFlow>(world, party, camera, flags, _clock, *encounter);
		_ordinary.deadline = _encounter->cosmeticDeadline();
	} else _ordinary.deadline = _clock() + 100;
	if (!_gameplayBorrow) {
		_encounter->journeySavePreimage().check();
		_gameplayBorrow.reset(new XeenWorld::GameplayBorrow(world,party,camera,flags));
		_encounter->adoptJourneyFlowBorrow();
	}
	refresh(true);
}

IndexedFrame XeenEventFlow::refresh(bool reconstruct) {
	requireCurrentOwners();
	if (_dispatching || _fatal || _saving) return frameCopy();
	DispatchScope dispatch(_dispatching);
	if (_encounter) {
		if (_encounter->combat() && reconstruct) invalidateInventorySelection();
		return renderEncounter();
	}
	if (reconstruct) invalidateInventorySelection();
	refreshScene(reconstruct, OrdinaryCause::None);
	return frameCopy();
}

bool XeenEventFlow::refreshScene(bool reconstruct, OrdinaryCause cause, bool committedTransition) {
	requireCurrentOwners();
	if (_encounter && (journey() || _encounter->combat() || completed())) { if (!journey()) renderEncounter(); return true; }
	try {
	const bool reset = committedTransition || _ordinary.mapId != _camera.mapId ||
		_ordinary.direction != _camera.direction;
	bool stepped = false;
	if (reset || (cause != OrdinaryCause::None && _world.map(_camera.mapId).geometry.isOutdoors()))
		stepped = updateOrdinaryPhase(cause, reset, _clock());
	const bool cameraChanged = !sameCamera(_camera, _renderedCamera);
	// M15's disabled set only grows during a session. Its size is an exact,
	// constant-time change detector for the only supported visual mutation.
	const auto count = _world.sessionState().disabledObjectCount();
	if (reconstruct || reset || !_frame.isValid() || cameraChanged || count != _disabledObjects ||
			(stepped && _ordinary.containsOrdinaryAnimation)) {
		// Always use the committed camera, never logicalAddress/workingCamera.
		const auto composition = _compose(_ordinary.phase);
		requireCurrentOwners();
		_ordinary.containsOrdinaryAnimation = composition.containsOrdinaryAnimation;
		if (cameraChanged && !_pending) _presenter.clear();
		_frame = _presenter.rebase(composition.frame);
		_inventoryUnderlay = _frame;
		_renderedCamera = _camera;
		_disabledObjects = count;
		if (inventoryOpen()) drawInventory();
		return true;
	}
	return false;
	} catch (const std::exception &e) {
		requireCurrentOwners();
		if (_fatal) throw;
		if (inventoryOpen()) { recoverInventory(); return true; }
		if (!_pending) throw;
		presentationFailed(e);
		return true;
	}
}

template<class Result> IndexedFrame XeenEventFlow::drive(Result result, bool automatic, bool reconstruct,
		OrdinaryCause cause, bool committedTransition) {
	requireCurrentOwners();
	for (;;) {
		// Adopt ownership before composition, reporting or presentation can fail.
		auto *suspended = std::get_if<XeenEventExecutionSuspended>(&result);
		if (suspended) _pending.emplace(Pending{std::move(suspended->state), automatic, ++_generation});
		try {
		refreshScene(reconstruct, cause, committedTransition);
		reconstruct = false;
		cause = OrdinaryCause::None;
		committedTransition = false;
		if (suspended && !_pending) return frameCopy();
		// Reporting receives a value snapshot; Flow has already adopted ownership
		// and can account for it if constructing this snapshot fails.
		if (suspended) suspended->state = _pending->state;
		if constexpr (std::is_same_v<Result, XeenManualEventResult>) {
			if (reportManual) reportManual(result);
		} else {
			if (reportAutomatic) reportAutomatic(result);
		}
		if (!suspended) return frameCopy();
		if (!_pending) return frameCopy(); // A reporting callback may explicitly abandon.
		_frame = _presenter.dismissSelection();
		XeenPresentationUpdate update;
		update = _presenter.present(_frame, _pending->state.pendingPresentation->request);
		_frame = std::move(update.frame);
		if (reportText) for (const auto &message : _presenter.diagnostics()) reportText(message);
		requireCurrentOwners();
		if (!update.response) return frameCopy();
		auto pending = std::move(*_pending);
		_pending.reset(); // Consume before calling into the execution system.
		if constexpr (std::is_same_v<Result, XeenManualEventResult>)
			result = _events.resumeManualEvent(std::move(pending.state), *update.response,
				_world, _party, _camera, _flags);
		else
			result = _events.resumeAutomaticEvent(std::move(pending.state), *update.response,
				_world, _party, _camera, _flags);
		} catch (const std::exception &e) {
			if (!_pending) throw;
			return presentationFailed(e);
		}
	}
}

IndexedFrame XeenEventFlow::acceptManual(XeenManualEventResult result) {
	requireCurrentOwners();
	if (_encounter || blocksGameplay()) throw std::logic_error("Cannot replace pending event; abandon before dispatching replacement");
	DispatchScope dispatch(_dispatching);
	return drive(std::move(result), false);
}
IndexedFrame XeenEventFlow::acceptAutomatic(XeenAutomaticEventResult result) {
	requireCurrentOwners();
	if (_encounter || blocksGameplay()) throw std::logic_error("Cannot replace pending event; abandon before dispatching replacement");
	DispatchScope dispatch(_dispatching);
	return drive(std::move(result), true);
}
IndexedFrame XeenEventFlow::initial() {
	requireCurrentOwners();
	if (_encounter || blocksGameplay()) return frameCopy();
	DispatchScope dispatch(_dispatching);
	return drive(_navigation.processInitialEvent(_world, _party, _camera, _flags), true);
}
bool XeenEventFlow::canCancelInteraction() const {
	return _pending && _pending->state.pendingPresentation &&
		_pending->state.pendingPresentation->request.response ==
			XeenPresentationResponseRequirement::CharacterSelection;
}
bool XeenEventFlow::pendingNpc() const {
	return _pending && _pending->state.pendingPresentation &&
		_pending->state.pendingPresentation->request.kind == XeenPresentationKind::NpcAcknowledgment;
}
bool XeenEventFlow::handlesEscape() const {
	if (journey()) return !_fatal && !_handoffPending && inventoryOpen();
	if (_encounter) return false;
	return inventoryOpen() || canCancelInteraction() || pendingNpc() || (_pending && _pending->state.rewardPhase != XeenRewardPhase::Running);
}
XeenRewardReceipt XeenEventFlow::cleanup(XeenRewardDiscard reason) noexcept {
	XeenRewardReceipt outcome;
	if (_pending) {
		xeenDiscardRewards(_pending->state.pendingRewards, _pending->state.rewardReceipt, reason);
		outcome = _pending->state.rewardReceipt;
		_pending.reset();
	}
	_presenter.discardTransient();
	return outcome;
}
XeenEventFlow::~XeenEventFlow() {
	const auto outcome = cleanup(XeenRewardDiscard::Abandoned);
	if (outcome.discarded) {
		try { std::cerr << "Rewards discarded on shutdown: " << outcome.discarded << '\n'; } catch (...) {}
	}
}
void XeenEventFlow::abandonPresentation() {
	requireCurrentOwners();
	if (_encounter) return; // Presentation cleanup cannot reset encounter authority.
	if (_dispatching && !_pending) return;
	DispatchScope dispatch(_dispatching);
	closeInventory();
	const auto outcome = cleanup(XeenRewardDiscard::Abandoned);
	_frame = _presenter.frame();
	if (outcome.discarded) {
		const auto message = "Rewards discarded on abandonment: " + std::to_string(outcome.discarded);
		std::cerr << message << '\n';
		if (reportText) reportText(message);
	}
}
IndexedFrame XeenEventFlow::presentationFailed(const std::exception &exception) {
	const auto automatic = _pending->automatic;
	const auto count = _pending->state.instructionCount;
	const auto address = _pending->state.logicalAddress;
	auto source = std::move(_pending->state.pendingPresentation->request.source);
	const auto outcome = cleanup(XeenRewardDiscard::PresentationFailure);
	_frame = _presenter.frame();
	XeenEventExecutionError error{XeenEventExecutionErrorKind::PresentationFailed,
		std::string("Event presentation: ") + exception.what(), count,
		address, std::move(source), std::nullopt, outcome};
	if (automatic) {
		if (reportAutomatic) reportAutomatic(error);
	} else {
		try { if (reportManual) reportManual(error); }
		catch (...) { std::cerr << "Manual presentation reporting failed after cleanup\n"; }
	}
	return frameCopy();
}
std::optional<IndexedFrame> XeenEventFlow::updatePresentation() {
	requireCurrentOwners();
	if (_dispatching || _fatal || _saving || (journey() && _handoffPending)) return std::nullopt;
	DispatchScope dispatch(_dispatching);
	if (_encounter) {
		const bool hadJourneyCombat = journey() && _encounter->combat();
		const bool changed = _encounter->idle(_cycle);
		prepareJourneyTransition();
		if (!journey() && !_pending && !inventoryOpen() && !_equipmentSelection && !_inventoryConfirmation)
			_encounter->retireVictory();
		if (_encounter->combatOperationStale()) {
			_fatal = true;
			throw std::runtime_error("Stale automatic combat operation");
		}
		if (!_encounter->current(_encounter->ticket())) { _fatal = true; throw std::runtime_error("Stale encounter idle"); }
		const bool ordinary = advanceEncounterOrdinary();
		if (changed || ordinary) return renderEncounter(completed() || (hadJourneyCombat && !_encounter->combat()));
		return std::nullopt;
	}
	const bool recomposed = refreshScene(false, OrdinaryCause::Idle);
	if (!pendingNpc()) return recomposed ? std::optional<IndexedFrame>{_frame} : std::nullopt;
	try {
		auto changed = _presenter.updateNpc();
		if (changed) _frame = *changed;
		return (changed || recomposed) ? std::optional<IndexedFrame>{_frame} : std::nullopt;
	} catch (const std::exception &e) {
		if (!_pending) throw;
		return presentationFailed(e);
	}
}
std::optional<std::uint64_t> XeenEventFlow::presentationGeneration() const {
	return _pending ? std::optional<std::uint64_t>{_pending->generation} : std::nullopt;
}
bool XeenEventFlow::respond(std::uint64_t generation, XeenPresentationResponse response) {
	requireCurrentOwners();
	if (_encounter || _dispatching || inventoryOpen() || _fatal) return false;
	DispatchScope dispatch(_dispatching);
	return resumePending(generation, response);
}
bool XeenEventFlow::resumePending(std::uint64_t generation, XeenPresentationResponse response) {
	requireCurrentOwners();
	if (!_pending || _pending->generation != generation) return false;
	if (!_pending->state.pendingPresentation ||
		!xeenResponseMatches(_pending->state.pendingPresentation->request.response, response)) return false;
	try { _frame = _presenter.finishPresentation(); }
	catch (const std::exception &e) { presentationFailed(e); return true; }
	requireCurrentOwners();
	auto pending = std::move(*_pending);
	_pending.reset();
	if (pending.automatic)
		drive(_events.resumeAutomaticEvent(std::move(pending.state), response,
			_world, _party, _camera, _flags), true);
	else
		drive(_events.resumeManualEvent(std::move(pending.state), response,
			_world, _party, _camera, _flags), false);
	return true;
}
IndexedFrame XeenEventFlow::handle(const PlayerAction &action, std::optional<std::uint64_t> displayedInput) {
	requireCurrentOwners();
	if (journey() && !journeyInputCurrent(displayedInput)) return frameCopy();
	if (std::holds_alternative<SaveGameAction>(action) || _dispatching || _fatal || _saving) return frameCopy();
	if (completed() && (!displayedInput || *displayedInput != _inputGeneration || !_displayedCompleted ||
		!_encounter->current(*_displayedCompleted) || _handoffPending)) return frameCopy();
	if (_encounter && _encounter->combat() && (!displayedInput || *displayedInput != _inputGeneration ||
		!_displayedCombat || !_encounter->combat()->current(*_displayedCombat))) return frameCopy();
	if (!_encounter && std::holds_alternative<WaitAction>(action)) return frameCopy();
	if (!_encounter && (std::holds_alternative<AttackAction>(action) || std::holds_alternative<BlockAction>(action) ||
		std::holds_alternative<BeginEncounterAction>(action) || std::holds_alternative<RevisitCompletedAction>(action))) return frameCopy();
	DispatchScope dispatch(_dispatching);
	if (_encounter) {
		if (journey() && !_encounter->combat()) {
			if (inventoryOpen() || std::holds_alternative<InspectInventoryAction>(action)) {
				if (!_encounter->journeyMutable()) return frameCopy();
				handleInventory(action);
				return renderEncounter(true);
			}
			if (std::holds_alternative<InteractionAction>(action)) {
				if (!_encounter->journeyQuiet()) return frameCopy();
				try { _encounter->journeyRead([&] {
					const auto result = _navigation.processInteraction(_world,_party,_camera,_flags);
					if (!std::holds_alternative<XeenManualEventNoEvent>(result)) throw std::runtime_error("Journey requires the admitted event-free footprint");
					if (reportManual) reportManual(result);
				}); } catch (...) {
					if (!_encounter->current(_encounter->ticket())) throw;
					// No gameplay publication is admitted by this event-free operation.
					// Rebuild once; never repeat interaction or its reporting callback.
				}
				return renderEncounter();
			}
		}
		if (completed()) {
			if (std::holds_alternative<CancelInteractionAction>(action)) return frameCopy();
			if (inventoryOpen() && (std::holds_alternative<RevisitCompletedAction>(action) ||
				std::holds_alternative<TransferInventoryAction>(action) || std::holds_alternative<EquipmentInventoryAction>(action) ||
				std::holds_alternative<AcknowledgeAction>(action) || std::holds_alternative<AttackAction>(action) ||
				std::holds_alternative<BlockAction>(action) || std::holds_alternative<WaitAction>(action))) return frameCopy();
			if (inventoryOpen() || std::holds_alternative<InspectInventoryAction>(action)) {
				_encounter->holdCompleted();
				const auto entry = _encounter->ticket();
				try {
					handleInventory(action);
					if (!_encounter->current(entry)) throw std::runtime_error("Stale completed inspection");
				} catch (...) {
					if (!_encounter->fail(entry)) { _fatal = true; _encounter->closeCompleted(); throw; }
					closeInventory();
				}
				return renderEncounter();
			}
			if (std::holds_alternative<RevisitCompletedAction>(action)) {
				try {
					const auto input = _inputGeneration, inventory = _inventoryEpoch;
					const auto result = _encounter->reenter(completedMonsters, completedEvents, completedPreflight, [&] {
						if (!_gameplayBorrow->current() || _fatal || !_dispatching || _saving ||
							input != _inputGeneration || inventory != _inventoryEpoch || inventoryOpen())
							throw std::logic_error("Completed re-entry UI boundary changed");
					});
					_encounter->feedback("Re-entered " + std::to_string(result.oldGeneration) + " -> " + std::to_string(result.newGeneration) + "; (13,1) North");
					_ordinary = {0, 0, _camera.mapId, _camera.direction, false};
				} catch (const std::exception &) {
					if (_fatal || !_encounter->canSave()) { _fatal = true; _encounter->closeCompleted(); throw; }
					_encounter->feedback("Revisit failed; entry unchanged");
				}
				return renderEncounter(true);
			}
			return frameCopy();
		}
		if (combatPreparation()) {
			if (std::holds_alternative<CancelInteractionAction>(action)) return frameCopy();
			if (inventoryOpen() || std::holds_alternative<InspectInventoryAction>(action)) {
				const auto entry = _encounter->ticket();
				try {
					handleInventory(action);
					syncCombatInventory();
					return renderEncounter();
				} catch (...) { _encounter->fail(entry); throw; }
			}
		}
		const auto entry = _encounter->ticket();
		const bool changed = _encounter->handle(action, _cycle, _displayedCombat);
		prepareJourneyTransition();
		if (_encounter->combatOperationStale()) {
			_fatal = true;
			throw std::runtime_error("Stale combat input operation");
		}
		if (!_encounter->current(_encounter->ticket())) { _fatal = true; throw std::runtime_error("Stale encounter input"); }
		if (changed) {
			if (_encounter->combat() && std::holds_alternative<BeginEncounterAction>(action))
				_ordinary.deadline = _encounter->cosmeticDeadline();
			// Gameplay action and its distinct pulse are already adopted. Consume
			// this physical navigation's visual cause once, never during recovery.
			const auto &accepted = _encounter->actionResult();
			if (std::holds_alternative<NavigationAction>(action) &&
				((_encounter->combat() && _encounter->state().revision() > entry.state.revision()) ||
				(accepted.revision > entry.state.revision() &&
				(accepted.outcome == XeenEncounterOutcome::Accepted || accepted.outcome == XeenEncounterOutcome::Blocked))))
				advanceEncounterOrdinary(OrdinaryCause::Action);
			return renderEncounter(true);
		}
		return frameCopy();
	}
	const bool inventoryOnly = std::holds_alternative<InspectInventoryAction>(action) ||
		std::holds_alternative<SelectInventorySlotAction>(action) || std::holds_alternative<TransferInventoryAction>(action) ||
		std::holds_alternative<EquipmentInventoryAction>(action);
	if (_pending && inventoryOnly) return frameCopy();
	if (inventoryOpen() || std::holds_alternative<InspectInventoryAction>(action)) return handleInventory(action);
	if (inventoryOnly) return frameCopy();
	if (std::holds_alternative<SelectMemberAction>(action) && !canCancelInteraction())
		return frameCopy();
	const bool hadPending = _pending.has_value();
	refreshScene(false, OrdinaryCause::None);
	if (hadPending && !_pending) return frameCopy(); // Rebase failure must not dispatch this input.
	if (_pending) {
		const auto generation = _pending->generation;
		XeenPresentationUpdate update;
		try { update = _presenter.handle(action, false); }
		catch (const std::exception &e) {
			return presentationFailed(e);
		}
		_frame = std::move(update.frame);
		if (!update.response) return frameCopy();
		resumePending(generation, *update.response);
		return frameCopy();
	}
	_presenter.clear(); // M14 labels last until the next gameplay action.
	if (const auto *navigation = std::get_if<NavigationAction>(&action)) {
		const auto beforeMovement = _camera;
		auto result = _navigation.processNavigationAction(_world, _party, _camera,
			_flags, *navigation);
		// Recompose cleared labels even after blocked movement, but only after
		// drive adopts any suspension before refresh/report callbacks.
		const bool transition = beforeMovement.mapId != result.cameraAfterMovement.mapId ||
			beforeMovement.direction != result.cameraAfterMovement.direction;
		auto frame = drive(std::move(result.automaticEvent), true, true, OrdinaryCause::Action, transition);
		try { if (reportMovement) reportMovement(result.movementResult); }
		catch (const std::exception &e) { if (_pending) return presentationFailed(e); throw; }
		return frame;
	}
	if (std::holds_alternative<InteractionAction>(action))
		return drive(_navigation.processInteraction(_world, _party, _camera, _flags), false, true, OrdinaryCause::Action);
	refreshScene(true, OrdinaryCause::None);
	return frameCopy();
}
}
