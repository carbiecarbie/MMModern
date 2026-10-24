#include "app/XeenEventFlow.h"
#include "app/XeenAudio.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenIndoorScene.h"
#include <algorithm>
#include <stdexcept>

namespace mmodern {
namespace {
struct InnBusy {bool &value;explicit InnBusy(bool &v):value(v){value=true;}~InnBusy(){value=false;}};
std::string innSaveError(const XeenDosText &text,const std::exception &error) {
    const auto message=std::string(error.what());
    return message.find("Windows error 112")!=std::string::npos || message.find("Windows error 39")!=std::string::npos?
        std::string(text.scalar("SAVE_AS_SPACE")):message;
}
}
// Sign In uses the Tavern's already reserved addTime day, never Rest/changeTime.
bool XeenEncounterFlow::signIn() {
    if(!_smith || _smith->serviceAction!=3 || _smith->departed || _smith->frame || _busy || !smithCapacity(20,4))
        throw std::logic_error("Sign In authority unavailable");
    InnBusy busy(_busy);_journeyPreimage->check();checkSmithReservation();
    XeenCamera door=_camera;door.direction=XeenDirection::West;
    switch(door.mapId.number) {
    case 28:door.x=18;door.y=4;break;
    case 30:door.x=19;door.y=16;break;
    case 49:door.x=11;door.y=12;break; // DOS explicit Shangri-La correction.
    default:throw std::logic_error("Sign In outside a Clouds tavern");
    }
    if(door.mapId.side!=XeenSide::Clouds)throw std::logic_error("Unsupported Inn side");
    if(!_smith->departure->service(64,[&]{_journeyPreimage->check();},
        [&]{checkSmithBoundary(XeenSmithBoundary::StockComplete);}))return false;
    *_smith->binding=SmithDepartureBinding(*_smith->departure);
    const auto &day=*_smith->departure;
    auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
    prepared->retainResources(*_journeyPreimage);
    prepared->context=day.context();prepared->economy=day.economy();prepared->s._journeyRandom=day.continuation();
    prepared->cameraValue=door;prepared->insideInn=true;
    for(auto id:_party.party.activeRosterIds()) {prepared->characters[id].savedMaze=door.mapId.number;prepared->characters[id].xeenSide=0;}
    _journeyPreimage->check();
    _party.encounterContext=day.context();_party.serviceEconomy=day.economy();
    _world._sessionState._journeyRandom=day.continuation();_camera=door;_party.insideInn=true;
    for(auto id:_party.party.activeRosterIds()) {auto &c=_party.roster.at(id);c.savedMaze=door.mapId.number;c.xeenSide=0;}
    _smith->departed=true; // The ordinary farewell must not charge another day.
    ++_world._sessionState._journeyGeneration;++_generation;
    prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
    return true;
}
bool XeenEncounterFlow::innMembership(const Ticket &entry,std::vector<std::uint8_t> ids) {
    if(!_party.insideInn || !current(entry) || _busy || _combat || !_boundary.quiet() || !journeyCapacity())return false;
    InnBusy busy(_busy);_journeyPreimage->check();
    // Validate a complete detached successor before touching live membership.
    auto candidate=_world.transitionCandidate();XeenPartyState p;candidate->copyEventParty(_party,p);
    p.party=p.party.reordered(ids);p.firstSerializedCount=p.effectiveSerializedCount=ids.size();
    xeenValidateJourneyParty(p);
    // The live roster already owns complete records; removing never rewrites lodging.
    auto next=_party.party.reordered(ids);
    auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
    prepared->retainResources(*_journeyPreimage);prepared->membership=ids;
    prepared->first=prepared->effective=ids.size();
    _journeyPreimage->check();
    _party.party=std::move(next);_party.firstSerializedCount=_party.effectiveSerializedCount=ids.size();
    ++_generation;prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
    _retiredCombatResult={};_combatObservation={};_combatAward={};return true;
}
bool XeenEncounterFlow::innRecord(const Ticket &entry,XeenCharacter character,XeenCombatInputs inputs,std::vector<std::uint8_t> ids) {
    if(!_party.insideInn || !current(entry) || _busy || _combat || !_boundary.quiet() || !journeyCapacity())return false;
    InnBusy busy(_busy);_journeyPreimage->check();
    const auto owner=unsigned(character.rosterId);
    if(owner>=24)return false;
    auto candidate=_world.transitionCandidate();XeenPartyState p;candidate->copyEventParty(_party,p);
    p.roster.at(owner)=std::move(character);p.roster._combatInputs[owner]=std::move(inputs);
    p.party=p.party.reordered(ids);p.firstSerializedCount=p.effectiveSerializedCount=ids.size();
    xeenValidateJourneyParty(p);
    auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
    prepared->retainResources(*_journeyPreimage);
    prepared->characters[owner]=p.roster.at(owner);prepared->inputs[owner]=p.roster.combatInputs(owner);
    prepared->membership=ids;prepared->first=prepared->effective=ids.size();
    _journeyPreimage->check();
    _party.roster.at(owner)=std::move(p.roster.at(owner));
    _party.roster._combatInputs[owner]=std::move(p.roster._combatInputs[owner]);
    _party.party=std::move(p.party);_party.firstSerializedCount=_party.effectiveSerializedCount=ids.size();
    ++_generation;prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
    _retiredCombatResult={};_combatObservation={};_combatAward={};return true;
}
void XeenEncounterFlow::innLeaving() {
    if(!_party.insideInn || !_party.party.size() || _busy || _combat || !_boundary.quiet() || !current(ticket()))
        throw std::logic_error("Inn Exit unavailable");
    _journeyPreimage->check();_journeyPreimage->insideInn=false;
    _party.insideInn=false;++_generation;_journeyPreimage->adoptMutationBoundary();
}
void XeenEncounterFlow::innReturn() {
    _journeyPreimage->check();
    if(_party.insideInn || _busy || _combat || !_boundary.quiet() || !journeyCapacity())throw std::logic_error("Inn return unavailable");
    auto candidate=_world.transitionCandidate();XeenPartyState p;candidate->copyEventParty(_party,p);
    auto camera=_camera;auto flags=_flags;XeenActorView arrival;XeenEventFile events;
    {
        XeenRestoreGuard guard(*candidate,p,camera,flags);
        XeenRestoreGuard::EventProviders providers(*candidate,p,camera,flags,guard,[&]{_journeyPreimage->check();},[&]{_journeyPreimage->failed=true;});
        candidate->discardMapCache();
        candidate->loadEventDestination(camera.mapId,camera.mapId);
        events=_travelEventsProvider(camera.mapId);_journeyPreimage->check();
        arrival=candidate->prepareTransitionArrival(camera);
    }
    auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
    prepared->retainResources(*_journeyPreimage);prepared->prepareSpellMapPublication(*candidate);
    _world.checkTransitionTreasure(*candidate);_journeyPreimage->check();
    _world.publishTransition(*candidate);_journeyEvents=std::move(events);
    ++_generation;prepared->adoptMutationBoundary();_journeyPreimage.swap(prepared);
    auto cleared=_world.treasureStaging().value();cleared.clearPending();
    _journeyPreimage->prepareStagingPublication(cleared);
    _world.treasureStaging().enterGame();_journeyPreimage->adoptMutationBoundary();
    publishArrival(arrival);
}
void XeenEventFlow::forcedInnSave() {
    if(!saveInn || _forcedInnSave)throw std::logic_error("Inn Save transaction unavailable");
    if(!((_smithUi && _smithUi->signIn && _smithSettlement && _encounter->journeyEvent() && !_encounter->_smith) ||
         (_innUi && _innUi->phase==InnUi::Phase::Menu &&
          (!_party.insideInn || _innUi->edit==InnUi::Edit::SavePending))))
        throw std::logic_error("Inn Save site unavailable");
    auto &cap=*_encounter->_journeyCapture;
    const auto &boundary=_encounter->_boundary;
    if(_smithUi) {
        // Exactly the retained Event continuation may survive Service release.
        // Never bless an unrelated lease by adopting its generation.
        if(!_encounter->_smithEventSettlement || !boundary.only(XeenCombatBoundary::Work::Event,_encounter->_eventLease))
            throw std::logic_error("Inn Save service release unavailable");
        cap.generation=boundary.generation();
    } else if(!boundary.quiet() || cap.generation!=boundary.generation())
        throw std::logic_error("Stale Inn Exit capture");
    struct Lease {bool &flow,&capture;bool dispatch;XeenEventFlow &owner;
        ~Lease(){flow=capture=false;owner._dispatching=dispatch;}} lease{_forcedInnSave,cap.innSave,_dispatching,*this};
    _forcedInnSave=cap.innSave=true;_dispatching=false;
    saveInn();_encounter->journeySavePreimage().check();
    _encounterFrame=_encounter->ticket();
}
IndexedFrame XeenEventFlow::finishSignIn() {
    if(!_smithUi || !_smithUi->signIn || !_smithSettlement)throw std::logic_error("Sign In settlement unavailable");
    if(!_signInSaveDone) {
        if(_program.audio)_program.audio->requestSong(_program.audio->mapSong(_world.map(_camera.mapId).geometry),XeenSceneArchive::Clouds,223,presentationNow());
        try {forcedInnSave();_signInSaveDone=true;}
        catch(const std::exception &e) {
            _encounter->journeySavePreimage().check();
            _smithUi->feedback=innSaveError(dosText(),e);
            ++_smithUi->revision;return renderEncounter();
        }
    }
    auto closing=_frame;
    auto settled=settleSmithEvent();
    if(_smithUi)return settled;
    _signInSaveDone=false;_explorationPass=false;_manualInput.reset();
    _encounter->_journeyCapture->inputPassPending=false;
    _innUi.emplace();_innUi->phase=InnUi::Phase::Closing;_innUi->scroll=22;_innUi->closing=std::move(closing);
    _innUi->deadline=PresentationClock::after(presentationNow(),1);
    return renderInn();
}
IndexedFrame XeenEventFlow::openInn(bool closing) {
    requireCurrentOwners();
    if(!journey() || !_party.insideInn || _innUi)throw std::logic_error("Inn entry unavailable");
    if(!closing && !_program.innSongStarted) {
        if(_program.audio)_program.audio->requestInn(presentationNow());_program.innSongStarted=true;
    }
    _innUi.emplace();_innUi->closing=_frame;
    if(closing){_innUi->phase=InnUi::Phase::Closing;_innUi->scroll=22;}
    _innUi->deadline=closing?PresentationClock::after(presentationNow(),1):PresentationClock::retraceAfter(presentationNow());
    _explorationPass=false;_manualInput.reset();_encounter->_journeyCapture->inputPassPending=false;
    return renderInn();
}
std::shared_ptr<const DialogInput> XeenEventFlow::innInput() const {
    DialogInput input;
    if(_innUi->phase==InnUi::Phase::Closing || _innUi->phase==InnUi::Phase::Opening)
        input.anyKey=input.anyClick=input.keyMouseWait=true;
    else if(_innUi->phase==InnUi::Phase::Menu) {
        const auto ids=xeenInnList(_party,_camera.mapId,_innUi->townOnly);
        const auto rows=ids.size()-std::min(ids.size(),_innUi->top);
        if(_innUi->edit==InnUi::Edit::DeleteSelect)input=xeenInnDeleteInput(rows);
        else if(_innUi->edit==InnUi::Edit::DeleteConfirm)input=xeenConfirmInput(false);
        else input=xeenInnInput(rows,_party.party.size(),_innUi->remove);
        if(!_innUi->feedback.empty()){input={};input.anyKey=input.anyClick=input.keyMouseWait=true;}
    }
    return std::make_shared<const DialogInput>(std::move(input));
}
IndexedFrame XeenEventFlow::renderInn(bool cosmetic) {
    if(!loadInnScreen)throw std::runtime_error("Missing original Inn screen provider");
    const auto ticket=_encounter->ticket();IndexedFrame returned;
    if(!_encounter->prepareJourneyFrame(ticket,[&]{
        auto &ui=*_innUi;
        const bool opening=ui.phase==InnUi::Phase::Palette || ui.phase==InnUi::Phase::Opening;
        const bool closing=ui.phase==InnUi::Phase::Closing || ui.phase==InnUi::Phase::Closed;
        auto frame=closing?ui.closing:ui.opening.isValid()?ui.opening:loadInnScreen(_party);
        if(ui.phase==InnUi::Phase::Menu || (opening && !ui.opening.isValid())) {
            const auto ids=xeenInnList(_party,_camera.mapId,ui.townOnly);
            if(!ui.opening.isValid())frame=drawXeenInn(dosText(),frame,_inventoryFont,_party,ids,ui.top,drawDialogSprite);
            if(opening)ui.opening=frame;
            else ui.opening={};
            if(ui.remove)frame=drawXeenInnRemove(dosText(),frame,_inventoryFont,drawDialogSprite);
            if(ui.edit==InnUi::Edit::DeleteSelect)frame=drawXeenInnDelete(dosText(),frame,_inventoryFont,drawDialogSprite);
            if(ui.edit==InnUi::Edit::DeleteConfirm) {
                const auto &c=_party.roster.at(*ui.selected);
                frame=drawXeenConfirm(frame,_inventoryFont,xeenDialogFormat(dosText().scalar("INN_DELETE_CONFIRM"),
                    {std::string(c.name),dosText().table("CLASS_NAMES").at(unsigned(c.characterClass))}),false,drawDialogSprite);
            }
            _inventoryUnderlay=frame;
            if(inventoryOpen())frame=drawCharacterDialog(frame);
            if(_summary)frame=drawSummary(frame);
            if(!ui.feedback.empty())frame=drawXeenErrorScroll(frame,_inventoryFont,ui.feedback);
        }
        if(ui.phase!=InnUi::Phase::Menu)frame=drawXeenScroll(frame,ui.scroll,drawDialogSprite);
        // Keep the old palette until fadeIn(0x81)'s retrace. The cache retains
        // the full Inn palette for the first opening frame after that wait.
        if(ui.phase==InnUi::Phase::Palette)frame.palette=ui.closing.palette;
        _frame=std::move(frame);returned=_frame;sealFrame(returned);
    },cosmetic))throw std::runtime_error("Inn presentation failed");
    if(!cosmetic)++_inputGeneration;
    _encounterFrame=_encounter->ticket();_handoffPending=!cosmetic;_cosmeticPending=cosmetic;
    _innUi->acquired=false;return returned;
}
std::optional<IndexedFrame> XeenEventFlow::updateInn() {
    if(_dispatching || _fatal || _saving || _handoffPending || !_innUi->acquired)return {};
    auto &ui=*_innUi;
    if(ui.phase==InnUi::Phase::Menu) {
        if(_sheet && !_itemsVisible && !_statPopup && !_dialogError && _clock()>=_sheet->deadline) {
            _sheet->deadline=PresentationClock::afterMilliseconds(_clock(),4);_sheet->blink=!_sheet->blink;return renderInn(true);
        }
        return {};
    }
    if(presentationNow()<ui.deadline)return {};
    if(ui.phase==InnUi::Phase::Closing) {
        if(ui.scroll>1) {--ui.scroll;ui.deadline=PresentationClock::after(ui.deadline,1);}
        else {ui.phase=InnUi::Phase::Closed;ui.scroll=1;ui.deadline=presentationNow();}
    } else if(ui.phase==InnUi::Phase::Closed) {
        ui.phase=InnUi::Phase::Palette;ui.scroll=1;
        // DOS fadeIn(0x81): restore the full palette at one VGA retrace.
        ui.deadline=PresentationClock::retraceAfter(presentationNow());
        if(!_program.innSongStarted) {if(_program.audio)_program.audio->requestInn(presentationNow());_program.innSongStarted=true;}
    } else if(ui.phase==InnUi::Phase::Palette) {
        ui.phase=InnUi::Phase::Opening;ui.deadline=PresentationClock::after(presentationNow(),1);
    } else if(ui.phase==InnUi::Phase::Opening) {
        if(ui.scroll<22) {++ui.scroll;ui.deadline=PresentationClock::after(ui.deadline,1);}
        else {ui.phase=InnUi::Phase::Menu;ui.scroll=23;}
    }
    return renderInn();
}
IndexedFrame XeenEventFlow::handleInn(const PlayerAction &action) {
    auto &ui=*_innUi;
    if(_dispatching || _fatal || _saving || _handoffPending)return frameCopy();
    if(ui.acquired && (ui.phase==InnUi::Phase::Closing || ui.phase==InnUi::Phase::Opening)) {
        InnBusy dispatch(_dispatching);
        // The DOS poll consumes the key/click. Its return is never a menu key.
        ui.phase=ui.phase==InnUi::Phase::Closing?InnUi::Phase::Closed:InnUi::Phase::Menu;
        ui.scroll=ui.phase==InnUi::Phase::Closed?1:23;ui.deadline=presentationNow();
        return renderInn(); // Final image flips immediately, with no timed hold.
    }
    if(ui.phase!=InnUi::Phase::Menu)return frameCopy();
    InnBusy dispatch(_dispatching);
    if(_summary) {
        unsigned key=0;if(const auto *k=std::get_if<DialogKeyAction>(&action))key=k->key;
        if((*_summary==SummaryDialog::Awards || *_summary==SummaryDialog::Quests) && key!=InputKey::Escape)
            _journalUi.key(key,journalLines().size());
        else {_summary.reset();_summaryUnderlay={};_journalStrings.clear();}
        return renderInn();
    }
    if(inventoryOpen()) {handleCharacterDialog(action);return renderInn();}
    unsigned key=0;
    if(const auto *k=std::get_if<DialogKeyAction>(&action))key=k->key;
    if(const auto *m=std::get_if<SelectMemberAction>(&action))key=InputKey::F1+m->partyIndex;
    if(std::holds_alternative<CancelInteractionAction>(action))key=InputKey::Escape;
    const auto saveEdit=[&] {
        // A published record is never changed again when the disk write is retried.
        try {forcedInnSave();ui.edit=InnUi::Edit::None;ui.selected.reset();}
        catch(const std::exception &e) {_encounter->journeySavePreimage().check();ui.feedback=innSaveError(dosText(),e);}
    };
    if(!ui.feedback.empty()) {
        ui.feedback.clear();if(ui.edit==InnUi::Edit::SavePending)saveEdit();return renderInn();
    }
    if(!innInput()->key(key))return frameCopy();
    auto ids=std::vector<std::uint8_t>(_party.party.activeRosterIds());
    const auto changed=[&]{_inventory={};_sheet.reset();_quickFightMember.reset();_lastCaster.reset();_equipmentSelection.reset();advanceInventoryEpoch();};
    if(ui.edit==InnUi::Edit::DeleteSelect) {
        if(key==InputKey::Escape)ui.edit=InnUi::Edit::None;
        else if(key>='1' && key<='4') {
            const auto list=xeenInnList(_party,_camera.mapId,ui.townOnly);const auto row=ui.top+key-'1';
            if(row>=list.size())return frameCopy();
            ui.selected=list[row];const auto &c=_party.roster.at(*ui.selected);
            ui.edit=InnUi::Edit::None;
            if(std::any_of(c.weapons.begin(),c.weapons.end(),[](const auto &item){return item.id==34;}))
                ui.feedback=dosText().scalar("INN_DELETE_SWORD");
            else ui.edit=InnUi::Edit::DeleteConfirm;
        }
    } else if(ui.edit==InnUi::Edit::DeleteConfirm) {
        if(key=='n' || key==InputKey::Escape){ui.edit=InnUi::Edit::None;ui.selected.reset();}
        else if(key=='y') {
            const auto owner=*ui.selected;
            ids.erase(std::remove(ids.begin(),ids.end(),owner),ids.end());
            if(!_encounter->innRecord(*_encounterFrame,xeenDeletedCharacter(owner),xeenDeletedCombatInputs(),ids))return frameCopy();
            changed();ui.townOnly=true;ui.edit=InnUi::Edit::SavePending;saveEdit();
        }
    } else if(ui.remove) {
        if(key==InputKey::Escape)ui.remove=false;
        else if(key>=InputKey::F1 && key<InputKey::F1+ids.size()) {
            ids.erase(ids.begin()+key-InputKey::F1);_encounter->innMembership(_encounter->ticket(),ids);changed();ui.remove=false;
        }
    } else if(key>=InputKey::F1 && key<InputKey::F1+ids.size()) {
        handleInventory(InspectInventoryAction{});_inventory.source=key-InputKey::F1;_inventory.sourceOwner=ids[_inventory.source];
        _sheet.emplace();_sheet->deadline=PresentationClock::afterMilliseconds(_clock(),4);_itemsVisible=false;
    } else if(key>='1' && key<='4') {
        const auto list=xeenInnList(_party,_camera.mapId,_innUi->townOnly);const auto row=ui.top+key-'1';
        if(row>=list.size() || std::find(ids.begin(),ids.end(),list[row])!=ids.end())return frameCopy();
        if(ids.size()==6){_encounter->fmEffect(21);ui.feedback=dosText().scalar("INN_FULL");}
        else {ids.push_back(list[row]);_encounter->innMembership(_encounter->ticket(),ids);changed();}
    } else if(key==InputKey::Up) {if(ui.top>=4)ui.top-=4;}
    else if(key==InputKey::Down) {if(ui.top+4<xeenInnList(_party,_camera.mapId,ui.townOnly).size())ui.top+=4;}
    else if(key=='r')ui.remove=!ids.empty();
    else if(key=='c')ui.feedback="Create: not supported yet";
    else if(key=='d') {if(!xeenInnList(_party,_camera.mapId,ui.townOnly).empty())ui.edit=InnUi::Edit::DeleteSelect;}
    else if(key=='e' || key=='x' || key==InputKey::Escape || key==InputKey::Space) {
        if(ids.empty())ui.feedback=dosText().scalar("INN_NO_MEMBERS");
        else {
            _encounter->innLeaving();
            try {forcedInnSave();}
            catch(const std::exception &e) {
                _encounter->journeySavePreimage().check();
                _party.insideInn=true;_encounter->journeySavePreimage().insideInn=true;
                _encounter->journeySavePreimage().adoptMutationBoundary();
                ui.feedback=innSaveError(dosText(),e);return renderInn();
            }
            _program.innSongStarted=false;
            _encounter->innReturn();_innUi.reset();_presenter.clear();_journeyEventLayers=false;
            _encounterFrame=_encounter->ticket();_handoffPending=false;
            // Ordinary flagged-cell pass at the door, distinct from Load/New Game.
            _innReturnPassPending=true;return renderEncounter();
        }
    }
    return renderInn();
}
}
