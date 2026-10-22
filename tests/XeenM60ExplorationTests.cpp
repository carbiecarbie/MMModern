#include "XeenTestInstallation.h"
#include "formats/xeen/XeenVoc.h"
#include "XeenTrainingTestSupport.h"
#include "XeenM42Evidence.h"
#include "XeenMonsterTestAccess.h"
#include "games/xeen/XeenJumpRules.h"
#include "games/xeen/XeenEventTrigger.h"
#include <iostream>
#include <sstream>
using namespace training_test;
namespace {
void tick(Fixture &f) {
 f.now+=125;f.flow->beginCycle(++f.cycle);
 if(auto frame=f.flow->updatePresentation())f.present(*frame);
}
void quiet(Fixture &f) {
 for(unsigned n=0;n<1000 && !f.flow->canSave();++n) {
  const auto input=f.flow->inputContext(f.flow->frame().presentation());
  if(f.flow->presentationGeneration() || f.w.sessionState().journeyActivity()==XeenJourneyActivity::Reward || (input.dialog && input.dialog->anyKey))f.act(AcknowledgeAction{});else tick(f);
 }
 if(!f.flow->canSave())throw std::runtime_error("M60 exploration did not settle activity="+std::to_string(unsigned(f.w.sessionState().journeyActivity()))+" projectiles="+std::to_string(f.flow->encounter()->appearance().projectiles.size())+" input="+std::to_string(f.flow->journeyInputCurrent(f.flow->displayedInput()))+" frame="+std::to_string(f.flow->acceptsInputFrame(f.flow->frame().presentation()))+" committed="+std::to_string(f.flow->encounter()->castingCommitted()));
}
XeenSaveSnapshot source(Inputs &in,unsigned spell,bool indoor) {
 auto s=in.service();s.camera=indoor?XeenCamera{28,18,4,XeenDirection::West}:XeenCamera{23,9,11,XeenDirection::East};
 for(auto *actors:{&s.map(23).monsters->slots,&s.map(28).monsters->slots})for(auto &a:*actors) {
  a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
 }
 save_test::clearExplored(s);for(auto &c:s.characters)c.skills[4]=0;
 auto &caster=s.characters[s.activeRosterIds[4]];
 caster.characterClass=spell==33?XeenCharacterClass::Cleric:spell==17?XeenCharacterClass::Sorcerer:XeenCharacterClass::Druid;caster.hasSpells=true;caster.currentSp=100;
 if(!XeenLearnedSpellRules::slotForSpell(*XeenLearnedSpellRules::categoryForClass(caster.characterClass),spell))
  caster.characterClass=XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Wizardry,spell)?XeenCharacterClass::Sorcerer:XeenCharacterClass::Cleric;
 for(auto *items:{&caster.weapons,&caster.armor,&caster.accessories})for(auto &item:*items)item.frame=0;
 caster.learnedSpells.emplace();caster.currentSpell=*XeenLearnedSpellRules::slotForSpell(*XeenLearnedSpellRules::categoryForClass(caster.characterClass),spell);
 caster.learnedSpells->at(caster.currentSpell)=1;
 s.journey->purse->gems=100;s.journey->context->ctr24=23;
 return s;
}
void begin(Fixture &f) {
 f.act(CastSpellAction{});f.act(SelectMemberAction{4});
 check(!f.flow->canSave(),"Cast selector permits saving");
 f.act(DialogKeyAction{'c'});
}
void effects(Inputs &in) {
 for(bool indoor:{false,true})for(unsigned id:{10u,59u,75u,20u,27u,45u,50u,60u,2u,17u,33u,38u,61u,62u,72u,73u}) {
  auto s=source(in,id,indoor);auto &target=s.characters[s.activeRosterIds[0]];
  target.currentHp=1;target.conditions[2]=3;
  const auto cost=XeenLearnedSpellRules::cost(id,s.characters[s.activeRosterIds[4]].currentLevel());
  Fixture f(in,s);const auto random=f.w.sessionState().journeyRandom();const auto minute=f.p.encounterContext->minutes;
  XeenCombatRandom expected(*random);if(id==20)while(!expected.draw(2,6)){}
  begin(f);check(f.flow->encounter()->castingCommitted(),"M60 spell failed paid admission");
  if(id==10 || id==59)f.act(SelectMemberAction{0});
  quiet(f);
  check(f.p.roster.at(s.activeRosterIds[4]).currentSp==100-int(cost.sp),"M60 SP debit differs");
  check(f.p.purse->gems==100-cost.gems,"M60 gem debit differs");
  check(f.p.encounterContext->minutes==minute+(indoor?1:10) && f.p.encounterContext->ctr24==23,"Cast time/counter differs");
  check(f.w.sessionState().journeyRandom()==expected.continuation(),"Empty/healing spell preparation RNG differs");
  check(f.w.sessionState().exploredTiles().empty(),"Eye/cast without Cartography wrote explored bits");
  if(id==10)check(f.p.roster.at(s.activeRosterIds[0]).currentHp==16,"Cure Wounds did not heal 15");
  if(id==59)check(f.p.roster.at(s.activeRosterIds[0]).conditions[2]==0,"Revitalize retained Weak");
  if(id==75) {
   check(f.p.encounterContext->effects[XeenPartyEffect::WizardEye]==1 && f.p.encounterContext->effects[XeenPartyEffect::AutomapOn]==0,"Eye flags differ");
   f.act(ToggleAutomapAction{});quiet(f);check(f.p.encounterContext->effects[XeenPartyEffect::WizardEye]==1,"Automap cleared Eye");
  }
  const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
  check(saved==XeenSaveFormat::encode(restored.snapshot()),"M60 generic restore replayed cast or changed state");
 }
}
void blockedProjectiles(Inputs &in) {
 XeenWorld query(in.mapLoader());std::optional<XeenCamera> blocked;
 for(int y=0;y<16 && !blocked;++y)for(int x=0;x<16 && !blocked;++x)for(unsigned d=0;d<4 && !blocked;++d) {
  XeenCamera c{28,x,y,XeenDirection(d)};if(xeenPlayerRayRows(query,c)==1)blocked=c;
 }
 check(bool(blocked),"Original blocked projectile fixture missing");
 for(unsigned id:{20u,27u,45u,50u,60u,2u,17u,33u,38u,61u,62u,72u,73u}) {
  auto s=source(in,id,true);s.camera=*blocked;Fixture f(in,s);const auto random=f.w.sessionState().journeyRandom();
  XeenCombatRandom expected(*random);if(id==20)while(!expected.draw(2,6)){}
  const auto cost=XeenLearnedSpellRules::cost(id,f.p.roster.at(s.activeRosterIds[4]).currentLevel());begin(f);quiet(f);
  check(f.p.roster.at(s.activeRosterIds[4]).currentSp==100-int(cost.sp) && f.p.purse->gems==100-cost.gems &&
   f.p.encounterContext->minutes==s.journey->context->minutes+1 && f.w.sessionState().journeyRandom()==expected.continuation(),
   "Blocked projectile cost/time/draw semantics differ");
 }
}
void eyeExpiry(Inputs &in) {
 for(bool dawn:{false,true}) {
  auto s=source(in,75,true);if(dawn){s.journey->context->minutes=299;s.journey->context->newDay=true;s.journey->context->rested=true;}
  Fixture f(in,s);f.flow->loadRestDream=[&]{return in.assets.restDreamImage();};f.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){return xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,archive));};begin(f);quiet(f);
  if(!dawn) {check(f.p.encounterContext->effects[XeenPartyEffect::WizardEye]==1,"Eye missing before Rest");f.act(RestAction{});f.act(DialogKeyAction{'y'});quiet(f);}
  check(f.p.encounterContext->effects[XeenPartyEffect::WizardEye]==0 && f.w.sessionState().exploredTiles().empty(),"resetTemps retained Eye or wrote reveal bits");
 }
}
void refunds(Inputs &in) {
 for(unsigned id:{10u,59u}) {
  auto s=source(in,id,true);Fixture f(in,s);begin(f);f.act(CancelInteractionAction{});quiet(f);
  check(f.p.roster.at(s.activeRosterIds[4]).currentSp==100 && f.p.purse->gems==100,"Member target Escape refund differs");
  check(f.p.encounterContext->minutes==s.journey->context->minutes+1,"Refund lost paid cast time");
 }
}
void jumpEvent(Inputs &in) {
 auto s=source(in,40,false);s.camera={23,5,7,XeenDirection::North};
 s.characters[s.activeRosterIds[4]].skills[4]=1;
 Fixture f(in,s);unsigned signRuns=0;
 // The automatic sign runs through the detached candidate, which reports manual results.
 f.flow->reportManual=[&](const XeenManualEventResult &result) {
  if(const auto *v=std::get_if<XeenEventExecutionSuspended>(&result))if(v->request.kind==XeenPresentationKind::SceneLabelSign)++signRuns;
 };
 begin(f);check(f.c.x==5 && f.c.y==9,"Jump did not reach original sign cell");
 unsigned presentations=0;
 for(unsigned n=0;n<1000 && !f.flow->canSave();++n) {
  if(f.flow->presentationGeneration()) {++presentations;f.act(AcknowledgeAction{});}else tick(f);
 }
 check(f.flow->canSave() && signRuns==1 && presentations==0,"Jump sign Event did not execute exactly once");
 check(f.p.encounterContext->ctr24==23,"Jump sign arrival incremented ctr24");
 const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
 check(saved==XeenSaveFormat::encode(restored.snapshot()),"Jump arrival replayed on load");
}
void impacts(Inputs &in) {
 for(unsigned spell:{20u,27u,45u,46u,50u,60u})for(unsigned distance:{1u,2u,3u}) {
  auto s=source(in,spell,true);s.camera={28,15,0,XeenDirection::North};
  const bool group=spell==46 || spell==50 || spell==60;const unsigned count=group?3:1;
  for(unsigned i=0;i<count;++i) {
   auto &a=s.map(28).monsters->slots.at(35+i);a.x=15;a.y=distance;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;
  }
  Fixture f(in,s);
  // Disclosed live wounds on original Vertigo monsters; current HP drives Mass Distortion.
  if(spell==46)for(unsigned i=0;i<count;++i)XeenMonsterTestAccess::setLive(*f.flow,f.w,28,35+i,1,true);
  XeenCombatRandom expected(*f.w.sessionState().journeyRandom());
  const auto &caster=f.p.roster.at(s.activeRosterIds[4]);
  XeenSpellDamagePreparation preparation(*XeenLearnedSpellRules::supported(spell),caster.currentLevel());
  XeenConsequenceDraw preparationDraw{expected};check(preparation.service(preparationDraw),"Spell preparation did not settle");
  auto treasure=f.w.treasureStaging().value();unsigned xp=0;
  for(unsigned i=0;i<count;++i) {
   const auto &actor=f.w.sessionState().regionalActors(28).at(35+i);
   XeenSpellDamageCandidate candidate(caster.permanentLevel,caster.temporaryLevel,actor.statistics->magicResistance(),actor.original.resourceId,
    *XeenLearnedSpellRules::supported(spell),actor.statistics->physicalResistance(),actor.status,actor.statistics->monsterType(),&preparation,actor.hp);
   XeenConsequenceDraw draw{expected};check(candidate.service(draw) && candidate.damage>=1,"Live spell fixture must be lethal");
   xp+=actor.statistics->experience()/6;
   {
    // Checks the record-driven drop draws and RNG continuation after an
    // exploration spell kill, with an independent chance table and generator.
    constexpr unsigned chance[]{0,10,20,50,100,100,100};
    const unsigned field=actor.statistics->raw[45];
    m42_test::StockOracle oracle{expected.state(),expected.position()};
    const bool generated=field && oracle.draw(1,100)<=chance[field];
    std::optional<std::pair<unsigned,XeenItem>> item;
    if(generated)item=oracle.item(field);
    XeenMonsterDropCandidate drop(treasure,*actor.statistics,actor.id.mapId);check(drop.service(draw),"Live spell bounded drop");
    check(expected.continuation()==XeenJourneyRandomState{1,oracle.state,oracle.count} &&
     (item?xeenSameItem(drop.generated,item->second):drop.outcome==XeenMonsterDropOutcome::None),"Independent spell/drop generation and RNG differ");
    treasure=drop.treasure;
   }
  }
  std::array<unsigned,6> priorXp{};for(unsigned i=0;i<6;++i)priorXp[i]=f.p.roster.combatInputs(s.activeRosterIds[i])->experience;
  const auto gold=f.p.purse->gold;begin(f);try{quiet(f);}catch(const std::exception &e){throw std::runtime_error("spell="+std::to_string(spell)+" distance="+std::to_string(distance)+" "+e.what());}
  for(unsigned i=0;i<count;++i)check(f.w.sessionState().regionalActors(28).at(35+i).lifecycle==XeenActorLifecycle::Defeated &&
   f.w.sessionState().accountedMonsters().count({28,35+i}),"Live spell death was not accounted");
  for(unsigned i=0;i<6;++i)check(f.p.roster.combatInputs(s.activeRosterIds[i])->experience==priorXp[i]+xp*(f.p.roster.at(s.activeRosterIds[i]).permanentLevel<15?2:1),"Live spell XP duplicated or lost");
  check(f.p.purse->gold==gold+treasure.gold,"Live spell reward duplicated or lost");
  check(f.w.sessionState().journeyRandom()==expected.continuation(),"Live spell impact/drop draw tape differs");
  const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
  check(saved==XeenSaveFormat::encode(restored.snapshot()),"Live spell restore replayed impacts/rewards");
 }
}
void m73Guild(Inputs &in) {
 for(unsigned spell:{38u,61u,62u,72u}) {
  auto s=source(in,spell,true);s.camera={28,20,13,XeenDirection::North};s.journey->purse->gold=20000;
  auto &c=s.characters[s.activeRosterIds[4]];c.awards[0]=1;c.learnedSpells->fill(1);c.learnedSpells->at(c.currentSpell)=0;
  Fixture guild(in,s);guild.flow->drawGuildArt=[&in](auto &image){in.assets.drawGuild(image);};
  guild.act(InteractionAction{});guild.prepare();guild.act(SelectMemberAction{4});
  guild.act(DialogKeyAction{'b'});guild.act(DialogKeyAction{'1'});guild.act(DialogKeyAction{'y'});
  check(XeenLearnedSpellRules::known(guild.p.roster.at(s.activeRosterIds[4]),c.currentSpell),"Vertigo Guild did not teach M73 spell");
  check(guild.p.purse->gold==20000-XeenLearnedSpellRules::purchasePrice(spell,c.characterClass),"Vertigo Guild M73 price differs");
  guild.act(CancelInteractionAction{});guild.act(CancelInteractionAction{});quiet(guild);
  auto learned=guild.snapshot();learned.camera={28,15,0,XeenDirection::North};
  const auto original=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,28),in.statistics);
  const auto bug=std::find_if(original.begin(),original.end(),[](const auto &a){return a.original.resourceId==2;});
  check(bug!=original.end(),"Vertigo has no original Doom Bug");
  auto &a=learned.map(28).monsters->slots.at(bug->id.recordIndex);a.x=15;a.y=3;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;
  Fixture fight(in,learned);XeenMonsterTestAccess::setLive(*fight.flow,fight.w,28,bug->id.recordIndex,1,false);
  const auto sp=fight.p.roster.at(s.activeRosterIds[4]).currentSp;const auto gems=fight.p.purse->gems;
  const auto minutes=fight.p.encounterContext->minutes;begin(fight);quiet(fight);
  const auto &after=fight.w.sessionState().monsters(28).at(bug->id.recordIndex);
  check(spell==61?(after.hp==1 && after.status==XeenActorStatus::Physical):after.lifecycle==XeenActorLifecycle::Defeated,"Purchased spell/Doom Bug effect differs");
  const auto cost=XeenLearnedSpellRules::cost(spell,c.currentLevel());
  check(fight.p.roster.at(s.activeRosterIds[4]).currentSp==sp-int(cost.sp) && fight.p.purse->gems==gems-cost.gems &&
   fight.p.encounterContext->minutes==minutes+1,"Purchased M73 spell costs differ");
 }
}

void m73Live(Inputs &in) {
 check(in.statistics.at(2).monsterType()==2 && in.statistics.at(6).monsterType()==3 &&
  in.statistics.at(8).monsterType()==4 && in.statistics.at(9).monsterType()==4,"Original Doom Bug/Orc/Skeleton/Zombie species differ");
 for(unsigned spell:{2u,17u,33u,38u,61u,62u,72u,73u,25u,0u,28u,36u,51u})for(bool item:{false,true}) {
  try {
  auto s=source(in,spell,false);
  const auto originals=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,23),in.statistics);
  std::vector<unsigned> slots{7,8,9};
  XeenWorld query(in.mapLoader());std::bitset<256> common;common.set();
  for(auto slot:slots){check(originals.at(slot).original.resourceId==6,"Expected original Orc slot");common &= xeenActorClosure(query.map(23),originals.at(slot));}
  std::optional<std::pair<int,int>> position;
  for(int y=0;y<16 && !position;++y)for(int x=0;x<13 && !position;++x)
   if(common[y*16+x] && common[y*16+x+3]) {
    XeenCamera camera{23,x+3,y,XeenDirection::West};
    if(xeenPlayerRayRows(query,camera)==4){position={{x,y}};s.camera=camera;}
   }
  check(bool(position),"Mainland has no reachable three-cell Orc ray");
  for(auto slot:slots){auto &saved=s.map(23).monsters->slots.at(slot);
   saved.x=position->first;saved.y=position->second;saved.lifecycle=XeenActorLifecycle::Present;saved.accounted=false;
  }
  check(slots.size()==3,"Mainland fixture needs three original Orc slots");
  if(spell==0)++s.map(23).monsters->slots.at(slots[0]).x;
  const unsigned count=XeenLearnedSpellRules::range(spell)==XeenSpellRange::Single?1:3;
  const unsigned power=spell==51?46:spell==25?35:spell==0?38:spell==28?33:spell==36?64:spell==2?17:spell==17?40:spell==33?31:spell==38?12:spell==61?7:spell==62?10:spell==72?13:19;
  if(item)s.characters[s.activeRosterIds[4]].miscellaneous[0]={1,std::uint8_t(power),2,0};
  Fixture f(in,s);
  for(auto slot:slots)XeenMonsterTestAccess::setLive(*f.flow,f.w,23,slot,1,true);
  const auto &caster=f.p.roster.at(s.activeRosterIds[4]);
  auto expected=XeenCombatRandom(*f.w.sessionState().journeyRandom());auto treasure=f.w.treasureStaging().value();
  std::array<XeenActorStatus,3> status{};std::array<int,3> hp{1,1,1};
  XeenSpellDamagePreparation preparation(*XeenLearnedSpellRules::supported(spell),caster.currentLevel());
  XeenConsequenceDraw preparationDraw{expected};check(preparation.service(preparationDraw),"Cast-wide preparation incomplete");
  XeenPartyVolley volley;volley.eligible.fill(true);
  for(unsigned i=0;i<count;++i) {
   const auto &a=f.w.sessionState().monsters(23).at(slots[i]);
   volley.nextTarget();
   do {
    if(spell==51 && !volley.select())break;
    const auto shooter=s.activeRosterIds[volley.shooter];
    XeenSpellDamageCandidate effect=spell==51 ?
     XeenSpellDamageCandidate(f.p.roster.at(shooter),*f.p.roster.combatInputs(shooter),*a.statistics,6,f.p.encounterContext->year,f.p.encounterContext->difficulty,a.status,true) :
     XeenSpellDamageCandidate(caster.permanentLevel,caster.temporaryLevel,a.statistics->magicResistance(),6,
      *XeenLearnedSpellRules::supported(spell),a.statistics->physicalResistance(),a.status,a.statistics->monsterType(),&preparation);
    XeenConsequenceDraw draw{expected};check(effect.service(draw),"Exploration expected cast did not finish");
    hp[i]=std::max(hp[i]-effect.damage,0);status[i]=effect.status;
    if(!hp[i]){XeenMonsterDropCandidate drop(treasure,*a.statistics,a.id.mapId);check(drop.service(draw),"Target settlement did not finish");treasure=drop.treasure;}
    if(spell!=51)break;
    volley.settle(!effect.miss);
   }while(hp[i]>0);
  }
  // The existing regional opportunity also accounts for ordinary Orc ranged
  // responses when the spell leaves them able to act; recovery stays Part A's rule.
  const auto live=f.w.sessionState().monsters(23);
  std::vector<XeenActor> anticipated(live.begin(),live.end());
  for(unsigned i=0;i<3;++i){auto &a=anticipated.at(slots[i]);a.hp=hp[i];a.status=status[i];
   if(!hp[i]){a.lifecycle=XeenActorLifecycle::Defeated;a.x=a.y=-128;a.activated=false;}}
  XeenConsequenceCharacters characters;XeenConsequenceInputs inputs;
  for(unsigned i=0;i<6;++i){const auto owner=s.activeRosterIds[i];characters[i]=f.p.roster.at(owner);inputs[i]=*f.p.roster.combatInputs(owner);}
  XeenRegionalOpportunityCandidate response(query.map(23),anticipated,s.camera,characters,inputs,f.p.encounterContext->year,63);
  for(unsigned n=0;n<100;++n){XeenConsequenceDraw draw{expected};if(response.service(draw))break;}
  for(unsigned i=0;i<3;++i)status[i]=response.actors.at(slots[i]).status;
  const auto minutes=f.p.encounterContext->minutes;const auto sp=caster.currentSp;const auto gems=f.p.purse->gems;
  if(item){f.act(SelectMemberAction{4});f.act(DialogKeyAction{'i'});f.act(DialogKeyAction{'m'});f.act(DialogKeyAction{'1'});f.act(DialogKeyAction{'u'});}
  else begin(f);
  quiet(f);
  for(unsigned i=0;i<3;++i){const auto &a=f.w.sessionState().monsters(23).at(slots[i]);check(a.hp==hp[i] && a.status==status[i],"M73 real exploration/item group effect differs");}
  if(f.w.sessionState().journeyRandom()!=expected.continuation())throw std::runtime_error("M73 target/drop/recovery order: spell="+std::to_string(spell)+" item="+std::to_string(item)+" actual="+std::to_string(f.w.sessionState().journeyRandom()->count)+" expected="+std::to_string(expected.position()));
  if(spell==2 || spell==33 || spell==61)check(std::any_of(status.begin(),status.end(),[](auto s){return s!=XeenActorStatus::Physical;}),"M73 real-cast fixture must retain a successful control at save");
  const auto cost=XeenLearnedSpellRules::cost(spell,caster.currentLevel());
  check(f.p.encounterContext->minutes==minutes+(item?0:10) && caster.currentSp==sp-(item?0:int(cost.sp)) &&
   f.p.purse->gems==gems-(item?0:cost.gems),"M73 exploration/item costs differ");
  if(item)check(caster.miscellaneous[0].state==1,"M73 item did not spend exactly one charge");
  const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
  check(saved==XeenSaveFormat::encode(restored.snapshot()),"M73 cast status restore replayed RNG/time/rewards");
  for(const auto &a:restored.w.sessionState().monsters(23))check(a.status==XeenActorStatus::Physical,"M73 real-cast status was not canonicalized");
  for(auto slot:slots)XeenMonsterTestAccess::status(*f.flow->encounter(),f.w,23,slot,XeenActorStatus::Physical);
  check(saved==XeenSaveFormat::encode(f.snapshot()),"Save with real-cast statuses differs from otherwise identical Physical actors");
  }catch(const std::exception &e){throw std::runtime_error("spell="+std::to_string(spell)+" item="+std::to_string(item)+": "+e.what());}
 }
}

void m73DeadDragonRecovery(Inputs &in) {
 // Retain the all-lifecycle recovery exemption independently of the live cast.
 auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,23),in.statistics);
 for(auto &a:actors){a.status=XeenActorStatus::Physical;a.lifecycle=XeenActorLifecycle::Defeated;a.x=a.y=-128;a.hp=0;}
 auto &a=actors.front();a.original.resourceId=68;a.statistics=in.statistics.at(68);a.status=XeenActorStatus::DragonSleep;
 XeenConsequenceCharacters chars;XeenConsequenceInputs inputs;
 auto base=source(in,61,false);for(unsigned i=0;i<6;++i){chars[i]=base.characters[base.activeRosterIds[i]];inputs[i]=base.journey->supplements[base.activeRosterIds[i]].inputs;}
 XeenWorld world(in.mapLoader());XeenRegionalOpportunityCandidate recovery(world.map(23),actors,{23,9,11,XeenDirection::East},chars,inputs,610,63);
 XeenCombatRandom none(std::vector<XeenCombatRandom::Draw>{});XeenConsequenceDraw empty{none};
 check(recovery.service(empty) && !none.position() && recovery.actors.front().status==XeenActorStatus::DragonSleep,
  "Dead Fire Dragon slot drew recovery");
}

void m73LiveDragon(Inputs &in) {
 // Synthetic MOB injects the actual Fire Dragon profile into an existing map;
 // this proves the production cast/response/damage path, not a dragon route.
 const auto dragon=in.statistics.at(68);check(dragon.monsterType()==6,"Fire Dragon must be Dragon species");
 auto objects=in.maps.loadObjects(in.assets,23);objects.entities.monsters.at(8).resourceId=68;
 auto s=source(in,17,false);s.camera={23,9,11,XeenDirection::East};
 auto &saved=s.map(23).monsters->slots.at(8);
 saved.x=12;saved.y=11;saved.lifecycle=XeenActorLifecycle::Present;saved.accounted=false;
 auto &caster=s.characters[s.activeRosterIds[4]];caster.miscellaneous[0]={1,2,2,0};caster.miscellaneous[1]={1,3,2,0};
 // Fixed original-data vector: R=0 skips both gates; Dragon Sleep and
 // Magic Arrow each draw one U(1,118) save. The Arrow save never halves.
 check(dragon.magicResistance()==0,"Fire Dragon resistance vector changed");
 s.journey->random=XeenCombatRandom(3).continuation();
 XeenCombatRandom expected(*s.journey->random);
 const auto control=expected.draw(1,118);check(control && *control>68,"Dragon Sleep fixture must succeed");
 const auto asleepRandom=expected.continuation();
 check(expected.draw(1,118).has_value(),"Dragon damage fixture save rejected");
 const auto awakeRandom=expected.continuation();
 XeenRegionalManifest synthetic=[](const auto &,const auto &,const auto &,const auto &){};
 Fixture f(in,s,false,nullptr,{},synthetic,{},nullptr,
  [&in,objects](auto id){return id==XeenMapIdentity(23)?objects:in.maps.loadObjects(in.assets,id);});
 const auto id=f.w.sessionState().monsters(23).at(8).id;
 const auto assertDragon=[&](XeenActorStatus status,int hp) {
  const auto &a=f.w.sessionState().monsters(23).at(8);
  check(a.id==id && a.original.resourceId==68 && a.statistics->raw==dragon.raw &&
   a.lifecycle==XeenActorLifecycle::Present && a.hp==hp && a.x==12 && a.y==11 && a.status==status,
   "Live Fire Dragon identity, HP, map position or status differs");
 };
 assertDragon(XeenActorStatus::Physical,dragon.baseHp());
 begin(f);quiet(f);assertDragon(XeenActorStatus::DragonSleep,dragon.baseHp());
 check(f.w.sessionState().journeyRandom()==asleepRandom,"Dragon Sleep cast/response RNG differs");
 const auto minutes=f.p.encounterContext->minutes;
 f.act(SelectMemberAction{4});f.act(DialogKeyAction{'i'});f.act(DialogKeyAction{'m'});
 f.act(DialogKeyAction{'1'});f.act(DialogKeyAction{'u'});quiet(f);
 assertDragon(XeenActorStatus::DragonSleep,dragon.baseHp());
 check(f.w.sessionState().journeyRandom()==asleepRandom && f.flow->encounter()->result().movementOpportunities==1,
  "Live Fire Dragon response drew recovery or repeated its opportunity");
 f.act(SelectMemberAction{4});f.act(DialogKeyAction{'i'});f.act(DialogKeyAction{'m'});
 f.act(DialogKeyAction{'2'});f.act(DialogKeyAction{'u'});
 // Observe the wake and HP publication before the now-awake dragon's response.
 bool woke=false,damaged=false;
 for(unsigned n=0;n<1000 && !damaged;++n) {
  const auto &a=f.w.sessionState().monsters(23).at(8);
  woke=woke || a.status==XeenActorStatus::Physical;
  damaged=a.hp==dragon.baseHp()-8;
  if(damaged)break;
  tick(f);
 }
 assertDragon(XeenActorStatus::Physical,dragon.baseHp()-8);
 check(woke && damaged && f.w.sessionState().journeyRandom()==awakeRandom &&
  f.p.roster.at(s.activeRosterIds[4]).miscellaneous[0].state==1 &&
  f.p.roster.at(s.activeRosterIds[4]).miscellaneous[1].state==1 && f.p.encounterContext->minutes==minutes,
  "Normal item damage did not wake the same live Fire Dragon exactly once");
}

void targets(Inputs &in) {
 auto s=source(in,60,false);Fixture f(in,s);
 const auto live=f.w.sessionState().monsters(23);std::vector<XeenActor> actors(live.begin(),live.end());
 for(unsigned i=0;i<4;++i) {
  auto &a=actors.at(i);a.lifecycle=XeenActorLifecycle::Present;a.hp=1;a.x=i<3?10:11;a.y=11;a.activated=true;
 }
 const auto group=xeenSpellTargets(f.w,f.c,actors,{},XeenSpellRange::Group);
 for(unsigned i=0;i<3;++i)check(group[i]==actors[i].id,"First reachable group traversal differs");
 const auto single=xeenSpellTargets(f.w,f.c,actors,{},XeenSpellRange::Single);
 check(single[0]==group[0] && !single[1] && !single[2],"Single target reaches extra monsters");
 for(unsigned i=0;i<3;++i)actors[i].x=9;
 const auto selected=xeenSpellTargets(f.w,f.c,actors,actors[2].id,XeenSpellRange::Group);
 check(selected[0]==actors[2].id && selected[1]==actors[0].id && selected[2]==actors[1].id,"Selected contact order differs");
 for(bool indoor:{false,true})for(unsigned rows:{2u,4u}) {
  std::optional<XeenCamera> origin;
  for(int y=0;y<13 && !origin;++y)for(int x=0;x<13 && !origin;++x) {
   XeenCamera camera{indoor?28:23,x,y,XeenDirection::East};
   if(xeenPlayerRayRows(f.w,camera)==rows)origin=camera;
  }
  check(bool(origin),"All range wall fixture missing");
  const unsigned width=indoor?2:3; // Preserve the pin's indoor slot-5/slot-2 quirk.
  for(unsigned i=0;i<12;++i) {
   auto &a=actors.at(i);a.lifecycle=i<width*4?XeenActorLifecycle::Present:XeenActorLifecycle::Defeated;a.hp=100;
   a.id.mapId=origin->mapId;
   a.x=i<width*4?int(origin->x)+int(i/width):-128;a.y=i<width*4?int(origin->y):-128;a.activated=true;
  }
  const auto all=xeenSpellTargets(f.w,*origin,actors,{},XeenSpellRange::All);
  for(unsigned i=0;i<12;++i)if(!(i<rows*width?all[i]==actors[i].id:!all[i]))throw std::runtime_error("All range indoor="+std::to_string(indoor)+" rows="+std::to_string(rows)+" slot="+std::to_string(i)+" actual="+(all[i]?std::to_string(all[i]->recordIndex):"none")+" camera="+std::to_string(origin->x)+","+std::to_string(origin->y));
 }
}
void m74Empty(Inputs &in) {
 for(unsigned spell:{0u,5u,11u,14u,18u,22u,23u,24u,25u,28u,29u,32u,35u,36u,37u,43u,46u,47u,48u,51u,54u,63u,65u}) {
  auto s=source(in,spell,false);const auto owner=s.activeRosterIds[4];s.characters[owner].currentSp=1000;
  for(auto id:s.activeRosterIds)s.characters[id].currentHp=1;
  Fixture f(in,s);XeenCombatRandom expected(*f.w.sessionState().journeyRandom());XeenConsequenceDraw draw{expected};
  XeenSpellDamagePreparation prep(*XeenLearnedSpellRules::supported(spell),f.p.roster.at(owner).currentLevel());check(prep.service(draw),"Empty preparation incomplete");
  XeenMoonRayHealing healing;if(spell==48)check(healing.service(draw,f.p,f.p.encounterContext->year),"Empty Moon Ray incomplete");
  const auto cost=XeenLearnedSpellRules::cost(spell,f.p.roster.at(owner).currentLevel());
  begin(f);quiet(f);
  check(f.p.roster.at(owner).currentSp==1000-int(cost.sp) && f.p.purse->gems==100-cost.gems && f.p.encounterContext->minutes==s.journey->context->minutes+10,"Empty M74 cast refunded cost or changed time");
  check(f.w.sessionState().journeyRandom()==expected.continuation(),"Empty M74 preparation/healing RNG differs");
  for(const auto &effect:healing.result.effects)check(f.p.roster.at(effect.owner).currentHp==effect.hp,"Moon Ray did not use ordinary healing");
 }
}
void jump(Inputs &in) {
 for(bool unsupported:{false,true}) {
  auto s=source(in,40,false);s.camera={23,unsupported?15:9,11,XeenDirection::East};
  const auto original=in.mapLoader();
  const XeenWorld::MapLoader loader=unsupported?XeenWorld::MapLoader{[original](auto id){auto m=original(id);if(id==XeenMapIdentity(23))m.geometry.neighbors[1]=0;return m;}}:XeenWorld::MapLoader{};
  Fixture f(in,s,false,nullptr,loader);
  const auto before=XeenSaveFormat::encode(f.snapshot());begin(f);
  if(unsupported) {
   check(!f.flow->encounter()->castingCommitted(),"Unsupported Jump charged");
   f.act(AcknowledgeAction{});f.act(CancelInteractionAction{});quiet(f);
   check(before==XeenSaveFormat::encode(f.snapshot()),"Unsupported Jump changed costs/time/RNG/position");
  } else {
   check(f.c.x==11 && f.c.y==11,"Jump did not relocate exactly two cells");quiet(f);
   check(f.c.x==11 && f.p.encounterContext->ctr24==23,"Jump relocated twice or advanced ctr24");
  }
 }
 auto s=source(in,40,true);s.camera={28,18,4,XeenDirection::South};
 // Choose an original wall-blocked direction rather than adding a map fixture.
 XeenWorld query(in.mapLoader());bool found=false;
 for(unsigned d=0;d<4;++d)if(xeenJumpPreflight(query,{28,18,4,XeenDirection(d)}).admission==XeenJumpAdmission::Blocked) {
  s.camera.direction=XeenDirection(d);found=true;break;
 }
 check(found,"Original blocked Jump fixture missing");Fixture blocked(in,s);begin(blocked);
 check(blocked.flow->encounter()->castingCommitted(),"Wall-blocked Jump must pay");
 blocked.act(AcknowledgeAction{});quiet(blocked);
 check(blocked.p.roster.at(s.activeRosterIds[4]).currentSp==96 && blocked.p.encounterContext->ctr24==23 &&
  blocked.c.x==s.camera.x && blocked.c.y==s.camera.y,"Blocked Jump costs/counter/position differ");
}
}
int main(int argc,char **argv) {try {
 check(argc==2,"usage: m60-exploration <installation>");auto install=xeenTestInstallationDetector().detect(argv[1]);check(bool(install),"Missing installation");Inputs in(*install);
 std::ostringstream captured;auto *old=std::cout.rdbuf(captured.rdbuf());
 try {effects(in);blockedProjectiles(in);eyeExpiry(in);refunds(in);jump(in);jumpEvent(in);targets(in);impacts(in);m74Empty(in);
  try{m73Live(in);}catch(const std::exception &e){throw std::runtime_error(std::string("M73 live: ")+e.what());}
  try{m73DeadDragonRecovery(in);m73LiveDragon(in);}catch(const std::exception &e){throw std::runtime_error(std::string("M73 Dragon: ")+e.what());}
  try{m73Guild(in);}catch(const std::exception &e){throw std::runtime_error(std::string("M73 Guild: ")+e.what());}}catch(...){std::cout.rdbuf(old);throw;}
 std::cout.rdbuf(old);std::cout<<"M60 original-data exploration effects, blocked/empty/live projectiles, Eye reset, refunds, Jump and exact restore passed\n";
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
