#include "app/XeenEventFlow.h"
#include "games/xeen/XeenEventPublication.h"
#include "games/xeen/XeenVertigoRoute.h"
#include <type_traits>
#include <utility>
#include <iostream>
#include <chrono>
#include <limits>
#include <sstream>
#include <algorithm>

namespace mmodern {
namespace {
const XeenItemCatalog &fallbackCatalog() {
	static const auto catalog = XeenItemCatalog::unavailable();
	return catalog;
}
static_assert(std::is_nothrow_move_constructible<XeenEventExecutionState>::value,
	"Flow must adopt execution ownership without allocation");
static_assert(std::is_nothrow_copy_assignable<XeenMutableOptional<XeenEquipmentResult>>::value,
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

IndexedFrame noticeFrame(const IndexedFrame &base, const XeenFontFormat &font, const std::string &notice, bool roster, bool consequences = false) {
	XeenTextRenderOptions options;
	options.bounds = {9, 9, 222, 50}; options.x = 10; options.y = 10;
	options.size = XeenFontSize::Reduced;
	options.paginate = true;
	options.drawWindow = true; options.windowBounds = {8,8,223,51};
	if (roster) {
		options.bounds = {3,137,229,200}; options.windowBounds = {1,135,231,200};
		options.x = 3; options.y = 137;
	}
	if(consequences){options.bounds={3,107,229,200};options.windowBounds={1,105,231,200};options.y=107;}
	const auto split = roster ? notice.find("\n\n") : std::string::npos;
	auto rendered = XeenTextRenderer(font).render(base, notice.substr(0,split), options);
	if (rendered.pages.size() != 1) { std::cerr << "Encounter notice overflow: " << notice.substr(0,split) << std::endl; throw std::runtime_error("Encounter notice did not fit"); }
	if (split != std::string::npos) {
		options.bounds = {235,3,318,198}; options.windowBounds = {233,1,320,200};
		options.x=235; options.y=3;
		rendered = XeenTextRenderer(font).render(rendered.pages.front(),notice.substr(split+2),options);
		if (rendered.pages.size()!=1) { std::cerr << "Roster notice overflow: " << notice.substr(split+2) << std::endl; throw std::runtime_error("Combat roster notice did not fit"); }
	}
	return std::move(rendered.pages.front());
}
}

void XeenEventFlow::requireCurrentOwners() const {
	if (!_gameplayBorrow->current()) throw std::runtime_error("Stale Flow borrowed owner lifetime");
	if (_eventPublication) _eventPublication->check();
}

bool XeenEventFlow::canSave() const noexcept {
	return _gameplayBorrow->current() && !_fatal && !_dispatching && !_saving && !_handoffPending && !_transition && !_arrivalPending &&
		!inventoryOpen() && !_pending && !_equipmentSelection &&
		(!_encounter || (_encounter->canSave() && encounterFrameCurrent()));
}

XeenEventFlow::SaveBoundary XeenEventFlow::beginSave() {
	if (!canSave()) throw std::logic_error("Save boundary is unavailable");
	if (_saveOperation == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Save generation exhausted");
	SaveBoundary boundary;
	if (journey()) boundary.journey = _encounter->beginJourneySave();
	_saving = true;
	boundary.operation = ++_saveOperation;
	boundary.owner = this; boundary.generation = _generation;
	boundary.inventory = _inventoryEpoch; boundary.input = _inputGeneration;
	_saveBoundary = boundary;
	return boundary;
}
InputContext XeenEventFlow::inputContext(const IndexedFrame::Presentation &origin) {
    const auto *combat = _encounter ? _encounter->combat() : nullptr;
    // Panels are strict throughout their lifetime, including preparation/result work.
    const unsigned panel = _fatal ? 1 : _trainingUi ? 2 : _smithUi ? 3 : inventoryOpen() ? 4 :
        (_castingUi || (combat && combat->cast())) ? 5 :
        (_pending || _transition || (_encounter && _encounter->journeyEvent())) ? 6 :
        (_encounter && _encounter->monsterReward()) ? 7 :
        (combat && (combat->phase() == XeenCombatPhase::VictoryAwaitingEnd ||
            combat->phase() == XeenCombatPhase::Victory || combat->phase() == XeenCombatPhase::Disengaged ||
            combat->phase() == XeenCombatPhase::Defeat || combat->phase() == XeenCombatPhase::SupportStopped ||
            combat->phase() == XeenCombatPhase::Failed)) ? 8 :
        (_encounter && !_encounter->combat() && _encounter->state().phase() != XeenEncounterPhase::Exploring) ? 9 : 0;
    const QueueContext context{_camera.mapId, panel, combat,
        inventoryOpen() ? (_dialogError ? 5 : _statPopup ? 4 : _itemOption ? 3 : _sheet && !_itemsVisible ? 2 : 1) :
        _smithUi ? 100+unsigned(_smithUi->phase)*4+unsigned(_smithUi->mode)+(!_smithUi->feedback.empty()?1000:0) :
        _trainingUi ? 200+unsigned(_trainingUi->phase)+(!_trainingUi->feedback.empty()?1000:0) :
        _pending ? _pending->generation : 0};
    if (!_queueContext || !(*_queueContext == context)) {
        if (_queueContextId == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Input context exhausted");
        ++_queueContextId;
        _queueContext = context;
        _mainScreenNotice.clear();
    }
    const bool queueable = panel == 0 && (!_encounter || journey() || combat);
    bool ready = !_dispatching && !_fatal && !_saving && !_handoffPending && !_arrivalPending &&
        acceptsInputFrame(origin) && (!journey() || journeyInputCurrent(displayedInput()));
    if (queueable && _encounter) {
        ready = ready && !_encounter->_busy && !_encounter->projectilesPending() &&
            !_encounter->_shootIntent && !_encounter->_shoot && !_encounter->_castingSettlement &&
            !_encounter->_regionalAutomatic && !_encounter->_regionalWork &&
            _encounter->boundary().quiet();
        if (combat) ready = ready && combat->phase() == XeenCombatPhase::PlayerReady &&
            _displayedCombat && combat->current(*_displayedCombat) && !_encounter->_scheduleAfterFrame;
        else ready = ready && _encounter->journeyMutable();
    }
    return {_queueContextId, queueable, ready, journey() && queueable ?
        (combat ? MainScreen::Combat : MainScreen::Exploration) : MainScreen::None,_smithUi || _trainingUi ? serviceDialogInput() : characterDialogInput()};
}

IndexedFrame XeenEventFlow::drawMainScreenNotice(const IndexedFrame &base, const std::string &notice) const {
    // M46 keeps the existing temporary status text, but confines it to the
    // scene so the original buttons, Tab and portraits remain visible.
    XeenTextRenderOptions options;
    options.bounds={9,70,222,135}; options.windowBounds={8,68,223,136};
    options.x=10; options.y=70; options.size=XeenFontSize::Reduced;
    options.drawWindow=true; options.paginate=true;
    std::istringstream lines(notice.substr(0,notice.find("\n\n")));
    std::string text,line;
    while(std::getline(lines,line)) {
        // Keyboard help is already in the title/controls; target rows are drawn
        // in their original hit areas below. Keep actual gameplay feedback.
        if(line.rfind("Arrows move/turn",0)==0 || line.rfind("I inventory;",0)==0) continue;
        const auto start=!line.empty() && line.front()=='>' ? 1u : 0u;
        if(_encounter->combat() && line.size()>start+1 && line[start]>='1' && line[start]<='3' && line[start+1]==' ') continue;
        const auto controls=line.find(": Space/B;");
        if(controls!=std::string::npos) line=line.substr(0,controls)+": ready";
        if(!text.empty()) text+='\n';
        text+=line;
    }
    if (!_mainScreenNotice.empty()) text=_mainScreenNotice+"\n"+text;
    options.y=std::max(9,125-10*static_cast<int>(std::count(text.begin(),text.end(),'\n')));
    options.bounds.top=options.y; options.windowBounds.top=std::max(8,options.y-2);
    auto result=XeenTextRenderer(_inventoryFont).render(base,text,options);
    while(result.pages.size()!=1 && options.y>9) {
        // Allow wrapped feedback more room, keeping ordinary short notices
        // at the bottom of the scene and every control visible.
        options.y=std::max(9,options.y-10);
        options.bounds.top=options.y; options.windowBounds.top=std::max(8,options.y-2);
        result=XeenTextRenderer(_inventoryFont).render(base,text,options);
    }
    if(result.pages.size()!=1) throw std::runtime_error("Main screen notice overflow");
    auto frame=std::move(result.pages.front());
    if (const auto *combat=_encounter->combat()) {
        if (drawCombatButtons) drawCombatButtons(frame);
        // Original target rows are in window 2, above the action icons.
        options.bounds={235,11,318,69}; options.windowBounds={233,9,320,71};
        options.x=239; options.y=13;
        frame=XeenTextRenderer(_inventoryFont).render(frame,"Combat",options).pages.front();
        const auto rows=combat->contacts();
        for(unsigned row=0;row<rows.size();++row) if(rows[row]) {
            const auto &actor=_world.sessionState().regionalActors(rows[row]->mapId).at(rows[row]->recordIndex);
            auto label=std::string(rows[row]==combat->selectedTarget()?">":" ")+std::to_string(row+1)+" "+actor.statistics->name();
            const XeenTextRenderer renderer(_inventoryFont);
            while(renderer.textWidth(label,XeenFontSize::Reduced)>73) label.pop_back();
            options.bounds={239,27+int(row)*10,312,37+int(row)*10};
            options.x=239; options.y=options.bounds.top; options.drawWindow=false;
            frame=renderer.render(frame,label,options).pages.front();
        }
    }
    return frame;
}

bool XeenEventFlow::journeyInputCurrent(std::optional<std::uint64_t> input) const noexcept {
	return journey() && input && *input == _inputGeneration && !_handoffPending && !_fatal && !_dispatching && !_saving && encounterFrameCurrent();
}
void XeenEventFlow::prepareJourneyTransition() {
 if (!journey()) return;
 if (_encounter->combat() && (_encounter->combat()->phase()==XeenCombatPhase::Victory ||
  _encounter->combat()->phase()==XeenCombatPhase::Disengaged)) {
  if(_encounter->projectilesPending()) return;
  if(!_encounter->retireJourney(_encounter->ticket())) throw std::runtime_error("Journey retirement failed");
  _queueContext.reset(); // Combat retirement, even if the next attachment reuses storage.
  _displayedCombat.reset();
 }
 if(!_encounter->combat() && _world.sessionState().journeyActivity()==XeenJourneyActivity::Attachment) {
  _queueContext.reset(); // New combat incarnation.
  closeInventory();
  if(!_encounter->attachJourney(_encounter->ticket(),prepareJourneySprites)) throw std::runtime_error("Journey attachment failed");
 }
 _encounter->beginMonsterReward(_catalog);
 if(!_encounter->combat() && _encounter->_regionalAutomatic && !_encounter->_regionalWork && !_encounter->_shoot &&
  !_encounter->monsterReward() && !_encounter->state().pending() && _encounter->state().phase()==XeenEncounterPhase::Exploring) {
  _encounter->beginJourneyEvent();_journeyEventLayers=true;
  journeyEventWork([&] {
   if(xeenRegionalInteraction(_encounter->_journeyEvents,_camera)==XeenRegionalInteraction::VertigoDoor)
    drive(beginVertigoEvent(XeenRegionalInteraction::VertigoDoor),false);
   else drive(_events.runAutomaticEvent(_world,_party,_camera,_flags,_eventPublication),true);
  },true);
 }
 if(_encounter->_shootIntent && !_encounter->state().pending() && !_encounter->_regionalWork && !_encounter->projectilesPending()) _encounter->beginShoot();
}

bool XeenEventFlow::saveCurrent(const SaveBoundary &b) const noexcept {
	return b.owner == this && _saving && !_fatal && !_dispatching && _gameplayBorrow->current() &&
		b.operation == _saveOperation && (!b.journey || _encounter->journeySaveCurrent(*b.journey)) &&
		b.generation == _generation && b.inventory == _inventoryEpoch && b.input == _inputGeneration &&
		!inventoryOpen() && !_pending && !_equipmentSelection;
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
	_saving = false;
	_saveBoundary.reset();
}

void XeenEventFlow::framePresented(const IndexedFrame::Presentation &presented, bool deferCosmeticInput) {
	if (!acceptsFrame(presented)) throw std::logic_error("Unmatched concrete frame handoff");
	requireCurrentOwners();
	if (_dispatching || _saving) throw std::logic_error("Frame handoff during dispatch");
	if (!encounterFrameCurrent()) throw std::logic_error("Stale successful frame handoff");
	if (_cosmeticPending) {
		// A remains actionable through composition, upload and successful B
		// acquisition. Native SDL drains its last A-origin batch before switching.
		_acquiredCosmeticFrame = presented;
		if (!deferCosmeticInput) completeInputHandoff(presented);
		return;
	}
	if (journey() && _handoffPending) {
		if (_encounter->combat()) {
			_encounter->presented(*_encounterFrame);
			// Contact can attach combat before the cast's ranged visuals finish.
			// Release only on the concrete presentation after their final service step.
			if (!_encounter->projectilesPending()) _encounter->_castingSettlement=false;
		}
		else if (!_encounter->presentJourney(*_encounterFrame)) throw std::logic_error("Stale Journey frame handoff");
		_handoffPending = false; _encounterFrame = _encounter->ticket();
		_arrivalPending = false;
		authorizeInputFrame(presented);
	}

	_actionableFrame = presented;
}

void XeenEventFlow::completeInputHandoff(const IndexedFrame::Presentation &presented) {
	// An A action may have superseded B during SDL's bounded queue drain.
	if (!_cosmeticPending || _acquiredCosmeticFrame != presented || !acceptsFrame(presented)) return;
	requireCurrentOwners();
	if (_dispatching || _saving || _handoffPending) throw std::logic_error("Input handoff during dispatch");
	authorizeInputFrame(presented);
	_actionableFrame = presented;
	_cosmeticPending = false;
	_acquiredCosmeticFrame.reset();
}

void XeenEventFlow::authorizeInputFrame(const IndexedFrame::Presentation &presented) {
	if (journey()) {
		if (_encounter->combat())
			_encounter->authorizeCombatCastFrame(*_encounterFrame,_inputGeneration,presented);
		if (_castingUi && _encounter->castingActive())
			_encounter->authorizeCastingFrame(*_encounterFrame,_inputGeneration,presented);
		if (_smithUi && _encounter->_smith)
			_encounter->authorizeSmithFrame(_inputGeneration,presented);
		if (_trainingUi && _encounter->_training)
			_encounter->authorizeTrainingFrame(_inputGeneration,presented);
		if (_inventory.mode==XeenInventoryMode::UseTarget &&
			(!_itemUseGeneration || !_encounter->authorizeItemUseTarget(*_encounterFrame,*_itemUseGeneration,
				_inventoryEpoch,_inputGeneration,presented))) {
			closeGameplay();
			throw std::logic_error("Antidote target frame authority unavailable");
		}
	}
}

void XeenEventFlow::closeGameplay() noexcept {
	if (_gameplayBorrow->current() && journey()) _encounter->fail(_encounter->ticket());
	_fatal = true;
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

		_encounter->fail(entry);
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
	if (cause!=OrdinaryCause::None) _world.scenePresentation().advance(_camera.mapId);
	_ordinary.deadline = now + 100;
	if (reset) {
		_ordinary.mapId = _camera.mapId;
		_ordinary.direction = _camera.direction;
	}
	return true;
}

bool XeenEventFlow::advanceEncounterOrdinary(OrdinaryCause cause) {
	if (cause == OrdinaryCause::Idle && (_encounter->terminal())) return false;
	const auto entry = _encounter->ticket();
	try {
		std::uint64_t now;
		_encounter->guardCallback(entry,[&] { now = _clock(); });
		if (!_encounter->current(entry)) throw std::runtime_error("Stale ordinary animation callback");
		const bool reset = _ordinary.mapId != _camera.mapId || _ordinary.direction != _camera.direction;
		return updateOrdinaryPhase(cause, reset, now) && _ordinary.containsOrdinaryAnimation;
	} catch (...) {
		if (!_encounter->fail(entry, XeenEncounterStop::Preparation)) { _fatal = true; throw; }
		return true;
	}
}

void XeenEventFlow::sealFrame(IndexedFrame &returned) {
	inputContext({}); // Observe every published context, including intermediate panels.
	// Keeping old snapshots alive prevents pointer reuse/ABA; another Flow
	// (even at the same address) cannot issue this identity.
	auto snapshot = std::make_shared<IndexedFrame>(_frame);
	snapshot->_presentation.reset(); // No chain of previous frames.
	_frame._presentation = std::move(snapshot);
	returned._presentation = _frame._presentation;
}

IndexedFrame XeenEventFlow::renderEncounter(bool report, bool cosmeticInput) {
	if (_smithUi) {
		cosmeticInput=_smithRenderedRevision && *_smithRenderedRevision==_smithUi->revision;
		if (_encounter->_smith && _encounter->_smith->frame &&
			(!xeenSmithAuthorityRoom(_inputGeneration,8) || !_encounter->smithCapacity(16,4))) return frameCopy();
	}
	if(_trainingUi) {
		cosmeticInput=_trainingRenderedRevision && *_trainingRenderedRevision==_trainingUi->revision;
		if(_encounter->_training && _encounter->_training->frame &&
			(!xeenSmithAuthorityRoom(_inputGeneration,8) || !_encounter->smithCapacity(16,4)))return frameCopy();
	}

	// A consumed Training frame is a strict retry even if its phase/revision
	// survived an exception. Only an unconsumed, acquired origin can overlap B.
	const bool cosmetic = (journey() || _encounter->combat()) && cosmeticInput && _actionableFrame &&
		!_handoffPending && encounterFrameCurrent() &&
		(!_trainingUi || (_encounter->_training && _encounter->_training->frame == _actionableFrame)) &&
		(!_smithUi || (_encounter->_smith && _encounter->_smith->frame == _actionableFrame));
	_acquiredCosmeticFrame.reset();
	if (!cosmetic) { _actionableFrame.reset(); _cosmeticPending = false; }
	if (journey() && !_encounter->combat()) {
		report=report || _encounter->state().phase()==XeenEncounterPhase::SupportStopped;
		if (!cosmetic) _encounter->holdJourneyFrame();
		for (unsigned attempt=0;attempt<2;++attempt) {
			IndexedFrame returned;
			const auto t = _encounter->ticket();
			if (_encounter->prepareJourneyFrame(t,[&] {
				if (attempt) { _world.discardMapCache(); if (rebuildEncounterPresentation) rebuildEncounterPresentation(); }
				validateRegionalEvents();
				if (_journeyEventLayers) {
					XeenEventPublication validation(_encounter->journeySavePreimage(),_encounter->_journeyEvents,[&] {
						if (!_encounter->current(t)) throw std::logic_error("Stale objective reconstruction");
					});
					validation.script(_events.scriptForMap(_camera.mapId).file());
					_events.textForMap(_camera.mapId); validation.check();
				}
				_world.scenePresentation().appearance(_encounter->appearance());
				auto composed = _encounterCompose(_ordinary.phase,_encounter->appearance());
				if (!_encounter->current(t)) throw std::logic_error("Stale Journey composition");
				if (!composed.frame.isValid()) throw std::runtime_error("Invalid Journey frame");
				auto rendered = _journeyEventLayers ? _presenter.rebase(composed.frame) :
                    _castingUi ? noticeFrame(composed.frame,_inventoryFont,castingText(),true,true) :
                    drawMainScreenNotice(composed.frame,_encounter->notice());
				if(_encounter->monsterReward()) {
					if(!_monsterReceiptPresented) {
						_presenter.clear();XeenPresentationRequest request;
						request.kind=XeenPresentationKind::RewardReceipt;request.response=XeenPresentationResponseRequirement::Acknowledgment;
						request.mapId=_camera.mapId;request.text=_encounter->_monsterReceiptText;
						rendered=_presenter.present(composed.frame,request).frame;_monsterReceiptPresented=true;
					} else rendered=_presenter.rebase(composed.frame);
				}
				_inventoryUnderlay = rendered;
				if (inventoryOpen()) rendered = drawCharacterDialog(rendered);
				if (_smithUi) rendered=drawSmith(composed.frame);
				if (_trainingUi) rendered=drawTraining(composed.frame);
				if (report && !attempt) {
					if (reportText) reportText(_encounter->notice());
					if (!_encounter->current(t)) throw std::logic_error("Stale Journey reporting");
					std::cout << _encounter->journeyInspection();
				}
				if (!attempt && beforeEncounterFrameCopy) beforeEncounterFrameCopy();
				if (!_encounter->current(t)) throw std::logic_error("Stale Journey frame copy");
				returned = rendered; _frame = std::move(rendered);
				_ordinary.containsOrdinaryAnimation = composed.containsOrdinaryAnimation;
				sealFrame(returned);
			}, cosmetic)) {
				if (_inputGeneration == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Journey input generation exhausted");
				// A cosmetic frame renews frame authority, not the semantic input epoch.
				if (!cosmeticInput) ++_inputGeneration;
				if(_trainingUi)_trainingRenderedRevision=_trainingUi->revision;
				if(_smithUi)_smithRenderedRevision=_smithUi->revision;
				_encounterFrame = _encounter->ticket(); _handoffPending = !cosmetic;
				_cosmeticPending = cosmetic;
				return returned;
			}
			if (!_encounter->current(t)) break;
		}
		_fatal = true; throw std::runtime_error("Journey presentation recovery failed");
	}
	const bool hadFrame = _frame.isValid();
	for (unsigned attempt = 0; attempt < 2; ++attempt) {
		const auto entry = _encounter->ticket();
		try {
            IndexedFrame returned;
            const auto prepare = [&] {
			if (!_encounter->current(entry)) throw std::runtime_error("Stale encounter composition");
			if (attempt) {
				_world.discardMapCache();
				if (rebuildEncounterPresentation) rebuildEncounterPresentation();
				if (!_encounter->current(entry)) throw std::runtime_error("Stale encounter rebuild");
			}
			_world.scenePresentation().appearance(_encounter->appearance());
			const auto composed=_encounterCompose(_ordinary.phase,_encounter->appearance());
			if (!_encounter->current(entry)) throw std::runtime_error("Stale encounter frame");
			if (!composed.frame.isValid()) throw std::runtime_error("Invalid encounter frame");
			const auto notice = _encounter->combat() && _encounter->combat()->cast() ? combatCastingText() : _encounter->notice();
			auto rendered = journey() && _encounter->combat() && !_encounter->combat()->cast() ?
                drawMainScreenNotice(composed.frame,notice) :
                noticeFrame(composed.frame, _inventoryFont, notice, _encounter->combat(),journey());
			if (report && !attempt && reportText) reportText(notice);
			if (!_encounter->current(entry)) throw std::runtime_error("Stale encounter report");
			// Complete the fallible return copy before installing the frame.
			if (!attempt && beforeEncounterFrameCopy) beforeEncounterFrameCopy();
			if (!_encounter->current(entry)) throw std::runtime_error("Stale encounter frame copy");
			returned = rendered;
			_frame = std::move(rendered);
			if (_encounter->combat()) {
				_inventoryUnderlay = _frame;
				if (inventoryOpen()) {
					_frame = drawCharacterDialog(_inventoryUnderlay);
					returned = _frame;
				}
				if (_encounter->combat() && ((journey() && !cosmeticInput) || !_displayedCombat || !_encounter->combat()->current(*_displayedCombat))) {
					if (_inputGeneration == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Combat input generation exhausted");
					++_inputGeneration;
					_displayedCombat = _encounter->combat()->ticket();
				}
				if (!journey()) _encounter->presented(entry);
			}
			_ordinary.containsOrdinaryAnimation = composed.containsOrdinaryAnimation;
			_encounterFrame = entry;
			_cosmeticPending = cosmetic;
			if (journey()) _handoffPending = !cosmetic;
			sealFrame(returned);
            };
            if(journey() && _encounter->combat()) _encounter->combat()->preparePresentation(*entry.combat,prepare);
            else prepare();
			return returned;
		} catch (...) {
			if (journey() && !attempt && hadFrame && _encounter->current(entry)) continue;
			if (!_encounter->fail(entry) || attempt || !hadFrame) { _fatal = true; throw; }
			if (_encounter->combat()) closeInventory();
		}
	}
	throw std::logic_error("Unreachable encounter recovery");
}

XeenEventFlow::XeenEventFlow(XeenWorld &world, XeenEventSystem &events,
		XeenPartyState &party, XeenCamera &camera, XeenGameFlags &flags,
		const XeenFontFormat &font, Compose compose, XeenEventPresenter::NpcDraw npcDraw,
		XeenEventPresenter::Clock clock, XeenEventPresenter::RandomFrame randomFrame, const XeenItemCatalog *catalog,
		EncounterCompose encounterCompose, const XeenJourneySetup *journey,
		TransitionCompose transitionCompose) :
	_transitionCompose(std::move(transitionCompose)),
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
		if (!encounterCompose) throw std::invalid_argument("Journey requires exclusive presentation setup");
		_encounterCompose = std::move(encounterCompose);
		if (journey) _encounter = std::make_unique<XeenEncounterFlow>(world,party,camera,flags,_clock,*journey);
		else _encounter = std::make_unique<XeenEncounterFlow>(world,party,camera,flags,_clock,XeenJourneyRestoreTag{});
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
	if (journey() && _eventPublication) {
		auto composition=_encounterCompose(_ordinary.phase,_encounter->appearance());
		requireCurrentOwners();
		_frame=_presenter.rebase(composition.frame);
		requireCurrentOwners();
		_ordinary.containsOrdinaryAnimation=composition.containsOrdinaryAnimation;
		return true;
	}
	if (_encounter && (journey() || _encounter->combat())) { if (!journey()) renderEncounter(); return true; }
	try {
	const bool reset = committedTransition || _ordinary.mapId != _camera.mapId ||
		_ordinary.direction != _camera.direction;
	bool stepped = false;
	if (reset || cause != OrdinaryCause::None)
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
		if constexpr (std::is_same_v<Result, XeenManualEventResult>)
			if (_transition) prepareVertigoResult(result);
		// Adopt ownership before composition, reporting or presentation can fail.
		auto *suspended = std::get_if<XeenEventExecutionSuspended>(&result);
		if (suspended && journey() && (suspended->request.kind==XeenPresentationKind::ArmorRepairService ||
			 suspended->request.kind==XeenPresentationKind::TempleService) && !xeenSmithAuthorityRoom(_generation,1))
			throw std::overflow_error("Smith Event ticket generation exhausted before preparation");
		if (suspended) _pending.emplace(Pending{std::move(suspended->state), automatic, ++_generation});
		if (suspended && (suspended->request.kind==XeenPresentationKind::ArmorRepairService ||
			suspended->request.kind==XeenPresentationKind::TrainingService ||
			suspended->request.kind==XeenPresentationKind::TempleService))
			return frameCopy(); // Exclusive Event transfers to Service after its final guard check.
		try {
		refreshScene(reconstruct, cause, committedTransition);
		reconstruct = false;
		cause = OrdinaryCause::None;
		committedTransition = false;
		if (suspended && !_pending) return frameCopy();
		// Reporting receives a value snapshot; Flow has already adopted ownership
		// and can account for it if constructing this snapshot fails.
		if (suspended) suspended->state = _pending->state;
		if (journey() && automatic && std::holds_alternative<XeenEventExecutionError>(result))
			throw std::runtime_error("Automatic Journey event failed");
		if constexpr (std::is_same_v<Result, XeenManualEventResult>) {
			if (reportManual) reportManual(result);
		} else {
			if (reportAutomatic) reportAutomatic(result);
		}
		requireCurrentOwners();
		if (!suspended) return frameCopy();
		if (!_pending) return frameCopy(); // A reporting callback may explicitly abandon.
		_frame = _presenter.dismissSelection();
		XeenPresentationUpdate update;
		update = _presenter.present(_frame, _pending->state.pendingPresentation->request);
		_frame = std::move(update.frame);
		if (reportText) for (const auto &message : _presenter.diagnostics()) { reportText(message); requireCurrentOwners(); }
		requireCurrentOwners();
		if (!update.response) return frameCopy();
		auto pending = std::move(*_pending);
		_pending.reset(); // Consume before calling into the execution system.
		if constexpr (std::is_same_v<Result, XeenManualEventResult>)
			result = _transition ? resumeVertigoEvent(std::move(pending.state),*update.response) :
				_events.resumeManualEvent(std::move(pending.state), *update.response,
					_world, _party, _camera, _flags, _eventPublication);
		else
			result = _events.resumeAutomaticEvent(std::move(pending.state), *update.response,
				_world, _party, _camera, _flags, _eventPublication);
		} catch (const std::exception &e) {
			if (!_pending) throw;
			requireCurrentOwners();
			return presentationFailed(e);
		}
	}
}

void XeenEventFlow::checkTransitionCandidate() {
	if (!_transition || !_transition->guard) return;
	try { _transition->guard->check(); }
	catch (...) { _encounter->journeySavePreimage().failed=true; throw; }
}

XeenManualEventResult XeenEventFlow::beginVertigoEvent(XeenRegionalInteraction kind) try {
	if (_transition || !_eventPublication || !_encounter->journeyEvent() ||
		!_transitionCompose)
		throw std::logic_error("Vertigo Event candidate is unavailable");
	auto work=std::make_unique<TransitionCandidate>();
	const auto mainland=_events.scriptForMap(23).file();
	const auto city=_events.scriptForMap(28).file();
	_eventPublication->check();
	try { xeenValidateVertigoRoute(mainland,city); }
	catch (const std::invalid_argument &) { _encounter->journeySavePreimage().failed=true; throw; }
	const auto mainText=_events.textForMap(23),cityText=_events.textForMap(28);
	_eventPublication->check();
	if(mainText.mapId!=XeenMapIdentity(23) || mainText.strings.size()!=44 ||
		cityText.mapId!=XeenMapIdentity(28) || cityText.strings.size()!=64)
		throw std::invalid_argument("Vertigo original text catalog changed");
	_eventPublication->text(mainText);_eventPublication->text(cityText);
	work->kind=kind;
	work->world=_world.transitionCandidate();
	work->camera=_camera;work->flags=_flags;
	work->party.party=XeenParty::fromRosterIds(_party.party.activeRosterIds());
	for(unsigned i=0;i<XeenRoster::kCharacterCount;++i)work->party.roster.at(i)=_party.roster.at(i);
	work->party.encounterContext=_party.encounterContext;
	work->party.monsterTreasure=_party.monsterTreasure;
	work->party.serviceEconomy=_party.serviceEconomy;
	work->party.questItems=_party.questItems;work->party.questFlags=_party.questFlags;
	work->party.regionalRecovery=_party.regionalRecovery;
	work->party.firstSerializedCount=_party.firstSerializedCount;
	work->party.effectiveSerializedCount=_party.effectiveSerializedCount;
	work->party.diagnostics=_party.diagnostics;
	{
		XeenRestoreGuard guard(*work->world,work->party,work->camera,work->flags);
		XeenRestoreGuard::Providers providers(guard,*work->world,[&] {_eventPublication->check();});
		if (!_encounter->_vertigoManifest) throw std::invalid_argument("Vertigo immutable manifest provider missing");
		try { _encounter->_vertigoManifest(*work->world,city,_encounter->_journeyStatistics); }
		catch (...) { guard.check();_eventPublication->check();throw; }
		guard.check();_eventPublication->check();
		work->guard=std::make_unique<XeenRestoreGuard>(*work->world,work->party,work->camera,work->flags);
		work->guard->retainResources(guard);
	}
	const auto destination=kind==XeenRegionalInteraction::VertigoExit ? XeenMapIdentity(23) : XeenMapIdentity(28);
	work->destinationEvents=_events.scriptForMap(destination).file();
	_eventPublication->check();
	work->guard->check();
	{
		XeenRestoreGuard::EventProviders providers(*work->world,work->party,work->camera,work->flags,
			*work->guard,[&] {_eventPublication->check();},
			[&] {_encounter->journeySavePreimage().failed=true;});
		for(int y=0;y<32;++y)for(int x=0;x<32;++x)
			if(!work->world->sampleCell(28,x,y))throw std::invalid_argument("Vertigo geometry tile is missing");
		const auto &cityMob=work->world->objectFile(28);
		if(cityMob.entities.objects.size()!=143 || cityMob.entities.monsters.size()!=46)
			throw std::invalid_argument("Vertigo original MOB catalog changed");
		if(kind==XeenRegionalInteraction::VertigoEntrance && !work->world->sessionState().hasRegionalActors(28))
			work->world->stageVertigoActors(cityMob,_encounter->_journeyStatistics);
	}
	_eventPublication->check();
	_transition=std::move(work);
	XeenRestoreGuard::EventProviders providers(*_transition->world,_transition->party,_transition->camera,
		_transition->flags,*_transition->guard,[&] {_eventPublication->check();},
		[&] {_encounter->journeySavePreimage().failed=true;});
	auto result=_events.runManualEvent(*_transition->world,_transition->party,_transition->camera,
		_transition->flags,nullptr);
	auto prepared=std::make_unique<XeenRestoreGuard>(*_transition->world,_transition->party,_transition->camera,_transition->flags);
	prepared->retainResources(*_transition->guard);_transition->guard.swap(prepared);
	return result;
} catch (const std::logic_error &) {
	_encounter->journeySavePreimage().failed=true;
	throw;
}

XeenManualEventResult XeenEventFlow::resumeVertigoEvent(XeenEventExecutionState state,
		XeenPresentationResponse response) {
	if (!_transition || !_eventPublication) throw std::logic_error("Vertigo Event continuation is absent");
	_eventPublication->check();
	checkTransitionCandidate();
	if(response==XeenPresentationResponse::No)_transition->refused=true;
	XeenRestoreGuard::EventProviders providers(*_transition->world,_transition->party,_transition->camera,
		_transition->flags,*_transition->guard,[&] {_eventPublication->check();},
		[&] {_encounter->journeySavePreimage().failed=true;});
	auto result=_events.resumeManualEvent(std::move(state),response,*_transition->world,
		_transition->party,_transition->camera,_transition->flags,nullptr);
	_eventPublication->check();
	auto prepared=std::make_unique<XeenRestoreGuard>(*_transition->world,_transition->party,_transition->camera,_transition->flags);
	prepared->retainResources(*_transition->guard);_transition->guard.swap(prepared);
	return result;
}

void XeenEventFlow::prepareVertigoResult(const XeenManualEventResult &result) try {
	if (!_transition || !_eventPublication) return;
	checkTransitionCandidate();
	auto &work=*_transition;
	_eventPublication->check();
	if (const auto *suspended=std::get_if<XeenEventExecutionSuspended>(&result)) {
		if(work.kind==XeenRegionalInteraction::VertigoExit && !work.preludePublished &&
			suspended->state.logicalAddress.mapId==XeenMapIdentity(28) &&
			suspended->state.logicalAddress.x==15 && suspended->state.logicalAddress.y==0 &&
			suspended->state.logicalAddress.line==2 && suspended->state.callStack.empty()) {
			// Return from the original flag prelude is its own publication boundary.
			_encounter->journeySavePreimage().prepareVertigoPrelude(suspended->state.workingGameFlags);
			_flags=suspended->state.workingGameFlags;
			_encounter->journeySavePreimage().adoptMutationBoundary();
			work.preludePublished=true;
		}
		return;
	}
	if (std::holds_alternative<XeenEventExecutionError>(result)) { _transition.reset(); return; }
	const auto *done=std::get_if<XeenManualEventCompleted>(&result);
	if (!done) throw std::logic_error("Vertigo Event did not reach a terminal result");
	if (work.camera.mapId==_camera.mapId) {
		if(done->cameraChanged ||
			(work.kind!=XeenRegionalInteraction::VertigoDoor && !work.refused) ||
			(work.kind==XeenRegionalInteraction::VertigoExit && !work.preludePublished))
			throw std::logic_error("Vertigo Event ended without its required teleport");
		_transition.reset();return;
	}
	const auto expected=work.kind==XeenRegionalInteraction::VertigoExit ? XeenMapIdentity(23) : XeenMapIdentity(28);
	if(work.camera.mapId!=expected || !done->cameraChanged ||
		(work.kind==XeenRegionalInteraction::VertigoExit && !work.preludePublished))
		throw std::logic_error("Vertigo Event ended at an unexpected destination");
	const auto before=_encounter->ticket();
	const auto arrival=[&] {
		XeenRestoreGuard::EventProviders providers(*work.world,work.party,work.camera,work.flags,
			*work.guard,[&] {_eventPublication->check();},
			[&] {_encounter->journeySavePreimage().failed=true;});
		if(work.world->sessionState().hasRegionalActors(28))
			xeenValidateVertigoActors(*work.world,work.world->sessionState().regionalActors(28));
		return work.world->prepareTransitionArrival(work.camera);
	}();
	_eventPublication->check();
	XeenRestoreGuard candidateGuard(*work.world,work.party,work.camera,work.flags);
	candidateGuard.retainResources(*work.guard);
	XeenRestoreGuard::Providers providers(candidateGuard,*work.world,[&] {_eventPublication->check();});
	const auto composed=[&] {
		try {
			auto frame=_transitionCompose(*work.world,work.party,work.camera,0,XeenMonsterAppearance{0});
			candidateGuard.check();return frame;
		} catch (...) { candidateGuard.check();throw; }
	}();
	if(!composed.frame.isValid())throw std::runtime_error("Vertigo destination frame is invalid");
	_eventPublication->check();
	if(!_encounter->current(before))throw std::logic_error("Stale Vertigo preparation");
	// Storage and the destination frame are ready. No provider or callback follows
	// until all owner writes and the new frame authority have been installed.
	_encounter->journeySavePreimage().prepareVertigoPublication(candidateGuard);
	_world.publishTransition(*work.world);
	_camera=work.camera;_flags=work.flags;
	// Retain the classified destination under the Event lease. Keep the Event's
	// concrete ticket intact until its final provider/publication checks finish;
	// contact changes the encounter phase at callback-free retirement below.
	_encounter->_result.view=arrival;
	using std::swap;
	swap(_encounter->_journeyEvents,work.destinationEvents);
	_encounter->journeySavePreimage().adoptMutationBoundary();
	_presenter.clear();_journeyEventLayers=false;
	_arrivalPending=true;
	_transition.reset();
} catch (const std::logic_error &) {
	_encounter->journeySavePreimage().failed=true;
	throw;
}

void XeenEventFlow::validateRegionalEvents() {
	if (!journey()) return;
	const auto entry=_encounter->ticket();
	XeenRestoreGuard currentCombat(_world,_party,_camera,_flags);
	currentCombat.retainResources(_encounter->journeySavePreimage());
	auto &guard=_encounter->combat()?currentCombat:_encounter->journeySavePreimage();
	{
		if (!_encounter->_learnedNamesProvider) throw std::logic_error("Learned spell name provider unavailable");
		guard.check();
		const auto value=_encounter->_learnedNamesProvider();
		_encounter->journeySavePreimage().verifyLearnedSpellNames(value);
		guard.admitLearnedSpellNames(value);
	}
	XeenEventPublication validation(guard,_encounter->_journeyEvents,[&] {
		if (!_encounter->current(entry)) throw std::logic_error("Stale regional event resources");
	});
	XeenRestoreGuard::Providers providers(guard,_world,[&] {validation.check();});
	validation.script(_events.scriptForMap(_camera.mapId).file());
	validation.text(_events.textForMap(_camera.mapId));
}
IndexedFrame XeenEventFlow::journeyEventWork(const std::function<void()> &operation, bool automatic) {
	automatic=automatic || (_pending && _pending->automatic);
	const auto entry=_encounter->ticket();
	try {
		XeenEventPublication publication(_encounter->journeySavePreimage(),_encounter->_journeyEvents,[&] {
			if (!_dispatching || !_encounter->journeyEvent() || !_encounter->current(entry))
				throw std::logic_error("Stale Journey event continuation");
		},beforeRewardEnqueue);
		XeenRestoreGuard::Providers providers(_encounter->journeySavePreimage(),_world,[&] { publication.check(); });
		_eventPublication=&publication;
		try { operation(); publication.check(); }
		catch (...) { _eventPublication=nullptr; throw; }
		_eventPublication=nullptr;
	} catch (const std::exception &error) {
		std::cerr << "Journey event: " << error.what() << '\n';
		_eventPublication=nullptr;
		if (_smithSettlement || _trainingSettlement) {
			if (_fatal || !_encounter->current(entry)) { _fatal=true;throw; }
			if(_smithUi)_smithUi->feedback="Event settlement pending; Enter retries.";
			if(_trainingUi)_trainingUi->feedback="Event settlement pending; Enter retries.";
			return renderEncounter();
		}
		cleanup(XeenRewardDiscard::PresentationFailure);
		_transition.reset();
		if (automatic) { _fatal=true;_encounter->fail(_encounter->ticket());throw; }
		if (!_encounter->current(entry)) { _fatal=true; throw; }
		// Only trusted published effects survive. A recovery frame never resumes the script.
	}
	if (_pending && _pending->state.pendingPresentation &&
		_pending->state.pendingPresentation->request.kind==XeenPresentationKind::ArmorRepairService && !_smithUi)
		prepareSmith();
	if (_pending && _pending->state.pendingPresentation &&
		_pending->state.pendingPresentation->request.kind==XeenPresentationKind::TempleService && !_smithUi)
		prepareSmith();
	if (_pending && _pending->state.pendingPresentation &&
		_pending->state.pendingPresentation->request.kind==XeenPresentationKind::TrainingService && !_trainingUi)
		prepareTraining();
	if (!_pending) {
		if (_arrivalPending) _encounter->publishArrival(_encounter->_result.view);
		_encounter->endJourneyEvent();
	}
	if (_arrivalPending) prepareJourneyTransition();
	return renderEncounter();
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
	if (_smithUi || _trainingUi) return true;
	if ((_encounter && _encounter->combat() && _encounter->combat()->cast()) || _castingUi || (journey() && _encounter->castingSettlement())) return true;
	return _pending && _pending->state.pendingPresentation &&
		_pending->state.pendingPresentation->request.response ==
			XeenPresentationResponseRequirement::CharacterSelection;
}
bool XeenEventFlow::pendingNpc() const {
	return _pending && _pending->state.pendingPresentation &&
		_pending->state.pendingPresentation->request.kind == XeenPresentationKind::NpcAcknowledgment;
}
bool XeenEventFlow::handlesEscape() const {
	// This is routing, not response authority: handle() still requires the exact
	// presented frame. An unpresented modal must never turn Escape into exit.
	if (journey()) return !_fatal && (_handoffPending || inventoryOpen() || canCancelInteraction() || _encounter->monsterReward());
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
	if(_trainingUi)return updateTraining();
	if (_smithUi && _smithUi->phase==SmithUi::Phase::Upgrade && !_dispatching && !_fatal && !_saving) {
		DispatchScope dispatch(_dispatching);
		try {
			if(!xeenSmithAuthorityRoom(_smithUi->revision,1))
				throw std::overflow_error("Temple UI revision exhausted before Heal publication");
			if(!_encounter->serviceTempleHeal())return std::nullopt;
			_smithUi->phase=SmithUi::Phase::Lobby;
            if(!_encounter->_smith->published) _smithUi->feedback="Heal: not supported yet at this date or capacity";
		} catch(const std::exception &) {
			_encounter->journeySavePreimage().check();
			if(_encounter->_smith && _encounter->_smith->published) {
				_smithUi->phase=SmithUi::Phase::Lobby;
				++_smithUi->revision;
				return renderEncounter();
			}
			return std::nullopt;
		}
		++_smithUi->revision;
		return renderEncounter();
	}
	if (_smithUi && _smithUi->phase==SmithUi::Phase::Preparation && !_dispatching && !_fatal && !_saving) {
		DispatchScope dispatch(_dispatching);
		try {
			if (!xeenSmithAuthorityRoom(_inputGeneration,9))
				throw std::overflow_error("Smith admission would consume mandatory input authority");
			if (!_encounter->serviceSmithPreparation()) return std::nullopt;
			_smithUi->phase=SmithUi::Phase::Lobby;
		} catch (const std::exception &error) {
			_encounter->journeySavePreimage().check();
			if (_encounter->_smith) {
				_smithUi->phase=SmithUi::Phase::Lobby;
				_smithUi->feedback="Retry; one-day departure still owed.";
			} else {
				_encounter->_smithPreparation.reset();_smithUi.reset();_pending.reset();
				_encounter->_journeyRefusal=std::string("Ironworks preparation refused: ")+error.what();
				_encounter->endJourneyEvent();
			}
		}
		if (_smithUi) ++_smithUi->revision;
		return renderEncounter();
	}
	if (_dispatching || _fatal || _saving || _smithUi || (journey() && _handoffPending)) return std::nullopt;
	DispatchScope dispatch(_dispatching);
	if(inventoryOpen()) {
		if(_sheet && !_itemsVisible && !_statPopup && !_dialogError && _clock()>=_sheet->deadline) {
			_sheet->deadline=_clock()+200;_sheet->blink=!_sheet->blink;
			if(_encounter) return renderEncounter(false,true);
			drawInventory();return _frame;
		}
		return std::nullopt;
	}
	validateRegionalEvents();
	if (_encounter) {
		if (journey() && _encounter->journeyEvent()) {
			const auto before = _encounter->ticket();
			if (advanceEncounterOrdinary()) return renderEncounter(false,
				_encounter->journeyEvent() && _encounter->current(before));
			return std::nullopt;
		}
		const auto beforeIdle = _encounter->ticket();
		const bool quietBefore = journey() && _encounter->journeyMutable();
		const bool hadJourneyCombat = journey() && _encounter->combat();
		const bool changed = _encounter->idle(_cycle);
		if (_castingUi && !_encounter->castingActive()) _castingUi.reset();
		prepareJourneyTransition();
		if (_encounter->combatOperationStale()) {
			_fatal = true;
			throw std::runtime_error("Stale automatic combat operation");
		}
		if (!_encounter->current(_encounter->ticket())) { _fatal = true; throw std::runtime_error("Stale encounter idle"); }
		const bool ordinary = advanceEncounterOrdinary();
		// Combat appearance/ordinary animation can replace the concrete frame while
		// retaining the exact semantic ticket. Do not retire fresh keys for that redraw.
		const bool combatCosmetic = _encounter->combat() && beforeIdle.combat &&
			_encounter->combat()->current(*beforeIdle.combat);
		if (changed || ordinary) return renderEncounter((hadJourneyCombat && !_encounter->combat()),
			combatCosmetic || (quietBefore && _encounter->journeyMutable() && _encounter->current(beforeIdle)));
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
	if (journey()) {
		if (_smithUi || _trainingUi) return false;
		if (_dispatching || _fatal || _saving || _handoffPending || !encounterFrameCurrent() ||
			!_encounter->journeyEvent() || !_pending || _pending->generation!=generation ||
			!_pending->state.pendingPresentation || !xeenResponseMatches(_pending->state.pendingPresentation->request.response,response)) return false;
		if (_presenter.pageIndex()+1<_presenter.pageCount()) return false;
		if (const auto *selected=std::get_if<SelectedCharacter>(&response.value))
			if (selected->partyIndex>=_pending->state.pendingPresentation->request.members.size()) return false;
		DispatchScope dispatch(_dispatching);
		journeyEventWork([&] { resumePending(generation,response); });
		return true;
	}
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
	if (_transition)
		drive(resumeVertigoEvent(std::move(pending.state),response),false);
	else if (pending.automatic)
		drive(_events.resumeAutomaticEvent(std::move(pending.state), response,
			_world, _party, _camera, _flags, _eventPublication), true);
	else
		drive(_events.resumeManualEvent(std::move(pending.state), response,
			_world, _party, _camera, _flags, _eventPublication), false);
	return true;
}
IndexedFrame XeenEventFlow::handle(const PlayerAction &physicalAction, std::optional<std::uint64_t> displayedInput,
        const IndexedFrame::Presentation &inputFrame) {
	PlayerAction action=physicalAction;
	requireCurrentOwners();
	// Concrete origin is independent of semantic meaning. A retry/redraw may
	// retain the generation, but input from its retired frame cannot authorize it.
	if ((inputFrame && !acceptsInputFrame(inputFrame)) || (_trainingUi && !inputFrame) ||
		(_smithUi && !inputFrame)) return frameCopy();
	if (journey() && !journeyInputCurrent(displayedInput)) return frameCopy();
	if (std::holds_alternative<SaveGameAction>(action) || _dispatching || _fatal || _saving) return frameCopy();
	if(_trainingUi) {
		DispatchScope dispatch(_dispatching);
		return handleTraining(action,*displayedInput,inputFrame);
	}
	if (_smithUi) {
		DispatchScope dispatch(_dispatching);
		return handleSmith(action,*displayedInput,inputFrame);
	}
	if (_encounter && _encounter->combat() && (!displayedInput || *displayedInput != _inputGeneration ||
		!_displayedCombat || !_encounter->combat()->current(*_displayedCombat))) return frameCopy();
	if (!_encounter && std::holds_alternative<WaitAction>(action)) return frameCopy();
	if (!_encounter && (std::holds_alternative<AttackAction>(action) || std::holds_alternative<BlockAction>(action) || std::holds_alternative<RunAction>(action) ||
		std::holds_alternative<BeginEncounterAction>(action) || std::holds_alternative<RevisitCompletedAction>(action))) return frameCopy();
	if (journey() && _encounter->combat() && std::holds_alternative<RevisitCompletedAction>(action)) action=RunAction{};
    if (const auto *unsupported=std::get_if<UnsupportedMainScreenAction>(&action)) {
        const auto context=inputContext(inputFrame);
        if (context.mainScreen==MainScreen::None || !context.readyForAction) return frameCopy();
        _mainScreenNotice=std::string(unsupported->label)+": not supported yet";
        DispatchScope dispatch(_dispatching);
        if (reportText) reportText(_mainScreenNotice);
        return renderEncounter();
    }
    _mainScreenNotice.clear();
	DispatchScope dispatch(_dispatching);
    if(inventoryOpen()) {
        handleCharacterDialog(action);
        return _encounter?renderEncounter(true):_frame;
    }
    if(std::holds_alternative<SelectMemberAction>(action) ||
        (_encounter && _encounter->combat() && std::holds_alternative<UseItemAction>(action))) {
        const bool quiet=!_pending && !_castingUi && (!_encounter || (_encounter->combat() ?
            _encounter->combat()->phase()==XeenCombatPhase::PlayerReady && !_encounter->combat()->cast():_encounter->journeyMutable()));
        if(quiet) {
            const auto *selected=std::get_if<SelectMemberAction>(&action);
            const auto member=selected?dialogMember(selected->partyIndex):std::optional<std::size_t>{std::size_t(_encounter->combat()->participant())};
            if(member) {
                handleInventory(InspectInventoryAction{});
                _inventory.source=*member;_inventory.sourceOwner=_party.party.activeRosterIds()[*member];
                if(selected) {_sheet.emplace();_sheet->deadline=_clock()+200;_itemsVisible=false;}
                else {_combatItems=true;_inventory.category=XeenInventoryCategory::Miscellaneous;}
                drawInventory();return _encounter?renderEncounter():_frame;
            }
        }
        if(!_pending && !_castingUi && !(_encounter && _encounter->combat() && _encounter->combat()->cast())) return frameCopy();
    }
    if(journey() && _encounter->combat() && (_encounter->combat()->cast() || std::holds_alternative<CastSpellAction>(action)))
        return handleCombatCasting(action,*displayedInput);
	validateRegionalEvents();
	if (_encounter) {
		if (journey() && !_encounter->combat()) {
			if (_castingUi) return handleCasting(action,*displayedInput);
			if (_encounter->castingSettlement() && !_encounter->monsterReward() && !_encounter->journeyEvent()) return frameCopy();
			if (std::holds_alternative<CastSpellAction>(action)) {
				if (!_encounter->beginCasting(_encounter->ticket())) return frameCopy();
				// beginCasting acquired its lease; only completed Event layers may be retired here.
				if (_journeyEventLayers) { _presenter.clear(); _journeyEventLayers=false; }
				_castingUi.emplace();
				return renderEncounter();
			}
			if(_encounter->monsterReward()) {
				const auto update=_presenter.handle(action);
				if(update.response) {
					_encounter->acknowledgeMonsterReward();_monsterReceiptPresented=false;_presenter.clear();prepareJourneyTransition();
				}
				return renderEncounter();
			}
			if ((std::holds_alternative<AttackAction>(action) || std::holds_alternative<BlockAction>(action) || std::holds_alternative<RunAction>(action) ||
				 std::holds_alternative<BeginEncounterAction>(action) || std::holds_alternative<RevisitCompletedAction>(action))) {
				if (_encounter->journeyMutable()) {
					_encounter->_journeyRefusal="Unsupported regional action: combat/Run/Begin/Revisit";
					return renderEncounter();
				}
				return frameCopy();
			}
			if (_encounter->journeyEvent()) {
				if (!_pending) return frameCopy();
				return journeyEventWork([&] {
					const auto generation=_pending->generation;
					auto update=_presenter.handle(action,false);
					_frame=std::move(update.frame);
					if (update.response) resumePending(generation,*update.response);
				});
			}
			if (_journeyEventLayers) { _presenter.clear(); _journeyEventLayers=false; }
			if (inventoryOpen() || std::holds_alternative<InspectInventoryAction>(action)) {
				if (!_encounter->journeyMutable() && !(_encounter->itemUseActive() && inventoryOpen())) return frameCopy();
				handleInventory(action);
				if (inventoryOpen()) _encounter->retireCastingFeedback();
				return renderEncounter(true);
			}
			if (std::holds_alternative<InteractionAction>(action)) {
				if (!_encounter->journeyQuiet()) return frameCopy();
				{
					if (xeenRegionalInteraction(_encounter->_journeyEvents,_camera)!=XeenRegionalInteraction::None) {
						_encounter->beginJourneyEvent();_journeyEventLayers=true;
						const auto interaction=xeenRegionalInteraction(_encounter->_journeyEvents,_camera);
						return journeyEventWork([&] {
							if(interaction==XeenRegionalInteraction::VertigoEntrance ||
								interaction==XeenRegionalInteraction::VertigoDoor ||
								interaction==XeenRegionalInteraction::VertigoExit)
								drive(beginVertigoEvent(interaction),false);
							else drive(_events.runManualEvent(_world,_party,_camera,_flags,_eventPublication),false);
						});
					}
					try { _encounter->journeyRead([&] {
						XeenEventPublication validation(_encounter->journeySavePreimage(),_encounter->_journeyEvents,[&] {
							if (!_encounter->current(_encounter->ticket())) throw std::logic_error("Stale regional event lookup");
						});
						validation.script(_events.scriptForMap(_camera.mapId).file());
						const auto record=xeenRegionalEvent(_encounter->_journeyEvents,_camera);
						if (record) _encounter->_journeyRefusal="Unsupported event "+std::to_string(*record)+" at ("+std::to_string(_camera.x)+","+std::to_string(_camera.y)+")";
						else if (reportManual) reportManual(XeenManualEventNoEvent{});
					}); } catch (...) {
						if (!_encounter->current(_encounter->ticket())) throw;
						_encounter->_journeyRefusal="Regional interaction failed; no event executed";
					}
					return renderEncounter();
				}

				try { _encounter->journeyRead([&] {
					const XeenManualEventResult result = XeenManualEventNoEvent{};
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

		const auto entry = _encounter->ticket();
		const auto cameraBeforeAction=_camera;
		const bool changed = _encounter->handle(action, _cycle, _displayedCombat);
		prepareJourneyTransition();
		if (_encounter->combatOperationStale()) {
			_fatal = true;
			throw std::runtime_error("Stale combat input operation");
		}
		if (!_encounter->current(_encounter->ticket())) { _fatal = true; throw std::runtime_error("Stale encounter input"); }
		if (changed) {
			// Gameplay action and its distinct pulse are already adopted. Consume
			// this physical navigation's visual cause once, never during recovery.
			const auto &accepted = _encounter->actionResult();
			if (std::holds_alternative<NavigationAction>(action) &&
				((_encounter->combat() && _encounter->state().revision() > entry.state.revision()) ||
				(accepted.revision > entry.state.revision() &&
				(accepted.outcome == XeenEncounterOutcome::Accepted || accepted.outcome == XeenEncounterOutcome::Blocked))))
			{
				_world.scenePresentation().navigation(std::get<NavigationAction>(action),
					cameraBeforeAction.x!=_camera.x || cameraBeforeAction.y!=_camera.y || cameraBeforeAction.mapId!=_camera.mapId, bool(entry.combat));
				advanceEncounterOrdinary(OrdinaryCause::Action);
			}
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
		_world.scenePresentation().navigation(*navigation,result.movementResult==XeenMovementResult::Moved);
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
