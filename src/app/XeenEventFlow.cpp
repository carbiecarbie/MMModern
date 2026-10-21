#include "app/XeenEventFlow.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenEventPublication.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenCdSpeechTiming.h"
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
	return _gameplayBorrow->current() && !_death && !_fatal && !_dispatching && !_saving && !_handoffPending && !_transition && !_arrivalPending &&
		!_portraitSequence && !inventoryOpen() && !_quickFightMember && !_summary && !_pending && !_equipmentSelection &&
		(!_encounter || (!_encounter->_needsRestNotice && _encounter->canSave() && encounterFrameCurrent()));
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
    const unsigned panel = _travelUi ? 17 : _death ? 16 : _encounter && !_encounter->_combatCastRefusal.empty() ? 15 : !_whoWillRefusal.empty() ? 14 : _quickFightMember ? 13 : _summary ? 12 : _encounter && _encounter->_rest ? 11 : _encounter && _encounter->_needsRestNotice ? 10 : _fatal ? 1 : _trainingUi ? 2 : _smithUi ? 3 : inventoryOpen() ? 4 :
        (_castingUi || (combat && combat->cast())) ? 5 :
        (_pending || _transition || (_encounter && _encounter->journeyEvent())) ? 6 :
        (_encounter && _encounter->monsterReward()) ? 7 :
        (combat && (combat->phase() == XeenCombatPhase::VictoryAwaitingEnd ||
            combat->phase() == XeenCombatPhase::Victory || combat->phase() == XeenCombatPhase::Disengaged ||
            combat->phase() == XeenCombatPhase::Defeat || combat->phase() == XeenCombatPhase::SupportStopped ||
            combat->phase() == XeenCombatPhase::Failed)) ? 8 :
        (_encounter && !_encounter->combat() && _encounter->state().phase() != XeenEncounterPhase::Exploring) ? 9 : 0;
    const QueueContext context{_camera.mapId, panel, combat,
        _quickFightMember ? 500+*_quickFightMember : _summary ? 400+unsigned(*_summary) : inventoryOpen() ? (_exchange ? 6 : _dialogError ? 5 : _statPopup ? 4 : _itemOption ? 3 : _sheet && !_itemsVisible ? 2 : 1) :
        _smithUi ? 100+unsigned(_smithUi->phase)*8+unsigned(_smithUi->mode)+(!_smithUi->feedback.empty()?1000:0) :
        _trainingUi ? 200+unsigned(_trainingUi->phase)+(!_trainingUi->feedback.empty()?1000:0) :
        _encounter && _encounter->_rest ? 300+unsigned(_encounter->_rest->phase) : _pending ? _pending->generation :
        _encounter && _encounter->monsterReward()?600+4*_encounter->_monsterReceipt->cursor+unsigned(_encounter->_monsterReceipt->phase):0};
    if (!_queueContext || !(*_queueContext == context)) {
        if (_queueContextId == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Input context exhausted");
        ++_queueContextId;
        _queueContext = context;
        _mainScreenNotice.clear();
    }
    const bool worldQueueable = panel == 0 && (!_encounter || journey() || combat);
    const bool npcQueueable = panel==6 && pendingNpc() && bool(_npcSequence);
    const bool queueable=worldQueueable || npcQueueable;
    bool ready = !_dispatching && !_fatal && !_saving && !_handoffPending && !_arrivalPending &&
        acceptsInputFrame(origin) && (!journey() || journeyInputCurrent(displayedInput()));
    bool navigationReady = ready && worldQueueable;
	if(_treasureWait)ready=ready && _treasureWait->acquired && presentationNow()>=_ordinary.deadline;
    // Rest consumes an already armed countdown through sleeping chargeStep.
    // It cannot enter around a retained opportunity or an unacquired frame.
    bool restReady=ready && worldQueueable && journey() && !combat && _encounter->state().pending() &&
        _encounter->state().phase()==XeenEncounterPhase::Exploring && !_encounter->_busy &&
        !_encounter->_regionalWork && !_encounter->_regionalAutomatic && !_encounter->_shoot &&
        !_encounter->_shootIntent && !_encounter->projectilesPending() && !_encounter->_casting &&
        !_encounter->_castingSettlement && _encounter->boundary().quiet();
    if (worldQueueable && _encounter) {
        ready = ready && !_encounter->_busy && !_encounter->projectilesPending() &&
            !_encounter->_shootIntent && !_encounter->_shoot && !_encounter->_castingSettlement &&
            !_encounter->_regionalAutomatic && !_encounter->_regionalWork &&
            _encounter->boundary().quiet();
        navigationReady = ready && (combat ? combat->phase()==XeenCombatPhase::PlayerReady &&
            _displayedCombat && combat->current(*_displayedCombat) && !_encounter->_scheduleAfterFrame :
            _encounter->journeyNavigationReady());
        if (combat) ready = ready && combat->phase() == XeenCombatPhase::PlayerReady &&
            _displayedCombat && combat->current(*_displayedCombat) && !_encounter->_scheduleAfterFrame;
        else ready = ready && _encounter->journeyMutable();
    }
    if((_smithUi && (_smithUi->goodbye || _smithUi->animation.pendingIndex)) ||
        (_trainingUi && _trainingUi->animation.pendingIndex))ready=false;
    if(_portraitSequence)ready=navigationReady=restReady=false;
    auto dialog=_smithUi || _trainingUi ? serviceDialogInput() : characterDialogInput();
    if(_castingUi || (combat && combat->cast()))dialog=castingDialogInput();
    if(_travelUi) {
        DialogInput input;input.textEntry=true;input.keys={InputKey::Enter,InputKey::Escape,8,127,'0','1','2','3','4','5','6','7','8','9'};
        if(_travelUi->spell==XeenLearnedSpell::LloydsBeacon) {
            input.textEntry=false;input.keys={InputKey::Escape,'s','t','r'};
            input.hits={{242,108,266,128,'t',InputButton{"lloyds.icn",2,242,108}},
                {281,108,305,128,'r',InputButton{"lloyds.icn",0,281,108}}};
        }
        if(!_travelUi->refusal.empty())input.anyKey=input.anyClick=input.keyMouseWait=true;
        dialog=std::make_shared<const DialogInput>(std::move(input));
    }
    if(_presenter.blocksGameplay() &&
        ((_pending && _pending->state.pendingPresentation &&
          _pending->state.pendingPresentation->request.kind!=XeenPresentationKind::CharacterSelection) ||
         (_encounter && _encounter->monsterReward())))
        dialog=std::make_shared<const DialogInput>(_presenter.input());
    if(_quickFightMember)dialog=std::make_shared<const DialogInput>(xeenQuickFightInput());
    if(_summary) {DialogInput input;
        if(*_summary==SummaryDialog::Quests || *_summary==SummaryDialog::Awards)input=xeenJournalInput(*_summary==SummaryDialog::Awards);
        else input.anyKey=input.anyClick=input.keyMouseWait=true;
        dialog=std::make_shared<const DialogInput>(std::move(input));
    }
    const bool whoWill=_pending && _pending->state.pendingPresentation &&
        _pending->state.pendingPresentation->request.kind==XeenPresentationKind::CharacterSelection;
    if(whoWill || (_barrier && _barrier->rule && _barrier->rule->selection)) {
        DialogInput input;constexpr int x[]{10,45,81,117,153,189};
        const auto count=whoWill?_pending->state.pendingPresentation->request.members.size():_party.party.size();
        for(unsigned member=0;member<count;++member) {
            input.hits.push_back({x[member],150,x[member]+32,182,InputKey::F1+member});
            input.keys.push_back(InputKey::F1+member);
        }
        input.keys.push_back(InputKey::Escape);dialog=std::make_shared<const DialogInput>(std::move(input));
    }
    if(!_whoWillRefusal.empty()) {DialogInput input;input.anyKey=input.anyClick=input.keyMouseWait=true;dialog=std::make_shared<const DialogInput>(std::move(input));}
    if(_encounter && _encounter->_needsRestNotice) {DialogInput input;input.anyKey=true;input.anyClick=true;dialog=std::make_shared<const DialogInput>(std::move(input));}
    if(_encounter && !_encounter->_combatCastRefusal.empty()) {
        DialogInput input;input.keys={'c'};input.anyKey=input.anyClick=input.keyMouseWait=true;
        dialog=std::make_shared<const DialogInput>(std::move(input));
    }
    bool hideCursor=false;
    if(_entranceUi){ready=false;navigationReady=false;dialog=std::make_shared<const DialogInput>();hideCursor=true;}
    if(_encounter && _encounter->_rest) {
        using P=XeenEncounterFlow::RestContinuation::Phase;const auto &rest=*_encounter->_rest;
        ready=ready && (rest.phase==P::Confirm || rest.phase==P::Refused || rest.phase==P::Complete);
        DialogInput input;
        if(rest.phase==P::Confirm)input=xeenConfirmInput();
        else if(rest.phase==P::Refused || rest.phase==P::Complete)input.anyKey=input.anyClick=input.keyMouseWait=true;
        dialog=std::make_shared<const DialogInput>(std::move(input));
        hideCursor=rest.phase==P::Dream && rest.dreamScene();
    }
    if(_death) {
        ready=ready && _death->phase==DeathContinuation::Phase::Scroll;
        navigationReady=false;
        DialogInput input;input.anyKey=input.anyClick=input.keyMouseWait=true;
        dialog=std::make_shared<const DialogInput>(std::move(input));hideCursor=true;
    }
    // sealFrame observes context while composition owns the callback lease.
    // Only an actionable acquired origin may sample an external clock here.
    if(npcQueueable)ready=ready && !_npcResponse && _npcSequence->acquired && presentationNow()>=_npcSequence->deadline;
    const bool drawFinished=worldQueueable && (ready || navigationReady || restReady) &&
        presentationNow()>=_ordinary.deadline;
    const bool navigationDrawPending=navigationReady && !dialog && !drawFinished;
    navigationReady=navigationReady && !dialog && drawFinished;
    if(worldQueueable)ready=ready && drawFinished;
    return {_queueContextId, queueable, ready, journey() && worldQueueable ?
        (combat ? MainScreen::Combat : MainScreen::Exploration) : MainScreen::None,std::move(dialog),hideCursor,restReady,
        navigationReady,navigationDrawPending};
}
std::uint64_t XeenEventFlow::presentationNow() {
    requireCurrentOwners();
    std::uint64_t now=0;
    if(_encounter)_encounter->guardCallback(_encounter->ticket(),[&]{now=_presentationClock();});
    else {now=_presentationClock();requireCurrentOwners();}
    return now;
}
std::optional<std::uint64_t> XeenEventFlow::nextPresentationDeadline() {
    const auto now=presentationNow();
    if(_entranceUi)return _entranceUi->animation.awaitingFrame()?std::optional<std::uint64_t>{}:_entranceUi->animation.deadline();
    if(_travelUi) {if(!_travelUi->refusal.empty())return std::nullopt;if(_travelUi->spell==XeenLearnedSpell::LloydsBeacon)return std::nullopt;const auto at=_travelUi->digits.deadline();return at?PresentationClock::fromMilliseconds(*at):now;}
    if(_presenter.wordInputActive()) {
        const auto deadline=_presenter.wordDeadline();
        return deadline?PresentationClock::fromMilliseconds(*deadline):now;
    }
    if(_portraitSequence)return _portraitSequence->acquired?_portraitSequence->deadline:now;
    if(_death)return _death->acquired?std::optional<std::uint64_t>{_death->deadline}:std::nullopt;
    if(_smithUi) {
        if(_smithUi->animation.pendingIndex && !_smithUi->animation.awaitingFrame)return now;
        if(_smithUi->goodbye)return _smithUi->animation.sample.deadline();
        if(_smithUi->phase==SmithUi::Phase::Preparation || _smithUi->phase==SmithUi::Phase::Upgrade ||
            _smithUi->phase==SmithUi::Phase::Departure)return now;
        auto next=_smithUi->animation.deadline;
        if(_smithUi->phase==SmithUi::Phase::Amount && _smithUi->feedback.empty()) {
            const auto cursor=_smithUi->amountCursor.deadline();
            next=std::min(next,cursor?PresentationClock::fromMilliseconds(*cursor):now);
        }
        return next;
    }
    if(_trainingUi) {
        if(_trainingUi->phase!=TrainingUi::Phase::Menu)return now;
        return _trainingUi->animation.deadline;
    }
    if((pendingNpc() || (_pending && _presenter.hasNpcOffer()))) {
        return _npcSequence ? _npcSequence->deadline : PresentationClock::after(now,3);
    }
    auto next=_ordinary.deadline;
    if(inventoryOpen() || (_summary && *_summary==SummaryDialog::QuickReference))next=PresentationClock::after(now,1);
    if(_sheet && !_itemsVisible && !_statPopup && !_dialogError)next=PresentationClock::fromMilliseconds(_sheet->deadline);
    if(_summary && *_summary==SummaryDialog::Map)next=PresentationClock::fromMilliseconds(_mapDeadline);
    if(_barrier && _barrier->trapWaiting)next=std::min(next,_barrier->deadline);
    else if(_barrier && _barrier->published)next=std::min(next,PresentationClock::fromMilliseconds(_barrier->deadline));
    if(_encounter && !_encounter->journeyEvent() && _encounter->deadline())
        next=std::min(next,PresentationClock::fromMilliseconds(*_encounter->deadline()));
    if(_encounter && _encounter->_rest && _encounter->_rest->presented) {
        using P=XeenEncounterFlow::RestContinuation::Phase;
        const auto &rest=*_encounter->_rest;
        if(rest.phase==P::Dream)return rest.dreamDeadline;
        if(rest.phase!=P::Confirm && rest.phase!=P::Refused && rest.phase!=P::Complete)
            next=std::min(next,PresentationClock::fromMilliseconds(rest.deadline));
    }
    if(_encounter && _encounter->_shoot && _encounter->_shoot->presented)
        next=std::min(next,PresentationClock::fromMilliseconds(_encounter->_shoot->deadline));
    return next;
}

bool XeenEventFlow::partyDeathRequired() const noexcept {
    if(!_encounter || _pending || _transition || _barrier || _portraitSequence)return false;
    for(const auto &portrait:_world.scenePresentation().portraits)
        if(portrait.damageTicks || portrait.spellFrame<4)return false;
    if((_encounter->combat() && _encounter->combat()->phase()==XeenCombatPhase::Defeat) ||
        _encounter->state().reason()==XeenEncounterStop::Defeat)return true;
    // Party::checkPartyDead, also reached after Event damage: wait for all
    // retained effects and their publication before handing off the session.
    if(!journey() || _encounter->combat() || _encounter->_rest || _encounter->_failure ||
        _encounter->_regionalWork || _encounter->_regionalAutomatic || _encounter->_shoot ||
        _encounter->_shootIntent || _encounter->_casting || _encounter->_castingSettlement ||
        _encounter->projectilesPending() || _encounter->monsterReward())return false;
    for(auto owner:_party.party.activeRosterIds())if(xeenCombatTargetable(_party.roster.at(owner)))return false;
    return true;
}
bool XeenEventFlow::partyDeathComplete() const noexcept {
    return _death && _death->phase==DeathContinuation::Phase::Done;
}
IndexedFrame XeenEventFlow::drawDeath(const IndexedFrame &base) {
    using P=DeathContinuation::Phase;
    if(!_death) {
        if(!loadSampleDuration || !drawDialogSprite)throw std::invalid_argument("Original death providers are absent");
        DeathContinuation candidate;candidate.gameplay=base;
        const auto fireDuration=loadSampleDuration("fire.voc",XeenSceneArchive::Clouds);
        candidate.laughDuration=loadSampleDuration("xeenlaff.voc",XeenSceneArchive::Clouds);
        if(!fireDuration || !candidate.laughDuration)throw std::invalid_argument("Invalid original death sample duration");
        // The final pair 34/35 is drawn over the last fire composition (all
        // seventeen death.vga pairs), not over the gameplay: pin
        // WorldOfXeenEngine::death and the DOS capture Morte.mkv (2026-10-13).
        candidate.final=drawXeenDeathFinal(drawXeenDeathFire(base,34,drawDialogSprite),drawDialogSprite);
        _death=std::move(candidate);
    }
    const auto &death=*_death;
    if(death.phase==P::Fire)return drawXeenDeathFire(death.gameplay,death.fire,drawDialogSprite);
    if(death.phase==P::Laugh)return drawXeenDeathLaugh(death.final,
        death.laugh?std::optional<unsigned>{death.laugh-1}:std::nullopt,drawDialogSprite);
    if(death.phase==P::Scroll || death.phase==P::Closed || death.phase==P::FadeOut || death.phase==P::Done) {
        auto frame=death.final;
        const auto hand=[&](unsigned index,int x=0) {
            const auto name="hand"+std::string(index<10?"0":"")+std::to_string(index)+".vga";
            drawDialogSprite(frame,name.c_str(),0,x,0);
        };
        const auto marble=[&](unsigned sheet,unsigned index) {
            const auto name="marb0"+std::to_string(sheet)+".vga";
            drawDialogSprite(frame,name.c_str(),index,0,0);
        };
        if(death.phase!=P::Scroll) {hand(0);marble(1,0);}
        else {
            const auto index=death.scroll;
            // DAT 0x0aec3..0x0afd2, calibrated against the closing hands in
            // Morte.mkv; Screen::doScroll(true,false) in the pinned reference.
            constexpr int left[]{29,23,15,-5,-11,-23,-49,-71};
            constexpr int right[]{165,171,198,218,228,245,264,281};
            if(index>14) {hand(14,left[index-15]);hand(15,right[index-15]);}
            else hand(index-1);
            if(index<=20)marble((index-1)/5+1,(index-1)%5);
        }
        if(death.phase==P::FadeOut)
            for(auto &component:frame.palette)component=std::uint8_t((unsigned(component)*death.fadeLevel*2)>>8);
        return frame;
    }
    return death.final;
}
void XeenEventFlow::deathPresented() {
    if(!_death || _death->acquired)return;
    auto &death=*_death;using P=DeathContinuation::Phase;
    const auto now=presentationNow();
    death.acquired=true;
    if(death.phase==P::Fire || death.phase==P::FadeOut)death.deadline=PresentationClock::retraceAfter(now);
    // Laughter and scroll reset before composition, not after upload. A late
    // acquisition satisfies that wait but never permits an unacquired successor.
    else if(death.phase!=P::Laugh && death.phase!=P::Scroll)death.deadline=now;
}
bool XeenEventFlow::advanceDeath() {
    if(!_death || !_death->acquired)return false;
    auto &death=*_death;using P=DeathContinuation::Phase;
    const auto now=presentationNow();
    if(now<death.deadline || death.phase==P::Done)return false;
    switch(death.phase) {
    case P::Fire:
        if(death.fire<34)death.fire+=2;else death.phase=P::Final;
        break;
    case P::Final:
        death.phase=P::Laugh;death.laughEnd=PresentationClock::checkedAdd(now,death.laughDuration);break;
    case P::Laugh:
        // DAT 0x0b2f7..0x0b346: sample status is sampled after the two-tick
        // draw hold, with one restart at index 10 and no final sound tail.
        if(now>=death.laughEnd || death.laugh==22)death.phase=P::Restore;
        else if(death.laugh==10 && !death.restarts){death.laugh=0;++death.restarts;}
        else ++death.laugh;
        break;
    case P::Restore:death.phase=P::Scroll;break;
    case P::Scroll:
        if(death.scroll>1)--death.scroll;else death.phase=P::Closed;
        break;
    case P::Closed:death.phase=P::FadeOut;break;
    case P::FadeOut:
        if(death.fadeLevel)death.fadeLevel-=4;else {death.phase=P::Done;return false;}
        break;
    case P::Done:return false;
    }
    death.acquired=false;
    if(death.phase==P::Laugh)death.deadline=PresentationClock::after(now,2);
    else if(death.phase==P::Scroll)death.deadline=PresentationClock::after(now,1);
    return true;
}
IndexedFrame XeenEventFlow::drawRest(const IndexedFrame &base) {
    if(!_encounter->_rest)return base;
    using P=XeenEncounterFlow::RestContinuation::Phase;auto &rest=*_encounter->_rest;
    if(rest.phase==P::Confirm)return drawXeenConfirm(base,_inventoryFont,std::string(xeenDialogText(dosText(),XeenDialogText::SomeCharsMayDie)),false,drawDialogSprite);
    if(rest.phase==P::Refused)return drawXeenErrorScroll(base,_inventoryFont,rest.refusal.empty()?std::string(xeenDialogText(dosText(),XeenDialogText::TooDangerousToRest)):rest.refusal);
    if(rest.phase==P::Complete)return drawXeenErrorScroll(base,_inventoryFont,xeenDialogFormat(xeenDialogText(dosText(),XeenDialogText::RestComplete),{
        std::string(xeenDialogText(dosText(),rest.starving?XeenDialogText::PartyIsStarving:XeenDialogText::HitSpellPointsRestored)),std::to_string(rest.consumed)}));
    if(rest.phase!=P::Dream)return base;
    if(!rest.background.isValid())rest.background=base;
    auto frame=rest.dreamScene()?rest.dream:rest.background;
    const unsigned scale=rest.dreamScale();
    // Screen::loadPalette expands p6 to p8 first; fadeInner uses
    // (p8 * val * 2) >> 8, preserving eight-bit intermediate shades.
    for(unsigned n=0;n<frame.palette.size();++n)frame.palette[n]=std::uint8_t((unsigned(rest.background.palette[n])*scale*2)>>8);
    return frame;
}

IndexedFrame XeenEventFlow::drawTerminalEncounterNotice(const IndexedFrame &base, const std::string &notice) const {
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
        const auto controls=line.find(": A/B;");
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
            auto label=std::to_string(row+1)+" "+actor.statistics->name();
            const XeenTextRenderer renderer(_inventoryFont);
            while(renderer.textWidth(label,XeenFontSize::Reduced)>73) label.pop_back();
            options.bounds={239,27+int(row)*10,312,37+int(row)*10};
            options.x=239; options.y=options.bounds.top; options.drawWindow=false;
            frame=renderer.render(frame,label,options).pages.front();
            if(rows[row]==combat->selectedTarget() && drawDialogSprite)
                drawDialogSprite(frame,"combat.icn",32,233,27+int(row)*10);
        }
    }
    return frame;
}

IndexedFrame XeenEventFlow::drawMainScreenNotice(const IndexedFrame &base, const std::string &) const {
    if((_encounter->combat() && _encounter->combat()->phase()==XeenCombatPhase::Defeat) ||
        _encounter->state().reason()==XeenEncounterStop::Defeat)return base;
    auto frame=base;
    if(const auto *combat=_encounter->combat()) {
        if(drawCombatButtons)drawCombatButtons(frame);
        const auto contacts=combat->contacts();
        std::array<XeenCombatPanelRow,3> rows{};
        std::optional<unsigned> selected;
        for(unsigned i=0;i<contacts.size();++i)if(contacts[i]) {
            const auto &actor=_world.sessionState().regionalActors(contacts[i]->mapId).at(contacts[i]->recordIndex);
            rows[i]={actor.statistics->name(),actor.hp,actor.statistics->baseHp()};
            if(contacts[i]==combat->selectedTarget())selected=i;
        }
        frame=drawXeenCombatPanel(dosText(),frame,_inventoryFont,rows,selected,drawDialogSprite);
    }
    const auto message=_mainScreenNotice.empty()?_encounter->playerNotice():_mainScreenNotice;
    if(!message.empty())frame=drawXeenErrorScroll(frame,_inventoryFont,message);
    return frame;
}

void XeenEventFlow::drawPartyPresentation(IndexedFrame &frame) const {
 if(!drawDialogSprite || !_encounter)return;
 const auto *combat=_encounter->combat();
 const unsigned mask=combat?combat->participants():0x3f;
 XeenPartyState beforeBash;
 const XeenPartyState *visible=_transition?&_transition->party:&_party;
 if(_barrier && _barrier->published && _barrier->bash && !_barrier->rule->moved) {
  _barrier->world->copyEventParty(_party,beforeBash);
  for(const auto &c:_barrier->characters)beforeBash.roster.at(c.rosterId)=c;
  visible=&beforeBash;
 }
 drawDialogSprite(frame,"restorex.icn",0,8,149);
 for(const auto &p:CloudsUiComposer::buildPortraitPlacements(*visible,mask))
  drawDialogSprite(frame,p.resourceName.c_str(),unsigned(p.frame),p.x,p.y);
 for(const auto &p:CloudsUiComposer::buildHpPlacements(*visible,{visible->encounterContext->year},mask))
  drawDialogSprite(frame,"hpbars.icn",unsigned(p.frame),p.x,p.y);
 auto feedback=_world.scenePresentation();
 if(_barrier && (_barrier->portraitWaiting || _barrier->trapWaiting) && _barrier->rule && _barrier->rule->injury)
  feedback.portraitDamage(_barrier->rule->injury->impactOwner,_barrier->rule->injury->portraitFrame);
 for(const auto &p:CloudsUiComposer::buildPartyFeedbackPlacements(*visible,feedback,mask,combat && !_quickFightMember?combat->participant():-1))
  drawDialogSprite(frame,p.resourceName.c_str(),unsigned(p.frame),p.x,p.y);
}

bool XeenEventFlow::journeyInputCurrent(std::optional<std::uint64_t> input) const noexcept {
	return journey() && input && *input == _inputGeneration && !_handoffPending && !_fatal && !_dispatching && !_saving && encounterFrameCurrent();
}
void XeenEventFlow::prepareJourneyTransition() {
 if (!journey()) return;
 if(_encounter->_terrainFall && !_transition && !_encounter->combat()) {
  _encounter->beginTerrainTransition();
  journeyEventWork([&] {prepareTerrainTransition();},true);
  return;
 }
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
 // A pass over a cell with the Event flag calls the Event routine even when no
 // record matches, clearing WhoWill (0x11f04/0xd1a5, 0x26797). MMModern takes a
 // pass on arrival at each new position; later passes on the same cell are not
 // modelled (original-quirks.md#dev-whowill-event-passes).
 if(!_encounter->combat() && _encounter->journeyQuiet() && (!_whoWillPass || !xeen_state::sameCamera(*_whoWillPass,_camera))) {
  _whoWillPass=_camera;
  const auto sample=_world.sampleCell(_camera.mapId,_camera.x,_camera.y);
  if(!_encounter->_regionalAutomatic && sample && (sample->cell->rawAttributes&kXeenAutomaticEventFlag))_encounter->clearWhoWill();
 }
 // The original runs the cell's Events before ordinary delivery (0xd1a5, 0xd1b7);
 // delivery waits while an Event candidate is in progress.
 if(!_encounter->combat() && _encounter->_regionalAutomatic && !_encounter->_regionalWork && !_encounter->_shoot &&
  !_encounter->monsterReward() && !_encounter->state().pending() && _encounter->state().phase()==XeenEncounterPhase::Exploring) {
  // This run is the pass's Event call; its own WhoWill choice must survive.
  _whoWillPass=_camera;
  _encounter->beginJourneyEvent();_journeyEventLayers=true;
  journeyEventWork([&] {
   drive(beginDetachedEvent(xeenRegionalInteraction(_encounter->_journeyEvents,_camera),true),true);
  },true);
 }
 if(!_transition)_encounter->beginMonsterReward();
 if(_encounter->_shootIntent && !_encounter->state().pending() && !_encounter->_regionalWork && !_encounter->projectilesPending()) _encounter->beginShoot();
}

void XeenEventFlow::prepareTerrainTransition() {
 if(!_eventPublication || !_encounter->_terrainFall || _transition || !_transitionCompose)
  throw std::logic_error("Terrain destination preparation is unavailable");
 const auto rule=*_encounter->_terrainFall;
 auto work=std::make_unique<TransitionCandidate>();work->kind=XeenRegionalInteraction::Event;work->automatic=true;work->terrainDone=true;
 work->world=_world.transitionCandidate();work->world->copyEventParty(_party,work->party);
 work->initialCamera=_camera;work->initialFlags=_flags;work->camera=rule.transition.destination;work->flags=_flags;
 const auto admitted=work->world->prepareEventTransition(work->camera);_eventPublication->check();
 if(!admitted)throw std::logic_error("Terrain destination admission changed");
 work->world->loadEventDestination(_camera.mapId,work->camera.mapId);
 XeenConsequenceCharacters characters;XeenConsequenceInputs inputs;
 const auto owners=work->party.party.activeOrder();
 for(unsigned n=0;n<6;++n) {characters[n]=work->party.roster.at(owners[n]);inputs[n]=*work->party.roster.combatInputs(owners[n]);}
 XeenCombatRandom random(*work->world->sessionState().journeyRandom());
 XeenTerrainArrivalCandidate arrival(rule,characters,inputs,*work->party.encounterContext,&*work->party.serviceEconomy);
 XeenConsequenceDraw draw{random,64,[&] {_eventPublication->check();}};
 if(!arrival.service(draw))throw std::logic_error("Physical terrain fall requested random continuation");
 work->world->publishTerrainArrival(work->party,arrival,random.continuation());
 for(const auto owner:owners)_eventPublication->damageEffects[owner]=0;
 work->guard=std::make_unique<XeenRestoreGuard>(*work->world,work->party,work->camera,work->flags);
 _eventPublication->check();_transition=std::move(work);
 _encounter->_terrainFall.reset();
 drive(XeenManualEventResult{XeenManualEventCompleted{0,true,false}},false);
}

void XeenEventFlow::retainManualPrefix(bool party) {
 auto &work=*_transition;checkTransitionCandidate();
 const auto ending=*work.world->sessionState().journeyRandom();
 if(party || !work.observed) {
  auto prefix=std::make_unique<TransitionCandidate::ObservedPrefix>();
  prefix->world=(party?*work.world:_world).transitionCandidate();
  prefix->world->copyEventParty(party?work.party:_party,prefix->party);
  prefix->camera=party?work.camera:_camera;prefix->flags=party?work.flags:_flags;
  prefix->world->retainEventRandom(ending);
  for(auto &effect:prefix->world->scenePresentation().portraits) {effect.damageTicks=0;effect.spellFrame=4;}
  prefix->guard=std::make_unique<XeenRestoreGuard>(*prefix->world,prefix->party,prefix->camera,prefix->flags);
  work.observed=std::move(prefix);
 } else {
  auto &prefix=*work.observed;prefix.guard->prepareEventAnimationRandom(ending);
  prefix.world->retainEventRandom(ending);prefix.guard->adoptMutationBoundary();
 }
 _eventPublication->check();
}
void XeenEventFlow::publishManualPrefix() {
 auto &work=*_transition;if(work.automatic)return;checkTransitionCandidate();
 if(work.observed)_eventPublication->publishSequencePrefix(_world,_party,*work.observed->guard);
}

bool XeenEventFlow::serviceManualStep() {
 auto &work=*_transition;
 const auto retain=[&] {
  auto &step=*work.manualStep;
  work.world->retainEventStep(work.party,step,step.opportunity?step.opportunity->actors:step.actors);
  auto guard=std::make_unique<XeenRestoreGuard>(*work.world,work.party,work.camera,work.flags);
  guard->retainResources(*work.guard);work.guard.swap(guard);
 };
 if(!work.manualStep) {
  XeenRegionalActionCandidate step;step.owners=work.party.party.activeOrder();step.camera=work.camera;
  step.context=*work.party.encounterContext;step.actors=work.world->sessionState().regionalActors(work.camera.mapId);
  step.random=XeenCombatRandom(*work.world->sessionState().journeyRandom());
  for(unsigned n=0;n<6;++n) {step.characters[n]=work.party.roster.at(step.owners[n]);step.inputs[n]=*work.party.roster.combatInputs(step.owners[n]);}
  const auto charge=work.world->map(work.camera.mapId).geometry.isOutdoors()?10u:1u;
  auto ending=xeenPrepareTime(step.context,charge);if(ending.dailyProcessing)xeenResetPartyTemps(ending.context);
  // Refuse an unavailable fall before time, ranged injuries or any Event prefix
  // can escape the detached manual action. This check spends no random draw.
  const auto rule=work.world->prepareTerrainArrival(work.camera,step.characters,ending.context);
  _eventPublication->check();work.guard->check();
  if(!rule) {_encounter->_journeyRefusal=rule.transition.reason;_candidateResultRefused=true;_transition.reset();return false;}
  step.time.emplace(step.context,charge,step.characters,step.inputs,&*work.party.serviceEconomy);
  step.remaining=1+unsigned(_encounter->state().pending()!=0);work.manualStep.emplace(std::move(step));
 }
 auto &step=*work.manualStep;
 XeenConsequenceDraw draw{step.random,64,[&] {_eventPublication->check();work.guard->check();}};
 if(!step.timeDone) {
  if(!step.time->service(draw))return false;
  const auto ctr=step.context.ctr24;step.context=step.time->context;step.context.ctr24=ctr;
  step.characters=step.time->characters;step.inputs=step.time->inputs;step.timeDone=true;
  step.result.needsRest=step.time->needsRest;
  bool living=false;for(const auto &c:step.characters)living=living || xeenCombatTargetable(c);
  if(!living)step.remaining=0;
 }
 while(step.remaining) {
  if(!step.opportunity) {
   if(work.world->map(step.camera.mapId).geometry.isOutdoors())
    step.opportunity.emplace(work.world->map(step.camera.mapId),step.actors,step.camera,step.characters,step.inputs,step.context.year,0x3f,std::array<bool,6>{},XeenActorOpportunityContext{},step.context);
   else step.opportunity.emplace(*work.world,step.actors,step.camera,step.characters,step.inputs,step.context.year,0x3f,std::array<bool,6>{},XeenActorOpportunityContext{},step.context);
   step.opportunity->staged=true;
  }
  auto &op=*step.opportunity;
  // The detached world draws this step; Event publication adopts the flag.
  if(op.dangerSense)work.world->scenePresentation().dangerSense=*op.dangerSense;
  if(!op.service(draw)) {
   if(op.impactApplied)step.characters=op.characters;
   retain();
   if((!op.travelPresented && !op.travelPublished) || (op.impactOwner && !op.portraitPublished)) {
    _encounter->guardCallback(_encounter->ticket(),[&] {_encounter->_lastTime=_clock();});
    _encounter->observeRanged(op.presentation(),work.world.get(),&step.camera);
    if(!op.travelPresented)op.travelPublished=true;
    if(op.impactOwner)op.portraitPublished=true;
   }
   return false;
  }
  step.characters=op.characters;step.actors=op.actors;step.opportunity.reset();--step.remaining;++step.result.movementOpportunities;
 }
 retain();work.manualStepDone=true;return true;
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
    if(_entranceUi)_entranceUi->animation.acquired(presentationNow());
    if(_smithUi && (_smithUi->animation.awaitingFrame || _smithUi->animation.resetAfterFrame)) {
        _smithUi->animation.deadline=PresentationClock::after(presentationNow(),3);
        _smithUi->animation.resetAfterFrame=false;
        _smithUi->animation.awaitingFrame=false;_smithUi->animation.pendingIndex.reset();
    }
    if(_trainingUi && (_trainingUi->animation.awaitingFrame || _trainingUi->animation.resetAfterFrame)) {
        _trainingUi->animation.deadline=PresentationClock::after(presentationNow(),3);
        _trainingUi->animation.resetAfterFrame=false;
        _trainingUi->animation.awaitingFrame=false;_trainingUi->animation.pendingIndex.reset();
    }
    if(_portraitSequence && !(_transition && _transition->presentationRetry))_portraitSequence->acquired=true;
    if(_death)deathPresented();
	if(_treasureWait && !_treasureWait->acquired) {
		_treasureWait->deadline=PresentationClock::after(presentationNow(),_treasureWait->ticks);
		_treasureWait->acquired=true;
	}
    if(_encounter)_encounter->projectilesPresented();
    if(_npcSequence && (pendingNpc() || _presenter.hasNpcOffer())) {
        auto &npc=*_npcSequence;
        if(!npc.acquired) {
            npc.acquired=true;npc.composed=false;npc.choice.reset();
            npc.deadline=PresentationClock::after(presentationNow(),3);
            npc.timing.deadline=PresentationClock::ceilMilliseconds(npc.deadline);
            _presenter.retainNpcTiming(npc.timing);
        }
    }
	if (_cosmeticPending) {
		// A successfully acquired travel row arms only its cosmetic successor.
		if(_encounter) _encounter->castProjectilePresented();
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

 if(_transition && _transition->manualStep && _transition->manualStep->opportunity) {
  auto &op=*_transition->manualStep->opportunity;
  if(op.travelStarted && !_encounter->projectilesPending())op.travelPresented=true;
  if(op.impactOwner)op.impactPresented=true;
 }
	_actionableFrame = presented;
	if(_barrier && _barrier->portraitWaiting)_barrier->portraitPresented=true;
    if(_barrier && _barrier->trapWaiting)_barrier->trapPresented=true;
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
		if (_castingUi && _encounter->castingActive() && !_encounter->_rest)
			_encounter->authorizeCastingFrame(*_encounterFrame,_inputGeneration,presented);
		if (_smithUi && _encounter->_smith)
			_encounter->authorizeSmithFrame(_inputGeneration,presented);
		if (_trainingUi && _encounter->_training)
			_encounter->authorizeTrainingFrame(_inputGeneration,presented);
		if (_inventory.mode==XeenInventoryMode::UseTarget && !_encounter->itemUseReady() &&
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
    if((pendingNpc() || (_pending && _presenter.hasNpcOffer())))return false;
	if (!reset && cause != OrdinaryCause::Action &&
		(cause != OrdinaryCause::Idle || now < _ordinary.deadline)) return false;
	if (now > std::numeric_limits<std::uint64_t>::max() - 100 ||
		(!reset && _ordinary.phase == std::numeric_limits<std::uint64_t>::max()))
		throw std::overflow_error("Ordinary animation overflow");
	// One shared M22 policy: committed facing/map reset wins over the action step.
	_ordinary.phase = reset ? 0 : _ordinary.phase + 1;
	if (cause!=OrdinaryCause::None && !_sceneAdvancePending) {
        _world.scenePresentation().beginDraw();
        _sceneAdvancePending=true;
    }
    _world.scenePresentation().advanceFeedback(now/1000000);
	_ordinary.deadline = PresentationClock::after(now,2);
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
		_encounter->guardCallback(entry,[&] { now = _presentationClock(); });
		if (!_encounter->current(entry)) throw std::runtime_error("Stale ordinary animation callback");
		// Window 11 suppresses draw3d, including its scenery/pose advancement.
        if((pendingNpc() || (_pending && _presenter.hasNpcOffer())))return false;
		const bool reset = _ordinary.mapId != _camera.mapId || _ordinary.direction != _camera.direction;
		return updateOrdinaryPhase(cause, reset, now);
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
    finishOrdinaryDraw();
}
void XeenEventFlow::finishOrdinaryDraw() {
    _world.scenePresentation().retainDrawingState();
    if(_sceneAdvancePending) {
        _world.scenePresentation().advanceAfterDraw(_camera.mapId);
        _sceneAdvancePending=false;
    }
    _world.scenePresentation().composeFromRetained(true);
}

std::optional<IndexedFrame> XeenEventFlow::mrWizard() {
	if(!journey() || _encounter->combat() || _pending || _castingUi || _barrier || inventoryOpen() ||
		!_encounter->mrWizard(_encounter->ticket()))return std::nullopt;
	return renderEncounter();
}
IndexedFrame XeenEventFlow::renderEncounter(bool report, bool cosmeticInput, bool reconstruct) {
    syncSpellItems();
    if(_encounter && !_encounter->_rest &&
        ((_encounter->_casting && _encounter->_casting->effectDone && _encounter->_casting->shelterPending) ||
         (_encounter->_itemUse && _encounter->_itemUse->effectDone && _encounter->_itemUse->shelterPending))) {
        if(!_encounter->beginRest(loadRestDream,[this] {
            if(!loadSampleDuration)throw std::invalid_argument("Original Rest sample duration provider is absent");
            return std::array<std::uint64_t,2>{loadSampleDuration("dreams2.voc",XeenSceneArchive::Darkside),loadSampleDuration("laff1.voc",XeenSceneArchive::Darkside)};
        },true))throw std::logic_error("Super Shelter Rest entry failed");
    }
    prepareStandalonePortrait();
	if (_smithUi) {
		cosmeticInput=_smithRenderedRevision && *_smithRenderedRevision==_smithUi->revision;
        if(!cosmeticInput)_smithUi->animation.resetAfterFrame=true;
		if (_encounter->_smith && _encounter->_smith->frame &&
			(!xeenSmithAuthorityRoom(_inputGeneration,8) || !_encounter->smithCapacity(16,4))) return frameCopy();
	}
	if(_trainingUi) {
		cosmeticInput=_trainingRenderedRevision && *_trainingRenderedRevision==_trainingUi->revision;
        if(!cosmeticInput)_trainingUi->animation.resetAfterFrame=true;
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
		report=report || (!_death && _encounter->state().phase()==XeenEncounterPhase::SupportStopped);
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
				auto composed = composeJourneyScene(reconstruct);
				if (!_encounter->current(t)) throw std::logic_error("Stale Journey composition");
				if (!composed.frame.isValid()) throw std::runtime_error("Invalid Journey frame");
				// Party updates belong to the base: bottom Event windows cover them.
				drawPartyPresentation(composed.frame);
				auto rendered = _journeyEventLayers ? _presenter.rebase(composed.frame) :
                    _castingUi && !_encounter->_rest ? drawCasting(composed.frame) :
                    _encounter->_rest ? composed.frame : drawMainScreenNotice(composed.frame,_encounter->notice());
				if(_encounter->monsterReward()) {
					if(!_monsterReceiptPresented) {
						_presenter.clear();XeenPresentationRequest request;
						request.kind=XeenPresentationKind::RewardReceipt;request.response=XeenPresentationResponseRequirement::Acknowledgment;
						request.mapId=_camera.mapId;
                        const auto &delivery=*_encounter->_monsterReceipt;const auto &receipt=delivery.receipt;
                        request.treasure.emplace();auto &treasure=*request.treasure;
                        using Phase=XeenRewardPhase;
                        request.kind=delivery.phase==Phase::Warning?XeenPresentationKind::RewardWarning:XeenPresentationKind::RewardReceipt;
                        treasure.gold=receipt.gold;treasure.gems=receipt.gems;
                        treasure.stage=delivery.phase==Phase::Blank?XeenTreasureDisplay::Stage::Blank:
                            delivery.phase==Phase::Found?XeenTreasureDisplay::Stage::Found:XeenTreasureDisplay::Stage::Final;
                        std::size_t first=delivery.cursor;
                        if(delivery.phase==Phase::Receipt)for(std::size_t i=receipt.count;i>0;--i)if(receipt.entries[i-1].owner){first=i-1;break;}
                        for(std::size_t i=first;i<receipt.count && i==first;++i) {
                            const auto &entry=receipt.entries[i];if(!entry.owner)continue;
                            treasure.items.push_back({_party.roster.at(*entry.owner).name,
                                entry.quest?dosText().table("QUEST_ITEM_NAMES").at(entry.item.id-82):
                                _catalog.foundName(static_cast<XeenInventoryCategory>(entry.category),entry.item),entry.quest});
                        }
                        prepareTreasureWait(_encounter->_rewardLease,
                            delivery.phase==Phase::Blank?1:delivery.phase==Phase::Found?10:0,true);
                        _presenter.originalAssets(dialogText,drawDialogSprite);
						rendered=_presenter.present(composed.frame,request).frame;_monsterReceiptPresented=true;
					} else rendered=_presenter.rebase(composed.frame);
				}
                if(_encounter->_rest)rendered=drawRest(rendered);
                if(_encounter->_needsRestNotice)rendered=drawXeenErrorScroll(rendered,_inventoryFont,std::string(xeenDialogText(dosText(),XeenDialogText::PartyNeedsRest)));
    const bool heads=drawPendingHeads(rendered);
                if(!_whoWillRefusal.empty())rendered=drawXeenErrorScroll(rendered,_inventoryFont,_whoWillRefusal);
				_inventoryUnderlay = rendered;
				// Item travel keeps its inventory selection and leases, but the
				// original closes the item windows before the spell handler runs.
				if (inventoryOpen() && !_travelUi) rendered = drawCharacterDialog(rendered);
                if(_travelUi)rendered=drawTravelDialog(rendered);
				if (_smithUi) rendered=drawSmith(composed.frame);
				if (_trainingUi) rendered=drawTraining(composed.frame);
                if(_entranceUi)rendered=_presenter.rebase(_entranceUi->art);
				if(_summary)rendered=drawSummary(rendered);
                if(_quickFightMember)rendered=drawQuickFightOptions(rendered);
				if(partyDeathRequired() || _death)rendered=drawDeath(composed.frame);
				if (report && !attempt) {
					if (reportText) reportText(_encounter->notice());
					if (!_encounter->current(t)) throw std::logic_error("Stale Journey reporting");
					std::cout << _encounter->journeyInspection();
				}
				if (!attempt && beforeEncounterFrameCopy) beforeEncounterFrameCopy();
				if (!_encounter->current(t)) throw std::logic_error("Stale Journey frame copy");
				returned = rendered; _frame = std::move(rendered);
				_ordinary.containsOrdinaryAnimation = composed.containsOrdinaryAnimation || heads;
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
            _world.scenePresentation().composeFromRetained(reconstruct && !_sceneAdvancePending);
			const auto composed=_encounterCompose(_ordinary.phase,_encounter->appearance());
			if (!_encounter->current(entry)) throw std::runtime_error("Stale encounter frame");
			if (!composed.frame.isValid()) throw std::runtime_error("Invalid encounter frame");
			const auto notice = _encounter->combat() && _encounter->combat()->cast() ? std::string{} : _encounter->notice();
			auto rendered = journey() ? drawMainScreenNotice(composed.frame,notice) :
                noticeFrame(composed.frame, _inventoryFont, notice, _encounter->combat(),journey());
			if (report && !attempt && reportText) reportText(notice);
            drawPartyPresentation(rendered);
            if(_encounter->combat() && _encounter->combat()->cast())rendered=drawCasting(rendered);
                if(_encounter->_needsRestNotice)rendered=drawXeenErrorScroll(rendered,_inventoryFont,std::string(xeenDialogText(dosText(),XeenDialogText::PartyNeedsRest)));
			if (!_encounter->current(entry)) throw std::runtime_error("Stale encounter report");
			if(partyDeathRequired() || _death) {
                auto deathBase=composed.frame;drawPartyPresentation(deathBase);rendered=drawDeath(deathBase);
            }
            // Complete the fallible return copy before installing the frame.
			if (!attempt && beforeEncounterFrameCopy) beforeEncounterFrameCopy();
			if (!_encounter->current(entry)) throw std::runtime_error("Stale encounter frame copy");
			returned = rendered;
			_frame = std::move(rendered);
			if (_encounter->combat()) {
				_inventoryUnderlay = _frame;
				if (!_death && inventoryOpen() && !_travelUi) {
					_frame = drawCharacterDialog(_inventoryUnderlay);
					returned = _frame;
				}
                if(!_death && _travelUi){_frame=drawTravelDialog(_inventoryUnderlay);returned=_frame;}
				if(!_death && _summary){_frame=drawSummary(_frame);returned=_frame;}
                if(!_death && _quickFightMember){_frame=drawQuickFightOptions(_frame);returned=_frame;}
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

XeenEventFlow::XeenEventFlow(XeenProgramState &program, XeenWorld &world, XeenEventSystem &events,
		XeenPartyState &party, XeenCamera &camera, XeenGameFlags &flags,
		const XeenFontFormat &font, Compose compose, XeenEventPresenter::NpcDraw npcDraw,
		XeenEventPresenter::Clock clock, XeenEventPresenter::RandomFrame randomFrame, const XeenItemCatalog *catalog,
		EncounterCompose encounterCompose, const XeenJourneySetup *journey,
		TransitionCompose transitionCompose, const XeenDosText *text, PresentationClock::Source presentationClock) :
	_transitionCompose(std::move(transitionCompose)),
	_inventoryFont(font), _catalog(catalog ? *catalog : fallbackCatalog()),
	_program(xeenBorrowProgram(program,world)), _world(world), _gameplayBorrow(world.sessionState().journey() ? nullptr : new XeenWorld::GameplayBorrow(world,party,camera,flags)),
	_events(events), _party(party), _camera(camera), _flags(flags),
	_navigation(events), _clock(clock ? clock : XeenEventPresenter::Clock{[] {
		return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count());
	}}), _presentationClock(presentationClock ? std::move(presentationClock) : clock ?
        PresentationClock::Source{[this] {return PresentationClock::fromMilliseconds(_clock());}} :
        PresentationClock::Source{[] {return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());}}),
    _presenter(font, std::move(npcDraw)),
	_ordinary{0, 0, camera.mapId, camera.direction, false},
	_compose(std::move(compose)) {
    dialogText=text;
    _npcRandomFrame=std::move(randomFrame);
	if (journey || world.sessionState().journey()) {
		if (!encounterCompose) throw std::invalid_argument("Journey requires exclusive presentation setup");
		_encounterCompose = std::move(encounterCompose);
		if (journey) _encounter = std::make_unique<XeenEncounterFlow>(_program,world,party,camera,flags,_clock,*journey,&_presentationClock);
		else _encounter = std::make_unique<XeenEncounterFlow>(_program,world,party,camera,flags,_clock,XeenJourneyRestoreTag{},&_presentationClock);
        _encounter->_travelEventsProvider=[this](XeenMapIdentity id){return _events.scriptForMap(id).file();};
	}
    _ordinary.deadline=PresentationClock::after(_presentationClock(),2);
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
		return renderEncounter(false,false,true);
	}
	if (reconstruct) invalidateInventorySelection();
	refreshScene(reconstruct, OrdinaryCause::None);
	return frameCopy();
}

bool XeenEventFlow::textPromptOpen() const {
	return _presenter.wordInputActive() || (_smithUi && _smithUi->phase==SmithUi::Phase::Amount) ||
		(_travelUi && _travelUi->spell!=XeenLearnedSpell::LloydsBeacon && _travelUi->refusal.empty());
}

XeenEventFlow::Composition XeenEventFlow::composeJourneyScene(bool reconstruct) {
 _world.scenePresentation().textPrompt=textPromptOpen();
 if(_transition && _transition->manualStep && !_transition->terrainDone) {
  auto &work=*_transition;checkTransitionCandidate();
  {
   XeenRestoreGuard::EventProviders providers(*work.world,work.party,work.camera,work.flags,*work.guard,
       [&] {_encounter->journeySavePreimage().check();},[&] {_encounter->journeySavePreimage().failed=true;});
   const auto light=work.world->lightAfterScenePreparation(work.party,work.camera);
   _encounter->journeySavePreimage().check();
   work.party.encounterContext->lightAndResistances[0]=light;work.world->scenePresentation().lightingPrepared=true;
  }
  {
   auto prepared=std::make_unique<XeenRestoreGuard>(*work.world,work.party,work.camera,work.flags);
   prepared->retainResources(*work.guard);work.guard.swap(prepared);
  }
  work.world->scenePresentation().textPrompt=textPromptOpen();
  XeenRestoreGuard::Providers providers(*work.guard,*work.world,[&] {_encounter->journeySavePreimage().check();});
  work.world->scenePresentation().composeFromRetained(reconstruct && !_sceneAdvancePending);
  auto frame=_transitionCompose(*work.world,work.party,work.camera,_ordinary.phase,_encounter->appearance());
  checkTransitionCandidate();work.world->scenePresentation().completeLightingDraw();work.guard->adoptMutationBoundary();return frame;
 }

	if(_transition && _pending && _pending->state.continuedAtDestination) {
		auto &work=*_transition;
		const auto &state=_pending->state;
		const auto checkRetained=[&] {
			requireCurrentOwners();_encounter->journeySavePreimage().check();checkTransitionCandidate();
			if(!work.continuation)throw std::logic_error("Destination drawing lost its Event continuation");
			work.continuation->check(state);
			if(_eventPublication)_eventPublication->continuation(state,*work.continuation,true);
		};
		checkRetained();
		// DOS 0x2711f..0x27127 redraws before destination Events. Draw from the
		// retained candidate, including idle/retry frames, without publishing its
		// camera/effects or changing refusal and published-prefix ownership.
		XeenRestoreGuard candidate(*work.world,work.party,state.workingCamera,work.flags);
		candidate.retainResources(*work.guard);
		XeenRestoreGuard::Providers providers(candidate,*work.world,checkRetained);
		try {
			work.world->scenePresentation().composeFromRetained(reconstruct && !_sceneAdvancePending);
			auto frame=_transitionCompose(*work.world,work.party,state.workingCamera,_ordinary.phase,XeenMonsterAppearance{0});
			candidate.check();checkRetained();return frame;
		} catch(...) {candidate.check();checkRetained();throw;}
	}
	_world.scenePresentation().composeFromRetained(reconstruct && !_sceneAdvancePending);
	return _encounterCompose(_ordinary.phase,_encounter->appearance());
}

bool XeenEventFlow::refreshScene(bool reconstruct, OrdinaryCause cause, bool committedTransition) {
	requireCurrentOwners();
	if (journey() && _eventPublication) {
		auto composition=composeJourneyScene(reconstruct);
		requireCurrentOwners();
		_frame=_presenter.rebase(composition.frame);
        drawPartyPresentation(_frame);
  const bool heads=drawPendingHeads(_frame);
        if(!_whoWillRefusal.empty())_frame=drawXeenErrorScroll(_frame,_inventoryFont,_whoWillRefusal);
		requireCurrentOwners();
		_ordinary.containsOrdinaryAnimation=composition.containsOrdinaryAnimation || heads;
		return true;
	}
	if (_encounter && (journey() || _encounter->combat())) { if (!journey()) renderEncounter(); return true; }
	try {
	const bool reset = committedTransition || _ordinary.mapId != _camera.mapId ||
		_ordinary.direction != _camera.direction;
	bool stepped = false;
	if (reset || cause != OrdinaryCause::None)
		stepped = updateOrdinaryPhase(cause, reset, presentationNow());
	const bool cameraChanged = !sameCamera(_camera, _renderedCamera);
	// M15's disabled set only grows during a session. Its size is an exact,
	// constant-time change detector for the only supported visual mutation.
	const auto count = _world.sessionState().disabledObjectCount();
	if (reconstruct || reset || !_frame.isValid() || cameraChanged || count != _disabledObjects ||
			stepped) {
		// Always use the committed camera, never logicalAddress/workingCamera.
		_world.scenePresentation().composeFromRetained(!stepped && !_sceneAdvancePending);
		_world.scenePresentation().textPrompt=textPromptOpen();
		if(_party.encounterContext) {
            _party.encounterContext->lightAndResistances[0]=_world.lightAfterScenePreparation(_party,_camera);
            _world.scenePresentation().lightingPrepared=true;
        }
		const auto composition = _compose(_ordinary.phase);
		_world.scenePresentation().completeLightingDraw();
		requireCurrentOwners();
		_ordinary.containsOrdinaryAnimation = composition.containsOrdinaryAnimation;
		if (cameraChanged && !_pending) _presenter.clear();
		_frame = _presenter.rebase(composition.frame);
        finishOrdinaryDraw();
  _ordinary.containsOrdinaryAnimation=drawPendingHeads(_frame) || _ordinary.containsOrdinaryAnimation;
        if(!_whoWillRefusal.empty())_frame=drawXeenErrorScroll(_frame,_inventoryFont,_whoWillRefusal);
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
			if (_transition) {
                prepareDetachedResult(result);
                if(_transition && _transition->completion)return frameCopy();
            }
		// Adopt ownership before composition, reporting or presentation can fail.
		auto *suspended = std::get_if<XeenEventExecutionSuspended>(&result);
        if(!suspended && _presenter.hasNpcOffer()) {
            // DAT 0x28e92 -> CloseAll closes window 11, without Stop Audio.
            _presenter.closeNpcOffers();_npcSequence.reset();reconstruct=true;
        }
		if (suspended && journey() && (suspended->request.kind==XeenPresentationKind::ArmorRepairService ||
			 suspended->request.kind==XeenPresentationKind::TempleService ||
         suspended->request.kind==XeenPresentationKind::TrainingService ||
         suspended->request.kind==XeenPresentationKind::TavernService ||
         suspended->request.kind==XeenPresentationKind::BankService || suspended->request.kind==XeenPresentationKind::GuildService) && !xeenSmithAuthorityRoom(_generation,1))
			throw std::overflow_error("Smith Event ticket generation exhausted before preparation");
		if (suspended) {
			_pending.emplace(Pending{std::move(suspended->state), automatic, ++_generation});
			if(journey())_regionalEventContinuation.emplace(_pending->state,true);
            if(_pending->state.pendingPresentation->request.kind==XeenPresentationKind::PortraitEffect) {
                const auto &effect=_pending->state.pendingPresentation->request.portrait;
                if(!effect)throw std::logic_error("Portrait effect request lost its retained phase");
                preparePortraitSequence(*effect,_pending->generation);
            }
		}
		if (suspended && (suspended->request.kind==XeenPresentationKind::ArmorRepairService ||
			suspended->request.kind==XeenPresentationKind::TrainingService ||
			suspended->request.kind==XeenPresentationKind::TempleService ||
            suspended->request.kind==XeenPresentationKind::TavernService ||
            suspended->request.kind==XeenPresentationKind::BankService || suspended->request.kind==XeenPresentationKind::GuildService))
			return frameCopy(); // Exclusive Event transfers to Service after its final guard check.
		if(suspended && suspended->request.kind==XeenPresentationKind::RewardGeneration)
			return frameCopy(); // Bounded generation resumes on the next idle update.
		try {
		refreshScene(reconstruct, cause, committedTransition);
		reconstruct = false;
		cause = OrdinaryCause::None;
		committedTransition = false;
		if (suspended && !_pending) return frameCopy();
		// Reporting receives a value snapshot; Flow has already adopted ownership
		// and can account for it if constructing this snapshot fails.
		if (suspended) suspended->state = _pending->state;
		if (journey() && automatic && std::holds_alternative<XeenEventExecutionError>(result) && !_candidateResultRefused)
			throw std::runtime_error("Automatic Journey event failed");
		if constexpr (std::is_same_v<Result, XeenManualEventResult>) {
			if (reportManual) reportManual(result);
		} else {
			if (reportAutomatic) reportAutomatic(result);
		}
		requireCurrentOwners();
		if (!suspended) return frameCopy();
		if (!_pending) return frameCopy(); // A reporting callback may explicitly abandon.
		if(!(_presenter.hasNpcOffer() && _pending->state.pendingPresentation->request.kind==XeenPresentationKind::CharacterSelection))
			_frame = _presenter.dismissSelection();
		XeenPresentationUpdate update;
		update = presentPendingEvent();
		_frame = std::move(update.frame);
		if (reportText) for (const auto &message : _presenter.diagnostics()) { reportText(message); requireCurrentOwners(); }
		requireCurrentOwners();
		if (!update.response) return frameCopy();
		auto pending = std::move(*_pending);
		_pending.reset(); // Consume before calling into the execution system.
		if constexpr (std::is_same_v<Result, XeenManualEventResult>)
			result = _transition ? resumeDetachedEvent(std::move(pending.state),*update.response) :
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

XeenEventSystem XeenEventFlow::detachedEventResources() {
    return XeenEventSystem([this](XeenMapIdentity id) {
        _eventPublication->check();auto script=_events.scriptForMap(id);
        if(id==_encounter->_journeyEvents.mapId)_eventPublication->script(script.file());
        _eventPublication->check();return script;
    },[this](XeenMapIdentity id) {
        _eventPublication->check();auto text=_events.textForMap(id);
        _eventPublication->check();_eventPublication->text(text);return text;
    });
}

XeenManualEventResult XeenEventFlow::beginDetachedEvent(XeenRegionalInteraction kind, bool automatic) try {
	if (_transition || !_eventPublication || !_encounter->journeyEvent() ||
		!_transitionCompose)
		throw std::logic_error("Vertigo Event candidate is unavailable");
	_candidateResultRefused=false;
	_eventPublication->script(_events.scriptForMap(_camera.mapId).file());
	auto work=std::make_unique<TransitionCandidate>();
	work->kind=kind;work->automatic=automatic;
	work->world=_world.transitionCandidate();
	work->camera=_camera;work->flags=_flags;
	work->initialCamera=_camera;work->initialFlags=_flags;
	const auto root=xeenRegionalEvent(_encounter->_journeyEvents,_camera);
	if(!root)throw std::logic_error("Detached Event has no bound original entry");
	work->world->copyEventParty(_party,work->party);
	{
		XeenRestoreGuard guard(*work->world,work->party,work->camera,work->flags);
		XeenRestoreGuard::Providers providers(guard,*work->world,[&] {_eventPublication->check();});
		guard.check();_eventPublication->check();
		work->guard=std::make_unique<XeenRestoreGuard>(*work->world,work->party,work->camera,work->flags);
		work->guard->retainResources(guard);
	}
	_transition=std::move(work);
	XeenRestoreGuard::EventProviders providers(*_transition->world,_transition->party,_transition->camera,
		_transition->flags,*_transition->guard,[&] {_eventPublication->check();},
		[&] {_encounter->journeySavePreimage().failed=true;});
	const auto execute=[&]() -> XeenManualEventResult {
		auto events=detachedEventResources();
		if(!automatic)return events.runManualEvent(*_transition->world,_transition->party,_transition->camera,
			_transition->flags,nullptr);
		auto result=events.runAutomaticEvent(*_transition->world,_transition->party,_transition->camera,
			_transition->flags,nullptr);
		if(const auto *done=std::get_if<XeenAutomaticEventCompleted>(&result))
			return XeenManualEventCompleted{done->instructionCount,done->cameraChanged,done->flagsChanged};
		if(const auto *failed=std::get_if<XeenEventExecutionError>(&result))return *failed;
		if(auto *suspended=std::get_if<XeenEventExecutionSuspended>(&result))return std::move(*suspended);
		return XeenManualEventNoEvent{};
	};
	auto result=execute();
	auto prepared=std::make_unique<XeenRestoreGuard>(*_transition->world,_transition->party,_transition->camera,_transition->flags);
	prepared->retainResources(*_transition->guard);_transition->guard.swap(prepared);
	return result;
} catch (const std::logic_error &) {
	_encounter->journeySavePreimage().failed=true;
	throw;
}

XeenManualEventResult XeenEventFlow::resumeDetachedEvent(XeenEventExecutionState state,
		XeenPresentationResponse response) {
	if (!_transition || !_eventPublication) throw std::logic_error("Vertigo Event continuation is absent");
	_eventPublication->check();
	checkTransitionCandidate();
	if(response==XeenPresentationResponse::No)_transition->refused=true;
	if(std::holds_alternative<CharacterSelectionCancelled>(response.value))_transition->cancelled=true;
	if(!_transition->continuation) {
		_encounter->journeySavePreimage().failed=true;
		throw std::logic_error("Detached Event resume has no retained suspension");
	}
	_eventPublication->continuation(state,*_transition->continuation,true);
	_transition->continuation.reset(); // Consume before invoking the interpreter.
	if(_transition->cancelled)
		return XeenManualEventCompleted{state.instructionCount,false,false};
	XeenRestoreGuard::EventProviders providers(*_transition->world,_transition->party,_transition->camera,
		_transition->flags,*_transition->guard,[&] {_eventPublication->check();},
		[&] {_encounter->journeySavePreimage().failed=true;});
	auto events=detachedEventResources();
	auto result=events.resumeManualEvent(std::move(state),response,*_transition->world,
		_transition->party,_transition->camera,_transition->flags,nullptr);
	_eventPublication->check();
	auto prepared=std::make_unique<XeenRestoreGuard>(*_transition->world,_transition->party,_transition->camera,_transition->flags);
	prepared->retainResources(*_transition->guard);_transition->guard.swap(prepared);
	return result;
}

void XeenEventFlow::prepareDetachedResult(const XeenManualEventResult &result) try {
	if (!_transition || !_eventPublication) return;
	checkTransitionCandidate();
	auto &work=*_transition;
	_eventPublication->check();
	if (const auto *suspended=std::get_if<XeenEventExecutionSuspended>(&result)) {
		work.continuation.emplace(suspended->state,true);
		if(suspended->request.kind==XeenPresentationKind::RewardReceipt ||
			suspended->request.kind==XeenPresentationKind::RewardWarning)
			publishEventSequencePrefix(suspended->state);
        if(suspended->request.kind==XeenPresentationKind::PortraitEffect)publishEventSequencePrefix(suspended->state);
		const bool scene=suspended->request.kind==XeenPresentationKind::EntranceScene || suspended->request.arena.has_value();
        if(scene)work.entrance=true;
		const bool service=suspended->state.pendingPresentation->request.source.opcode==0x11 && !scene;
		if(service) {
            if(suspended->state.pendingPresentation->continuation!=XeenEventPendingContinuation::Terminate ||
                suspended->state.workingCamera.mapId!=work.initialCamera.mapId ||
                suspended->state.workingCamera.x!=work.initialCamera.x || suspended->state.workingCamera.y!=work.initialCamera.y ||
                !xeen_state::sameCamera(work.camera,work.initialCamera) || suspended->state.treasureChanged())
                throw std::logic_error("Detached terminal service capability changed");
            _serviceEventContinuation=work.continuation;
            // Flags remain in the interpreter until completion. Prepare their
            // detached owner/guard before the single candidate publication.
            work.guard->prepareServicePrelude(suspended->state.workingCamera,suspended->state.workingGameFlags);
            work.camera=suspended->state.workingCamera;
            work.flags=suspended->state.workingGameFlags;
            work.guard->adoptMutationBoundary();
            _eventPublication->publishCandidate(_world,_party,_camera,_flags,*work.world,*work.guard);
			_transition.reset();
		}
		return;
	}
	if (const auto *failure=std::get_if<XeenEventExecutionError>(&result)) {
        publishManualPrefix();
		// Both retained owner guards have passed above. No unpublished effects
		// escape, and a valid already-published exit prelude remains authoritative.
		_encounter->_journeyRefusal="This Event is not supported yet";
		_candidateResultRefused=true;
		_transition.reset(); return;
	}
	if(std::holds_alternative<XeenManualSpecialInteractionUnsupported>(result) || std::holds_alternative<XeenManualEventNoEvent>(result)) {
		_encounter->_journeyRefusal="Door or grate: not supported yet";
		_candidateResultRefused=true;
		_transition.reset();return;
	}
	const auto *done=std::get_if<XeenManualEventCompleted>(&result);
	if (!done) throw std::logic_error("Vertigo Event did not reach a terminal result");
	if(work.cancelled) {publishManualPrefix();_transition.reset();return;}
	// Keep the completed candidate across recoverable presentation/allocation
	// failures. Retrying prepares/publicizes this value, never re-executes Events.
	work.completion=*done;
	if(!work.terrainDone && !work.terrain && !work.manualStep && (done->cameraChanged!=!xeen_state::sameCamera(work.camera,work.initialCamera) ||
		done->flagsChanged!=(work.flags.values()!=work.initialFlags.values())))
		throw std::logic_error("Event completion metadata or prelude boundary changed");
	// perform Space calls doStepCode once after the whole manual Event chain;
	// an automatic Event was already reached through movement's arrival suffix.
	if(!work.automatic && !work.entrance && !work.terrainDone) {
        if(!work.manualStepDone && !serviceManualStep())return;
		if(!work.terrain) {
			XeenConsequenceCharacters characters;XeenConsequenceInputs inputs;
			const auto owners=work.party.party.activeOrder();
			for(unsigned n=0;n<6;++n) {characters[n]=work.party.roster.at(owners[n]);inputs[n]=*work.party.roster.combatInputs(owners[n]);}
			const auto rule=work.world->prepareTerrainArrival(work.camera,characters,*work.party.encounterContext);
			if(!rule) {_encounter->_journeyRefusal=rule.transition.reason;_candidateResultRefused=true;_transition.reset();return;}
			if(rule.falling) {
				const auto destination=rule.transition.destination;
				work.world->loadEventDestination(work.camera.mapId,destination.mapId);work.camera=destination;
				work.guard=std::make_unique<XeenRestoreGuard>(*work.world,work.party,work.camera,work.flags);
			}
			work.terrain.emplace(rule,characters,inputs,*work.party.encounterContext,&*work.party.serviceEconomy);
		}
		XeenCombatRandom random(*work.world->sessionState().journeyRandom());
		XeenConsequenceDraw draw{random,64,[&] {_eventPublication->check();}};
		if(!work.terrain->service(draw)) {work.world->retainEventRandom(random.continuation());
			work.guard=std::make_unique<XeenRestoreGuard>(*work.world,work.party,work.camera,work.flags);return;}
		work.terrain->context.ctr24=(work.terrain->context.ctr24+1)%24;
        work.world->publishTerrainArrival(work.party,*work.terrain,random.continuation());

		if(work.terrain->rule.damage)for(auto owner:work.party.party.activeRosterIds())
			_eventPublication->damageEffects[owner]=work.terrain->rule.type==XeenDamageType::Fire?1u:0u;
		_encounter->_terrainDead=work.terrain->rule.dead;
		work.guard=std::make_unique<XeenRestoreGuard>(*work.world,work.party,work.camera,work.flags);work.terrainDone=true;
	}
	const bool sameMap=work.camera.mapId==work.initialCamera.mapId;
	if(sameMap) {
		if(!work.world->sampleCell(work.camera.mapId,work.camera.x,work.camera.y))
			throw std::logic_error("Same-map Event escaped its physical continuation capability");
		work.destinationEvents=_encounter->_journeyEvents;
	} else {
		const auto transition=work.world->prepareEventTransition(work.camera);
		if (!transition) {
			_encounter->_journeyRefusal=transition.reason;
			_candidateResultRefused=true;
			_transition.reset();return;
		}
		work.destinationEvents=_events.scriptForMap(work.camera.mapId).file();
		_eventPublication->check();
	}
	const auto before=_encounter->ticket();
	const auto arrival=[&] {
		XeenRestoreGuard::EventProviders providers(*work.world,work.party,work.camera,work.flags,
			*work.guard,[&] {_eventPublication->check();},
			[&] {_encounter->journeySavePreimage().failed=true;});
		// Every staged indoor table stays valid across the transition.
		std::vector<XeenMapIdentity> indoor;
		for(const auto &entry:work.world->sessionState().mapStates())
			if(entry.second.monsters && !entry.second.outdoors)indoor.push_back(entry.first);
		for(const auto mapId:indoor)
			xeenValidateIndoorActors(*work.world,mapId,work.world->sessionState().regionalActors(mapId));
		if(!sameMap) {
			if (!work.world->sessionState().hasRegionalActors(work.camera.mapId))
				work.world->stageMapMonsters(work.world->objectFile(work.camera.mapId),_encounter->_journeyStatistics);
			// Teleport dispatch already applied map-load lifetimes before destination Events.
		}
		return work.world->prepareTransitionArrival(work.camera);
	}();
	// The arrival's map load and wake writes belong to the retained candidate:
	// a recoverable failure below retries from here with this preimage.
	{
        XeenRestoreGuard::EventProviders providers(*work.world,work.party,work.camera,work.flags,*work.guard,
            [&] {_eventPublication->check();},[&] {_encounter->journeySavePreimage().failed=true;});
        const auto light=work.world->lightAfterScenePreparation(work.party,work.camera);
        _eventPublication->check();work.party.encounterContext->lightAndResistances[0]=light;
        work.world->scenePresentation().lightingPrepared=true;
    }
	{
		auto prepared=std::make_unique<XeenRestoreGuard>(*work.world,work.party,work.camera,work.flags);
		prepared->retainResources(*work.guard);work.guard.swap(prepared);
	}
	_eventPublication->check();
	XeenRestoreGuard candidateGuard(*work.world,work.party,work.camera,work.flags);
	candidateGuard.retainResources(*work.guard);
	XeenRestoreGuard::Providers providers(candidateGuard,*work.world,[&] {_eventPublication->check();});
	std::optional<std::vector<XeenExploredTile>> exploration,retainedExploration;
	if(!sameMap || work.manualStep) {
		exploration=work.world->explorationAfterStep(work.party,work.camera);
		retainedExploration=*exploration;candidateGuard.s.reserveTiles(*retainedExploration);candidateGuard.check();
	}
	const auto composed=[&] {
		try {
			auto frame=_transitionCompose(*work.world,work.party,work.camera,0,XeenMonsterAppearance{0});
			candidateGuard.check();return frame;
		} catch (...) { candidateGuard.check();throw; }
	}();
	if(!composed.frame.isValid())throw std::runtime_error("Vertigo destination frame is invalid");
    work.world->scenePresentation().completeLightingDraw();candidateGuard.adoptMutationBoundary();work.guard->adoptMutationBoundary();
	_eventPublication->check();
	if(!_encounter->current(before))throw std::logic_error("Stale Vertigo preparation");
	// Storage and the destination frame are ready. No provider or callback follows
	// until all owner writes and the new frame authority have been installed.
	_eventPublication->publishCandidate(_world,_party,_camera,_flags,*work.world,candidateGuard);
    if(work.manualStep)_encounter->settleManualStep(work.manualStep->result.needsRest || work.terrain->needsRest);
	if(exploration) {
		_encounter->publishJourneyExploration(*exploration,*retainedExploration);
	}
	// Retain the classified destination under the Event lease. Keep the Event's
	// concrete ticket intact until its final provider/publication checks finish;
	// contact changes the encounter phase at callback-free retirement below.
	_encounter->_result.view=arrival;
	using std::swap;
	swap(_encounter->_journeyEvents,work.destinationEvents);
	_encounter->journeySavePreimage().adoptMutationBoundary();
	if(!sameMap) {_presenter.clear();_journeyEventLayers=false;}
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
	_candidateResultRefused=false;
	const auto entry=_encounter->ticket();
	try {
		if(!_pending){_regionalExecutedRecords.clear();_regionalEventContinuation.reset();}
		else {
			try {if(!_regionalEventContinuation)throw std::logic_error("Regional suspension preimage absent");_regionalEventContinuation->check(_pending->state);}
			catch(const std::logic_error &){_encounter->journeySavePreimage().failed=true;throw;}
		}
		XeenEventPublication publication(_encounter->journeySavePreimage(),_encounter->_journeyEvents,[&] {
			if (!_dispatching || !_encounter->journeyEvent() || !_encounter->current(entry))
				throw std::logic_error("Stale Journey event continuation");
		},beforeRewardEnqueue,&_regionalExecutedRecords);
		if(_pending)publication.continuation(_pending->state,*_regionalEventContinuation,bool(_transition));
		XeenRestoreGuard::Providers providers(_encounter->journeySavePreimage(),_world,[&] { publication.check(); });
		_eventPublication=&publication;
		try { operation(); publication.check();
            for(unsigned owner=0;owner<30 && !_portraitSequence;++owner) {
             if(publication.portraitEffectOwners&(1u<<owner))_world.scenePresentation().spellEffect(owner,_encounter->_lastTime);
             if(publication.damageEffects[owner])_world.scenePresentation().portraitDamage(owner,*publication.damageEffects[owner],_encounter->_lastTime);
            }
        }
		catch (...) { _eventPublication=nullptr; throw; }
		_eventPublication=nullptr;
	} catch (const std::exception &error) {
		std::cerr << "Journey event: " << error.what() << '\n';
		_eventPublication=nullptr;
		if (_smithSettlement || _trainingSettlement) {
			if (_fatal || !_encounter->current(entry)) { _fatal=true;throw; }
			if(_smithUi)_smithUi->feedback="Unable to finish. Press Enter to retry.";
			if(_trainingUi)_trainingUi->feedback="Unable to finish. Press Enter to retry.";
			return renderEncounter();
		}
		if(_transition && (_transition->completion || _pending)) {
			_encounter->journeySavePreimage().check();checkTransitionCandidate();
			if(_fatal || !_encounter->current(entry))throw;
			if(_pending)_transition->presentationRetry=true;
			_encounter->_journeyRefusal="Unable to display the message. Retrying.";
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
		(_pending->state.pendingPresentation->request.kind==XeenPresentationKind::TempleService ||
         _pending->state.pendingPresentation->request.kind==XeenPresentationKind::TavernService ||
         _pending->state.pendingPresentation->request.kind==XeenPresentationKind::BankService || _pending->state.pendingPresentation->request.kind==XeenPresentationKind::GuildService) && !_smithUi)
		prepareSmith();
	if (_pending && _pending->state.pendingPresentation &&
		_pending->state.pendingPresentation->request.kind==XeenPresentationKind::TrainingService && !_trainingUi)
		prepareTraining();
	if (!_pending && !_transition) {
		// A refused candidate's notice replaces its retired Event windows.
		if (_candidateResultRefused && _journeyEventLayers) { _presenter.clear(); _journeyEventLayers=false; }
		_serviceEventContinuation.reset();_serviceEventOwner=nullptr;
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
	if(_barrier && _barrier->rule && _barrier->rule->selection)return true;
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
    if(_death)return true;
    if(_fatal)return false;
    if(_pending || _presenter.blocksGameplay() || _summary || _quickFightMember ||
        _barrier || inventoryOpen() || canCancelInteraction())return true;
    if(_encounter && (_encounter->combat() || _encounter->_rest || _encounter->_needsRestNotice ||
        _encounter->monsterReward()))return true;
    return journey() && _handoffPending;
}
XeenRewardReceipt XeenEventFlow::cleanup(XeenRewardDiscard reason) noexcept {
	XeenRewardReceipt outcome;
	if (_pending) {
		xeenDiscardRewards(_pending->state.reward.receipt, _pending->state.treasure, _pending->state.treasureBase(), reason);
		outcome = _pending->state.reward.receipt;
		_pending.reset();
	}
    _portraitSequence.reset();
    _entranceUi.reset();
	_presenter.discardTransient();
	_npcSequence.reset();
	_npcResponse.reset();
	_treasureWait.reset();
	_whoWillRefusal.clear();
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
	if(_transition) {
		_encounter->journeySavePreimage().check();checkTransitionCandidate();
		if(_transition->continuation)_transition->continuation->check(_pending->state);
		_transition->presentationRetry=true;
		if(!_portraitSequence)_presenter.discardTransient();
		if(pendingNpc() && _npcSequence)_npcSequence->acquired=false;
		return frameCopy();
	}
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
XeenPresentationUpdate XeenEventFlow::presentPendingEvent() {
    _presenter.originalAssets(dialogText,drawDialogSprite);
	auto request=_pending->state.pendingPresentation->request;
    if(request.arena) {
        using P=XeenArenaPhase;const auto &arena=*request.arena;
        if(arena.phase==P::Pool) {
            if(!_transition || !loadArenaSprites)throw std::invalid_argument("Arena sprite provider is absent");
            for(auto species:arena.pool) {
                loadArenaSprites(_encounter->_journeyStatistics.at(species).image());
                _eventPublication->check();checkTransitionCandidate();
            }
            return {_frame,XeenPresentationResponse::Presented,true};
        }
        request.title=dosText().scalar("WARZONE_BATTLE_MASTER");
        if(arena.phase==P::Level || arena.phase==P::Count || arena.phase==P::Maxed)
            request.text=dosText().scalar(arena.phase==P::Level?"WARZONE_LEVEL":arena.phase==P::Count?"WARZONE_HOW_MANY":"WARZONE_MAXED");
        if(arena.phase==P::Victory)request.text=xeenDialogFormat(request.text,
            {std::to_string(arena.wins),dosText().table("WARZONE_SUFFIXES").at(xeenArenaSuffix(arena.wins))});
    }
    if(request.kind==XeenPresentationKind::EntranceScene) {
        prepareEntrance();
        return {_presenter.rebase(_entranceUi->art),std::nullopt,true};
    }
    if(!_cdSpeechSide || *_cdSpeechSide!=request.mapId.side) {
        _cdSpeechSide=request.mapId.side;_cdSpeechCue.reset();_cdSpeechEnd=0;
    }
	if(request.kind==XeenPresentationKind::NpcAcknowledgment && !request.cdSpeech && !request.arena)
        request.cdSpeech=_cdSpeechCue;
    if(request.kind==XeenPresentationKind::PortraitEffect) {
        if(!request.portrait)throw std::logic_error("Event portrait request lost its retained effect");
        preparePortraitSequence(*request.portrait,_pending->generation);
        return {_frame,std::nullopt,true};
    }
    if(request.kind==XeenPresentationKind::RewardReceipt || request.kind==XeenPresentationKind::RewardWarning) {
        request.text.clear();request.treasure.emplace();auto &treasure=*request.treasure;
        const auto &state=_pending->state;
        if(request.kind==XeenPresentationKind::RewardWarning) {
            treasure.gold=state.treasure.gold;treasure.gems=state.treasure.gems;
        } else {
            const auto &party=_transition?_transition->party:_party;
            treasure.gold=state.reward.receipt.gold;treasure.gems=state.reward.receipt.gems;
            treasure.stage=state.reward.phase==XeenRewardPhase::Blank?XeenTreasureDisplay::Stage::Blank:
                state.reward.phase==XeenRewardPhase::Found?XeenTreasureDisplay::Stage::Found:XeenTreasureDisplay::Stage::Final;
            const auto first=state.reward.phase==XeenRewardPhase::Receipt && state.reward.cursor?state.reward.cursor-1:state.reward.cursor;
            for(std::size_t i=first;i<state.reward.receipt.count && i==first;++i) {
                const auto &entry=state.reward.receipt.entries[i];if(!entry.owner)continue;
                treasure.items.push_back({party.roster.at(*entry.owner).name,
                    entry.quest?dosText().table("QUEST_ITEM_NAMES").at(entry.item.id-82):
                    _catalog.foundName(static_cast<XeenInventoryCategory>(entry.category),entry.item),entry.quest});
            }
        }
        prepareTreasureWait(_pending->generation,request.rewardWaitTicks);
    }
	if(request.kind==XeenPresentationKind::CharacterSelection) {
		request.layoutValue=36;
		request.text=xeenDialogFormat(dosText().scalar("WHO_WILL"),
			{request.text,dosText().table("WHO_ACTIONS").at(request.verbIndex.value_or(0)),std::to_string(request.members.size())});
	}
	if(request.kind==XeenPresentationKind::CenteredMessage && request.notEnough && request.text.empty())
		request.text=xeenNotEnough(dosText(),*request.notEnough);
	if(request.kind==XeenPresentationKind::CenteredMessage && request.lockPicked && request.text.empty())
		request.text=xeenDialogFormat(dosText().scalar(*request.lockPicked?"PICKS_THE_LOCK":"UNABLE_TO_PICK_LOCK"),{request.title});
	const auto retainedPage=request.kind==XeenPresentationKind::NpcAcknowledgment && _npcSequence &&
        _npcSequence->generation==_pending->generation?_npcSequence->page:0;
	auto presented=_presenter.present(_frame,request,retainedPage);
    // Op60 only changes retained command operands. Playing/stopping does not
    // consume them; ordinary same-disc Event cleanup preserves this audio state.
    if(request.kind==XeenPresentationKind::DeferredAudio && request.source.opcode==60)
        _cdSpeechCue=_pending->state.cdSpeech;
    if(request.kind==XeenPresentationKind::NpcAcknowledgment &&
        (!_npcSequence || _npcSequence->generation!=_pending->generation)) {
        const auto now=presentationNow();
        NpcSequence next;
        next.generation=_pending->generation;
        next.deadline=PresentationClock::after(now,3);
        next.timing.deadline=PresentationClock::ceilMilliseconds(next.deadline);
        if(request.cdSpeech && (request.cdSpeech->track || request.cdSpeech->start || request.cdSpeech->finish)) {
            const auto duration=xeenCdSpeechDurationNanoseconds(*request.cdSpeech);
            _cdSpeechEnd=PresentationClock::checkedAdd(now,duration);
        }
        next.speechEnd=_cdSpeechEnd;
        next.timing.remaining=now<_cdSpeechEnd?_cdSpeechEnd-now:0;
        _npcSequence=next;
        _presenter.retainNpcTiming(next.timing);
    } else if(request.kind==XeenPresentationKind::NpcAcknowledgment && _npcSequence) {
        if(const auto retained=_presenter.updateNpc(_npcSequence->timing))presented.frame=*retained;
    }
 if(drawPendingHeads(presented.frame))_ordinary.containsOrdinaryAnimation=true;
 return presented;
}
void XeenEventFlow::publishEventSequencePrefix(const XeenEventExecutionState &state) {
	if(!_transition || !_eventPublication || !_transition->guard || !_transition->continuation)
		throw std::logic_error("Event sequence prefix has no retained authority");
	_eventPublication->continuation(state,*_transition->continuation,true);
	checkTransitionCandidate();
	if(_transition->automatic)_eventPublication->publishSequencePrefix(_world,_party,*_transition->guard);
    else {
        // These effects have their interpreter-issued portrait presentation.
        // Keep their gameplay prefix detached, but consume the candidate's
        // visual flags so final publication cannot arm that portrait again.
        for(auto &effect:_transition->world->scenePresentation().portraits) {
            if(effect.damageTicks && !effect.damageDeadline)effect.damageTicks=0;
            if(effect.spellFrame<4 && !effect.spellDeadline)effect.spellFrame=4;
        }
        retainManualPrefix(true);
    }
	// Party/RNG stores do not swap the detached World. Its checked interpreter
	// image remains intact for the next resume; camera/flag settlement is deferred.
}
void XeenEventFlow::prepareTreasureWait(std::uint64_t generation,unsigned ticks,bool monster) {
	if(!ticks){_treasureWait.reset();return;}
	if(_treasureWait && _treasureWait->generation==generation && _treasureWait->monster==monster &&
		_treasureWait->ticks==ticks && (!monster ||
		(_treasureWait->monsterCursor==_encounter->_monsterReceipt->cursor &&
		 _treasureWait->monsterPhase==_encounter->_monsterReceipt->phase)))return;
	_treasureWait=TreasureWait{generation,0,ticks,monster,false};
	if(monster) {
		_treasureWait->monsterCursor=_encounter->_monsterReceipt->cursor;
		_treasureWait->monsterPhase=_encounter->_monsterReceipt->phase;
	}
}
std::optional<IndexedFrame> XeenEventFlow::advanceTreasureWait() {
	if(!_treasureWait || !_treasureWait->acquired || _dispatching || _fatal || _saving || _handoffPending || _cosmeticPending)
		return std::nullopt;
	const auto now=presentationNow();
	if(now<_treasureWait->deadline || now<_ordinary.deadline)return std::nullopt;
	const auto wait=*_treasureWait;
	DispatchScope dispatch(_dispatching);
	if(wait.monster) {
		// Journey frame acquisition renews presentation tickets during the hold.
		// The existing reward lease and item phase identify the held receipt.
		if(!_encounter || !_encounter->monsterReward() ||
			!_encounter->_boundary.holds(XeenCombatBoundary::Work::Reward,wait.generation) ||
			_encounter->_rewardLease!=wait.generation || _encounter->_monsterReceipt->cursor!=wait.monsterCursor ||
			_encounter->_monsterReceipt->phase!=wait.monsterPhase)
			throw std::logic_error("Stale monster treasure hold");
		_treasureWait.reset();_encounter->advanceMonsterReward();_monsterReceiptPresented=false;
		return renderEncounter();
	}
	if(!_pending || _pending->generation!=wait.generation)throw std::logic_error("Stale Event treasure hold");
	if(journey())return journeyEventWork([&]{resumePending(wait.generation,XeenPresentationResponse::Acknowledged);});
	resumePending(wait.generation,XeenPresentationResponse::Acknowledged);return frameCopy();
}
bool XeenEventFlow::drawPendingHeads(IndexedFrame &frame) {
 if(!_pending || !_pending->state.pendingPresentation || !_presenter.blocksGameplay())return false;
 const auto &request=_pending->state.pendingPresentation->request;
 const auto &party=_transition?_transition->party:_party;
 if(!drawDialogSprite || (request.kind!=XeenPresentationKind::CharacterSelection && request.response!=XeenPresentationResponseRequirement::YesNo) || !party.encounterContext || !party.encounterContext->effects[XeenPartyEffect::Clairvoyance])return false;
 auto &world=_transition?*_transition->world:_world;
 const auto &camera=_transition && _pending->state.continuedAtDestination?_pending->state.workingCamera:
  _transition?_transition->camera:_camera;
 const auto heads=world.headIndicators(camera);
 const auto frames=XeenScenePresentation::clairvoyanceFrames(heads.left,heads.right,_ordinary.phase);
 drawDialogSprite(frame,"border.icn",frames.first,0,32);
 drawDialogSprite(frame,"border.icn",frames.second,215,32);
 return true;
}
void XeenEventFlow::preparePortraitSequence(const XeenEventPortraitEffect &effect,std::uint64_t generation) {
    if(!_encounter)throw std::logic_error("Portrait sequence requires an Encounter-owned presentation");
    if(_portraitSequence) {
        if(_portraitSequence->generation!=generation || !(_portraitSequence->effect==effect))
            throw std::logic_error("Portrait sequence replaced before completion");
        return;
    }
    PortraitSequence sequence;sequence.effect=effect;sequence.event=generation!=0;sequence.generation=generation;
    if(effect.stage==XeenEventPortraitStage::Sparkle)_presenter.closeWindow12();
    else if(effect.stage==XeenEventPortraitStage::DamageOverlay)_presenter.clear();
    sequence.window11=_presenter.window11Enabled();
    sequence.countdown=_encounter->combat()?_encounter->combat()->movementCountdown():_encounter->state().pending();
    const auto now=presentationNow();
    sequence.deadline=effect.stage==XeenEventPortraitStage::DamageOverlay?now:PresentationClock::after(now,2);
    auto &presentation=_world.scenePresentation();
    if(effect.stage==XeenEventPortraitStage::Sparkle)presentation.spellFrame(effect.owner,0);
    else if(effect.stage==XeenEventPortraitStage::DamageOverlay)presentation.portraitDamage(effect.owner,effect.frame);
    _portraitSequence=sequence; // Retain admission before fallible draw/provider work.
    if(effect.stage!=XeenEventPortraitStage::DamageOverlay && !sequence.window11)
        updateOrdinaryPhase(OrdinaryCause::Action,false,now);
}
void XeenEventFlow::prepareStandalonePortrait() {
    if(_portraitSequence || _barrier || !_encounter || (_pending && _pending->state.pendingPresentation &&
        _pending->state.pendingPresentation->request.kind==XeenPresentationKind::PortraitEffect))return;
    auto &presentation=_world.scenePresentation();
    for(auto owner:_party.party.activeRosterIds())if(presentation.portraits[owner].spellFrame<4) {
        preparePortraitSequence({XeenEventPortraitStage::Sparkle,owner,0});return;
    }
    if(_encounter->combat() && _encounter->combat()->result().attackStage==XeenCombatAttackStage::MonsterDamageOverlay) {
        const auto &result=_encounter->combat()->result();
        preparePortraitSequence({XeenEventPortraitStage::DamageOverlay,*result.targetOwner,presentation.portraits[*result.targetOwner].damageFrame});return;
    }
    const auto manual=_transition && _transition->manualStep && _transition->manualStep->opportunity?
        _transition->manualStep->opportunity->portrait():std::nullopt;
    if(const auto ranged=manual?manual:_encounter->rangedPortrait()) {
        preparePortraitSequence({ranged->second?XeenEventPortraitStage::DamageHold:XeenEventPortraitStage::DamageOverlay,
            ranged->first,presentation.portraits[ranged->first].damageFrame});
        _portraitSequence->ranged=true;return;
    }
    for(const auto &portrait:presentation.portraits)if(portrait.damageTicks) {
        preparePortraitSequence({XeenEventPortraitStage::DamageHold,0,0});return;
    }
}
std::optional<IndexedFrame> XeenEventFlow::advancePortraitSequence() {
    if(!_portraitSequence || !_portraitSequence->acquired || _dispatching || _fatal || _saving || _handoffPending || _cosmeticPending)
        return std::nullopt;
    const auto now=presentationNow();if(now<_portraitSequence->deadline)return std::nullopt;
    DispatchScope dispatch(_dispatching);auto &sequence=*_portraitSequence;
    const auto count=_encounter->combat()?_encounter->combat()->movementCountdown():_encounter->state().pending();
    if(count!=sequence.countdown)throw std::logic_error("Suspended portrait movement countdown changed");
    if(sequence.effect.stage==XeenEventPortraitStage::Sparkle && sequence.frame<3) {
        ++sequence.frame;sequence.acquired=false;sequence.deadline=PresentationClock::after(now,2);
        _world.scenePresentation().spellFrame(sequence.effect.owner,sequence.frame);
        if(!sequence.window11)updateOrdinaryPhase(OrdinaryCause::Action,false,now);
        return renderEncounter(false,true);
    }
    const auto finished=sequence;
    if(finished.effect.stage==XeenEventPortraitStage::Sparkle)_world.scenePresentation().spellFrame(finished.effect.owner,4);
    else if(finished.effect.stage==XeenEventPortraitStage::DamageHold)_world.scenePresentation().clearDamage();
    _portraitSequence.reset();
    if(finished.event) {
        if(!_pending || _pending->generation!=finished.generation || !_pending->state.pendingPresentation ||
            _pending->state.pendingPresentation->request.kind!=XeenPresentationKind::PortraitEffect)
            throw std::logic_error("Stale Event portrait completion");
        return journeyEventWork([&]{resumePending(finished.generation,XeenPresentationResponse::Presented);});
    }
    if(finished.ranged && _transition && _transition->completion && _transition->manualStep) {
        const auto completed=*_transition->completion;
        return journeyEventWork([&]{drive(XeenManualEventResult{completed},false);});
    }
    if(finished.ranged || (_encounter->combat() && (finished.effect.stage==XeenEventPortraitStage::DamageOverlay ||
        _encounter->combat()->result().attackStage==XeenCombatAttackStage::MonsterDamageHold))) {
        (void)_encounter->idle(_cycle);
        prepareJourneyTransition();
    }
    return renderEncounter(false,true);
}

bool XeenEventFlow::advanceNpcSequence() {
    if(!_npcSequence || (!pendingNpc() && !_presenter.hasNpcOffer()))return false;
    auto &npc=*_npcSequence;
    const auto now=presentationNow();
    if(npc.composed)return false;
    if(!npc.choice && (!npc.acquired || now<npc.deadline))return false;
    if(!npc.choice) {
        const bool busy=now<npc.speechEnd;
        unsigned frame=0;
        if(busy) {
            if(journey()) {
                if(!_eventPublication)throw std::logic_error("NPC animation publication lease is absent");
                checkTransitionCandidate();
                if(_transition && !_transition->automatic) {
                    XeenCombatRandom random(*_transition->world->sessionState().journeyRandom());
                    frame=*random.draw(0,3);const auto ending=random.continuation();
                    _transition->guard->prepareEventAnimationRandom(ending);
                    _transition->world->retainEventRandom(ending);_transition->guard->adoptMutationBoundary();
                    retainManualPrefix(false);
                } else {
                    frame=_eventPublication->npcAnimationFrame();
                    if(_transition) {
                        const auto &ending=*_world.sessionState().journeyRandom();
                        _transition->guard->prepareEventAnimationRandom(ending);
                        _transition->world->retainEventRandom(ending);_transition->guard->adoptMutationBoundary();
                    }
                }
            } else {
                if(!_npcRandomFrame)throw std::logic_error("NPC random continuation is absent");
                frame=_npcRandomFrame();requireCurrentOwners();
            }
        }
        if(frame>3)throw std::invalid_argument("NPC random frame outside 0..3");
        npc.choice=frame;
        npc.timing.remaining=busy?npc.speechEnd-now:0;
    }
    auto timing=npc.timing;
    timing.displayedFrame=timing.nextFrame=*npc.choice;
    ++timing.phase;
    timing.deadline=PresentationClock::ceilMilliseconds(PresentationClock::after(now,3));
    const auto changed=_presenter.updateNpc(timing);
    if(!changed)return false;
    _frame=*changed;npc.timing=timing;npc.acquired=false;npc.composed=true;
    return true;
}
IndexedFrame XeenEventFlow::consumeNpcResponse() {
    const auto retained=*_npcResponse;
    if(!_pending || _pending->generation!=retained.generation || !pendingNpc()) {_npcResponse.reset();return frameCopy();}
    auto update=_presenter.handle(retained.action,false);
    _npcResponse.reset();
    _frame=std::move(update.frame);
    if(update.response)resumePending(retained.generation,*update.response);
    else if(_npcSequence && pendingNpc()) {
        // Page publication does not restart the CD request.
        _npcSequence->timing.displayedFrame=0;
        _npcSequence->acquired=false;
        _npcSequence->page=_presenter.pageIndex();
    }
    return journey()?renderEncounter():frameCopy();
}
void XeenEventFlow::prepareEntrance() {
    if(_entranceUi)return;
    if(!_transition || !loadSampleDuration || !drawEntranceSprite)
        throw std::invalid_argument("Original entrance scene providers are absent");
    const auto action=*_pending->state.pendingPresentation->request.entranceAction;
    const auto *site=xeenEntranceSite(action,_pending->state.workingCamera);
    if(!site)throw std::logic_error("Entrance scene site is absent");
    checkTransitionCandidate();
    XeenEntranceAnimation animation(action,xeenEntranceKey(*site,_transition->party.questItems),[&](const char *name) {
        const auto value=loadSampleDuration(name,XeenSceneArchive::Clouds);
        _eventPublication->check();checkTransitionCandidate();return value;
    });
    _entranceUi.emplace(std::move(animation),_frame);
    _presenter.clear();_journeyEventLayers=false;
    // Reuse the passive voice deferral already used by Event speech. It adds
    // neither a caption nor an input wait; sample intervals remain silent.
    XeenPresentationRequest notice;notice.kind=XeenPresentationKind::DeferredAudio;
    notice.text="Voice audio not supported yet";notice.response=XeenPresentationResponseRequirement::Presented;
    _presenter.originalAssets(dialogText,drawDialogSprite);
    _presenter.present(_frame,notice);
    advanceEntrance();
}
bool XeenEventFlow::advanceEntrance() {
    checkTransitionCandidate();_eventPublication->check();
    auto animation=_entranceUi->animation;
    XeenCombatRandom random(*_transition->world->sessionState().journeyRandom());
    const auto command=animation.advance(presentationNow(),[&](unsigned lo,unsigned hi) {
        const auto value=random.draw(lo,hi);
        if(!value)throw std::logic_error("Entrance frame conversion was rejected");return *value;
    });
    if(!command){_entranceUi->animation=std::move(animation);return false;}
    const auto zoom=command->saveZoom?_entranceUi->art:_entranceUi->zoom;
    auto frame=command->restore?(command->phase==XeenEntranceAnimation::Phase::Rise?zoom:_entranceUi->saved):_entranceUi->art;
    for(const auto &sprite:command->sprites) {
        drawEntranceSprite(frame,sprite.resource,sprite.frame,sprite.x,sprite.y,sprite.scale);
        _eventPublication->check();checkTransitionCandidate();
    }
    const auto ending=random.continuation();
    _transition->guard->prepareEventAnimationRandom(ending);
    _transition->world->retainEventRandom(ending);_transition->guard->adoptMutationBoundary();
    _entranceUi->zoom=zoom;_entranceUi->art=std::move(frame);_entranceUi->animation=std::move(animation);
    return true;
}
std::optional<IndexedFrame> XeenEventFlow::updateEntrance() {
    if(_dispatching || _fatal || _saving || _handoffPending || _entranceUi->animation.awaitingFrame() ||
        presentationNow()<_entranceUi->animation.deadline())return {};
    DispatchScope dispatch(_dispatching);
    return journeyEventWork([&] {
        advanceEntrance();
        if(_entranceUi->animation.finished()) {
            _frame=_entranceUi->saved;_presenter.clear();
            resumePending(_pending->generation,XeenPresentationResponse::Presented);
            _entranceUi.reset();
        }
    });
}

void XeenEventFlow::startServiceSample(ServiceArtState &state,const char *name,bool resetIndex) {
    // A missing provider models the original sound-unavailable status (zero).
    std::uint64_t duration=0;
    if(loadSampleDuration)_encounter->guardCallback(_encounter->ticket(),[&]{duration=loadSampleDuration(name,XeenSceneArchive::Clouds);});
    state.sample.start(presentationNow(),duration);
    if(resetIndex)state.index=XeenServiceAnimation::modulus(state.kind)-1;
}
void XeenEventFlow::serviceInputPolled(ServiceArtState &state) {
    state.deadline=PresentationClock::after(presentationNow(),3);
}
bool XeenEventFlow::pulseServiceArt(ServiceArtState &state, IndexedFrame &art) {
    if(!drawDialogSprite || _dispatching || _fatal || _saving || _handoffPending || _cosmeticPending)return false;
    DispatchScope dispatch(_dispatching);
    const auto now=presentationNow();
    if(!state.deadline) {state.deadline=PresentationClock::after(now,3);return false;}
    if(state.awaitingFrame)return true; // Retry the same retained composition.
    if(!state.pendingIndex) {
        if(now<state.deadline)return false;
        const auto pendingIndex=XeenServiceAnimation::next(state.kind,state.index);
        state.pendingSpeaking.reset();state.randomBefore.reset();state.randomAfter.reset();state.randomPublished=false;
        if(state.sample.playing(now)) {
            if(const auto speaking=XeenServiceAnimation::speaking(state.kind)) {
                const auto before=*_world.sessionState().journeyRandom();
                const auto *day=_encounter->_smith?_encounter->_smith->departure.get():_encounter->_training->departure.get();
                const auto reserve=std::uint64_t(2)+(day->pendingGeneration()?XeenServiceDayCandidate::kDeferredRandomReserve:0);
                if(UINT64_MAX-before.count<reserve) {
                    // Optional visual work cannot spend a mandatory departure suffix.
                    state.deadline=PresentationClock::after(now,3);return false;
                }
                state.randomBefore=before;
                XeenCombatRandom random(*state.randomBefore);
                while(!state.pendingSpeaking)state.pendingSpeaking=random.draw(speaking->first,speaking->last);
                state.randomAfter=random.continuation();
            }
        }
        state.pendingIndex=pendingIndex;
    }
    auto candidate=art;
    const auto entry=_encounter->ticket();
    try {
        for(const auto &sprite:XeenServiceAnimation::compose(state.kind,*state.pendingIndex,state.pendingSpeaking))
            _encounter->guardCallback(entry,[&]{drawDialogSprite(candidate,sprite.resource.c_str(),sprite.frame,XeenServiceAnimation::kX,XeenServiceAnimation::kY);});
    } catch(const std::exception &) {
        _encounter->journeySavePreimage().check();return false; // Keep choice/index for retry.
    }
    if(state.randomBefore && !state.randomPublished) {
        try {_encounter->publishServiceAnimationRandom(*state.randomBefore,*state.randomAfter);}
        catch(const std::bad_alloc &) {_encounter->journeySavePreimage().check();return false;}
        state.randomPublished=true;
    }
    art=std::move(candidate);state.index=*state.pendingIndex;state.awaitingFrame=true;
    state.deadline=PresentationClock::after(presentationNow(),3); // Reset after work; no catch-up.
    return true;
}
std::optional<IndexedFrame> XeenEventFlow::updatePresentation() {
    requireCurrentOwners();
    if(_entranceUi)return updateEntrance();
    if(_presenter.wordInputActive() && !_dispatching && !_fatal && !_saving && !_handoffPending &&
       _presenter.animateWord(_clock())) {
        DispatchScope dispatch(_dispatching);return renderEncounter(false,true);
    }
    if(!_portraitSequence && !_dispatching && !_fatal && !_saving && !_handoffPending && _world.scenePresentation().feedbackActive()) {
        DispatchScope dispatch(_dispatching);prepareStandalonePortrait();
        if(_portraitSequence)return renderEncounter(false,true);
    }
    if(_portraitSequence && !(_transition && _transition->presentationRetry))return advancePortraitSequence();
    if(_npcResponse && _npcSequence && _npcSequence->acquired && !(_transition && _transition->presentationRetry) &&
        !_dispatching && !_fatal && !_saving && !_handoffPending) {
        DispatchScope dispatch(_dispatching);
        try {return journey()?journeyEventWork([&] {consumeNpcResponse();}):consumeNpcResponse();}
        catch(const std::exception &error) {if(!_pending)throw;return presentationFailed(error);}
    }
    if(!_death && partyDeathRequired() && !_dispatching && !_fatal && !_saving && !_handoffPending) {
        DispatchScope dispatch(_dispatching);return renderEncounter();
    }
    if(_death) {
        if(_dispatching || _fatal || _saving || _handoffPending || !_death->acquired)return {};
        DispatchScope dispatch(_dispatching);
        if(!advanceDeath())return {};
        return renderEncounter();
    }
	if(auto treasure=advanceTreasureWait())return treasure;
	if(_transition && _transition->presentationRetry && _pending &&
		!_dispatching && !_fatal && !_saving && !_handoffPending) {
		DispatchScope dispatch(_dispatching);
		return journeyEventWork([&] {
			_transition->presentationRetry=false;
			try {
				refreshScene(false,OrdinaryCause::None);
				const auto generation=_pending->generation;
				auto update=presentPendingEvent();_frame=std::move(update.frame);
				if(update.response)resumePending(generation,*update.response);
			} catch(const std::exception &e) {
				if(!_pending)throw;
				presentationFailed(e);
			}
		});
	}
	if(_pending && _pending->state.pendingPresentation &&
		_pending->state.pendingPresentation->request.kind==XeenPresentationKind::RewardGeneration &&
		!_dispatching && !_fatal && !_saving && !_handoffPending) {
		DispatchScope dispatch(_dispatching);const auto generation=_pending->generation;
		return journeyEventWork([&]{resumePending(generation,XeenPresentationResponse::Presented);});
	}
	if(_transition && _transition->completion && !_dispatching && !_fatal && !_saving && !_handoffPending) {
		DispatchScope dispatch(_dispatching);
        if(_encounter->projectilesPending()) {
            _encounter->animateProjectiles();return renderEncounter();
        }
		const auto completed=*_transition->completion;
		return journeyEventWork([&]{drive(XeenManualEventResult{completed},false);});
	}
	if(_barrier && !_dispatching && !_fatal && !_saving) {
		DispatchScope dispatch(_dispatching);
		// ipause keeps draw3d animation running while the barrier owns input.
		if(!_handoffPending && (_barrier->published || _barrier->trapWaiting))advanceEncounterOrdinary();
		if(_barrier->published && !_handoffPending && _clock()>=_barrier->deadline) {
            _world.scenePresentation().clearDamage();
			_barrier.reset();_encounter->endJourneyEvent();
			_encounter->journeyPulse(_encounter->ticket());prepareJourneyTransition();return renderEncounter();
		}
		if(!_barrier->published && (!_barrier->rule || !_barrier->rule->selection) && !_handoffPending) {
			serviceBarrier();return renderEncounter();
		}
	}
	if(_trainingUi) {
        if(auto next=updateTraining())return next;
		const bool pulsed=_trainingUi->phase==TrainingUi::Phase::Menu && pulseServiceArt(_trainingUi->animation,_trainingUi->art);
		if(!pulsed)return std::nullopt;
		DispatchScope dispatch(_dispatching);return renderEncounter(false,true);
	}
    if(_smithUi && _smithUi->phase==SmithUi::Phase::Departure && !_dispatching && !_fatal && !_saving && !_handoffPending) {
        if(_smithUi->goodbye && _smithUi->animation.sample.playing(presentationNow()))return std::nullopt;
        DispatchScope dispatch(_dispatching);
        if(_smithSettlement){_handoffPending=true;return settleSmithEvent();}
        if(_encounter->_smith && _encounter->_smith->frame)
            _encounter->consumeSmithFrame(_inputGeneration,_actionableFrame);
        try {
            if(!_encounter->departSmith())return std::nullopt;
            _smithSettlement=true;return settleSmithEvent();
        } catch(const std::exception &) {
            _encounter->journeySavePreimage().check();
            if(!_encounter->_smith)throw;
            return renderEncounter(); // A committed prefix needs a fresh concrete recovery frame.
        }
    }
    if(_travelUi) {
        if(_travelUi->spell!=XeenLearnedSpell::LloydsBeacon && _travelUi->refusal.empty() && _travelUi->digits.animate(_clock())) {DispatchScope dispatch(_dispatching);return renderEncounter(false,true);}
        return std::nullopt;
    }
	const bool smithArtPulsed=_smithUi && _smithUi->phase!=SmithUi::Phase::Preparation && _smithUi->phase!=SmithUi::Phase::Upgrade &&
		_smithUi->phase!=SmithUi::Phase::Departure && pulseServiceArt(_smithUi->animation,_smithUi->art);
	if(_smithUi && _smithUi->phase==SmithUi::Phase::Amount && _smithUi->feedback.empty() &&
		!_dispatching && !_fatal && !_saving && !_handoffPending) {
		const bool cursor=_smithUi->amountCursor.animate(_clock());
		if(cursor || smithArtPulsed) {DispatchScope dispatch(_dispatching);return renderEncounter(false,true);}
		return std::nullopt;
	}
	if(smithArtPulsed) {DispatchScope dispatch(_dispatching);return renderEncounter(false,true);}
	if (_smithUi && _smithUi->phase==SmithUi::Phase::Upgrade && !_dispatching && !_fatal && !_saving) {
		DispatchScope dispatch(_dispatching);
		try {
			if(!xeenSmithAuthorityRoom(_smithUi->revision,1))
				throw std::overflow_error("Temple UI revision exhausted before Heal publication");
			if(!_encounter->serviceTempleHeal())return std::nullopt;
            if(_encounter->_smith->published)startServiceSample(_smithUi->animation,"ahh.voc");
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
			if (!_encounter->serviceSmithPreparation(_smithUi->templePriceRevision)) return std::nullopt;
			_smithUi->phase=SmithUi::Phase::Lobby;
            startServiceSample(_smithUi->animation,XeenServiceAnimation::greeting(_smithUi->animation.kind));
            serviceInputPolled(_smithUi->animation);
		} catch (const std::exception &error) {
			_encounter->journeySavePreimage().check();
			if (_encounter->_smith) {
				_smithUi->phase=SmithUi::Phase::Lobby;
				_smithUi->feedback="Unable to finish. Press Escape to retry.";
			} else {
				_encounter->_smithPreparation.reset();_smithUi.reset();_pending.reset();
				_encounter->_journeyRefusal="Unable to enter the Ironworks. Try again.";
				_encounter->endJourneyEvent();
			}
		}
		if (_smithUi) ++_smithUi->revision;
		return renderEncounter();
	}
	if (_dispatching || _fatal || _saving || _smithUi || (journey() && _handoffPending)) return std::nullopt;
	DispatchScope dispatch(_dispatching);
	if(_quickFightMember) {
        if(_encounter && advanceEncounterOrdinary())return renderEncounter(false,true);
        return std::nullopt;
    }
	if(_summary) {
		if(*_summary==SummaryDialog::Map) {
			std::uint64_t now=0;
			if(_encounter)_encounter->guardCallback(_encounter->ticket(),[&]{now=_clock();});else now=_clock();
			if(now<_mapDeadline)return std::nullopt;
			_mapDeadline=PresentationClock::afterMilliseconds(now,2);++_mapPhase;
			if(_encounter)return renderEncounter(false,true);
			_frame=drawSummary(_summaryUnderlay);return _frame;
		}
		// Info redraws the scene, never the encounter scheduler. Quick Reference
		// waits on its retained underlay exactly as the original window does.
		if(*_summary==SummaryDialog::Info && _encounter && advanceEncounterOrdinary())return renderEncounter(false,true);
		return std::nullopt;
	}
    if(_spellItemsUi && !_spellItemsUi->error.empty())return std::nullopt;
	if(inventoryOpen()) {
        if(_spellItemsUi && _spellItemsUi->waiting && _encounter->idle(_cycle))return renderEncounter();
        if(_encounter && _world.scenePresentation().feedbackActive()) {
            std::uint64_t now;_encounter->guardCallback(_encounter->ticket(),[&] {now=_clock();});
            if(now>std::numeric_limits<std::uint64_t>::max()-100)throw std::overflow_error("Portrait effect clock exhausted");
            if(_world.scenePresentation().advanceFeedback(now))return renderEncounter(false,true);
        }
		if(_sheet && !_itemsVisible && !_statPopup && !_dialogError && _clock()>=_sheet->deadline) {
			_sheet->deadline=PresentationClock::afterMilliseconds(_clock(),4);_sheet->blink=!_sheet->blink;
			if(_encounter) return renderEncounter(false,true);
			drawInventory();return _frame;
		}
		return std::nullopt;
	}
	validateRegionalEvents();
	if(_castingUi && _castingUi->phase==CastingUi::Phase::Settling && !_castingUi->refusal.empty())return std::nullopt;
	if (_encounter) {
		if (journey() && _encounter->journeyEvent()) {
			const auto before = _encounter->ticket();
			if (advanceEncounterOrdinary()) return renderEncounter(false,
				_encounter->journeyEvent() && _encounter->current(before));
			if((pendingNpc() || (_pending && _presenter.hasNpcOffer()))) {
				try {
                    if(!_npcSequence || _npcSequence->composed ||
                        (!_npcSequence->choice && (!_npcSequence->acquired || presentationNow()<_npcSequence->deadline)))
                        return std::nullopt;
                    DispatchScope dispatch(_dispatching);
                    return journeyEventWork([&] {advanceNpcSequence();});
                }
				catch(const std::exception &e) {return presentationFailed(e);}
			}
			return std::nullopt;
		}
		const auto beforeIdle = _encounter->ticket();
        // Ordinary actor countdowns are serviced by draw3d, never by a host
        // repaint between draws. Retained work keeps its existing continuation.
        if(!_encounter->combat() && _encounter->journeyNavigationReady() &&
            presentationNow()<_ordinary.deadline)return std::nullopt;
		const bool quietBefore = journey() && _encounter->journeyMutable();
		const bool castingBefore = journey() && _castingUi && _encounter->castingActive();
		const bool hadJourneyCombat = journey() && _encounter->combat();
		const bool changed = _encounter->idle(_cycle);
		if (_castingUi && !_encounter->castingActive()) _castingUi.reset();
		prepareJourneyTransition();
		if (_encounter->combatOperationStale()) {
			_fatal = true;
			throw std::runtime_error("Stale automatic combat operation");
		}
		if (!_encounter->current(_encounter->ticket())) { _fatal = true; throw std::runtime_error("Stale encounter idle"); }
		// dream() polls Events without drawing gameplay. Freeze ordinary scene
		// counters until the saved background has been restored by the fades.
		const bool dreaming=_encounter->_rest && _encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Dream;
		const bool ordinary = !dreaming && advanceEncounterOrdinary();
		// Combat appearance/ordinary animation can replace the concrete frame while
		// retaining the exact semantic ticket. Do not retire fresh keys for that redraw.
		const bool combatCosmetic = _encounter->combat() && beforeIdle.combat &&
			_encounter->combat()->current(*beforeIdle.combat);
		// A cast prompt holds the world still; its scene redraw retains the
		// prompt's input, as the original keeps polling the same buttons.
		const bool castingCosmetic = castingBefore && !changed && _castingUi && _encounter->castingActive() &&
			_encounter->current(beforeIdle);
		if (changed || ordinary) return renderEncounter((hadJourneyCombat && !_encounter->combat()),
			combatCosmetic || castingCosmetic || (quietBefore && _encounter->journeyMutable() && _encounter->current(beforeIdle)));
		return std::nullopt;
	}
	const bool recomposed = refreshScene(false, OrdinaryCause::Idle);
	if (!pendingNpc() && !(_pending && _presenter.hasNpcOffer())) return recomposed ? std::optional<IndexedFrame>{_frame} : std::nullopt;
	try {
		const auto changed = advanceNpcSequence();
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
    if(_portraitSequence || !_whoWillRefusal.empty())return false;
	if (journey()) {
		if (_smithUi || _trainingUi) return false;
		if (_transition && _transition->presentationRetry) return false;
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
	// Service continuations belong exclusively to their registered settlement,
	// regardless of the service kind or the response supplied by this caller.
	if(_serviceEventContinuation || _serviceEventOwner || (_pending && _pending->state.pendingPresentation &&
		_pending->state.pendingPresentation->request.source.opcode==0x11 &&
        !_pending->state.pendingPresentation->request.arena &&
        _pending->state.pendingPresentation->request.kind!=XeenPresentationKind::EntranceScene))
		throw std::logic_error("Service Event continuation may only resume through its settlement owner");
    if(_pending && _pending->state.pendingPresentation && _pending->state.pendingPresentation->request.kind==XeenPresentationKind::EntranceScene &&
        (!_entranceUi || !_entranceUi->animation.finished()))throw std::logic_error("Entrance scene has not finished its acquired sequence");
	if (!_pending || _pending->generation != generation) return false;
	if (!_pending->state.pendingPresentation ||
		!xeenResponseMatches(_pending->state.pendingPresentation->request.response, response)) return false;
	if(_pending->state.pendingPresentation->request.kind==XeenPresentationKind::RewardReceipt ||
		_pending->state.pendingPresentation->request.kind==XeenPresentationKind::RewardWarning)_treasureWait.reset();
	if(journey()) {
		try {if(!_regionalEventContinuation)throw std::logic_error("Regional suspension preimage absent");_regionalEventContinuation->check(_pending->state);}
		catch(const std::logic_error &){_encounter->journeySavePreimage().failed=true;throw;}
	}
	try { _frame = _presenter.finishPresentation(); }
	catch (const std::exception &e) { presentationFailed(e); return true; }
	// DAT 0x25c94: final ordinary dismissal stops CD. Passive mode 2 returns
	// through 0x25ac4 without this stop; pages never resume the interpreter.
	if(_pending->state.pendingPresentation->request.kind==XeenPresentationKind::NpcAcknowledgment &&
		_pending->state.pendingPresentation->request.npc->confirmationMode!=2) {
		_cdSpeechEnd=0;
		if(_npcSequence){_npcSequence->speechEnd=0;_npcSequence->timing.remaining=0;}
	}
	requireCurrentOwners();
	auto pending = std::move(*_pending);
	_pending.reset();
	if (_transition)
		drive(resumeDetachedEvent(std::move(pending.state),response),pending.automatic);
	else if (pending.automatic)
		drive(_events.resumeAutomaticEvent(std::move(pending.state), response,
			_world, _party, _camera, _flags, _eventPublication), true);
	else
		drive(_events.resumeManualEvent(std::move(pending.state), response,
			_world, _party, _camera, _flags, _eventPublication), false);
	return true;
}

void XeenEventFlow::claimServiceContinuation(const void *owner) {
	requireCurrentOwners();
	if(!owner || !_pending || !_serviceEventContinuation || !_pending->state.pendingPresentation ||
		_pending->state.pendingPresentation->request.source.opcode!=0x11 ||
		(_serviceEventOwner && _serviceEventOwner!=owner))
		throw std::logic_error("Service Event continuation cannot acquire this settlement owner");
	_serviceEventContinuation->check(_pending->state);
	_serviceEventOwner=owner;
}

void XeenEventFlow::requireServiceSettlementOwner(const void *owner) const {
	if(!owner || owner!=_serviceEventOwner || !_pending || !journey() || !_encounter->journeyEvent())
		throw std::logic_error("Service Event settlement owner is absent or stale");
}

void XeenEventFlow::requireServiceSettlementEntry(const void *owner) const {
	requireServiceSettlementOwner(owner);
	if(_eventPublication)throw std::logic_error("Service Event settlement cannot be entered recursively");
}

XeenManualEventResult XeenEventFlow::resumeOwnedServiceEvent(const void *owner) {
	// This is the sole service-resume entry point. Generic resume never calls it.
	requireServiceSettlementOwner(owner);
	if(!_eventPublication || !_serviceEventContinuation)
		throw std::logic_error("Service Event continuation was already consumed or is not authorized");
	auto state=_pending->state;
	_eventPublication->continuation(state,*_serviceEventContinuation);
	_serviceEventContinuation.reset(); // One guarded resume, before any terminal callback.
	return _events.resumeManualEvent(std::move(state),XeenPresentationResponse::Acknowledged,
		_world,_party,_camera,_flags,_eventPublication);
}
IndexedFrame XeenEventFlow::handle(const PlayerAction &physicalAction, std::optional<std::uint64_t> displayedInput,
        const IndexedFrame::Presentation &inputFrame) {
	PlayerAction action=physicalAction;
	requireCurrentOwners();
    if(_portraitSequence)return frameCopy();
	// Concrete origin is independent of semantic meaning. A retry/redraw may
	// retain the generation, but input from its retired frame cannot authorize it.
	if ((inputFrame && !acceptsInputFrame(inputFrame)) || (_trainingUi && !inputFrame) ||
		(_smithUi && !inputFrame)) return frameCopy();
	if (journey() && !journeyInputCurrent(displayedInput)) return frameCopy();
    if(_death) {
        if(_dispatching || _fatal || _saving || _handoffPending || !_death->acquired ||
            _death->phase!=DeathContinuation::Phase::Scroll)return frameCopy();
        DispatchScope dispatch(_dispatching);
        _death->phase=DeathContinuation::Phase::Closed;_death->acquired=false;
        return renderEncounter();
    }
    if(pendingNpc() && _npcSequence && !_dispatching && !_fatal && !_saving &&
        !std::holds_alternative<SaveGameAction>(action)) {
        if(_npcResponse || !_npcSequence->acquired || presentationNow()<_npcSequence->deadline)return frameCopy();
        _npcResponse=NpcResponse{action,_pending->generation};
        DispatchScope dispatch(_dispatching);
        if(journey())return journeyEventWork([&] {if(advanceNpcSequence())renderEncounter(false,true);});
        try {advanceNpcSequence();return frameCopy();}
        catch(const std::exception &error) {return presentationFailed(error);}
    }
    if(_encounter && !_encounter->_combatCastRefusal.empty() && !_dispatching && !_fatal && !_saving) {
        _encounter->_combatCastRefusal.clear();
        if(const auto *key=std::get_if<DialogKeyAction>(&action);key && key->key=='c')action=CastSpellAction{};
        else if(std::holds_alternative<AcknowledgeAction>(action) || std::holds_alternative<CancelInteractionAction>(action)) {
            DispatchScope dispatch(_dispatching);return renderEncounter();
        }
    }
    if(!_whoWillRefusal.empty() && !_dispatching && !_fatal && !_saving) {
        _whoWillRefusal.clear();DispatchScope dispatch(_dispatching);
        if(_encounter)return renderEncounter();
        refreshScene(true,OrdinaryCause::None);return _frame;
    }
    if(_pending && _pending->state.pendingPresentation &&
        _pending->state.pendingPresentation->request.kind==XeenPresentationKind::CharacterSelection) {
        if(const auto *key=std::get_if<DialogKeyAction>(&action)) {
            if(key->key==InputKey::Escape)action=CancelInteractionAction{};
            else if(key->key>=InputKey::F1 && key->key<InputKey::F1+_pending->state.pendingPresentation->request.members.size())
                action=SelectMemberAction{key->key-InputKey::F1};
        }
    }
	if(_summary && !_dispatching && !_fatal && !_saving) {
        if(*_summary==SummaryDialog::Quests || *_summary==SummaryDialog::Awards) {
            const auto *key=std::get_if<DialogKeyAction>(&action);
            if(!std::holds_alternative<CancelInteractionAction>(action) && (!key || key->key!=InputKey::Escape)) {
                DispatchScope dispatch(_dispatching);
                if(key)_journalUi.key(key->key,journalLines().size());
                if(_encounter)return renderEncounter();
                _frame=drawSummary(_summaryUnderlay);return _frame;
            }
        }
		_summary.reset();_summaryUnderlay={};_journalStrings.clear();
		DispatchScope dispatch(_dispatching);
		if(_encounter)return renderEncounter();
		if(inventoryOpen())drawInventory();else refreshScene(true,OrdinaryCause::None);
		return _frame;
	}
	if(_quickFightMember && !_dispatching && !_fatal && !_saving) {DispatchScope dispatch(_dispatching);return handleQuickFightOptions(action);}
	if (std::holds_alternative<SaveGameAction>(action) || _dispatching || _fatal || _saving) return frameCopy();
    if(_travelUi) {DispatchScope dispatch(_dispatching);return handleTravelDialog(action,*displayedInput);}
    if(_entranceUi)return frameCopy();
	if(_encounter && _encounter->_needsRestNotice) {
  _encounter->_needsRestNotice=false;
  DispatchScope dispatch(_dispatching);return renderEncounter(); // Consume acknowledgment without leaking an action.
 }
 if(_trainingUi) {
  DispatchScope dispatch(_dispatching);
		return handleTraining(action,*displayedInput,inputFrame);
	}
	if (_smithUi) {
		DispatchScope dispatch(_dispatching);
		return handleSmith(action,*displayedInput,inputFrame);
	}
    if(_encounter && _encounter->_rest) {
        DispatchScope dispatch(_dispatching);
        if(!_encounter->respondRest(action))return frameCopy();
        return renderEncounter();
    }
	if (_encounter && _encounter->combat() && (!displayedInput || *displayedInput != _inputGeneration ||
		!_displayedCombat || !_encounter->combat()->current(*_displayedCombat))) return frameCopy();
	if (!_encounter && std::holds_alternative<WaitAction>(action)) return frameCopy();
	if (!_encounter && (std::holds_alternative<AttackAction>(action) || std::holds_alternative<BlockAction>(action) || std::holds_alternative<RunAction>(action) ||
		std::holds_alternative<BeginEncounterAction>(action) || std::holds_alternative<RevisitCompletedAction>(action))) return frameCopy();
	if (journey() && _encounter->combat() && std::holds_alternative<RevisitCompletedAction>(action)) action=RunAction{};
    if(_encounter && _encounter->combat())if(const auto *nav=std::get_if<NavigationAction>(&action);
        nav && (*nav==NavigationAction::StrafeLeft || *nav==NavigationAction::StrafeRight))return frameCopy();
    if (journey() && _encounter->combat() && !_encounter->combat()->cast() && !inventoryOpen())
        if(const auto *nav=std::get_if<NavigationAction>(&action);nav && *nav!=NavigationAction::TurnLeft && *nav!=NavigationAction::TurnRight)
            action=UnsupportedMainScreenAction{"Combat movement"};
    if (const auto *unsupported=std::get_if<UnsupportedMainScreenAction>(&action)) {
        const auto context=inputContext(inputFrame);
        if (context.mainScreen==MainScreen::None || !context.readyForAction) return frameCopy();
        _mainScreenNotice=std::string(unsupported->label)+": not supported yet";
        DispatchScope dispatch(_dispatching);
        if(std::holds_alternative<NavigationAction>(physicalAction))advanceEncounterOrdinary(OrdinaryCause::Action);
        if (reportText) reportText(_mainScreenNotice);
        return renderEncounter();
    }
    _mainScreenNotice.clear();
	DispatchScope dispatch(_dispatching);
    if(_pending && _pending->state.pendingPresentation &&
        _pending->state.pendingPresentation->request.kind==XeenPresentationKind::CharacterSelection) {
        bool redraw=false;
        if(!_whoWillRefusal.empty()) {_whoWillRefusal.clear();redraw=true;}
        else if(const auto *selected=std::get_if<SelectMemberAction>(&action)) {
            const auto &party=_transition?_transition->party:_party;
            if(selected->partyIndex<party.party.size()) {
                const auto &member=party.party.member(party.roster,selected->partyIndex);
                if(!member.canAct()) {_whoWillRefusal=xeenDialogFormat(dosText().scalar("IN_NO_CONDITION"),{member.name});redraw=true;}
            }
        }
        if(redraw) {
            if(_encounter)return renderEncounter();
            refreshScene(true,OrdinaryCause::None);return _frame;
        }
    }
    if(std::holds_alternative<ToggleAutomapAction>(action)) {
        if(_encounter && _encounter->toggleAutomap(_encounter->ticket()))return renderEncounter();
        if(!_encounter && !_pending && !inventoryOpen() && _party.encounterContext) {
            _party.encounterContext->effects[XeenPartyEffect::AutomapOn]=!_party.encounterContext->effects[XeenPartyEffect::AutomapOn];
            refreshScene(true,OrdinaryCause::None);return _frame;
        }
        return frameCopy();
    }
    if(std::holds_alternative<QuickReferenceAction>(action) || std::holds_alternative<InfoAction>(action) || std::holds_alternative<MapAction>(action) || std::holds_alternative<QuestsAction>(action)) {
        const bool quiet=!_pending && !_castingUi && !inventoryOpen() && !_barrier &&
            (!_encounter || (_encounter->combat() ? _encounter->combat()->phase()==XeenCombatPhase::PlayerReady &&
                !_encounter->combat()->cast() : _encounter->journeyMutable()));
        if(!quiet)return frameCopy();
        _summaryUnderlay=_frame;
        if(std::holds_alternative<QuestsAction>(action)) {
            if(_encounter && _encounter->combat())return frameCopy();
            openJournal(false);
            if(_encounter)return renderEncounter();
            _frame=drawSummary(_frame);return _frame;
        }
        if(std::holds_alternative<MapAction>(action)) {
            if(!mapName)throw std::logic_error("Map name provider is missing");
            if(_encounter)_encounter->guardCallback(_encounter->ticket(),[&]{_mapName=mapName(_camera.mapId);_mapDeadline=PresentationClock::afterMilliseconds(_clock(),2);});
            else {_mapName=mapName(_camera.mapId);_mapDeadline=PresentationClock::afterMilliseconds(_clock(),2);}
            _mapPhase=0;_summary=SummaryDialog::Map;
        } else _summary=std::holds_alternative<InfoAction>(action)?SummaryDialog::Info:SummaryDialog::QuickReference;
        if(_encounter)return renderEncounter();
        _frame=drawSummary(_frame);return _frame;
    }
    if(std::holds_alternative<QuickFightOptionsAction>(action)) {
        if(!journey() || inventoryOpen() || !_encounter->combat() || _encounter->combat()->phase()!=XeenCombatPhase::PlayerReady || _encounter->combat()->cast())return frameCopy();
        _quickFightMember=std::size_t(_encounter->combat()->participant());
        // On entry retain the actor's compressed portrait slot. Later Options
        // selections highlight the pressed slot, including the DOS Run quirk.
        _quickFightHighlight=0;
        for(std::size_t member=0;member<*_quickFightMember;++member)
            if(_encounter->combat()->participants()&(1u<<member))++_quickFightHighlight;
        return renderEncounter();
    }
    if(_barrier)return handleBarrier(action);
    if(journey() && !_encounter->combat() && std::holds_alternative<BashAction>(action)) {
        if(beginBarrier(true))return renderEncounter();
        _encounter->_journeyRefusal="Bash is unavailable at this boundary";
        return renderEncounter();
    }
    if(_spellItemsUi)return handleSpellItems(action,*displayedInput);
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
                if(selected) {_sheet.emplace();_sheet->deadline=PresentationClock::afterMilliseconds(_clock(),4);_itemsVisible=false;}
                else {_combatItems=true;_inventory.category=XeenInventoryCategory::Miscellaneous;}
                drawInventory();return _encounter?renderEncounter():_frame;
            }
        }
        if(!_pending && !_castingUi && !(_encounter && _encounter->combat() && _encounter->combat()->cast())) return frameCopy();
    }
    if(journey() && _encounter->combat() && (_encounter->combat()->cast() || std::holds_alternative<CastSpellAction>(action) || (std::holds_alternative<QuickFightAction>(action) && _encounter->combat()->phase()==XeenCombatPhase::PlayerReady && _encounter->combat()->participant()>=0 && _party.party.member(_party.roster,_encounter->combat()->participant()).quickOption==1)))
        return handleCombatCasting(action,*displayedInput);
	validateRegionalEvents();
	if (_encounter) {
		if (journey() && !_encounter->combat()) {
			if (_castingUi) return handleCasting(action,*displayedInput);
            if(std::holds_alternative<RestAction>(action) && !_pending && !inventoryOpen()) {
                if(_encounter->beginRest(loadRestDream,[this] {
                    if(!loadSampleDuration)throw std::invalid_argument("Original Rest sample duration provider is absent");
                    return std::array<std::uint64_t,2>{loadSampleDuration("dreams2.voc",XeenSceneArchive::Darkside),
                        loadSampleDuration("laff1.voc",XeenSceneArchive::Darkside)};
                })) {
                    if(_journeyEventLayers) {_presenter.clear();_journeyEventLayers=false;}
                    return renderEncounter();
                }
                return frameCopy();
            }
			if (_encounter->castingSettlement() && !_encounter->monsterReward() && !_encounter->journeyEvent()) return frameCopy();
			if (std::holds_alternative<CastSpellAction>(action)) {
				if (!_encounter->beginCasting(_encounter->ticket())) return frameCopy();
				// beginCasting acquired its lease; only completed Event layers may be retired here.
				if (_journeyEventLayers) { _presenter.clear(); _journeyEventLayers=false; }
				_castingUi.emplace();initializeCaster();
				return renderEncounter();
			}
			if(_encounter->monsterReward()) {
				const auto update=_presenter.handle(action);
				if(update.response) {
					_treasureWait.reset();
					if(_encounter->_monsterReceipt->phase==XeenRewardPhase::Receipt) {
						_encounter->acknowledgeMonsterReward();_presenter.clear();prepareJourneyTransition();
					} else _encounter->advanceMonsterReward();
					_monsterReceiptPresented=false;
				}
				return renderEncounter();
			}
			if ((std::holds_alternative<AttackAction>(action) || std::holds_alternative<BlockAction>(action) || std::holds_alternative<RunAction>(action) ||
				 std::holds_alternative<BeginEncounterAction>(action) || std::holds_alternative<RevisitCompletedAction>(action))) {
				if (_encounter->journeyMutable()) {
					_encounter->_journeyRefusal="This action is not supported yet.";
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
				if(beginBarrier(false))return renderEncounter();
				{
					if (xeenRegionalInteraction(_encounter->_journeyEvents,_camera)!=XeenRegionalInteraction::None) {
						_encounter->beginJourneyEvent();_journeyEventLayers=true;
						const auto interaction=xeenRegionalInteraction(_encounter->_journeyEvents,_camera);
						return journeyEventWork([&] {
							drive(beginDetachedEvent(interaction),false);
						});
					}
                    bool noEvent=false;
                    try { _encounter->guardCallback(_encounter->ticket(),[&] {
                        XeenRestoreGuard::Providers providers(_encounter->journeySavePreimage(),_world);
                        XeenEventPublication validation(_encounter->journeySavePreimage(),_encounter->_journeyEvents,[&] {
                            if(!_encounter->current(_encounter->ticket()))throw std::logic_error("Stale regional Event lookup");
                        });
                        validation.script(_events.scriptForMap(_camera.mapId).file());
                        noEvent=!xeenRegionalEvent(_encounter->_journeyEvents,_camera);
                        if(!noEvent)_encounter->_journeyRefusal="This Event is not supported yet.";
                        else if(reportManual)reportManual(XeenManualEventNoEvent{});
                    }); } catch(...) {
                        if(!_encounter->current(_encounter->ticket()))throw;
                        noEvent=false;_encounter->_journeyRefusal="Regional interaction failed; no Event executed";
                    }
                    // Space not taken by a grate calls the Event routine, which clears
                    // WhoWill even when no record runs (0xd2be, 0x26797).
                    _encounter->clearWhoWill();
                    if(!noEvent)return renderEncounter();
                }
                action=WaitAction{};
            }
		}

		const auto entry = _encounter->ticket();
		const auto cameraBeforeAction=_camera;
        const auto combatRevisionBeforeAction=entry.combat ? _encounter->combatResult().revision : 0;
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
				((entry.combat && _encounter->combat() && _encounter->combatResult().operation==XeenCombatOperation::Rotate && _encounter->combatResult().revision > combatRevisionBeforeAction) ||
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
