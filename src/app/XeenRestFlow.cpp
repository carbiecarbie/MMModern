#include "app/XeenEncounterFlow.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenJourneyRules.h"
namespace mmodern {
namespace {
struct RestBusy {bool &value; explicit RestBusy(bool &v):value(v){value=true;} ~RestBusy(){value=false;}};
XeenConsequenceCharacters members(const XeenPartyState &p,const XeenPartyOrder &order) {
 XeenConsequenceCharacters r;for(unsigned n=0;n<6;++n)r[n]=p.roster.at(order[n]);return r;
}
XeenConsequenceInputs supplements(const XeenPartyState &p,const XeenPartyOrder &order) {
 XeenConsequenceInputs r;for(unsigned n=0;n<6;++n)r[n]=*p.roster.combatInputs(order[n]);return r;
}
}
void XeenEncounterFlow::restPublication() {
 ++_world._sessionState._journeyGeneration;++_generation;
 _journeyFramePrepared=false;_rest->presented=false;retainJourney();
}
bool XeenEncounterFlow::beginRest(const std::function<IndexedFrame()> &dream) {
 if(!_journey || _rest || _busy || _combat || _failure || _regionalWork || _regionalAutomatic ||
  _shoot || _shootIntent || _casting || _castingSettlement || projectilesPending() || monsterReward() ||
  !_boundary.quiet() || !current(ticket()) || _state.pending()>3 || _state.phase()!=XeenEncounterPhase::Exploring || !journeyCapacity())return false;
 RestBusy busy(_busy);
 try {
  const auto retained=_journeyPreimage;XeenRestoreGuard::Providers providers(*retained,_world);
  xeenValidateJourneyMelee(_party);
  // Sample seams, but RESTRICTION_REST belongs to the primary map. The pin's
  // byte cell flags can never contain 0x4000.
  const auto cell=_world.sampleCell(_camera.mapId,_camera.x,_camera.y);
  if(!cell)throw std::invalid_argument("Rest cell is absent");
  const auto &primary=_world.map(_camera.mapId).geometry;
  const auto surface=primary.surfaceTypes[cell->cell->surfaceIndex];
  bool navigator=false;for(auto owner:_party.party.activeRosterIds()) {
   const auto details=_party.roster.at(owner).originalDetails();
   navigator=navigator || (details && details->skills[10]);
  }
  // doStepCode follows resetTemps, which clears levitation. Evaluate that
  // foreseeable suffix before publishing sleep, charges, food or RNG.
  auto ending=*_party.encounterContext;xeenResetPartyTemps(ending);
  const auto terrain=xeenRestTerrain(surface,primary.isOutdoors(),navigator,ending.effects[2]!=0);
  auto candidate=std::make_unique<RestContinuation>();candidate->owners=_party.party.activeOrder();
  if(primary.flags&0x4000)candidate->phase=RestContinuation::Phase::Refused;
  else if(!terrain) {
   candidate->phase=RestContinuation::Phase::Refused;
   candidate->refusal="Rest terrain is not supported yet";
  }
  else {
   candidate->terrainMinutes=*terrain;
   candidate->phase=xeenRestDanger(members(_party,candidate->owners),supplements(_party,candidate->owners),_party.encounterContext->year)?
    RestContinuation::Phase::Confirm:RestContinuation::Phase::Charges;
   if(_party.encounterContext->profile==XeenBehaviorProfile::WorldOfXeenClouds) {
    if(!dream)throw std::invalid_argument("Original Rest dream provider is absent");
    candidate->dream=dream();
    if(candidate->dream.width!=320 || candidate->dream.height!=200 || !candidate->dream.isValid())
     throw std::invalid_argument("Invalid original Rest dream image");
   }
  }
  candidate->random=XeenCombatRandom(*_world.sessionState().journeyRandom());retained->check();
  if(candidate->phase==RestContinuation::Phase::Charges)
   for(auto owner:_party.party.activeRosterIds())_party.roster.at(owner).conditions[8]=1;
  _rest.swap(candidate);_world._sessionState._journeyActivity=XeenJourneyActivity::Presentation;
  _journeyRefusal.clear();restPublication();return true;
 } catch(...) {closeJourney();throw;}
}
bool XeenEncounterFlow::respondRest(const PlayerAction &action) {
 if(!_rest || _busy || !_rest->presented || !current(ticket()) || !journeyCapacity())return false;
 using P=RestContinuation::Phase;
 const auto key=std::get_if<DialogKeyAction>(&action);
 const auto answer=key?xeenConfirmAnswer(key->key):std::optional<bool>{};
 const bool yes=std::holds_alternative<YesAction>(action) || (answer && *answer);
 const bool no=std::holds_alternative<NoAction>(action) || std::holds_alternative<CancelInteractionAction>(action) || (answer && !*answer);
 if(_rest->phase!=P::Confirm && _rest->phase!=P::Refused && _rest->phase!=P::Complete)return false;
 if(_rest->phase==P::Confirm && !yes && !no)return false;
 RestBusy busy(_busy);_journeyPreimage->check();
 if(_rest->phase==P::Confirm && yes) {
  for(auto owner:_party.party.activeRosterIds())_party.roster.at(owner).conditions[8]=1;
  _rest->phase=P::Charges;restPublication();return true;
 }
 const bool completed=_rest->phase==P::Complete;_rest.reset();
 if(completed) {
  bool living=false;for(auto owner:_party.party.activeRosterIds())living=living || xeenCombatTargetable(_party.roster.at(owner));
  if(!living) {_state._phase=XeenEncounterPhase::SupportStopped;_state._reason=XeenEncounterStop::Defeat;_world._sessionState._encounterTerminal=true;}
 }
 _world._sessionState._journeyActivity=XeenJourneyActivity::Presentation;
 ++_world._sessionState._journeyGeneration;++_generation;retainJourney();schedule(_lastTime);return true;
}
bool XeenEncounterFlow::serviceRest() {
 if(!_rest || _busy || _combat || _failure || !_rest->presented || !current(ticket()) || !journeyCapacity())return false;
 using P=RestContinuation::Phase;auto &rest=*_rest;
 if(rest.phase==P::Confirm || rest.phase==P::Refused || rest.phase==P::Complete)return false;
 std::uint64_t now;if(!prepareTime(ticket(),now) || now<rest.deadline)return false;
 RestBusy busy(_busy);_lastTime=now;
 try {
  const auto retained=_journeyPreimage;const auto check=[&]{retained->check();};
  XeenRestoreGuard::Providers providers(*retained,_world);auto &session=_world._sessionState;
  const bool outdoor=_world.map(_camera.mapId).geometry.isOutdoors();check();
  if(rest.phase==P::Dream) {
   // Screen::fadeInner(4): 33 palette steps per fade and two seven-tick
   // holds. dreams2.voc/laff1.voc waits remain the deferred audio gap.
   if(++rest.dreamBeat==146)rest.phase=P::Recovery;
   rest.deadline=now+50;restPublication();return true;
  }
  if(rest.phase==P::Recovery) {
   if(!rest.recovery)rest.recovery.emplace(members(_party,rest.owners),supplements(_party,rest.owners),*_party.encounterContext,_party.food);
   auto &recovery=*rest.recovery;
   if(rest.terrainMinutes) {
    if(!rest.time)rest.time.emplace(recovery.context,rest.terrainMinutes,recovery.characters,recovery.inputs,
     &*_party.serviceEconomy,XeenTimeMode::Interactive,XeenTimeCall::Add);
    XeenConsequenceDraw draw{rest.random,64,check};if(!rest.time->service(draw))return true;
    recovery.context=rest.time->context;recovery.characters=rest.time->characters;recovery.inputs=rest.time->inputs;
   }
   check();
   for(unsigned n=0;n<6;++n) {
    _party.roster.at(rest.owners[n])=recovery.characters[n];
    _party.roster._combatInputs[rest.owners[n]]=recovery.inputs[n];
   }
   _party.food=recovery.food;_party.encounterContext=recovery.context;
   if(rest.time) {
    _party.serviceEconomy=rest.time->economy;session._journeyRandom=rest.random.continuation();
    _needsRestNotice=rest.time->needsRest;rest.time.reset();
   }
   rest.consumed=recovery.consumed;rest.starving=recovery.starving;rest.phase=P::Complete;
   restPublication();return true;
  }
  if(rest.phase==P::Charges && !_regionalWork) {
   auto candidate=std::make_unique<XeenRegionalActionCandidate>();auto &c=*candidate;
   c.camera=_camera;c.context=*_party.encounterContext;c.actors=session.regionalActors(_camera.mapId);
   c.owners=rest.owners;c.characters=members(_party,rest.owners);c.inputs=supplements(_party,rest.owners);c.random=rest.random;
   c.revision=_state.revision();c.pending=3;c.classify=true;c.sleeping=true;
   c.remaining=_state.pending()?1:0;c.result.outcome=XeenEncounterOutcome::Pulsed;
   c.time.emplace(c.context,outdoor?10:1,c.characters,c.inputs,&*_party.serviceEconomy,XeenTimeMode::Sleeping);
   check();_regionalWork.swap(candidate);
  }
  if(rest.phase==P::Remainder) {
   if(!rest.time)rest.time.emplace(*_party.encounterContext,outdoor?380:470,
    members(_party,rest.owners),supplements(_party,rest.owners),&*_party.serviceEconomy,XeenTimeMode::Sleeping);
   XeenConsequenceDraw draw{rest.random,64,check};if(!rest.time->service(draw))return true;
   const auto dream=draw.draw(1,20);if(!dream)return true;
   rest.phase=*dream==1 && _party.encounterContext->profile==XeenBehaviorProfile::WorldOfXeenClouds?P::Dream:P::Recovery;
   check();
   for(unsigned n=0;n<6;++n) {
    _party.roster.at(rest.owners[n])=rest.time->characters[n];
    _party.roster._combatInputs[rest.owners[n]]=rest.time->inputs[n];
   }
   _party.encounterContext=rest.time->context;_party.serviceEconomy=rest.time->economy;
   session._journeyRandom=rest.random.continuation();rest.time.reset();
   // Charge publications and every ranged impact use the existing Approach
   // candidate below. This suffix has no actor opportunity or stepTime call.
   restPublication();return true;
  }
  const auto boundaryGeneration=_boundary.generation();
  session._journeyActivity=XeenJourneyActivity::Approach;++session._journeyGeneration;
  _world._combatCheck=[this,boundaryGeneration] {
   if(_boundary.generation()!=boundaryGeneration)throw std::logic_error("Stale sleeping opportunity boundary");
   _journeyPreimage->check();
  };
  _world._combatAuthorized=[this,generation=_generation,boundaryGeneration] {
   return !_failure && generation==_generation && boundaryGeneration==_boundary.generation() && _journeyPreimage->ownersAlive();
  };
  _journeyPreimage->adoptJourneyCoordination();
  auto result=XeenActorApproach::regionalTransition(_world,_party,_camera,_state,_events,{},_regionalWork);
  if(!_journeyPreimage->ownersAlive() || boundaryGeneration!=_boundary.generation() ||
   result.outcome==XeenEncounterOutcome::Stale || result.outcome==XeenEncounterOutcome::Refused)
   throw std::logic_error("Sleeping opportunity lost publication authority");
  _world._combatCheck={};_world._combatAuthorized={};
  if(result.consequences)observeRanged(result.consequences);_result=result;
  const bool interrupted=_state.phase()!=XeenEncounterPhase::Exploring ||
   (result.outcome!=XeenEncounterOutcome::Pending && result.consequences && result.consequences->count && xeenRestRangedWake(members(_party,rest.owners)));
  if(interrupted) {
   _rest.reset();session._journeyActivity=_state.phase()==XeenEncounterPhase::Engaged?XeenJourneyActivity::Attachment:XeenJourneyActivity::Presentation;
   ++session._journeyGeneration;++_generation;retainJourney();schedule(now);return true;
  }
  session._journeyActivity=XeenJourneyActivity::Presentation;
  if(result.outcome!=XeenEncounterOutcome::Pending) {
   rest.random=XeenCombatRandom(*session._journeyRandom);if(++rest.charges==10)rest.phase=P::Remainder;
  }
  rest.deadline=now+100;restPublication();return true;
 }catch(...) {closeJourney();throw;}
}
}
