#include "app/XeenEncounterFlow.h"
#include "games/xeen/XeenJourneyProgression.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenItemCatalog.h"
#include "games/xeen/XeenIndoorScene.h"
#include <sstream>
#include <limits>
namespace mmodern {
namespace {
struct ConsequenceScope { bool &busy; explicit ConsequenceScope(bool &b):busy(b){busy=true;} ~ConsequenceScope(){busy=false;} };
XeenConsequenceCharacters activeCharacters(const XeenPartyState &p) {
 XeenConsequenceCharacters c;for(unsigned i=0;i<6;++i)c[i]=p.roster.at(kXeenCombatOwners[i]);return c;
}
XeenConsequenceInputs activeInputs(const XeenPartyState &p) {
 XeenConsequenceInputs c;for(unsigned i=0;i<6;++i)c[i]=*p.roster.combatInputs(kXeenCombatOwners[i]);return c;
}
bool unsupportedTime(const XeenGameplayContext &c) {
 const auto t=xeenPrepareTime(c,10);return t.dusks || t.dawns || t.midnights || t.yearRollovers || t.dailyProcessing;
}
}
bool XeenEncounterFlow::beginShoot() {
 if(_castingSettlement || !_journey || !xeenJourneyContent(_world.sessionState().journeyContract()).consequences() || _combat || _busy || _shoot || _regionalWork ||
  !current(ticket()) || !_boundary.quiet() || _state.phase()!=XeenEncounterPhase::Exploring) return false;
 const bool continuation=_shootIntent && !_regionalWork && !_regionalAutomatic && !projectilesPending() && !monsterReward();
 if((!_state.pending() && !journeyMutable() && !continuation) || !journeyCapacity()) return false;
 _shootIntent=false;
 if(_world.sessionState().journeyContract()==8 && _camera.mapId==XeenMapIdentity(28)) {
  _journeyRefusal="Shoot refused indoors";return true;
 }
 try {
  xeenValidateJourneyMelee(_party,_world.sessionState().journeyContract());
  if(unsupportedTime(*_party.encounterContext)) { _journeyRefusal="Shoot refused: charge crosses the supported time boundary";return true; }
  auto candidate=std::make_unique<XeenShootCandidate>();bool eligible=false;
  for(unsigned i=0;i<6;++i) {
   const auto &c=_party.roster.at(kXeenCombatOwners[i]);
   if(c.canAct()) for(const auto &w:c.weapons) if(w.frame==4) candidate->eligible[i]=eligible=true;
  }
  if(!eligible) { _journeyRefusal="Shoot refused: no awake eligible missile user";return true; }
  // Check before retaining intent or consuming old work, and again when that
  // work finishes: ranged damage can disable a previously eligible shooter.
  if(_state.pending()) { retireCastingFeedback();_shootIntent=true;journeyPulse(ticket());return true; }
  const auto heldPreimage=_journeyPreimage;
  XeenRestoreGuard::Providers providers(*heldPreimage,_world);
  const auto &map=_world.map(23);const auto view=XeenActorApproach::classify(_world.sessionState().actors(),_camera);
  const auto rows=xeenPlayerRayRows(map,_camera);
  candidate->projectileEnd.fill(rows-1);
  // Preserve the existing notice distinction: reaching the map edge is empty,
  // while an in-map obstruction identifies its blocked row.
  if(rows<4) {
   constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};const auto d=unsigned(_camera.direction);
   const int x=_camera.x+dx[d]*int(rows),y=_camera.y+dy[d]*int(rows);
   if(x>=0 && x<16 && y>=0 && y<16) candidate->blockedRow=rows;
  }
  for(unsigned row=0;row<rows;++row)
   for(unsigned slot=0;slot<3;++slot) candidate->targets[row*3+slot]=view.slots[row*3+slot];
  candidate->random=XeenCombatRandom(*_world.sessionState().journeyRandom());_journeyPreimage->check();
  retireCastingFeedback();_shoot.swap(candidate);_world._sessionState._journeyActivity=XeenJourneyActivity::Shoot;
  ++_world._sessionState._journeyGeneration;++_generation;_journeyPreimage->adoptJourneyCoordination();
  _journeyRefusal.clear();return true;
 } catch(const std::invalid_argument &e) {
  if(!_journeyPreimage->current()) { closeJourney();throw; }_journeyRefusal=e.what();return true;
 }
}
bool XeenEncounterFlow::serviceShoot() {
 if(!_shoot || monsterReward() || _busy || _failure || !current(ticket()) || !_boundary.quiet()) return false;
 ConsequenceScope busy(_busy);
 try {
  auto &work=*_shoot;auto &session=_world._sessionState;
  const auto heldPreimage=_journeyPreimage;
  XeenRestoreGuard::Providers providers(*heldPreimage,_world);
  XeenActorApproach::validateEnvironment(_world,session.actors(),_events,session.journeyContract());
  const auto check=[&] { _journeyPreimage->check(); };
  XeenConsequenceDraw draw{work.random,64,check};
  if(!work.volleyDone) {
   while(work.target<12) {
    const auto id=work.targets[work.target];
    if(!id || session.actors().at(id->recordIndex).lifecycle!=XeenActorLifecycle::Present) { ++work.target;work.shooter=0;continue; }
    while(work.shooter<6 && (!work.eligible[work.shooter] || work.spent[work.shooter])) ++work.shooter;
    if(work.shooter==6) { ++work.target;work.shooter=0;continue; }
    const auto &actor=session.actors().at(id->recordIndex);
    if(!(actor.id==*id)) throw std::logic_error("Shoot retained identity mismatch");
    const auto owner=kXeenCombatOwners[work.shooter];
    if(!work.attack) work.attack.emplace(_party.roster.at(owner),*_party.roster.combatInputs(owner),*actor.statistics,actor.original.resourceId,_party.encounterContext->year,true);
    if(!work.attackDone) {
     if(!work.attack->service(draw)) return true;
     if(work.attack->damage>=actor.hp && actor.original.resourceId==6) work.drop.emplace(*_party.monsterTreasure,id->recordIndex,session.journeyContract());
     work.attackDone=true;
    }
    if(work.drop && !work.drop->service(draw)) return true;
    std::optional<XeenJourneyLethal> lethal;
    if(work.attack->damage>=actor.hp) {
     std::array<const XeenCharacter *,6> owners{};
     for(unsigned i=0;i<6;++i) owners[i]=&_party.roster.at(kXeenCombatOwners[i]);
     lethal=xeenPrepareJourneyLethal(actor,owners,activeInputs(_party),session.accountedMonsters(),0x3f);
    }
    auto feedback="Shoot owner "+std::to_string(owner)+" -> actor "+std::to_string(id->recordIndex)+
     (work.attack->hit?" hit ":" miss ")+std::to_string(work.attack->damage)+(lethal?" defeated":"");
    if(work.drop) {
     feedback+="; gold +10 pending";
     if(work.drop->outcome==XeenMonsterDropOutcome::ReferenceMiscellaneousDropLoss)feedback+="; misc drop lost";
     if(work.drop->outcome==XeenMonsterDropOutcome::CategoryCapacityLoss)feedback+="; full category: drop lost";
    }
    if(session._encounterRevision==std::numeric_limits<std::uint64_t>::max() || !journeyCapacity()) throw std::overflow_error("Shoot generation exhausted");
    check();
    auto &live=session._actors.at(id->recordIndex);
    if(lethal) {
     live=lethal->actor;session._accountedMonsters.swap(lethal->accounted);
     for(unsigned i=0;i<6;++i) _party.roster._combatInputs[kXeenCombatOwners[i]]->experience=lethal->experience[i];
     if(work.drop) _party.monsterTreasure=work.drop->treasure;
    } else live.hp-=work.attack->damage;
    if(work.attack->hit) work.projectileEnd[work.shooter]=work.target/3;
    work.spent[work.shooter]=work.attack->hit;session._journeyRandom=work.random.continuation();
    ++session._encounterRevision;_state._revision=session._encounterRevision;++session._journeyGeneration;++_generation;
    ++work.shooter;work.attack.reset();work.drop.reset();work.attackDone=false;
    _journeyRefusal.swap(feedback);retainJourney();return true;
   }
   // One fired visual per admitted shooter, including an empty/blocked ray.
   // Miss continuation across targets does not launch additional missiles.
   std::vector<XeenProjectileAppearance> projectile;
   for(unsigned i=0;i<6;++i) if(work.eligible[i]) projectile.push_back({false,0,i,work.projectileEnd[i],{}});
   check();
   _projectiles.swap(projectile);_projectileCursor=0;_projectileDeadline=_lastTime+100;
   if(work.blockedRow<4)_journeyRefusal="Shoot stopped at terrain row "+std::to_string(work.blockedRow);
   else if(_journeyRefusal.empty())_journeyRefusal="Shoot: empty center rows";
   work.volleyDone=true;return true;
  }
  if(!work.time) work.time.emplace(*_party.encounterContext,10,activeCharacters(_party),activeInputs(_party));
  if(!work.time->service(draw)) return true;
  if(session._encounterRevision==std::numeric_limits<std::uint64_t>::max() || !journeyCapacity())throw std::overflow_error("Shoot charge generation exhausted");
  check();bool living=false;
  for(const auto &v:work.time->characters) {
   auto &c=_party.roster.at(v.rosterId);c.currentHp=v.currentHp;c.conditions=v.conditions;
   living=living || xeenCombatTargetable(c);
  }
  _party.encounterContext=work.time->context;session._journeyRandom=work.random.continuation();_state._pending=living?3:0;
  if(!living) { _state._phase=XeenEncounterPhase::SupportStopped;_state._reason=XeenEncounterStop::Defeat;session._encounterTerminal=true; }
  ++session._encounterRevision;_state._revision=session._encounterRevision;++session._journeyGeneration;++_generation;
  _shoot.reset();session._journeyActivity=XeenJourneyActivity::Presentation;retainJourney();return true;
 } catch(...) { closeJourney();throw; }
}
bool XeenEncounterFlow::beginMonsterReward(const XeenItemCatalog &catalog) {
 if(!xeenJourneyContent(_world.sessionState().journeyContract()).consequences() || monsterReward() || _combat || _busy || _failure || _regionalWork || projectilesPending() ||
  _state.pending() || _state.phase()!=XeenEncounterPhase::Exploring || (_shoot && !_shoot->volleyDone) || !_boundary.quiet()) return false;
 if(!_party.monsterTreasure || !_party.monsterTreasure->pending()) return false;
 const auto &actors=_world.sessionState().regionalActors(_camera.mapId);
 const auto view=_world.sessionState().journeyContract()==8 && _camera.mapId==XeenMapIdentity(28)
  ? XeenIndoorScene().classifyActors(_world,_camera,actors) : XeenActorApproach::classify(actors,_camera);
 for(const auto &slot:view.slots) if(slot) return false;
 if(!current(ticket()) || !journeyCapacity()) throw std::logic_error("Stale monster delivery");
 ConsequenceScope busy(_busy);
 try {
  auto delivery=xeenPrepareMonsterDelivery(*_party.monsterTreasure,activeCharacters(_party),_world.sessionState().journeyContract());
  std::ostringstream text;text<<"Monster treasure\n";
  if(delivery.globallyFull) text<<"All packs full. Items lost; gold retained.\n";
  for(unsigned i=0;i<delivery.count;++i) {
   const auto &r=delivery.records[i];text<<"Orc "<<unsigned(r.production.source)<<' '<<catalog.describe(r.armor?XeenInventoryCategory::Armor:XeenInventoryCategory::Weapons,r.production.item).displayName<<" M/ID/S/F 0/"<<unsigned(r.production.item.id)<<"/0/0 ";
   if(r.recipient) text<<"to "<<_party.roster.at(*r.recipient).name;else text<<(r.loss==XeenMonsterDeliveryLoss::GloballyFull?"lost: all packs full":r.loss==XeenMonsterDeliveryLoss::CategoryTailsFull?"lost: category tails full":"lost: no eligible recipient");text<<'\n';
  }
  text<<"Gold +"<<delivery.treasure.pendingGold<<" on final acknowledgment.";
  auto receiptText=text.str();_journeyPreimage->check();
  _rewardLease=_boundary.hold(XeenCombatBoundary::Work::Reward);_world._sessionState._journeyActivity=XeenJourneyActivity::Reward;
  for(const auto &v:delivery.characters) { auto &c=_party.roster.at(v.rosterId);c.weapons=v.weapons;c.armor=v.armor; }
  _party.monsterTreasure=delivery.treasure;_monsterReceipt.emplace(std::move(delivery));_monsterReceiptText.swap(receiptText);
  ++_world._sessionState._journeyGeneration;++_generation;retainJourney();return true;
 } catch(...) { closeJourney();throw; }
}
void XeenEncounterFlow::acknowledgeMonsterReward() {
 if(!monsterReward() || _busy || !current(ticket()) || !journeyCapacity()) throw std::logic_error("Stale monster receipt");
 ConsequenceScope busy(_busy);
 try {
  const auto credited=xeenPrepareMonsterGoldCredit(*_party.monsterTreasure,_world.sessionState().journeyContract());_journeyPreimage->check();
  _party.monsterTreasure=credited;_monsterReceipt.reset();
  _world._sessionState._journeyActivity=_shoot?XeenJourneyActivity::Shoot:XeenJourneyActivity::Presentation;
  _boundary.release(XeenCombatBoundary::Work::Reward,_rewardLease);_rewardLease=0;
  ++_world._sessionState._journeyGeneration;++_generation;retainJourney();
 } catch(...) { closeJourney();throw; }
}
void XeenEncounterFlow::observeRanged(std::shared_ptr<const XeenRegionalObservation> observation) {
 std::vector<XeenProjectileAppearance> prepared;
 for(unsigned i=0;i<observation->count;++i) {
  const auto &shot=observation->shots[i];
  if(shot.direction==_camera.direction && shot.distance>=1 && shot.distance<=3)
   prepared.push_back({true,shot.distance-1,unsigned(prepared.size()%6),shot.distance,shot.source});
 }
 _rangedObservation=std::move(observation);_projectiles.swap(prepared);_projectileCursor=0;_projectileDeadline=_lastTime+100;
}
bool XeenEncounterFlow::animateProjectiles() {
 if(_busy || !projectilesPending() || !current(ticket())) return false;
 std::uint64_t now;if(!prepareTime(ticket(),now)) return false;
 if(now<_projectileDeadline) return false;
 _lastTime=now;_projectileDeadline=now+100;
 auto &p=_projectiles[_projectileCursor];
 if(p.enemy ? p.row==0 : p.row>=p.distance) ++_projectileCursor;
 else if(p.enemy) --p.row;else ++p.row;
 return true;
}

std::string XeenEncounterFlow::consequenceNotice() const {
 std::ostringstream out;
 out<<"("<<_camera.x<<','<<_camera.y<<") "<<"NESW"[unsigned(_camera.direction)]<<" T="<<_party.encounterContext->minutes;
 const bool stopped=_failure || (_combat ? _combat->phase()==XeenCombatPhase::Defeat ||
  _combat->phase()==XeenCombatPhase::SupportStopped || _combat->phase()==XeenCombatPhase::Failed :
  _state.phase()==XeenEncounterPhase::SupportStopped);
 if(_combat && (_combat->phase()==XeenCombatPhase::SupportStopped || _combat->phase()==XeenCombatPhase::Failed)) {
  out<<(_combat->phase()==XeenCombatPhase::Failed?" FAILED: ":" SUPPORT STOP: ");
  switch(_combat->result().failure) {
  case XeenCombatFailure::Integrity: out<<"state/resource changed";break;
  case XeenCombatFailure::Preparation: out<<"preparation failed";break;
  case XeenCombatFailure::Time: out<<"time boundary";break;
  case XeenCombatFailure::Observation: out<<"presentation failed";break;
  case XeenCombatFailure::Overflow: out<<"numeric limit";break;
  default: out<<"unsupported combat";break;
  }
 } else if((_combat && _combat->phase()==XeenCombatPhase::Defeat) || _state.reason()==XeenEncounterStop::Defeat)out<<" DEFEAT";
 else if(_failure)out<<" FAILED: state/resource or preparation";
 else if(stopped) {
  out<<" SUPPORT STOP: ";
  switch(_state.reason()) {
  case XeenEncounterStop::Time: out<<"time boundary";break;
  case XeenEncounterStop::Preparation: out<<"preparation failed";break;
  case XeenEncounterStop::Reporting: out<<"presentation failed";break;
  case XeenEncounterStop::Overflow: out<<"numeric limit";break;
  case XeenEncounterStop::Domain: out<<"unsupported content";break;
  case XeenEncounterStop::Ranged: out<<"ranged operation";break;
  case XeenEncounterStop::RegionalContact: out<<"combat contact";break;
  case XeenEncounterStop::Envelope: out<<"regional boundary";break;
  default: out<<"unsupported operation";break;
  }
 } else if(_combat)out<<" Combat";
 else out<<" Quiet/approach";
 out<<" G"<<_party.monsterTreasure->gold<<"/"<<_party.monsterTreasure->gems<<'\n';
 if(stopped)out<<"Gameplay unavailable. Esc exits; restart last save.\n";
 if(_combat) {
  if(!stopped && _combat->phase()==XeenCombatPhase::PlayerReady) out<<_party.roster.at(kXeenCombatOwners[_combat->participant()]).name<<(xeenJourneyContent(_world.sessionState().journeyContract()).disengagement()?": Space/B; R Run; 1-3 target\n":": Space/B; 1-3 target\n");
  else if(!stopped)out<<"Automatic combat / End\n";
  const auto rows=_combat->contacts();
  for(unsigned i=0;i<rows.size();++i)if(rows[i]) {const auto &a=_world.sessionState().regionalActors(rows[i]->mapId).at(rows[i]->recordIndex);out<<(rows[i]==_combat->selectedTarget()?">":"")<<i+1<<' '<<a.statistics->name()<<" #"<<a.id.recordIndex<<" HP"<<a.hp<<'\n';}
  const auto &r=_combatObservation;
  if(r.operation==XeenCombatOperation::PlayerRun && r.runRoll)out<<_party.roster.at(*r.actingOwner).name<<(r.runSuccess?" escaped":" Run failed")<<" ("<<r.runRoll<<")\n";
  else if(r.monsterDrop)out<<"Orc "<<unsigned(r.generatedItem.source)<<" +10 gold pending; "<<(*r.monsterDrop==XeenMonsterDropOutcome::ReferenceMiscellaneousDropLoss?"misc drop lost":*r.monsterDrop==XeenMonsterDropOutcome::CategoryCapacityLoss?"full category: lost":*r.monsterDrop==XeenMonsterDropOutcome::Item?"item produced":"no item")<<'\n';
  else if(r.injuryCount)out<<"Damage "<<r.damage<<"; armor broken "<<r.armorCount<<'\n';
  else if(!r.monsterDrop && r.targetMonster && r.attackOutcome!=XeenCombatAttackOutcome::NotApplicable)out<<"Actor "<<r.targetMonster->recordIndex<<(r.attackOutcome==XeenCombatAttackOutcome::Miss?" missed":" damage ")<<r.damage<<'\n';

 } else if(!stopped) {
  const bool city=_world.sessionState().journeyContract()==8 && _camera.mapId==XeenMapIdentity(28);
  out<<"Arrows move/turn; . Wait; F Shoot:";
  bool eligible=false;
  for(unsigned i=0;i<6;++i){const auto &c=_party.roster.at(kXeenCombatOwners[i]);if(c.canAct())for(const auto &w:c.weapons)if(w.frame==4){out<<' '<<i+1;eligible=true;break;}}
  if(!eligible)out<<" none";
  out<<"\nI inventory; F9 quiet save; Space interact\n";
  const auto &active=_world.sessionState().regionalActors(_camera.mapId);
  const auto selected=city ? XeenIndoorScene().classifyActors(const_cast<XeenWorld &>(_world),_camera,active) : XeenActorApproach::classify(active,_camera);
  unsigned visible=0;for(const auto &id:selected.slots)if(id){if(!visible){const auto &a=active.at(id->recordIndex);out<<a.statistics->name()<<" #"<<a.id.recordIndex<<" HP"<<a.hp;}++visible;}
  if(visible>1)out<<" +"<<visible-1<<" threats";
  if(visible)out<<'\n';
  if(_party.monsterTreasure->dormant())out<<"Dormant items; no gold owed\n";
 if(_party.monsterTreasure->pending())out<<"Ready treasure: +"<<_party.monsterTreasure->pendingGold<<" gold\n";
  if(!_journeyRefusal.empty())out<<_journeyRefusal<<'\n';
  if(_rangedObservation && _rangedObservation->count) {const auto &v=_rangedObservation->shots[(_lastTime/500)%_rangedObservation->count];out<<"Enemy shot #"<<v.source.recordIndex<<" from "<<"NESW"[unsigned(v.direction)]<<" damage "<<v.attack.damage<<'\n';}
 }
 if(!stopped && _itemUseResult) {
  const auto &use=*_itemUseResult;
  out<<"Antidote: ";
  if(!use.target)out<<"target cancelled after charge spent";
  else if(use.poisonBefore)out<<"Poison cleared for "<<_party.roster.at(*use.target).name;
  else out<<"no Poison on "<<_party.roster.at(*use.target).name;
  if(use.exhausted)out<<"; item exhausted";
  out<<'\n';
 }
	if (!_castingResult.empty()) {
		out<<_castingResult;
		if (_castingSettlement && !_combat && (_state.pending() || _regionalWork || projectilesPending())) out<<"; actor work pending";
		out<<'\n';
	}
 const bool finishNotice = _disengagementNoticeRevision &&
  _retiredCombatResult.operation==XeenCombatOperation::FinishDisengagement &&
  (_combat ? _disengagementNoticeCombat && _combat->current(*_disengagementNoticeCombat) :
   *_disengagementNoticeRevision==_state.revision());
 if(!stopped && finishNotice) {
  out<<"Disengaged; gold forfeited "<<_retiredCombatResult.forfeitedGold<<'\n';
  if(_retiredCombatResult.casualties) {
   out<<"Abandoned:";for(unsigned i=0;i<6;++i)if(_retiredCombatResult.casualties&(1u<<i))out<<' '<<_party.roster.at(kXeenCombatOwners[i]).name;
   out<<'\n';
  }
  if(_combat && _party.monsterTreasure->dormant())out<<"Dormant items; no gold owed\n";
  if(_combat && _party.monsterTreasure->pending())out<<"Ready treasure: +"<<_party.monsterTreasure->pendingGold<<" gold\n";
 }
 auto text=out.str();if(!text.empty()&&text.back()=='\n')text.pop_back();text+="\n\n";
 unsigned participantSlot=0;
 for(auto owner:kXeenCombatOwners) {
  const auto &c=_party.roster.at(owner);text+=c.name+" HP"+std::to_string(c.currentHp)+"\n";
  text+="P"+std::to_string(c.conditions[3])+" S"+std::to_string(c.conditions[8])+" D"+std::to_string(c.conditions[4]);
  if(c.conditions[13])text+=" Dead";else if(c.conditions[12])text+=" Uncon";
  text+="\nXP"+std::to_string(_party.roster.combatInputs(owner)->experience);
  if(_combat && !(_combat->participants()&(1u<<participantSlot)))text+=" Esc";
  int damage=0;for(unsigned i=0;i<_combatObservation.injuryCount;++i)if(_combatObservation.injuries[i].owner==owner)damage+=_combatObservation.injuries[i].amount;
  if(_combat && damage)text+=" -"+std::to_string(damage);
  text+='\n';++participantSlot;
 }
 if(!text.empty())text.pop_back();
 return text;
}

}
