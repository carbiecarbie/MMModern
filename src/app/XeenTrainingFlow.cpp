#include "app/XeenEncounterFlow.h"
#include "app/XeenEventFlow.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenEventPublication.h"
#include <sstream>
#include <stdexcept>
namespace mmodern {
namespace {
struct TrainingBusy { bool &flag; explicit TrainingBusy(bool &f):flag(f){flag=true;} ~TrainingBusy(){flag=false;} };
bool matches(const XeenServiceDayCandidate &day,const XeenPartyState &p,const XeenWorld &w) {
    return day.reservationReady() && day.beforeContext()==*p.encounterContext && day.beforeEconomy()==*p.serviceEconomy &&
        day.beforeRandom()==*w.sessionState().journeyRandom();
}

}
void XeenEncounterFlow::checkTrainingBoundary(XeenTrainingBoundary boundary) {
    _journeyPreimage->check();
    try {if(_trainingBoundary)_trainingBoundary(boundary);}catch(...){_journeyPreimage->check();throw;}
    _journeyPreimage->check();
}
void XeenEncounterFlow::advanceTraining() noexcept {
    ++_world._sessionState._journeyGeneration;++_generation;
    _journeyPreimage->adoptJourneyCoordination();
}
bool XeenEncounterFlow::beginTraining(const std::function<void()> &preflight) {
    if(!journeyEvent() || _busy || _training || _trainingPreparation || _smith || !current(ticket()) ||
        _camera.mapId!=XeenMapIdentity(28) || !smithCapacity(17,4))return false;
    TrainingBusy busy(_busy);_journeyPreimage->check();
    if(!xeenPrepareSmithDeparture(*_party.encounterContext))return false;
    xeenValidateJourneyParty(_party);
    checkTrainingBoundary(XeenTrainingBoundary::BeforeReservation);
    auto next=std::make_unique<TrainingContinuation>();next->owner=_party.party.activeRosterIds().front();
    next->departure=std::make_unique<XeenServiceDayCandidate>(*_party.encounterContext,*_party.serviceEconomy,
        *_world.sessionState().journeyRandom(),XeenScriptServiceCharge::OneDay,false,true);
    checkTrainingBoundary(XeenTrainingBoundary::BeforeAdmission);
    try {XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);preflight();_journeyPreimage->check();}
    catch(...){_journeyPreimage->check();throw;}
    _trainingPreparation=std::move(next);return true;
}
bool XeenEncounterFlow::serviceTrainingPreparation() {
    if(!_trainingPreparation || _training || _busy || !journeyEvent() || !current(ticket()))return false;
    TrainingBusy busy(_busy);_journeyPreimage->check();
    if(!smithCapacity(17,4))throw std::overflow_error("Training admission authority exhausted");
    auto &day=*_trainingPreparation->departure;

    checkTrainingBoundary(XeenTrainingBoundary::BankPrepared);
    if(!matches(day,_party,_world)){_journeyPreimage->failed=true;throw std::logic_error("Training reservation preimage changed");}
    xeenValidateCurrentServiceEconomy(day.economy());
    _trainingPreparation->lease=_boundary.hold(XeenCombatBoundary::Work::Service);
    _training=std::move(_trainingPreparation);_world._sessionState._journeyActivity=XeenJourneyActivity::Service;
    advanceTraining();checkTrainingBoundary(XeenTrainingBoundary::AfterAdmission);return true;
}
void XeenEncounterFlow::authorizeTrainingFrame(std::uint64_t input,const IndexedFrame::Presentation &frame) {
    if(!_training || !input || !frame || _busy || _failure || !current(ticket()) ||
        !_boundary.holds(XeenCombatBoundary::Work::Service,_training->lease))
        throw std::logic_error("Training frame authority unavailable");
    _training->input=input;_training->frame=frame;
}
bool XeenEncounterFlow::consumeTrainingFrame(std::uint64_t input,const IndexedFrame::Presentation &frame) {
    if(!_training || !frame || _busy || _failure || _training->input!=input || _training->frame!=frame ||
        !current(ticket()) || !_boundary.holds(XeenCombatBoundary::Work::Service,_training->lease))return false;
    _training->input=0;_training->frame.reset();_journeyPreimage->check();return true;
}
void XeenEncounterFlow::quoteTraining(std::size_t member) {
    if(!_training || _busy || _training->frame || member>=_party.party.size() || !smithCapacity(17,4) ||
        !xeenSmithAuthorityRoom(_training->operation,1))throw std::logic_error("Training quote authority unavailable");
    TrainingBusy busy(_busy);_journeyPreimage->check();xeenValidateJourneyParty(_party);
    const auto owner=_party.party.activeRosterIds()[member];
    auto result=xeenQuoteTraining(_party.roster.at(owner),*_party.roster.combatInputs(owner),
        _party.monsterTreasure->gold,*_party.encounterContext);
    if(result.outcome==XeenTrainingOutcome::Quoted && !_training->trained.test(owner) &&
        !xeenPrepareSmithDeparture(_training->departure->context()))result.outcome=XeenTrainingOutcome::Capacity;
    _training->owner=owner;_training->result=result;_training->quoted=result.outcome==XeenTrainingOutcome::Quoted;
    _training->published=false;++_training->operation;advanceTraining();checkTrainingBoundary(XeenTrainingBoundary::Quote);
}
void XeenEncounterFlow::confirmTraining() {
    if(!_training || !_training->quoted || _training->published || _training->pending || _busy || _training->frame ||
        !smithCapacity(18,4))throw std::logic_error("Training confirmation authority unavailable");
    TrainingBusy busy(_busy);_journeyPreimage->check();
    if(!matches(*_training->departure,_party,_world)) {
        _journeyPreimage->failed=true;throw std::logic_error("Mandatory Training departure changed");
    }
    const bool newMember=!_training->trained.test(_training->owner);
    const auto &context=newMember?_training->departure->context():*_party.encounterContext;
    if(newMember) {
        const auto next=xeenPrepareSmithDeparture(context);
        if(!next || (xeenServiceDayRegenerates(context.day,next->day,1440) &&
            !XeenServiceDayCandidate::randomRoom(*_world.sessionState().journeyRandom()))) {
            _training->result.outcome=XeenTrainingOutcome::Capacity;_training->quoted=false;advanceTraining();return;
        }
    }
    auto delta=std::make_unique<XeenTrainingCandidate>(xeenPrepareTraining(_party,_training->owner,context));
    if(delta->result.outcome!=XeenTrainingOutcome::Trained) {
        _journeyPreimage->failed=true;throw std::logic_error("Training quoted owner preimage changed");
    }

    _journeyPreimage->check();_training->pending=std::move(delta);
    _training->quoted=false;advanceTraining();
}
bool XeenEncounterFlow::serviceTrainingLevel() {
    if(!_training || !_training->pending || _busy || !current(ticket()))return false;
    TrainingBusy busy(_busy);_journeyPreimage->check();
    auto &visit=*_training;
    if(!matches(*visit.departure,_party,_world)) {_journeyPreimage->failed=true;throw std::logic_error("Training member-day reservation changed");}
    const bool newMember=!visit.trained.test(visit.owner);
    if(newMember && !visit.nextDeparture) {
        if(!visit.departure->service(64,[&]{_journeyPreimage->check();},
            [&]{checkTrainingBoundary(XeenTrainingBoundary::StockComplete);}))return false;
        visit.nextDeparture=std::make_unique<XeenServiceDayCandidate>(visit.pending->context,
            visit.departure->economy(),visit.departure->continuation(),XeenScriptServiceCharge::OneDay,false,true);
        checkTrainingBoundary(XeenTrainingBoundary::BankPrepared);
        visit.nextDeparture->validateReservation();
    }

    if(visit.nextDeparture && (!(visit.nextDeparture->beforeContext()==visit.pending->context) ||
        visit.nextDeparture->beforeEconomy()!=visit.departure->economy() ||
        visit.nextDeparture->beforeRandom()!=visit.departure->continuation())) {
        _journeyPreimage->failed=true;throw std::logic_error("Replacement Training departure changed");
    }
    if(!smithCapacity(17,4))throw std::overflow_error("Training publication would consume mandatory settlement authority");
    const auto &delta=*visit.pending;
    auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
    prepared->retainResources(*_journeyPreimage);
    for(unsigned n=0;n<delta.count;++n) {
        const auto id=delta.characters[n].rosterId;
        prepared->characters[id]=delta.characters[n];prepared->inputs[id]=delta.inputs[n];
    }
    prepared->treasure->gold=delta.result.goldAfter;
    const bool memberDay=bool(visit.nextDeparture);
    if(memberDay) {
        prepared->economy=visit.departure->economy();
        prepared->s._journeyRandom=visit.departure->continuation();
    }
    prepared->context=delta.context;
    // Every allocation and validation precedes the callback-free publication.
    const XeenMutableOptional<XeenGameplayContext> endingContext=prepared->context;
    const XeenMutableOptional<XeenServiceEconomy> endingEconomy=prepared->economy;
    const XeenMutableOptional<XeenJourneyRandomState> endingRandom=prepared->s._journeyRandom;
    _journeyPreimage->check();checkTrainingBoundary(XeenTrainingBoundary::BeforeLevel);
    _party.monsterTreasure->gold=delta.result.goldAfter;
    _party.encounterContext=endingContext;
    if(memberDay) {
        _party.serviceEconomy=endingEconomy;
        _world._sessionState._journeyRandom=endingRandom;
    }
    for(unsigned n=0;n<delta.count;++n) {
        const auto id=delta.characters[n].rosterId;
        auto &c=_party.roster.at(id);auto &i=*_party.roster._combatInputs[id];
        c.permanentLevel=delta.characters[n].permanentLevel;i.experience=delta.inputs[n].experience;
        xeenResetCharacterTemps(c,i);
        if(id==visit.owner){c.currentHp=delta.result.hpAfter;c.currentSp=delta.result.spAfter;}
    }
    visit.result=delta.result;visit.published=true;visit.trained.set(visit.owner);
    if(memberDay)visit.departure.swap(visit.nextDeparture);
    visit.pending.reset();visit.nextDeparture.reset();
    ++_world._sessionState._journeyGeneration;++_generation;
    prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
    checkTrainingBoundary(XeenTrainingBoundary::LevelPublished);return true;
}
bool XeenEncounterFlow::departTraining() {
    if(!_training || _busy || _training->frame || !smithCapacity(_training->departed?5:6,2))
        throw std::logic_error("Training departure authority unavailable");
    TrainingBusy busy(_busy);_journeyPreimage->check();auto &visit=*_training;
    if(!visit.departed) {
        if(!matches(*visit.departure,_party,_world)){_journeyPreimage->failed=true;throw std::logic_error("Owed Training departure changed");}
        if(!visit.departure->service(64,[&]{_journeyPreimage->check();},
            [&]{checkTrainingBoundary(XeenTrainingBoundary::StockComplete);}))return false;
        std::vector<XeenExploredTile> exploration;
        {XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
            exploration=_world.explorationAfterStep(_party,_camera);_journeyPreimage->check();}
        auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
        prepared->retainResources(*_journeyPreimage);prepared->context=visit.departure->context();
        prepared->s._exploredTiles=exploration;
        prepared->economy=visit.departure->economy();prepared->s._journeyRandom=visit.departure->continuation();
        const XeenMutableOptional<XeenGameplayContext> context=prepared->context;
        const XeenMutableOptional<XeenServiceEconomy> economy=prepared->economy;
        const XeenMutableOptional<XeenJourneyRandomState> random=prepared->s._journeyRandom;
        _journeyPreimage->check();checkTrainingBoundary(XeenTrainingBoundary::BeforeDeparture);
        _party.encounterContext=context;_party.serviceEconomy=economy;_world._sessionState._journeyRandom=random;
        _world._sessionState._exploredTiles.swap(exploration);
        visit.departed=true;++_world._sessionState._journeyGeneration;++_generation;
        prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
        checkTrainingBoundary(XeenTrainingBoundary::DeparturePublished);
    }
    XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
    const auto view=XeenIndoorScene().classifyActors(_world,_camera,_world.sessionState().regionalActors(28));
    _journeyPreimage->check();checkTrainingBoundary(XeenTrainingBoundary::Return);publishArrival(view);
    const auto lease=visit.lease;_world._sessionState._journeyActivity=XeenJourneyActivity::Event;
    advanceTraining();_training.reset();_boundary.release(XeenCombatBoundary::Work::Service,lease);
    _trainingEventSettlement=true;return true;
}
void XeenEventFlow::prepareTraining() {
    try {
        if(!xeenSmithAuthorityRoom(_inputGeneration,8))throw std::overflow_error("Training Event input authority exhausted");
        _encounter->_trainingBoundary=[this](XeenTrainingBoundary stage){if(trainingBoundary)trainingBoundary(stage);};
        const bool admitted=_encounter->beginTraining([&] {
            if(!drawTrainingArt)throw std::runtime_error("Training artwork provider unavailable");
            const auto text=_events.textForMap(28);_encounter->journeySavePreimage().admitVertigoText(text);
            TrainingUi ui;ui.art=_frame;ui.animation.kind=XeenServiceArt::Training;
            try {drawTrainingArt(ui.art);}catch(const std::invalid_argument &){_encounter->journeySavePreimage().failed=true;throw;}
            _encounter->journeySavePreimage().check();
            if(!ui.art.isValid() || ui.art.width!=320 || ui.art.height!=200)throw std::runtime_error("Invalid Training art frame");
            _trainingUi=ui;(void)drawTraining(_frame);
        });
        if(!admitted) {
            _trainingUi.reset();_pending.reset();_journeyEventLayers=false;
            _encounter->_journeyRefusal="Training is unavailable at this date.";
        } else {claimServiceContinuation(&*_trainingUi);_presenter.clear();_journeyEventLayers=false;}
    } catch(const std::exception &) {
        _encounter->journeySavePreimage().check();
        if(!_encounter->_training) {
            _encounter->_trainingPreparation.reset();_trainingUi.reset();_pending.reset();_journeyEventLayers=false;
            _encounter->_journeyRefusal="Training preparation failed. Try entry again.";
        } else {_presenter.clear();_journeyEventLayers=false;_trainingUi->feedback="Escape to leave.";}
    }
}
std::string XeenEventFlow::trainingText() const {
    return _trainingUi->feedback.empty()?xeenLocationText(dosText(),XeenLocationDialog::Training,_party,_trainingUi->member):_trainingUi->feedback;
}
IndexedFrame XeenEventFlow::drawTraining(const IndexedFrame &world) const {
    const auto &ui=*_trainingUi;
    auto frame=drawXeenLocation(dosText(),world,ui.art,_inventoryFont,XeenLocationDialog::Training,_party,ui.member,drawDialogSprite);
    if(!ui.feedback.empty()) frame=drawXeenErrorScroll(frame,_inventoryFont,ui.feedback);
    return frame;
}
IndexedFrame XeenEventFlow::settleTrainingEvent() {
    requireServiceSettlementEntry(_trainingUi?static_cast<const void *>(&*_trainingUi):nullptr);
    if(!_trainingSettlement)throw std::logic_error("Service has not reached its settlement boundary");
    return journeyEventWork([&] {
        _encounter->checkTrainingBoundary(XeenTrainingBoundary::BeforeEventSettlement);
        if(!_trainingTerminalResult) {
            auto result=resumeOwnedServiceEvent(&*_trainingUi);
            if(!std::holds_alternative<XeenManualEventCompleted>(result)){_fatal=true;throw std::runtime_error("Training terminal Event failed");}
            _trainingTerminalResult=std::move(result);
        }
        if(!_trainingReported){if(reportManual)reportManual(*_trainingTerminalResult);_eventPublication->check();_trainingReported=true;}
        _encounter->checkTrainingBoundary(XeenTrainingBoundary::AfterEventSettlement);
        _pending.reset();_trainingUi.reset();_trainingTerminalResult.reset();_trainingRenderedRevision.reset();
        _trainingSettlement=_trainingReported=false;
    });
}
IndexedFrame XeenEventFlow::handleTraining(const PlayerAction &action,std::uint64_t input,const IndexedFrame::Presentation &inputFrame) {
    auto &ui=*_trainingUi;
    if(ui.phase==TrainingUi::Phase::Preparation || ui.phase==TrainingUi::Phase::Candidate) return frameCopy();
    unsigned key=0;
    if(const auto *dialog=std::get_if<DialogKeyAction>(&action)) key=dialog->key;
    else if(const auto *member=std::get_if<SelectMemberAction>(&action)) key=InputKey::F1+member->partyIndex;
    else if(std::holds_alternative<CancelInteractionAction>(action)) key=InputKey::Escape;
    else if(std::holds_alternative<AcknowledgeAction>(action)) key=InputKey::Enter;
    const bool member=key>=InputKey::F1 && key<InputKey::F1+_party.party.size();
    const bool allowed=ui.phase==TrainingUi::Phase::Departure?(key==InputKey::Escape || key==InputKey::Enter):
        (member || key==InputKey::Escape || key=='t');
    if((ui.feedback.empty() || ui.phase==TrainingUi::Phase::Departure) && (!allowed || (member && key-InputKey::F1==ui.member))) return frameCopy();
    if(_trainingSettlement) {_handoffPending=true;return settleTrainingEvent();}
    if(!_encounter->consumeTrainingFrame(input,inputFrame)) return frameCopy();
    serviceInputPolled(ui.animation);
    const bool cancel=key==InputKey::Escape;
    if(ui.phase!=TrainingUi::Phase::Departure && !cancel &&
        (!xeenSmithAuthorityRoom(_inputGeneration,10) || !_encounter->smithCapacity(20,4) ||
        !xeenSmithAuthorityRoom(ui.revision,key=='t'?2:1) || !xeenSmithAuthorityRoom(_encounter->_training->operation,1))) {
        ui.phase=TrainingUi::Phase::Departure;ui.feedback="Further training unavailable. Escape: depart.";
        if(ui.revision!=UINT64_MAX) ++ui.revision;return renderEncounter();
    }
    const auto originalPhase=ui.phase;
    bool failed=false,revisionAdvanced=false;
    try {
        if(ui.phase!=TrainingUi::Phase::Departure && !ui.feedback.empty()) {ui.feedback.clear();_encounter->advanceTraining();}
        else if(ui.phase==TrainingUi::Phase::Menu) {
            if(key>=InputKey::F1 && key<InputKey::F1+_party.party.size()) {
                ui.member=key-InputKey::F1;_encounter->_training->quoted=false;_encounter->advanceTraining();
            } else if(key=='t') {
                const auto &c=_party.party.member(_party.roster,ui.member);
                const auto quote=xeenQuoteTraining(c,*_party.roster.combatInputs(c.rosterId),_party.monsterTreasure->gold,*_party.encounterContext);
                if(quote.outcome==XeenTrainingOutcome::Cap || quote.outcome==XeenTrainingOutcome::MissingExperience)
                    startServiceSample(ui.animation,quote.outcome==XeenTrainingOutcome::Cap?"trainin1.voc":"trainin0.voc",true);
                else if(quote.outcome==XeenTrainingOutcome::InsufficientGold) ui.feedback=xeenNotEnoughGold(dosText());
                else if(quote.outcome==XeenTrainingOutcome::Quoted) {
                    if(!_encounter->_training->quoted) _encounter->quoteTraining(ui.member);
                    if(_encounter->_training->quoted) {
                        _encounter->confirmTraining();
                        if(_encounter->_training->pending) {startServiceSample(ui.animation,"trainin2.voc",true);ui.phase=TrainingUi::Phase::Candidate;}
                    } else ui.feedback="Training: not supported yet at this date or capacity";
                }
            } else if(cancel) ui.phase=TrainingUi::Phase::Departure;
        }
        if(ui.phase==TrainingUi::Phase::Departure && (cancel || key==InputKey::Enter)) {
            if(originalPhase!=ui.phase && ui.revision!=UINT64_MAX) {++ui.revision;revisionAdvanced=true;}
            if(!_encounter->departTraining())return renderEncounter();_trainingSettlement=true;return settleTrainingEvent();
        }
    } catch(const std::exception &) {
        failed=true;_encounter->journeySavePreimage().check();if(!_encounter->_training) throw;
        if(_encounter->_training->pending) ui.phase=TrainingUi::Phase::Candidate;
        else if(_encounter->_training->published && ui.phase!=TrainingUi::Phase::Departure) ui.phase=TrainingUi::Phase::Menu;
        else if(ui.phase==TrainingUi::Phase::Departure) ui.feedback="Training departure failed. Escape: retry.";
    }
    if(!revisionAdvanced && (!failed || originalPhase!=ui.phase) && ui.revision!=UINT64_MAX) ++ui.revision;
    if(reportText && !ui.feedback.empty()) reportText(ui.feedback);
    return renderEncounter();
}
std::optional<IndexedFrame> XeenEventFlow::updateTraining() {
    if(!_trainingUi || _dispatching || _fatal || _saving || _handoffPending)return std::nullopt;
    const auto phase=_trainingUi->phase;
    if(phase==TrainingUi::Phase::Departure) {
        TrainingBusy dispatch(_dispatching);
        if(_trainingSettlement){_handoffPending=true;return settleTrainingEvent();}
        if(_encounter->_training && _encounter->_training->frame)
            _encounter->consumeTrainingFrame(_inputGeneration,_actionableFrame);
        try {
            if(!_encounter->departTraining())return std::nullopt;
            _trainingSettlement=true;return settleTrainingEvent();
        } catch(const std::exception &) {
            _encounter->journeySavePreimage().check();
            if(!_encounter->_training)throw;
            return renderEncounter(); // Refresh authority after a committed departure prefix.
        }
    }
    if(phase!=TrainingUi::Phase::Preparation && phase!=TrainingUi::Phase::Candidate)return std::nullopt;
    // Keep the shared Flow dispatch flag across bounded private preparation.
    TrainingBusy dispatch(_dispatching);
    try {
        if(phase==TrainingUi::Phase::Preparation && !xeenSmithAuthorityRoom(_inputGeneration,9))
            throw std::overflow_error("Training admission would consume mandatory input authority");
        const bool complete=phase==TrainingUi::Phase::Preparation?_encounter->serviceTrainingPreparation():_encounter->serviceTrainingLevel();
        if(!complete)return std::nullopt;
        _trainingUi->phase=TrainingUi::Phase::Menu;
        if(phase==TrainingUi::Phase::Preparation) {
            startServiceSample(_trainingUi->animation,XeenServiceAnimation::greeting(XeenServiceArt::Training));
            serviceInputPolled(_trainingUi->animation);
        }
        if(phase==TrainingUi::Phase::Candidate && !_encounter->_training->published)
            _trainingUi->feedback="Training: not supported yet at this date or capacity";
    } catch(const std::exception &error) {
        _encounter->journeySavePreimage().check();
        if(!_encounter->_training) {
            _encounter->_trainingPreparation.reset();_trainingUi.reset();_pending.reset();_journeyEventLayers=false;
            _encounter->_journeyRefusal="Unable to enter Training. Try again.";_encounter->endJourneyEvent();
        } else if(_encounter->_training->published) _trainingUi->phase=TrainingUi::Phase::Menu;
        else if(_encounter->_training->pending) {
            _trainingUi->phase=TrainingUi::Phase::Candidate;return std::nullopt;
        } else {
            _encounter->_training->pending.reset();_encounter->_training->nextDeparture.reset();
            _encounter->_training->quoted=false;_trainingUi->phase=TrainingUi::Phase::Menu;
            _trainingUi->feedback="Training preparation failed. Escape to leave.";
        }
    }
    if(_trainingUi)++_trainingUi->revision;
    return renderEncounter();
}
}
