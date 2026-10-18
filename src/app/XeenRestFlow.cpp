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
bool XeenEncounterFlow::beginRest(const std::function<IndexedFrame()> &dream,
 const std::function<std::array<std::uint64_t,2>()> &durations,bool shelter) {
 if(!_journey || _rest || _busy || _combat || _failure || _regionalWork || _regionalAutomatic ||
  _shoot || _shootIntent || (!shelter && (_casting || _castingSettlement || !_boundary.quiet())) || projectilesPending() || monsterReward() ||
  !current(ticket()) || _state.pending()>3 || _state.phase()!=XeenEncounterPhase::Exploring || !journeyCapacity())return false;
 if(shelter && !((_casting && _casting->shelterPending) || (_itemUse && _itemUse->shelterPending)))return false;
 if(shelter && !((_casting && _boundary.only(XeenCombatBoundary::Work::Casting,_casting->lease)) ||
  (_itemUse && _boundary.only(XeenCombatBoundary::Work::ItemUse,_itemUse->lease))))return false;
 RestBusy busy(_busy);
 try {
  const auto retained=_journeyPreimage;XeenRestoreGuard::Providers providers(*retained,_world);
  xeenValidateJourneyMelee(_party);
  // DOS lookup latches attribute bit 0x40 (0x11fe7); Rest reads it at
  // 0x1e50d. The map restriction still belongs to the primary map.
  const auto cell=_world.sampleCell(_camera.mapId,_camera.x,_camera.y);
  if(!cell)throw std::invalid_argument("Rest cell is absent");
  const auto &primary=_world.map(_camera.mapId).geometry;
  // doStepCode follows resetTemps, which clears levitation. Evaluate that
  // foreseeable suffix before publishing sleep, charges, food or RNG.
  auto ending=*_party.encounterContext;xeenResetPartyTemps(ending);
  const auto terrain=_world.prepareTerrainArrival(_camera,members(_party,_party.party.activeOrder()),ending);
  auto candidate=std::make_unique<RestContinuation>();candidate->owners=_party.party.activeOrder();
  candidate->shelter=shelter;
  if(!shelter && ((cell->cell->rawAttributes&0x40) || (primary.flags&0x4000)))candidate->phase=RestContinuation::Phase::Refused;
  else if(!terrain) {
   candidate->phase=RestContinuation::Phase::Refused;
   candidate->refusal=terrain.transition.reason;
  }
  else {
   candidate->phase=xeenRestDanger(members(_party,candidate->owners),supplements(_party,candidate->owners),_party.encounterContext->year)?
    RestContinuation::Phase::Confirm:shelter?RestContinuation::Phase::Remainder:RestContinuation::Phase::Charges;
   if(_party.encounterContext->profile==XeenBehaviorProfile::WorldOfXeenClouds) {
    if(!dream || !durations)throw std::invalid_argument("Original Rest dream providers are absent");
    candidate->sampleDurations=durations();
    if(!candidate->sampleDurations[0] || !candidate->sampleDurations[1])
     throw std::invalid_argument("Invalid original Rest sample duration");
    candidate->dream=dream();
    if(candidate->dream.width!=320 || candidate->dream.height!=200 || !candidate->dream.isValid())
     throw std::invalid_argument("Invalid original Rest dream image");
   }
  }
  candidate->random=XeenCombatRandom(*_world.sessionState().journeyRandom());retained->check();
  // Rest owns its nested presentation and terrain suffix. Suspend the spell's
  // modal lease so the existing terrain transition can run if Levitate expires.
  // The retained continuations still prevent input and quiet-save admission.
  if(shelter) {
   if(_casting) {_boundary.release(XeenCombatBoundary::Work::Casting,_casting->lease);_casting->lease=0;}
   if(_itemUse) {_boundary.release(XeenCombatBoundary::Work::ItemUse,_itemUse->lease);_itemUse->lease=0;}
  }
  if(candidate->phase==RestContinuation::Phase::Charges || candidate->phase==RestContinuation::Phase::Remainder)
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
  _rest->phase=_rest->shelter?P::Remainder:P::Charges;restPublication();return true;
 }
 const bool completed=_rest->phase==P::Complete,shelter=_rest->shelter;_rest.reset();
 if(shelter) {
  if(_casting) {_casting->lease=_boundary.hold(XeenCombatBoundary::Work::Casting);_casting->shelterPending=false;_casting->random=XeenCombatRandom(*_world.sessionState().journeyRandom());}
  if(_itemUse) {_itemUse->lease=_boundary.hold(XeenCombatBoundary::Work::ItemUse);_itemUse->shelterPending=false;}
 }
 if(completed) {
  bool living=false;for(auto owner:_party.party.activeRosterIds())living=living || xeenCombatTargetable(_party.roster.at(owner));
  living=living && !_terrainDead;
  if(!living) {_state._phase=XeenEncounterPhase::SupportStopped;_state._reason=XeenEncounterStop::Defeat;_world._sessionState._encounterTerminal=true;}
 }
 _world._sessionState._journeyActivity=_casting?XeenJourneyActivity::Casting:_itemUse?XeenJourneyActivity::ItemUse:XeenJourneyActivity::Presentation;
 ++_world._sessionState._journeyGeneration;++_generation;retainJourney();schedule(_lastTime);return true;
}
bool XeenEncounterFlow::serviceRest() {
 if(!_rest || _busy || _combat || _failure || !_rest->presented || !current(ticket()) || !journeyCapacity())return false;
 using P=RestContinuation::Phase;auto &rest=*_rest;
 if(rest.phase==P::Confirm || rest.phase==P::Refused || rest.phase==P::Complete)return false;
 std::uint64_t now;if(!prepareTime(ticket(),now) ||
  (rest.phase==P::Dream?_lastNanoseconds<rest.dreamDeadline:now<rest.deadline))return false;
 RestBusy busy(_busy);_lastTime=now;
 try {
  const auto retained=_journeyPreimage;const auto check=[&]{retained->check();};
  XeenRestoreGuard::Providers providers(*retained,_world);auto &session=_world._sessionState;
  const bool outdoor=_world.map(_camera.mapId).geometry.isOutdoors();check();
  if(rest.phase==P::Dream) {
   // DAT 0x1e605..0x1e72a. Each fade publishes every level; each
   // seven-tick hold has one reset, and the two sample intervals are sequential.
   using D=RestContinuation::Dream;
   if(rest.dreamFade() && rest.fadeStep<32)++rest.fadeStep;
   else {
    rest.fadeStep=0;
    switch(rest.dreamPhase) {
    case D::FadeOutGameplay:rest.dreamPhase=D::FadeInDream;break;
    case D::FadeInDream:
     rest.dreamPhase=D::BeforeDreams;rest.dreamDeadline=PresentationClock::after(_lastNanoseconds,7);break;
    case D::BeforeDreams:
     rest.dreamPhase=D::Dreams;rest.dreamDeadline=PresentationClock::checkedAdd(_lastNanoseconds,rest.sampleDurations[0]);break;
    case D::Dreams:
     rest.dreamPhase=D::Laff;rest.dreamDeadline=PresentationClock::checkedAdd(_lastNanoseconds,rest.sampleDurations[1]);break;
    case D::Laff:
     rest.dreamPhase=D::AfterLaff;rest.dreamDeadline=PresentationClock::after(_lastNanoseconds,7);break;
    case D::AfterLaff:rest.dreamPhase=D::FadeOutDream;break;
    case D::FadeOutDream:rest.dreamPhase=D::FadeInGameplay;break;
    case D::FadeInGameplay:rest.phase=P::Recovery;break;
    }
   }
   // Acquisition arms the next retrace. A late host wake advances one level
   // or one interval only; it cannot spend elapsed time on later stages.
   if(rest.phase==P::Dream) {restPublication();return true;}
   // The last fade has completed. Recovery follows it immediately; do not
   // invent another two-tick draw/hold between the restored scene and summary.
  }
  if(rest.phase==P::Recovery) {
   if(!rest.recovery)rest.recovery.emplace(members(_party,rest.owners),supplements(_party,rest.owners),*_party.encounterContext,_party.food);
   auto &recovery=*rest.recovery;
   if(!rest.terrain) {
    auto rule=_world.prepareTerrainArrival(_camera,recovery.characters,recovery.context);
    if(!rule)throw std::logic_error("Rest arrival admission changed");
    if(rule.falling) {_terrainFall=rule;rule.damage=0;}
    rest.terrain.emplace(rule,recovery.characters,recovery.inputs,recovery.context,&*_party.serviceEconomy);
   }
   XeenConsequenceDraw draw{rest.random,64,check};if(!rest.terrain->service(draw))return true;
   recovery.context=rest.terrain->context;recovery.characters=rest.terrain->characters;recovery.inputs=rest.terrain->inputs;
   auto exploration=_world.explorationAfterStep(_party,_camera);check();
   for(unsigned n=0;n<6;++n) {
    _party.roster.at(rest.owners[n])=recovery.characters[n];
    _party.roster._combatInputs[rest.owners[n]]=recovery.inputs[n];
   }
   _party.food=recovery.food;_party.encounterContext=recovery.context;
   _party.serviceEconomy=rest.terrain->economy;session._journeyRandom=rest.random.continuation();
   _needsRestNotice=rest.terrain->needsRest;_terrainDead=rest.terrain->rule.dead;
   if(rest.terrain->rule.damage)for(auto owner:rest.owners)
    _world.scenePresentation().portraitDamage(owner,rest.terrain->rule.type==XeenDamageType::Fire?1u:0u,_lastTime);
   rest.consumed=recovery.consumed;rest.starving=recovery.starving;rest.phase=P::Complete;
   session.publishExploration(exploration);restPublication();return true;
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
   if(!rest.time)rest.time.emplace(*_party.encounterContext,rest.shelter?480:outdoor?380:470,
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
  rest.deadline=PresentationClock::afterMilliseconds(now,2);restPublication();return true;
 }catch(...) {closeJourney();throw;}
}
}
