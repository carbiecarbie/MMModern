#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "platform/sdl/XeenMainScreenInput.h"
#include <iostream>
using namespace training_test;
namespace mmodern {
struct XeenRestTestAccess {
 static bool active(const XeenEventFlow &f){return bool(f._encounter->_rest);}
 static bool complete(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Complete;}
 static bool confirm(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Confirm;}
 static bool refused(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Refused;}
 static unsigned dream(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Dream?f._encounter->_rest->dreamBeat+1:0;}
 static unsigned charges(const XeenEventFlow &f){return f._encounter->_rest->charges;}
 static void pending(XeenEventFlow &f,unsigned value){auto &e=*f._encounter;e._state._pending=value;f._encounterFrame=e.ticket();}
 static void scene(XeenEventFlow &f,const XeenGameplayContext &context) {
  f._encounterCompose=[&context](auto,auto) {
   auto image=training_test::frame();image.pixels[0]=(context.minutes<300 || context.minutes>=1260)?2:1;
   for(unsigned n=0;n<image.palette.size();++n)image.palette[n]=(n%64)<<2;
   return XeenEventFlow::Composition{image,false};
  };
 }
 static IndexedFrame background(const XeenEventFlow &f){return f._encounter->_rest->background;}
};
}
namespace {
void wire(Fixture &f){f.flow->loadRestDream=[&]{return f.in.assets.restDreamImage();};}
void pulse(Fixture &f){f.now+=100;f.flow->beginCycle(++f.cycle);if(const auto frame=f.flow->updatePresentation())f.present(*frame);}
void finish(Fixture &f) {
 for(unsigned n=0;n<1000;++n) {
  if(XeenRestTestAccess::confirm(*f.flow)) {f.act(DialogKeyAction{'y'});continue;}
  if(XeenRestTestAccess::complete(*f.flow)) {f.act(AcknowledgeAction{});continue;}
  if(f.flow->canSave())return;
  check(!f.flow->encounter()->combat(),"Successful Rest fixture entered combat");pulse(f);
 }
 throw std::runtime_error("Rest did not reach a quiet boundary: "+f.flow->encounter()->notice());
}
std::vector<XeenActor> actors(Inputs &in,const XeenSaveSnapshot &s) {
 auto result=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,s.camera.mapId),in.statistics);
 const auto &saved=s.camera.mapId==XeenMapIdentity(23)?s.journey->actors:*s.journey->vertigoActors;
 for(unsigned n=0;n<result.size();++n) {
  result[n].x=saved[n].x;result[n].y=saved[n].y;result[n].hp=saved[n].hp;
  result[n].activated=saved[n].activated;result[n].lifecycle=saved[n].lifecycle;
 }
 return result;
}
XeenSaveSnapshot source(Inputs &in,bool indoor,bool nearby=false,bool ranged=false,bool seam=false) {
 auto s=indoor?in.service():in.base();XeenWorld world(in.mapLoader(),in.objectLoader());
 const auto live=actors(in,s);const auto mainland=XeenMovement::component(world.map(23),9,11,{});
 for(int y=1;y<(indoor?31:15);++y)for(int x=1;x<(indoor?31:15);++x)for(unsigned d=0;d<4;++d) {
  if(!indoor && !mainland[y*16+x])continue;
  if(seam && x<16 && y<16)continue;
  const XeenCamera camera{indoor?28u:23u,x,y,XeenDirection(d)};
  const auto cell=world.sampleCell(camera.mapId,x,y);if(!cell || (cell->cell->rawAttributes&0x80))continue;
  if(indoor && cell->cell->surfaceIndex==4)continue;
  auto view=indoor?XeenIndoorScene().classifyActors(world,camera,live):XeenActorApproach::classify(live,camera);
  if(view.engaged())continue;
  bool close=false,ray=false;
  for(const auto &a:live)if(a.lifecycle==XeenActorLifecycle::Present) {
   if(std::abs(int(a.x)-x)<=3 && std::abs(int(a.y)-y)<=3)close=true;
   if(a.statistics->raw[32] && std::abs(int(a.x)-x)+std::abs(int(a.y)-y)>=2 &&
    std::abs(int(a.x)-x)<=3 && std::abs(int(a.y)-y)<=3 &&
    (indoor?xeenIndoorRangedRay(world,camera,a):xeenOutdoorRangedRay(world.map(23),camera,a)))ray=true;
  }
  if(nearby?!close || (ranged && !ray):close)continue;
  if(indoor && nearby && !ranged) {
   const auto moved=XeenActorApproach::move(live,camera,[&](const auto &a,int x,int y){return xeenIndoorActorTerrain(world,a,x,y);},true,{},XeenActorOpportunityContext{true});
   if(!XeenIndoorScene().classifyActors(world,camera,moved).engaged())continue;
  }
  s.camera=camera;auto &saved=indoor?*s.journey->vertigoActors:s.journey->actors;
  for(unsigned n=0;n<saved.size();++n)saved[n].activated=saved[n].activated || view.activation[n];
  std::cout<<"Rest fixture "<<camera.mapId<<' '<<x<<','<<y<<" facing "<<d<<" nearby="<<nearby<<" ranged="<<ranged<<'\n';
  return s;
 }
 throw std::runtime_error("No original Rest fixture");
}
void success(Inputs &in,bool indoor) {
 auto start=source(in,indoor);start.food=8;
 for(auto owner:start.activeRosterIds) {start.characters[owner].currentHp=1;start.characters[owner].currentSp=0;start.characters[owner].endurance.temporary=2;}
 Fixture key(in,start),mouse(in,start);wire(key);wire(mouse);
 key.act(RestAction{});const auto click=xeenMainScreenClick(290,80,MainScreen::Exploration);
 check(click && std::holds_alternative<RestAction>(*click),"Original Rest button path");mouse.act(*click);
 check(!key.flow->canSave() && key.flow->inputContext(key.flow->frame().presentation()).mainScreen==MainScreen::None,"Rest owns strict input and save boundary");
 const auto stale=key.flow->frame().presentation();const auto input=*key.flow->displayedInput();
 key.flow->handle(RestAction{},input,stale); // Acquired busy frame cannot restart Rest.
 finish(key);finish(mouse);save_test::sameSnapshot(key.snapshot(),mouse.snapshot());
 check(key.p.food==2 && key.p.encounterContext->minutes==start.journey->context->minutes+480 &&
  key.p.encounterContext->ctr24==start.journey->context->ctr24,"Original eight-hour Rest and six food, no stepTime");
 for(auto owner:start.activeRosterIds)check(!key.p.roster.at(owner).conditions[8] && key.p.roster.at(owner).currentHp==XeenCharacterRules::maxHp(key.p.roster.at(owner),{key.p.encounterContext->year}),"Original Rest wake/max HP");
 const auto saved=key.snapshot();Fixture loaded(in,saved);wire(loaded);save_test::sameSnapshot(saved,loaded.snapshot());
 key.act(RestAction{});loaded.act(RestAction{});finish(key);finish(loaded);
 check(key.p.food==0,"Partial-food exhaustion");save_test::sameSnapshot(key.snapshot(),loaded.snapshot());
 key.act(RestAction{});finish(key);check(key.p.food==0,"Repeated zero-food Rest permitted");
 const auto before=XeenSaveFormat::encode(key.snapshot());
 key.flow->handle(RestAction{},input,stale);check(XeenSaveFormat::encode(key.snapshot())==before,"Stale Rest input replays no state");
}
void entry(Inputs &in) {
 auto start=source(in,true);
 // Stat danger from temporary curse counters, preserving canonical birth year.
 start.characters[start.activeRosterIds[0]].birthYear=592;
 start.journey->supplements[start.activeRosterIds[0]].inputs.might.permanent=0;
 Fixture declined(in,start);wire(declined);const auto before=XeenSaveFormat::encode(declined.snapshot());
 declined.act(RestAction{});check(XeenRestTestAccess::confirm(*declined.flow),"Original SOME_CHARS_MAY_DIE confirmation");
 declined.act(DialogKeyAction{'n'});finish(declined);
 check(XeenSaveFormat::encode(declined.snapshot())==before,"Declined Rest preserves gameplay/time/RNG");
 auto valid=source(in,true,false,false,true);
 const auto restricted=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id==XeenMapIdentity(28))map.geometry.flags|=0x4000;
  if(id!=XeenMapIdentity(23))for(auto &cell:map.geometry.cells)cell.flags=0xf8;return map;};
 Fixture refusal(in,valid,false,nullptr,restricted);wire(refusal);const auto refusalBefore=XeenSaveFormat::encode(refusal.snapshot());
 refusal.act(RestAction{});check(XeenRestTestAccess::refused(*refusal.flow),"Primary Rest restriction");refusal.act(AcknowledgeAction{});finish(refusal);
 check(XeenSaveFormat::encode(refusal.snapshot())==refusalBefore,"Rest refusal preserves gameplay");
 const auto cellFlags=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id!=XeenMapIdentity(23))for(auto &cell:map.geometry.cells)cell.flags=0xf8;return map;};
 Fixture allowed(in,valid,false,nullptr,cellFlags);wire(allowed);allowed.act(RestAction{});finish(allowed);
 const auto neighborRestriction=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id==XeenMapIdentity(109) || id==XeenMapIdentity(110) || id==XeenMapIdentity(111))map.geometry.flags|=0x4000;return map;};
 Fixture neighbor(in,valid,false,nullptr,neighborRestriction);wire(neighbor);neighbor.act(RestAction{});finish(neighbor);
}
void interruption(Inputs &in,bool ranged,unsigned pending,bool indoor=false) {
 auto start=source(in,indoor,true,ranged);start.journey->random=XeenCombatRandom(ranged?17:23).continuation();
 check(start.camera.x==(indoor?6:ranged?3:2) && start.camera.y==(ranged?8:1) && start.camera.direction==XeenDirection::North,
  "Fixed interruption camera changed");
 for(auto owner:start.activeRosterIds)start.characters[owner].currentHp=1000;
 Fixture f(in,start);wire(f);XeenRestTestAccess::pending(*f.flow,pending);
 if(pending) {const auto context=f.flow->inputContext(f.flow->frame().presentation());
  check(context.restAvailable && !context.readyForAction,"Positive countdown must admit only Rest on its acquired frame");}
 f.act(RestAction{});bool contact=false;
 for(unsigned n=0;n<200 && XeenRestTestAccess::active(*f.flow);++n) {pulse(f);contact=contact || bool(f.flow->encounter()->combat());}
 check(!XeenRestTestAccess::active(*f.flow),"Nearby actors did not interrupt Rest");
 check(f.p.food==start.food && f.p.encounterContext->minutes<start.journey->context->minutes+100 &&
  !f.p.encounterContext->rested,"Interrupted Rest has only elapsed charges, no food/recovery/dream");
 bool asleep=false;for(auto owner:start.activeRosterIds)asleep=asleep || f.p.roster.at(owner).conditions[8];
 check(asleep,"Unhit interrupted members remain asleep");
 const unsigned interruptingCharge=ranged?1:2;
 check(f.p.encounterContext->minutes==480+interruptingCharge*(indoor?1:10) &&
  f.w.sessionState().journeyRandom()->count==(ranged?4:0),"Fixed interrupting charge/RNG count changed");
 check(contact==!ranged && (f.flow->encounter()->state().phase()==XeenEncounterPhase::Engaged)==!ranged,
  "Fixed wake reason must distinguish contact from ranged-only return");
 for(unsigned n=0;n<6;++n) {
  const auto &member=f.p.roster.at(start.activeRosterIds[n]);
  check(member.conditions[8]==(ranged && n<2?0:1),"Fixed awake/asleep member mask changed");
  check(member.currentHp==(ranged && n<2?int(n==0?993:994):1000),"Fixed interruption injuries changed");
 }
 XeenConsequenceCharacters observed;for(unsigned n=0;n<6;++n)observed[n]=f.p.roster.at(start.activeRosterIds[n]);
 check(xeenRestRangedWake(observed)==ranged,"Literal Depressed/Confused/Good ranged wake reason changed");
 const auto beforeActors=actors(in,start);const auto live=f.w.sessionState().regionalActors(f.c.mapId);
 for(unsigned n=0;n<live.size();++n) {
  int x=beforeActors[n].x,y=beforeActors[n].y;
  if(indoor) {if(n>=14 && n<=16){x=5;y=3;}if(n==31 || n==32){x=6;y=1;}}
  else if(ranged) {if(n==7 || n==8){x=3;y=9;}}
  else {if(n==12){x=4;y=0;}if(n==17){x=2;y=2;}if(n==18){x=2;y=1;}}
  check(live[n].x==x && live[n].y==y,"Fixed interruption actor positions changed");
 }
 if(ranged) {
  const auto &shots=*f.flow->encounter()->result().consequences;
  check(shots.count==2 && shots.shots[0].source.recordIndex==7 && shots.shots[1].source.recordIndex==8 &&
   shots.shots[0].attack.targetedMembers==1 && shots.shots[1].attack.targetedMembers==2,"Fixed ranged source/target order changed");
 }
 std::cout<<"Fixed interruption charge="<<interruptingCharge<<" reason="<<(ranged?"ranged-only":"contact")<<" passed\n";
 {
 struct QuietOutput {std::ostringstream output;std::streambuf *before;QuietOutput():before(std::cout.rdbuf(output.rdbuf())){}~QuietOutput(){std::cout.rdbuf(before);}} quiet;
 for(unsigned n=0;n<3000 && !f.flow->canSave();++n) {
  if(const auto combat=f.flow->encounter()->combat();combat && combat->phase()==XeenCombatPhase::PlayerReady)f.act(AttackAction{});
  else if(f.w.sessionState().journeyActivity()==XeenJourneyActivity::Reward)f.act(AcknowledgeAction{});
  else pulse(f);
  check(f.flow->encounter()->state().phase()!=XeenEncounterPhase::SupportStopped,"Interrupted Rest ordinary combat stopped");
 }
 }
 check(f.flow->canSave() && f.p.food==start.food && !XeenRestTestAccess::active(*f.flow),"Combat returns to ordinary quiet without resuming Rest");
 const auto aftermath=f.snapshot();Fixture loaded(in,aftermath);wire(loaded);save_test::sameSnapshot(aftermath,loaded.snapshot());
 bool stillAsleep=false;for(auto owner:start.activeRosterIds)stillAsleep=stillAsleep || loaded.p.roster.at(owner).conditions[8];
 check(stillAsleep,"Combat/save restoration cleared unhit members' persistent sleep");
 std::cout<<"Interrupted-rest aftermath asleep="<<stillAsleep<<" exact round-trip\n";
}
void reentrant(Inputs &in) {
 const auto start=source(in,true);Fixture f(in,start);unsigned calls=0;
 const auto input=f.flow->displayedInput();const auto frame=f.flow->frame().presentation();
 f.flow->loadRestDream=[&] {
  ++calls;f.flow->handle(RestAction{},input,frame);
  check(!f.p.roster.at(start.activeRosterIds[0]).conditions[8] && f.p.encounterContext==start.journey->context && f.p.food==start.food,
   "Fallible Rest preflight published sleep/time/food early");
  return in.assets.restDreamImage();
 };
 f.act(RestAction{});finish(f);check(calls==1 && f.p.food==start.food-6,"Reentrant Rest entry double-charged");
}
void combatOutcomes(Inputs &in,bool run) {
 auto start=source(in,false,true);start.journey->random=XeenCombatRandom(run?101:29).continuation();
 for(auto owner:start.activeRosterIds)start.characters[owner].currentHp=run?1000:1;
 Fixture f(in,start);wire(f);f.act(RestAction{});
 {
 struct QuietOutput {std::ostringstream output;std::streambuf *before;QuietOutput():before(std::cout.rdbuf(output.rdbuf())){}~QuietOutput(){std::cout.rdbuf(before);}} quiet;
 for(unsigned n=0;n<3000;++n) {
  const auto combat=f.flow->encounter()->combat();
  if(run && f.flow->canSave())break;
  if(!run && combat && combat->phase()==XeenCombatPhase::Defeat)break;
  if(combat && combat->phase()==XeenCombatPhase::PlayerReady)f.act(run?PlayerAction{RunAction{}}:PlayerAction{AttackAction{}});
  else if(f.w.sessionState().journeyActivity()==XeenJourneyActivity::Reward)f.act(AcknowledgeAction{});
  else pulse(f);
 }
 }
 check(!XeenRestTestAccess::active(*f.flow) && f.p.food==start.food && !f.p.encounterContext->rested,"Run/death cannot resume or complete interrupted Rest");
 if(run)check(f.flow->canSave() && f.c.x==10 && f.c.y==12,"Interrupted Rest uses original Run destination and quiet continuation");
 else check(f.flow->encounter()->combat() && f.flow->encounter()->combat()->phase()==XeenCombatPhase::Defeat && !f.flow->canSave(),"Interrupted Rest uses ordinary party-death handling");
 std::cout<<"Interrupted Rest "<<(run?"Run":"defeat")<<" passed\n";
}
void conditionDeath(Inputs &in) {
 auto start=source(in,true);start.journey->context->minutes=1439;
 for(auto owner:start.activeRosterIds)start.characters[owner].conditions[2]=254; // FF is -1 in conditionMod, not lethal weakness.
 Fixture f(in,start);wire(f);f.act(RestAction{});check(XeenRestTestAccess::confirm(*f.flow),"All-member stat danger confirmation missing");
 f.act(DialogKeyAction{'y'});
 for(unsigned n=0;n<500 && !XeenRestTestAccess::complete(*f.flow);++n)pulse(f);
 check(XeenRestTestAccess::complete(*f.flow) && f.p.encounterContext->minutes==479 && f.p.food==start.food,
  "Condition deaths must finish eight hours and the original completion, consuming no food for dead members");
 for(auto owner:start.activeRosterIds)check(f.p.roster.at(owner).conditions[13] && !f.p.roster.at(owner).conditions[8],"Condition-dead member recovery order");
 f.act(AcknowledgeAction{});check(f.flow->encounter()->state().reason()==XeenEncounterStop::Defeat && !f.flow->canSave(),"Rest's final party-death check missing");
}
void noTargets(Inputs &in) {
 auto start=source(in,false,true,true);start.journey->context->minutes=479;
 start.journey->random=XeenCombatRandom(17).continuation();
 for(auto owner:start.activeRosterIds)start.characters[owner].conditions[2]=254;
 Fixture f(in,start);wire(f);XeenRestTestAccess::pending(*f.flow,3);
 f.act(RestAction{});check(XeenRestTestAccess::confirm(*f.flow),"noTargets danger confirmation");f.act(DialogKeyAction{'y'});
 for(unsigned n=0;n<500 && XeenRestTestAccess::active(*f.flow);++n)pulse(f);
 const auto &result=f.flow->encounter()->result();
 check(!XeenRestTestAccess::active(*f.flow) && !f.flow->encounter()->combat() &&
  f.flow->encounter()->state().reason()==XeenEncounterStop::Defeat,"Ranged noTargets did not end Rest in ordinary defeat");
 check(result.consequences && result.consequences->count && !result.view.engaged(),"noTargets must be ranged-only, not contact");
 for(unsigned n=0;n<result.consequences->count;++n)
  check(!result.consequences->shots[n].attack.targetedMembers && !result.consequences->shots[n].attack.injuryCount,"noTargets unexpectedly selected/injured a member");
 check(f.p.food==start.food && f.p.encounterContext->minutes==489 && !f.p.encounterContext->rested && !f.flow->canSave(),"noTargets published recovery/completion or remaining hours");
 for(auto owner:start.activeRosterIds)check(f.p.roster.at(owner).conditions[13] && f.p.roster.at(owner).conditions[8],"noTargets cleared unhit sleep");
 check(f.w.sessionState().journeyRandom()->count==15,"noTargets fixed RNG count changed");
 const auto original=actors(in,start);const auto live=f.w.sessionState().regionalActors(23);
 for(unsigned n=0;n<live.size();++n)check(live[n].x==(n==7 || n==8?3:int(original[n].x)) &&
  live[n].y==(n==7 || n==8?9:int(original[n].y)),"noTargets fixed actor positions changed");
 std::cout<<"noTargets charge=1 draws=15 passed\n";
}
void sleepingHit(Inputs &in) {
 // Synthetic supported Sleep/ranged MON profile on a city-only species.
 // Restoration binds both actors and the city's immutable statistics to this
 // same fixture; every production geometry/identity/publication guard runs.
 auto start=in.service();start.camera={28,11,1,XeenDirection::South};
 start.journey->random=XeenCombatRandom(1).continuation();
 for(auto owner:start.activeRosterIds)start.characters[owner].currentHp=1000;
 start.characters[0].characterClass=XeenCharacterClass::Knight;
 start.characters[0].permanentLevel=1;start.journey->supplements[0].inputs.luck->permanent=0;
 for(unsigned n=0;n<start.journey->vertigoActors->size();++n) {
  auto &a=start.journey->vertigoActors->at(n);if(a.lifecycle!=XeenActorLifecycle::Present)continue;
  if(n==0){a.x=11;a.y=4;a.activated=false;}
  else {a.x=a.y=-128;a.hp=0;a.activated=false;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 }
 const auto previous=in.statistics[2];auto &monster=in.statistics[2];
 monster.raw[24]=1;monster.raw[25]=unsigned(start.characters[0].characterClass);monster.raw[26]=1;monster.raw[27]=0;
 monster.raw[28]=1;monster.raw[29]=0;monster.raw[30]=9;monster.raw[31]=5;monster.raw[32]=1;
 const auto loader=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);
  if(id!=XeenMapIdentity(23))for(auto &cell:map.geometry.cells){cell.rawWord=0;xeenGet<XeenIndoorWalls>(cell.geometry).walls.fill(0);}return map;};
 Fixture f(in,start,false,nullptr,loader);in.statistics[2]=previous;wire(f);XeenRestTestAccess::pending(*f.flow,3);
 f.act(RestAction{});if(XeenRestTestAccess::confirm(*f.flow))f.act(DialogKeyAction{'y'});
 for(unsigned n=0;n<500 && XeenRestTestAccess::active(*f.flow) && !XeenRestTestAccess::charges(*f.flow);++n)pulse(f);
 check(XeenRestTestAccess::active(*f.flow) && XeenRestTestAccess::charges(*f.flow)==1 && !f.flow->encounter()->combat(),"Sleeping ranged hit incorrectly ended Rest");
 check(f.p.roster.at(0).currentHp==999 && f.p.roster.at(0).worstCondition()==XeenCondition::Asleep &&
  f.p.encounterContext->minutes==481 && f.p.food==start.food,"Sleep ability must leave the hit member Asleep without recovery");
 const auto live=f.w.sessionState().regionalActors(28);
 check(live[0].x==11 && live[0].y==3 && f.w.sessionState().journeyRandom()->count==2,"Sleeping hit movement/RNG auto-hit oracle changed");
 for(auto owner:start.activeRosterIds)check(f.p.roster.at(owner).conditions[8],"Sleeping hit unexpectedly woke a member");
 for(unsigned n=0;n<500 && XeenRestTestAccess::active(*f.flow);++n)pulse(f);
 check(!XeenRestTestAccess::active(*f.flow) && f.flow->encounter()->combat() && f.p.encounterContext->minutes==483 &&
  f.p.food==start.food,"Rest must continue after sleepy hit until later contact");
 std::cout<<"Sleeping hit continues after charge 1; contact interrupts charge 3\n";
}
void terrain(Inputs &in) {
 const auto manifest=[&](unsigned surface) -> XeenRegionalManifest {
  // Explicit synthetic surface table, checked in addition to every original
  // manifest invariant. Restore only that fixture field in a detached copy
  // for the original validator; no production guard or source file changes.
  return [&,surface](const auto &m,const auto &o,const auto &e,const auto &s) {
   for(auto value:m.geometry.surfaceTypes)check(value==surface,"Synthetic terrain table changed");
   auto original=m;original.geometry.surfaceTypes=in.maps.loadGeometryMap(in.assets,23).geometry.surfaceTypes;
   xeenValidateRegionalManifest(original,o,e,s,in.assets.readInitialResource("maze0023.dat"),in.assets.readInitialResource("maze0023.mob"),in.assets.readInitialResource("maze0023.evt"));
  };
 };
 // Scan a superset of Vertigo's reachable cells, including all neighbor tiles,
 // and the mainland traversal component. Use the primary map's surface table.
 XeenWorld world(in.mapLoader(),in.objectLoader());
 const auto mainland=XeenMovement::component(world.map(23),9,11,{});
 for(bool indoor:{false,true}) {
  std::array<unsigned,16> count{};unsigned cells=0;
  const auto primary=indoor?28u:23u;
  for(int y=0;y<(indoor?32:16);++y)for(int x=0;x<(indoor?32:16);++x) {
   if(!indoor && !mainland[y*16+x])continue;
   const auto sampled=world.sampleCell(primary,x,y);if(!sampled)continue;
   ++cells;++count[world.map(primary).geometry.surfaceTypes[sampled->cell->surfaceIndex]];
  }
  std::cout<<"Surface audit map="<<primary<<" cells="<<cells;
  for(unsigned s:{5u,6u,10u,13u,15u}){std::cout<<" surface"<<s<<'='<<count[s];check(!count[s],"Original reachable hazardous Rest surface found");}
  std::cout<<'\n';
 }
 for(bool indoor:{false,true})for(unsigned surface:{5u,6u,10u,13u,15u}) {
  if(!indoor && surface==15)continue; // Space cannot restore an outdoor traversal anchor; covered indoors and by the rules oracle.
  auto start=source(in,indoor);if(surface==13)start.journey->context->effects[2]=1;
  const auto loader=[&](auto id) {
   auto map=in.maps.loadGeometryMap(in.assets,id);if(id==start.camera.mapId)map.geometry.surfaceTypes.fill(surface);return map;
  };
  Fixture f(in,start,false,nullptr,loader,indoor?XeenRegionalManifest{}:manifest(surface));wire(f);unsigned providers=0;
  f.flow->loadRestDream=[&]{++providers;return in.assets.restDreamImage();};
  const auto before=XeenSaveFormat::encode(f.snapshot());f.act(RestAction{});
  if(surface!=6) {
   check(XeenRestTestAccess::refused(*f.flow) && !providers,"Unsupported surface was not preflighted before dream/sleep");
   f.act(AcknowledgeAction{});finish(f);check(XeenSaveFormat::encode(f.snapshot())==before,"Unsupported terrain changed state/time/RNG");
  } else {
   finish(f);check(f.p.encounterContext->minutes==start.journey->context->minutes+480+(indoor?0:170),"Desert Rest addTime/indoor suffix");
  }
 }
 {
  // Synthetic immutable Navigator skill in the decoded CHR fixture only.
  // Commercial resources remain read-only, and all production owners/guards run.
  const auto start=source(in,false);const auto slot=start.activeRosterIds[0]*XeenCharacter::kSerializedSize+49;
  const auto previous=in.chr[slot];in.chr[slot]=1;
  const auto loader=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(id==start.camera.mapId)map.geometry.surfaceTypes.fill(6);return map;};
  Fixture f(in,start,false,nullptr,loader,manifest(6));in.chr[slot]=previous;wire(f);f.act(RestAction{});finish(f);
  check(f.p.encounterContext->minutes==start.journey->context->minutes+480,"Navigator did not suppress desert addTime");
 }
 const auto seam=source(in,true,false,false,true);
 for(bool primaryUnsupported:{false,true}) {
  const auto loader=[&](auto id) {auto map=in.maps.loadGeometryMap(in.assets,id);
   if(id!=XeenMapIdentity(23))map.geometry.surfaceTypes.fill((id==seam.camera.mapId)==primaryUnsupported?5:0);return map;};
  Fixture f(in,seam,false,nullptr,loader);wire(f);const auto before=XeenSaveFormat::encode(f.snapshot());f.act(RestAction{});
  check(XeenRestTestAccess::refused(*f.flow)==primaryUnsupported,"Rest surface table came from physical neighbor");
  if(primaryUnsupported){f.act(AcknowledgeAction{});finish(f);check(XeenSaveFormat::encode(f.snapshot())==before,"Primary terrain refusal mutated state");}
  else finish(f);
 }
}
void rolloverAndDream(Inputs &in) {
 auto start=source(in,true);start.food=3;start.journey->context->day=99;start.journey->context->minutes=1439;
 start.journey->context->effects[2]=1;start.journey->context->lightAndResistances[0]=4;
 start.characters[start.activeRosterIds[0]].conditions[2]=255;
 Fixture f(in,start);wire(f);f.act(RestAction{});finish(f);
 check(f.p.encounterContext->day==0 && f.p.encounterContext->year==611 && f.p.encounterContext->minutes==479 &&
  !f.p.encounterContext->newDay && f.p.roster.at(start.activeRosterIds[0]).conditions[2]==255,"Rest year rollover, sleeping dawn and provisional Weak sentinel");
 check(!f.p.roster.at(start.activeRosterIds[0]).conditions[13] && f.p.food==0,"FF sentinel boosts stats and remains food-eligible");
 const auto save=f.snapshot();Fixture loaded(in,save);wire(loaded);f.act(RestAction{});loaded.act(RestAction{});finish(f);finish(loaded);
 f.act(NavigationAction::TurnLeft);loaded.act(NavigationAction::TurnLeft);finish(f);finish(loaded);
 save_test::sameSnapshot(f.snapshot(),loaded.snapshot());
 auto dreamStart=source(in,true);dreamStart.journey->context->minutes=1000;
 constexpr unsigned chosen=4;
 {
  auto context=*dreamStart.journey->context;XeenConsequenceCharacters c;XeenConsequenceInputs inputs;
  for(unsigned n=0;n<6;++n) {c[n]=dreamStart.characters[kXeenCombatOwners[n]];c[n].conditions[8]=1;inputs[n]=dreamStart.journey->supplements[kXeenCombatOwners[n]].inputs;}
  XeenCombatRandom random(chosen);
  for(unsigned step=0;step<11;++step) {
   XeenConditionTimeCandidate time(context,step<10?1:470,c,inputs,&*dreamStart.journey->serviceEconomy,XeenTimeMode::Sleeping);
   for(unsigned n=0;;++n) {check(n<1000,"Dream oracle bounded");XeenConsequenceDraw draw{random,64,{}};if(time.service(draw))break;}
   context=time.context;c=time.characters;inputs=time.inputs;
  }
  check(random.draw(1,20)==1,"Fixed dream seed oracle differs");
 }
 check(chosen,"Fixed dream seed absent");dreamStart.journey->random=XeenCombatRandom(chosen).continuation();
 Fixture dream(in,dreamStart);wire(dream);XeenRestTestAccess::scene(*dream.flow,*dream.p.encounterContext);
 dream.act(RestAction{});unsigned beats=0,hidden=0;
 std::optional<XeenJourneyRandomState> cursor;unsigned minutes=0;std::optional<unsigned> overall;
 for(unsigned n=0;n<500 && !XeenRestTestAccess::complete(*dream.flow);++n) {
  if(XeenRestTestAccess::dream(*dream.flow)) {
   ++beats;hidden+=dream.flow->inputContext(dream.flow->frame().presentation()).hideCursor;
   if(!cursor){cursor=dream.w.sessionState().journeyRandom();minutes=dream.p.encounterContext->minutes;}
   if(!overall)overall=dream.w.scenePresentation().overallFrame;
   check(dream.w.scenePresentation().overallFrame==*overall,"Dream advanced ordinary scene animation");
   check(dream.w.sessionState().journeyRandom()==cursor && dream.p.encounterContext->minutes==minutes,"Dream consumes no gameplay RNG or time");
   const auto background=XeenRestTestAccess::background(*dream.flow);
   check(background.pixels[0]==2 && minutes==40,"Dream captured pre-changeTime day background instead of redrawn night");
   const unsigned beat=XeenRestTestAccess::dream(*dream.flow)-1;
   const unsigned val=beat<33?128-4*beat:beat<66?4*(beat-33):beat<80?128:beat<113?128-4*(beat-80):4*(beat-113);
   for(unsigned n=0;n<background.palette.size();++n)
    check(dream.flow->frame().palette[n]==(((n%64)<<2)*val*2>>8),"Dream fade differs from pin's p8 formula");
   dream.act(RestAction{});check(!dream.flow->canSave(),"Dream reentrant input escaped boundary");
  }
  pulse(dream);
 }
 check(beats==146 && hidden==80,"Four original 33-step fades, two seven-tick holds and cursor restoration");finish(dream);
 std::cout<<"Dream seed="<<chosen<<" 146 visual beats; audio waits deferred\n";
}
}
int main(int argc,char **argv) {try {
 check(argc==2,"Rest original usage: installation");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation absent");
 Inputs in(*installation);sleepingHit(in);terrain(in);success(in,false);success(in,true);entry(in);interruption(in,false,0);interruption(in,true,3);interruption(in,false,0,true);rolloverAndDream(in);reentrant(in);combatOutcomes(in,true);combatOutcomes(in,false);conditionDeath(in);noTargets(in);
 std::cout<<"Original-resource Rest, interruption, food, UI path and mid-sequence save tests passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
