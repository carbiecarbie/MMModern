#include "app/XeenEncounterFlow.h"
#include "games/xeen/XeenJourneyProgression.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenItemCatalog.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenCharacterRules.h"
#include <sstream>
#include <limits>
namespace mmodern {
namespace {
struct ConsequenceScope { bool &busy; explicit ConsequenceScope(bool &b):busy(b){busy=true;} ~ConsequenceScope(){busy=false;} };
XeenConsequenceCharacters activeCharacters(const XeenPartyState &p,const XeenPartyOrder &order) {
 XeenConsequenceCharacters c;for(unsigned i=0;i<6;++i)c[i]=p.roster.at(order[i]);return c;
}
XeenConsequenceInputs activeInputs(const XeenPartyState &p,const XeenPartyOrder &order) {
 XeenConsequenceInputs c;for(unsigned i=0;i<6;++i)c[i]=*p.roster.combatInputs(order[i]);return c;
}
void nextMonsterRewardItem(XeenMonsterDeliveryCandidate &receipt) {
	std::array<XeenCharacter *,6> owners{};
	for(unsigned i=0;i<6;++i)owners[i]=&receipt.characters[i];
	while(receipt.cursor<receipt.count) {
		auto &entry=receipt.records[receipt.cursor];
		const auto category=entry.miscellaneous?XeenRewardCategory::Miscellaneous:entry.armor?XeenRewardCategory::Armor:XeenRewardCategory::Weapons;
		const auto recipient=xeenDeliverRewardItem(owners.data(),owners.size(),entry.production.item,category,{},false);
		if(recipient) {
			entry.recipient=receipt.characters[*recipient].rosterId;entry.loss=XeenMonsterDeliveryLoss::None;
			receipt.phase=XeenMonsterDeliveryCandidate::Phase::Blank;return;
		}
		entry.recipient.reset();entry.loss=XeenMonsterDeliveryLoss::GloballyFull;++receipt.cursor;
	}
	receipt.phase=XeenMonsterDeliveryCandidate::Phase::Final;
}

}
bool XeenEncounterFlow::beginShoot() {
 if(_castingSettlement || !_journey || _combat || _busy || _shoot || _regionalWork ||
  !current(ticket()) || !_boundary.quiet() || _state.phase()!=XeenEncounterPhase::Exploring) return false;
 const bool continuation=_shootIntent && !_regionalWork && !_regionalAutomatic && !projectilesPending() && !monsterReward();
 if((!_state.pending() && !journeyMutable() && !continuation) || !journeyCapacity()) return false;
 _shootIntent=false;
 try {
  const bool indoor=!_world.map(_camera.mapId).geometry.isOutdoors();
  const unsigned charge=indoor?1:10;
  xeenValidateJourneyMelee(_party);
  auto candidate=std::make_unique<XeenShootCandidate>();bool eligible=false;
  candidate->owners=_party.party.activeOrder();candidate->charge=charge;
  for(unsigned i=0;i<6;++i) {
   const auto &c=_party.roster.at(candidate->owners[i]);
   if(c.canAct()) for(const auto &w:c.weapons) if(w.frame==4) candidate->volley.eligible[i]=eligible=true;
  }
  // Nobody can shoot: the original only plays sound effect 21, with no message
  // (pin interface.cpp Shoot `!party.canShoot()`; maintainer DOSBox CD play,
  // 2026-10-15). Audio arrives with the audio arc; until then this is silent.
  if(!eligible)return true;
  {
   XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
   auto ending=xeenPrepareTime(*_party.encounterContext,charge);
   if(ending.dailyProcessing)xeenResetPartyTemps(ending.context);
   const auto arrival=_world.prepareTerrainArrival(_camera,activeCharacters(_party,candidate->owners),ending.context);
   _journeyPreimage->check();
   if(!arrival) {_journeyRefusal=arrival.transition.reason;return false;}
  }
  // Check before retaining intent or consuming old work, and again when that
  // work finishes: ranged damage can disable a previously eligible shooter.
  if(_state.pending()) { retireCastingFeedback();_shootIntent=true;journeyPulse(ticket());return true; }
  const auto heldPreimage=_journeyPreimage;
  XeenRestoreGuard::Providers providers(*heldPreimage,_world);
  const auto &active=_world.sessionState().regionalActors(_camera.mapId);
  const auto view=indoor?XeenIndoorScene().classifyActors(_world,_camera,active):XeenActorApproach::classify(active,_camera);
  const auto rows=xeenPlayerRayRows(_world,_camera);
  candidate->projectileEnd.fill(rows-1);
  candidate->rows=rows;
  std::vector<XeenProjectileAppearance> projectile;
  for(unsigned i=0;i<6;++i)if(candidate->volley.eligible[i])projectile.push_back({false,0,i,3,{}});
  // Preserve the existing notice distinction: reaching the map edge is empty,
  // while an in-map obstruction identifies its blocked row.
  if(rows<4) {
   constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};const auto d=unsigned(_camera.direction);
   const int x=_camera.x+dx[d]*int(rows),y=_camera.y+dy[d]*int(rows);
   if(indoor ? bool(_world.sampleCell(_camera.mapId,x,y)) : x>=0 && x<16 && y>=0 && y<16) candidate->blockedRow=rows;
  }
  for(unsigned row=0;row<rows;++row)
   for(unsigned slot=0;slot<3;++slot) candidate->targets[row*3+slot]=view.slots[row*3+slot];
  candidate->random=XeenCombatRandom(*_world.sessionState().journeyRandom());_journeyPreimage->check();
  retireCastingFeedback();_shoot.swap(candidate);_world._sessionState._journeyActivity=XeenJourneyActivity::Shoot;
  _projectiles.swap(projectile);_projectileDeadline=0;
  ++_world._sessionState._journeyGeneration;++_generation;_journeyPreimage->adoptJourneyCoordination();
  _journeyRefusal.clear();return true;
 } catch(const std::invalid_argument &e) {
  if(!_journeyPreimage->current()) { closeJourney();throw; }_journeyRefusal="Shoot is not supported yet for this party.";return true;
 }
}
bool XeenEncounterFlow::serviceShoot() {
 if(!_shoot || monsterReward() || _busy || _failure || !current(ticket()) ||
  (!_boundary.quiet() && !(_shoot->spell && ((_casting && _boundary.only(XeenCombatBoundary::Work::Casting,_casting->lease)) ||
    (_itemUse && _itemUse->effectDone && _boundary.only(XeenCombatBoundary::Work::ItemUse,_itemUse->lease)))))) return false;
 ConsequenceScope busy(_busy);
 try {
  auto &work=*_shoot;auto &session=_world._sessionState;
  const auto heldPreimage=_journeyPreimage;
  XeenRestoreGuard::Providers providers(*heldPreimage,_world);
  const auto active=session.regionalActors(_camera.mapId);
  auto &liveActors=session.mutableMonsters(_camera.mapId);
  std::vector<XeenActor> admission(active.begin(),active.end());
  if(work.stage==XeenShootCandidate::Stage::PostImpact && work.bound)admission.at(work.bound->id.recordIndex)=*work.bound;
  XeenActorApproach::validateEnvironment(_world,admission,_events);
  const auto check=[&] { _journeyPreimage->check(); };
  XeenConsequenceDraw draw{work.random,64,check};
  if(work.spell && !work.spellTargetsPrepared) {
   if(!work.spellPreparation->service(draw))return true;
   const auto targets=xeenSpellTargets(_world,_camera,active,{},XeenLearnedSpellRules::range(std::uint8_t(*work.spell)));
   std::array<unsigned,4> counts{};
   for(const auto &id:targets)if(id) {
    const auto &a=active.at(id->recordIndex);
    const unsigned row=std::abs(a.x-_camera.x)+std::abs(a.y-_camera.y);
    work.targets.at(row*3+counts.at(row)++)=id;
   }
   work.spellTargetsPrepared=true;
   if(_projectiles.empty()) {
    _projectiles.push_back({false,0,0,3,{},xeenSpellProjectile(*work.spell,work.spellPreparation->type,work.itemPower)});
    work.presented=false;return true;
   }
  }
  if(!work.volleyDone) {
   using Stage=XeenShootCandidate::Stage;
   if(!work.presented)return false;
   if(_lastTime<work.deadline)return false;
   // drawScene renders the current lanes, then animate3d advances every lane.
   // Consume that acknowledged draw once, independently of target traversal.
   if(work.advanceDraw) {for(auto &p:_projectiles)p.advance();work.advanceDraw=false;}
   if(work.retireLane) {for(auto &p:_projectiles)if(p.lane==*work.retireLane)p.active=false;work.retireLane.reset();}
   while(work.target<12) {
    const bool shooting=work.volley.remaining();
    if(!shooting) {work.target=12;work.row=work.rows-1;break;}
    if(work.target/3>work.row) {
     if(work.row+1>=work.rows) {work.target=12;break;}
     ++work.row;work.presented=false;return true;
    }
    const auto id=work.targets[work.target];
    if(!id || liveActors.at(id->recordIndex).lifecycle!=XeenActorLifecycle::Present) { ++work.target;work.volley.nextTarget();continue; }
    if(!work.volley.select()) { ++work.target;work.volley.nextTarget();continue; }
    const auto &actor=work.bound ? *work.bound : liveActors.at(id->recordIndex);
    if(!(actor.id==*id)) throw std::logic_error("Shoot retained identity mismatch");
    const auto owner=work.owners[work.volley.shooter];
    if(!work.attack && !work.spellAttack) {work.bound=actor;
     if(work.spell==XeenLearnedSpell::PoisonVolley) {
      work.spellAttack.emplace(_party.roster.at(owner),*_party.roster.combatInputs(owner),*actor.statistics,actor.original.resourceId,_party.encounterContext->year,_party.encounterContext->difficulty,actor.status,_camera.mapId.side==XeenSide::Clouds);
     } else if(work.spell) {const auto &caster=_party.roster.at(work.spellCaster);
      work.spellAttack.emplace(caster.permanentLevel,caster.temporaryLevel,actor.statistics->magicResistance(),actor.original.resourceId,*work.spell,actor.statistics->physicalResistance(),actor.status,actor.statistics->monsterType(),&*work.spellPreparation,actor.hp,_camera.mapId.side==XeenSide::Clouds);
     } else work.attack.emplace(_party.roster.at(owner),*_party.roster.combatInputs(owner),*actor.statistics,actor.original.resourceId,_party.encounterContext->year,true,_party.encounterContext->difficulty,actor.status);}
    const auto damage=[&] {return work.spellAttack?work.spellAttack->damage:work.attack->damage;};
    const auto status=[&] {return work.spellAttack?work.spellAttack->status:work.attack->status;};
    const auto hit=[&] {return work.spellAttack?(work.spell!=XeenLearnedSpell::PoisonVolley || !work.spellAttack->miss):work.attack->hit;};
    if(!work.attackDone) {
     if(work.spellAttack ? !work.spellAttack->service(draw) : !work.attack->service(draw)) return true;
     work.impactRandom=work.random.continuation();
     if(damage()>=actor.hp && actor.original.resourceId==6) work.drop.emplace(*_party.monsterTreasure,*id);
     work.attackDone=true;
    }
    // Lethal drop requests belong after the acquired post-HP draw, never
    // before attack2 publishes damage (0x2c184..0x2c54e).
    if(work.stage==Stage::PostImpact && work.drop && !work.drop->service(draw)) return true;
    if(!work.suffixPrepared) {
     if(damage()>=actor.hp) {
      std::array<const XeenCharacter *,6> owners{};
      for(unsigned i=0;i<6;++i) owners[i]=&_party.roster.at(work.owners[i]);
      work.lethal=xeenPrepareJourneyLethal(actor,owners,activeInputs(_party,work.owners),session.accountedMonsters(),0x3f);
     }
     work.feedback="Shoot owner "+std::to_string(owner)+" -> actor "+std::to_string(id->recordIndex)+
      (hit()?" hit ":" miss ")+std::to_string(damage())+(work.lethal?" defeated":"");
     if(work.drop) {
      work.feedback+="; gold +10 pending";
      if(work.drop->outcome==XeenMonsterDropOutcome::CategoryCapacityLoss)work.feedback+="; full category: drop lost";
     }
     if(work.spell)work.feedback.clear();
     if(session._encounterRevision>=std::numeric_limits<std::uint64_t>::max()-1 || !journeyCapacity()) throw std::overflow_error("Shoot generation exhausted");
     check();work.suffixPrepared=true;
    }
    if(work.stage==Stage::Travel) {
     // attack2 wakes before the pre-HP draw, including zero final damage.
     if(liveActors.at(id->recordIndex).status!=status()) {
      check();liveActors.at(id->recordIndex).status=status();
      ++session._encounterRevision;_state._revision=session._encounterRevision;
      ++session._journeyGeneration;++_generation;retainJourney();
     }
     // The logical shooter is spent, but hitMonster clears its visual slots
     // only after attack2 returns (0x2cbab..0x2ccba). Remaining visible arrows
     // continue through the common impact draws.
     if(hit() && !work.spell && work.attack->physical>0) {
      // attack2 arms the splat only for a positive physical share; the
      // elemental frame joins it with its own scale (interface_scene.cpp).
      std::array<std::optional<XeenMonsterIdentity>,3> front{};
      std::copy_n(work.targets.begin(),3,front.begin());
      _world.scenePresentation().hitSplat(work.target,damage(),0,xeenSplatAlternatePosition(front),_lastTime,
       work.attack->elementalFrame,work.attack->elemental);
     }
     if(work.spellAttack && work.spellAttack->impact)
       _world.scenePresentation().spellImpact(liveActors.at(id->recordIndex),*work.spell,work.spellPreparation->type);
     work.stage=damage() ? Stage::Impact : Stage::PostImpact;
     _appearanceIdentity=id;_frame=damage()?11:0;
     if(hit()) {work.presented=false;++_generation;return true;}
    }
    if(work.stage==Stage::Impact) {
     check();liveActors.at(id->recordIndex).hp=std::max(actor.hp-damage(),0);
     liveActors.at(id->recordIndex).status=status();
     session._journeyRandom=work.impactRandom;
     work.stage=Stage::PostImpact;work.presented=false;
     ++session._encounterRevision;_state._revision=session._encounterRevision;++session._journeyGeneration;++_generation;
     retainJourney();return true;
    }
    const bool removed=bool(work.lethal);
    if(session._encounterRevision==std::numeric_limits<std::uint64_t>::max() || !journeyCapacity()) throw std::overflow_error("Shoot generation exhausted");
    check();
    auto &live=liveActors.at(id->recordIndex);
    if(work.lethal) {
     if(work.target<3)_world.scenePresentation().splats[work.target]={};
     live=work.lethal->actor;session._accountedMonsters.swap(work.lethal->accounted);
     for(unsigned i=0;i<6;++i) _party.roster._combatInputs[work.owners[i]]->experience=work.lethal->experience[i];
     if(work.drop) _party.monsterTreasure=work.drop->treasure;
    } else live.hp=actor.hp-damage();
    live.status=status();
    if(hit()) work.projectileEnd[work.volley.shooter]=work.target/3;
    session._journeyRandom=work.random.continuation();
    if(hit() && (!work.spell || work.spell==XeenLearnedSpell::PoisonVolley)) {
     if(removed)work.retireLane=work.volley.shooter;
     else for(auto &p:_projectiles)if(p.lane==work.volley.shooter)p.active=false;
    }
    ++session._encounterRevision;_state._revision=session._encounterRevision;++session._journeyGeneration;++_generation;
    if(work.spell && work.spell!=XeenLearnedSpell::PoisonVolley) {
     ++work.target;work.volley.nextTarget();work.volley.spent.fill(false);
     bool remaining=false;for(unsigned n=work.target;n<work.targets.size();++n)remaining=remaining || bool(work.targets[n]);
     if(!remaining) {work.target=12;if(XeenLearnedSpellRules::range(std::uint8_t(*work.spell))!=XeenSpellRange::All)work.row=work.rows-1;}
    }
    else work.volley.settle(hit());
    work.spellAttack.reset();work.attack.reset();work.drop.reset();work.bound.reset();work.lethal.reset();work.attackDone=work.suffixPrepared=false;work.stage=Stage::Travel;
    _frame=0;_appearanceIdentity.reset();
    // Hit accounting is developer output, never a player-facing refusal.
    retainJourney();
    if(removed) {work.presented=false;return true;}
   }
   if(work.row+1<work.rows) {
    ++work.row;work.presented=false;return true;
   }
   check();
   _projectiles.clear();
   _journeyRefusal.clear();
   work.volleyDone=true;return true;
  }
  if(work.spell) {
   if(work.spell==XeenLearnedSpell::MoonRay) {
    if(!work.healing.service(draw,_party,_party.encounterContext->year))return true;
    check();
    for(const auto &effect:work.healing.result.effects) {
     auto &live=_party.roster.at(effect.owner);
     const bool sparkle=live.currentHp<=XeenCharacterRules::maxHp(live,{_party.encounterContext->year});
     live.currentHp=effect.hp;live.conditions=effect.conditions;
     if(sparkle)_world.scenePresentation().spellEffect(effect.owner,_lastTime,true);
    }
   }
   session._journeyRandom=work.random.continuation();
   if(_itemUse) {_itemUse->ready=true;session._journeyActivity=XeenJourneyActivity::ItemUse;}
   else {_casting->effectDone=true;_casting->random=work.random;session._journeyActivity=XeenJourneyActivity::Casting;}
   _shoot.reset();
   ++session._journeyGeneration;++_generation;retainJourney();return true;
  }
  if(!work.time) work.time.emplace(*_party.encounterContext,work.charge,activeCharacters(_party,work.owners),activeInputs(_party,work.owners),&*_party.serviceEconomy);
  if(!work.time->service(draw)) return true;
  if(!work.terrain) {
   auto rule=_world.prepareTerrainArrival(_camera,work.time->characters,work.time->context);
   if(!rule)throw std::logic_error("Shoot arrival admission changed");
   if(rule.falling) {_terrainFall=rule;rule.damage=0;}
   work.terrain.emplace(rule,work.time->characters,work.time->inputs,work.time->context,work.time->economy?&*work.time->economy:nullptr);
  }
  if(!work.terrain->service(draw))return true;
  work.time->characters=work.terrain->characters;work.time->inputs=work.terrain->inputs;
  work.time->context=work.terrain->context;work.time->economy=work.terrain->economy;
  work.time->needsRest=work.time->needsRest || work.terrain->needsRest;
  _terrainDead=work.terrain->rule.dead;
  if(session._encounterRevision==std::numeric_limits<std::uint64_t>::max() || !journeyCapacity())throw std::overflow_error("Shoot charge generation exhausted");
  auto exploration=_world.explorationAfterStep(_party,_camera);check();bool living=false;
  for(const auto &v:work.time->characters) {
   auto &c=_party.roster.at(v.rosterId);c=v;
   living=living || xeenCombatTargetable(c);
  }
  living=living && !_terrainDead;
  if(work.terrain->rule.damage)for(auto owner:work.owners)
   _world.scenePresentation().portraitDamage(owner,work.terrain->rule.type==XeenDamageType::Fire?1u:0u,_lastTime);
  if(work.time->needsRest)_needsRestNotice=true;
  _party.serviceEconomy=work.time->economy;for(unsigned n=0;n<6;++n)_party.roster._combatInputs[work.owners[n]]=work.time->inputs[n];
  _party.encounterContext=work.time->context;session._journeyRandom=work.random.continuation();_state._pending=living && !_terrainFall?3:0;
  if(!living) { _state._phase=XeenEncounterPhase::SupportStopped;_state._reason=XeenEncounterStop::Defeat;session._encounterTerminal=true; }
  ++session._encounterRevision;_state._revision=session._encounterRevision;++session._journeyGeneration;++_generation;
  session.publishExploration(exploration);
  _shoot.reset();session._journeyActivity=XeenJourneyActivity::Presentation;retainJourney();return true;
 } catch(...) { closeJourney();throw; }
}
bool XeenEncounterFlow::beginMonsterReward(const XeenItemCatalog &catalog) {
 if(monsterReward() || _combat || _busy || _failure || _regionalWork || projectilesPending() ||
  _state.pending() || _state.phase()!=XeenEncounterPhase::Exploring || (_shoot && !_shoot->volleyDone) || !_boundary.quiet()) return false;
 if(!_party.monsterTreasure || !_party.monsterTreasure->pending()) return false;
 const auto &actors=_world.sessionState().regionalActors(_camera.mapId);
 const auto view=!_world.map(_camera.mapId).geometry.isOutdoors()
  ? XeenIndoorScene().classifyActors(_world,_camera,actors) : XeenActorApproach::classify(actors,_camera);
 for(const auto &slot:view.slots) if(slot) return false;
 if(!current(ticket()) || !journeyCapacity()) throw std::logic_error("Stale monster delivery");
 ConsequenceScope busy(_busy);
 try {
  auto delivery=xeenPrepareMonsterDelivery(*_party.monsterTreasure,activeCharacters(_party,_party.party.activeOrder()));
  std::ostringstream text;text<<"Monster treasure\n";
  if(delivery.globallyFull) text<<"All packs full. Items lost; gold retained.\n";
  for(unsigned i=0;i<delivery.count;++i) {
   const auto &r=delivery.records[i];const auto &item=r.production.item;
   const auto category=r.miscellaneous?XeenInventoryCategory::Miscellaneous:r.armor?XeenInventoryCategory::Armor:XeenInventoryCategory::Weapons;
   text<<"Orc "<<unsigned(r.production.source)<<' '<<catalog.describe(category,item).displayName<<" M/ID/S/F "<<unsigned(item.material)<<'/'<<unsigned(item.id)<<'/'<<unsigned(item.state)<<'/'<<unsigned(item.frame)<<' ';
   if(r.recipient) text<<"to "<<_party.roster.at(*r.recipient).name;else text<<"lost: all packs full";text<<'\n';
  }
  text<<"Gold +"<<delivery.treasure.pendingGold<<" on final acknowledgment.";
  auto receiptText=text.str();_journeyPreimage->check();
  _rewardLease=_boundary.hold(XeenCombatBoundary::Work::Reward);_world._sessionState._journeyActivity=XeenJourneyActivity::Reward;
  // Keep the already-saveable staged treasure intact until final acknowledgment.
  // Only the next item's inventory prefix precedes its blank hold.
  delivery.characters=activeCharacters(_party,_party.party.activeOrder());
  delivery.phase=delivery.globallyFull?XeenMonsterDeliveryCandidate::Phase::Warning:XeenMonsterDeliveryCandidate::Phase::Final;
  if(!delivery.globallyFull)nextMonsterRewardItem(delivery);
  _monsterReceipt.emplace(std::move(delivery));_monsterReceiptText.swap(receiptText);
  for(const auto &v:_monsterReceipt->characters) {auto &c=_party.roster.at(v.rosterId);c.weapons=v.weapons;c.armor=v.armor;c.miscellaneous=v.miscellaneous;}
  ++_world._sessionState._journeyGeneration;++_generation;retainJourney();return true;
 } catch(...) { closeJourney();throw; }
}
void XeenEncounterFlow::acknowledgeMonsterReward() {
 if(!monsterReward() || _monsterReceipt->phase!=XeenMonsterDeliveryCandidate::Phase::Final || _busy || !current(ticket()) || !journeyCapacity()) throw std::logic_error("Stale monster receipt");
 ConsequenceScope busy(_busy);
 try {
  const auto credited=xeenPrepareMonsterGoldCredit(_monsterReceipt->treasure);_journeyPreimage->check();
  _party.monsterTreasure=credited;_monsterReceipt.reset();
  _world._sessionState._journeyActivity=_shoot?XeenJourneyActivity::Shoot:XeenJourneyActivity::Presentation;
  _boundary.release(XeenCombatBoundary::Work::Reward,_rewardLease);_rewardLease=0;
  ++_world._sessionState._journeyGeneration;++_generation;retainJourney();
 } catch(...) { closeJourney();throw; }
}
void XeenEncounterFlow::advanceMonsterReward() {
 if(!monsterReward() || _busy || !current(ticket()) || !journeyCapacity() ||
  _monsterReceipt->phase==XeenMonsterDeliveryCandidate::Phase::Final)throw std::logic_error("Stale monster treasure phase");
 ConsequenceScope busy(_busy);
 try {
  _journeyPreimage->check();auto &receipt=*_monsterReceipt;using Phase=XeenMonsterDeliveryCandidate::Phase;
  if(receipt.phase==Phase::Warning)receipt.phase=Phase::Final;
  else if(receipt.phase==Phase::Blank) {
   const auto &entry=receipt.records[receipt.cursor];
   if(!entry.recipient)throw std::logic_error("Monster found line has no delivered recipient");
   for(auto &owner:receipt.characters)if(owner.rosterId==*entry.recipient) {
    const auto category=entry.miscellaneous?XeenRewardCategory::Miscellaneous:entry.armor?XeenRewardCategory::Armor:XeenRewardCategory::Weapons;
    xeenCompactItems(xeenRewardItems(owner,category));
   }
   receipt.phase=Phase::Found;
  } else {++receipt.cursor;nextMonsterRewardItem(receipt);}
  for(const auto &v:receipt.characters) {auto &c=_party.roster.at(v.rosterId);c.weapons=v.weapons;c.armor=v.armor;c.miscellaneous=v.miscellaneous;}
  ++_world._sessionState._journeyGeneration;++_generation;retainJourney();
 }catch(...) {closeJourney();throw;}
}
void XeenEncounterFlow::observeRanged(std::shared_ptr<const XeenRegionalObservation> observation,const XeenWorld *environment,const XeenCamera *position) {
 const auto &world=environment?*environment:_world;const auto &camera=position?*position:_camera;
 if(observation->stage==XeenRegionalObservation::Stage::Portrait) {
  if(observation->impactSource && observation->impactOwner) {
   const auto &a=world.sessionState().regionalActors(observation->impactSource->mapId).at(observation->impactSource->recordIndex);
   _world.scenePresentation().portraitDamage(*observation->impactOwner,xeenPortraitDamageFrame(a.statistics->raw[29]),_lastTime);
  }
  _rangedObservation=std::move(observation);return;
 }
 if(observation->stage==XeenRegionalObservation::Stage::Published) {_rangedObservation=std::move(observation);return;}
 std::vector<XeenProjectileAppearance> prepared;
 unsigned pow=12;
 // Original monstersAttack chooses the first non-physical projectile resource
 // for the simultaneous volley; this lookup admits no new damage mechanics.
 for(unsigned i=0;i<observation->count;++i) {
  const auto &shot=observation->shots[i];
  const auto &actor=world.sessionState().regionalActors(shot.source.mapId).at(shot.source.recordIndex);
  const auto type=actor.statistics->raw[29];pow=xeenMonsterProjectile(type);
  if(pow!=12)break;
 }
 for(unsigned i=0;i<observation->count;++i) {
  const auto &shot=observation->shots[i];
  const auto &actor=world.sessionState().regionalActors(shot.source.mapId).at(shot.source.recordIndex);
  if(shot.direction==camera.direction && shot.distance>=1 && shot.distance<=3 && prepared.size()<6)
   prepared.push_back({true,shot.distance-1,unsigned(prepared.size()%6),shot.distance,shot.source,pow});
 }
 _rangedObservation=std::move(observation);_projectiles.swap(prepared);_projectileDeadline=0;
}
void XeenEncounterFlow::projectilesPresented() {
 const auto entry=ticket();if(!current(entry))throw std::logic_error("Stale projectile acquisition");
 if(projectilesPending() && !_projectileDeadline)_projectileDeadline=counterDeadline(_lastTime,2);
 if(_shoot && !_shoot->presented) {
  auto &shot=*_shoot;shot.presented=true;shot.advanceDraw=false;shot.deadline=counterDeadline(_lastTime,2);
  // drawScene/animate3d advances all remaining lanes, including the pre-HP,
  // post-HP and lethal-removal draws, independently of logical shooter flags.
  for(const auto &p:_projectiles)shot.advanceDraw=shot.advanceDraw || p.active;
 }
 if(_combat) {_combat->rangedPresented(*entry.combat,!projectilesPending());return;}
 if(_regionalWork && _regionalWork->opportunity) {
  _journeyPreimage->check();auto &op=*_regionalWork->opportunity;
  if(op.travelStarted && !projectilesPending())op.travelPresented=true;
  if(op.impactOwner)op.impactPresented=true;
 }
}
bool XeenEncounterFlow::animateProjectiles() {
 if(_busy || !projectilesPending() || !current(ticket())) return false;
 if(!_projectileDeadline)return false;
 std::uint64_t now;if(!prepareTime(ticket(),now)) return false;
 if(now<_projectileDeadline) return false;
 _lastTime=now;_projectileDeadline=0;
 for(auto &p:_projectiles)p.advance();
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
   if(!_combatCastRefusal.empty())out<<_combatCastRefusal<<'\n';
  if(!stopped && _combat->phase()==XeenCombatPhase::PlayerReady) out<<_party.roster.at(_party.party.activeRosterIds()[_combat->participant()]).name<<(": A/B; C Cast; R Run; 1-3 target\n");
  else if(!stopped)out<<"Automatic combat / End\n";
  const auto rows=_combat->contacts();
  for(unsigned i=0;i<rows.size();++i)if(rows[i]) {const auto &a=_world.sessionState().regionalActors(rows[i]->mapId).at(rows[i]->recordIndex);out<<(rows[i]==_combat->selectedTarget()?">":"")<<i+1<<' '<<a.statistics->name()<<" #"<<a.id.recordIndex<<" HP"<<a.hp<<'\n';}
  const auto &r=_combatObservation;
  if(r.operation==XeenCombatOperation::PlayerRun && r.runRoll)out<<_party.roster.at(*r.actingOwner).name<<(r.runSuccess?" escaped":" Run failed")<<" ("<<r.runRoll<<")\n";
  else if(r.monsterDrop)out<<"Orc "<<unsigned(r.generatedItem.source)<<" +10 gold pending; "<<(*r.monsterDrop==XeenMonsterDropOutcome::CategoryCapacityLoss?"full category: lost":*r.monsterDrop==XeenMonsterDropOutcome::Item?"item produced":"no item")<<'\n';
  else if(r.injuryCount)out<<"Damage "<<r.damage<<"; items changed "<<r.armorCount<<'\n';
  else if(!r.monsterDrop && r.targetMonster && r.attackOutcome!=XeenCombatAttackOutcome::NotApplicable)out<<"Actor "<<r.targetMonster->recordIndex<<(r.attackOutcome==XeenCombatAttackOutcome::Miss?" missed":" damage ")<<r.damage<<'\n';

 } else if(!stopped) {
  const bool city=!const_cast<XeenWorld &>(_world).map(_camera.mapId).geometry.isOutdoors();
  out<<"Arrows move/turn; . Wait; S Shoot:";
  bool eligible=false;
  for(unsigned i=0;i<6;++i){const auto &c=_party.roster.at(_party.party.activeRosterIds()[i]);if(c.canAct())for(const auto &w:c.weapons)if(w.frame==4){out<<' '<<i+1;eligible=true;break;}}
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
  if(_rangedObservation && _rangedObservation->stage==XeenRegionalObservation::Stage::Published && _rangedObservation->count) {const auto &v=_rangedObservation->shots[(_lastTime/500)%_rangedObservation->count];out<<"Enemy shot #"<<v.source.recordIndex<<" from "<<"NESW"[unsigned(v.direction)]<<" damage "<<v.attack.damage<<'\n';}
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
 const bool finishNotice = _disengagementNoticeRevision &&
  _retiredCombatResult.operation==XeenCombatOperation::FinishDisengagement &&
  (_combat ? _disengagementNoticeCombat && _combat->current(*_disengagementNoticeCombat) :
   *_disengagementNoticeRevision==_state.revision());
 if(!stopped && finishNotice) {
  out<<"Disengaged; gold forfeited "<<_retiredCombatResult.forfeitedGold<<'\n';
  if(_retiredCombatResult.casualties) {
   out<<"Abandoned:";for(unsigned i=0;i<6;++i)if(_retiredCombatResult.casualties&(1u<<i))out<<' '<<_party.roster.at(_party.party.activeRosterIds()[i]).name;
   out<<'\n';
  }
  if(_combat && _party.monsterTreasure->dormant())out<<"Dormant items; no gold owed\n";
  if(_combat && _party.monsterTreasure->pending())out<<"Ready treasure: +"<<_party.monsterTreasure->pendingGold<<" gold\n";
 }
 auto text=out.str();if(!text.empty()&&text.back()=='\n')text.pop_back();text+="\n\n";
 unsigned participantSlot=0;
 for(auto owner:_party.party.activeRosterIds()) {
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
