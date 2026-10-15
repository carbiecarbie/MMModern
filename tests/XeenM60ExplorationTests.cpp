#include "XeenTestInstallation.h"
#include "formats/xeen/XeenVocDuration.h"
#include "XeenTrainingTestSupport.h"
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
 caster.characterClass=XeenCharacterClass::Druid;caster.hasSpells=true;caster.currentSp=100;
 for(auto *items:{&caster.weapons,&caster.armor,&caster.accessories})for(auto &item:*items)item.frame=0;
 caster.learnedSpells.emplace();caster.currentSpell=*XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Druidic,spell);
 caster.learnedSpells->at(caster.currentSpell)=1;
 s.journey->treasure->gems=100;s.journey->context->ctr24=23;
 return s;
}
void begin(Fixture &f) {
 f.act(CastSpellAction{});f.act(SelectMemberAction{4});
 check(!f.flow->canSave(),"Cast selector permits saving");
 f.act(DialogKeyAction{'c'});
}
void effects(Inputs &in) {
 for(bool indoor:{false,true})for(unsigned id:{10u,59u,75u,20u,27u,45u,50u,60u}) {
  auto s=source(in,id,indoor);auto &target=s.characters[s.activeRosterIds[0]];
  target.currentHp=1;target.conditions[2]=3;
  const auto cost=XeenLearnedSpellRules::cost(id,s.characters[s.activeRosterIds[4]].currentLevel());
  Fixture f(in,s);const auto random=f.w.sessionState().journeyRandom();const auto minute=f.p.encounterContext->minutes;
  begin(f);check(f.flow->encounter()->castingCommitted(),"M60 spell failed paid admission");
  if(id==10 || id==59)f.act(SelectMemberAction{0});
  quiet(f);
  check(f.p.roster.at(s.activeRosterIds[4]).currentSp==100-int(cost.sp),"M60 SP debit differs");
  check(f.p.monsterTreasure->gems==100-cost.gems,"M60 gem debit differs");
  check(f.p.encounterContext->minutes==minute+(indoor?1:10) && f.p.encounterContext->ctr24==23,"Cast time/counter differs");
  check(f.w.sessionState().journeyRandom()==random,"Empty/healing spell consumed intrinsic RNG");
  check(f.w.sessionState().exploredTiles().empty(),"Eye/cast without Cartography wrote explored bits");
  if(id==10)check(f.p.roster.at(s.activeRosterIds[0]).currentHp==16,"Cure Wounds did not heal 15");
  if(id==59)check(f.p.roster.at(s.activeRosterIds[0]).conditions[2]==0,"Revitalize retained Weak");
  if(id==75) {
   check(f.p.encounterContext->effects[2]==1 && f.p.encounterContext->effects[1]==0,"Eye flags differ");
   f.act(ToggleAutomapAction{});quiet(f);check(f.p.encounterContext->effects[2]==1,"Automap cleared Eye");
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
 for(unsigned id:{20u,27u,45u,50u,60u}) {
  auto s=source(in,id,true);s.camera=*blocked;Fixture f(in,s);const auto random=f.w.sessionState().journeyRandom();
  const auto cost=XeenLearnedSpellRules::cost(id,f.p.roster.at(s.activeRosterIds[4]).currentLevel());begin(f);quiet(f);
  check(f.p.roster.at(s.activeRosterIds[4]).currentSp==100-int(cost.sp) && f.p.monsterTreasure->gems==100-cost.gems &&
   f.p.encounterContext->minutes==s.journey->context->minutes+1 && f.w.sessionState().journeyRandom()==random,
   "Blocked projectile cost/time/draw semantics differ");
 }
}
void eyeExpiry(Inputs &in) {
 for(bool dawn:{false,true}) {
  auto s=source(in,75,true);if(dawn){s.journey->context->minutes=299;s.journey->context->newDay=true;s.journey->context->rested=true;}
  Fixture f(in,s);f.flow->loadRestDream=[&]{return in.assets.restDreamImage();};f.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){return xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,archive));};begin(f);quiet(f);
  if(!dawn) {check(f.p.encounterContext->effects[2]==1,"Eye missing before Rest");f.act(RestAction{});f.act(DialogKeyAction{'y'});quiet(f);}
  check(f.p.encounterContext->effects[2]==0 && f.w.sessionState().exploredTiles().empty(),"resetTemps retained Eye or wrote reveal bits");
 }
}
void refunds(Inputs &in) {
 for(unsigned id:{10u,59u}) {
  auto s=source(in,id,true);Fixture f(in,s);begin(f);f.act(CancelInteractionAction{});quiet(f);
  check(f.p.roster.at(s.activeRosterIds[4]).currentSp==100 && f.p.monsterTreasure->gems==100,"Member target Escape refund differs");
  check(f.p.encounterContext->minutes==s.journey->context->minutes+1,"Refund lost paid cast time");
 }
}
void jumpEvent(Inputs &in) {
 auto s=source(in,40,false);s.camera={23,5,7,XeenDirection::North};
 s.characters[s.activeRosterIds[4]].skills[4]=1;
 Fixture f(in,s);unsigned signRuns=0;
 f.flow->reportAutomatic=[&](const XeenAutomaticEventResult &result) {
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
 for(unsigned spell:{20u,27u,45u,50u,60u})for(unsigned distance:{1u,2u,3u}) {
  auto s=source(in,spell,true);s.camera={28,15,0,XeenDirection::North};
  const bool group=spell==50 || spell==60;const unsigned count=group?3:1;
  for(unsigned i=0;i<count;++i) {
   auto &a=s.map(28).monsters->slots.at(35+i);a.x=15;a.y=distance;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;
  }
  Fixture f(in,s);XeenCombatRandom expected(*f.w.sessionState().journeyRandom());
  auto treasure=*f.p.monsterTreasure;unsigned xp=0;
  for(unsigned i=0;i<count;++i) {
   const auto &actor=f.w.sessionState().regionalActors(28).at(35+i);const auto &caster=f.p.roster.at(s.activeRosterIds[4]);
   XeenSpellDamageCandidate candidate(caster.permanentLevel,caster.temporaryLevel,actor.statistics->magicResistance(),actor.original.resourceId,
    *XeenLearnedSpellRules::supported(spell),actor.statistics->physicalResistance());
   XeenConsequenceDraw draw{expected};check(candidate.service(draw) && candidate.damage>=1,"Live spell fixture must be lethal");
   xp+=actor.statistics->experience()/6;
   if(actor.original.resourceId==6) {XeenMonsterDropCandidate drop(treasure,actor.id);check(drop.service(draw),"Live spell bounded drop");treasure=drop.treasure;}
  }
  std::array<unsigned,6> priorXp{};for(unsigned i=0;i<6;++i)priorXp[i]=f.p.roster.combatInputs(s.activeRosterIds[i])->experience;
  const auto gold=f.p.monsterTreasure->gold;begin(f);try{quiet(f);}catch(const std::exception &e){throw std::runtime_error("spell="+std::to_string(spell)+" distance="+std::to_string(distance)+" "+e.what());}
  for(unsigned i=0;i<count;++i)check(f.w.sessionState().regionalActors(28).at(35+i).lifecycle==XeenActorLifecycle::Defeated &&
   f.w.sessionState().accountedMonsters().count({28,35+i}),"Live spell death was not accounted");
  for(unsigned i=0;i<6;++i)check(f.p.roster.combatInputs(s.activeRosterIds[i])->experience==priorXp[i]+xp*(f.p.roster.at(s.activeRosterIds[i]).permanentLevel<15?2:1),"Live spell XP duplicated or lost");
  check(f.p.monsterTreasure->gold==gold+treasure.pendingGold,"Live spell reward duplicated or lost");
  check(f.w.sessionState().journeyRandom()==expected.continuation(),"Live spell impact/drop draw tape differs");
  const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
  check(saved==XeenSaveFormat::encode(restored.snapshot()),"Live spell restore replayed impacts/rewards");
 }
}
void targets(Inputs &in) {
 auto s=source(in,60,false);Fixture f(in,s);
 const auto live=f.w.sessionState().monsters(23);std::vector<XeenActor> actors(live.begin(),live.end());
 for(unsigned i=0;i<4;++i) {
  auto &a=actors.at(i);a.lifecycle=XeenActorLifecycle::Present;a.hp=1;a.x=i<3?10:11;a.y=11;a.activated=true;
 }
 const auto group=xeenSpellTargets(f.w,f.c,actors,{},true);
 for(unsigned i=0;i<3;++i)check(group[i]==actors[i].id,"First reachable group traversal differs");
 const auto single=xeenSpellTargets(f.w,f.c,actors,{},false);
 check(single[0]==group[0] && !single[1] && !single[2],"Single target reaches extra monsters");
 for(unsigned i=0;i<3;++i)actors[i].x=9;
 const auto selected=xeenSpellTargets(f.w,f.c,actors,actors[2].id,true);
 check(selected[0]==actors[2].id && selected[1]==actors[0].id && selected[2]==actors[1].id,"Selected contact order differs");
}
void jump(Inputs &in) {
 for(bool unsupported:{false,true}) {
  auto s=source(in,40,false);s.camera={23,unsupported?11:9,11,XeenDirection::East};Fixture f(in,s);
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
 try {effects(in);blockedProjectiles(in);eyeExpiry(in);refunds(in);jump(in);jumpEvent(in);targets(in);impacts(in);}catch(...){std::cout.rdbuf(old);throw;}
 std::cout.rdbuf(old);std::cout<<"M60 original-data exploration effects, blocked/empty/live projectiles, Eye reset, refunds, Jump and exact restore passed\n";
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
