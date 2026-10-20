#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenMapView.h"
#include "games/xeen/XeenEventTrigger.h"
#include "platform/sdl/XeenMainScreenInput.h"
#include <queue>
#include <map>
#include <sstream>
#include <iostream>
#include <tuple>
using namespace training_test;
namespace {
void quiet(Fixture &f) {
 for(unsigned n=0;n<2000 && !f.flow->canSave();++n) {
  if(f.flow->presentationGeneration())f.act(AcknowledgeAction{});
  else {f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);}
 }
 check(f.flow->canSave(),"Mapping action did not settle");
}
XeenSaveSnapshot clearedCity(Inputs &in) {
 auto source=in.service();source.camera={28,18,4,XeenDirection::West};save_test::clearExplored(source);
 // Synthetic safe route; every map, wall and Event remains original data.
 for(auto &a:source.map(28).monsters->slots)if(a.lifecycle==XeenActorLifecycle::Present) {
  a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
 }
 return source;
}
void configure(Fixture &f) {f.flow->mapName=[&](auto id){return f.in.assets.mapName(id);};}
void modal(Fixture &f) {
 configure(f);const auto before=XeenSaveFormat::encode(f.snapshot());
 const auto caller=f.flow->frame().presentation();const auto token=f.flow->displayedInput();
 f.act(MapAction{});check(!f.flow->canSave(),"Map blocks saving");
 const auto input=f.flow->inputContext(f.flow->frame().presentation());
 check(input.dialog && input.dialog->anyKey && input.dialog->anyClick && !input.acceptsQueuedInput,"Map uses original key/mouse wait");
 f.flow->handle(NavigationAction::MoveForward,token,caller);check(!f.flow->canSave(),"Stale caller cannot dismiss Map or move");
 for(unsigned n=0;n<10;++n) {f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);}
 f.act(DialogKeyAction{InputKey::Escape});quiet(f);
 check(before==XeenSaveFormat::encode(f.snapshot()),"Map redraw/dismiss spends no time, RNG or exploration");
 f.act(MapAction{});const auto dialog=f.flow->inputContext(f.flow->frame().presentation()).dialog;
 f.act(*dialog->click(100,100));quiet(f);
 check(before==XeenSaveFormat::encode(f.snapshot()),"Map mouse dismissal restores caller exactly");
}
void controls(Inputs &in) {
 auto source=clearedCity(in);for(auto &c:source.characters)c.skills[4]=0;
 Fixture f(in,source);modal(f);const auto baseline=f.snapshot();
 f.act(ToggleAutomapAction{});quiet(f);auto expected=baseline;
 expected.journey->context->effects[XeenPartyEffect::AutomapOn]=!expected.journey->context->effects[XeenPartyEffect::AutomapOn];
 check(XeenSaveFormat::encode(f.snapshot())==XeenSaveFormat::encode(expected),"Automap toggle changes existing PTY byte only");
 f.act(ToggleAutomapAction{});quiet(f);
 check(XeenSaveFormat::encode(f.snapshot())==XeenSaveFormat::encode(baseline),"Automap toggle exact inverse");
 f.act(NavigationAction::TurnLeft);quiet(f);check(f.w.sessionState().exploredTiles().empty(),"No Cartography records no cells");
 check(std::holds_alternative<MapAction>(*xeenMainScreenClick(236,118,MainScreen::Exploration)),"Original Map mouse button");
 check(std::holds_alternative<ToggleAutomapAction>(*xeenMainScreenClick(236,11,MainScreen::Exploration)) &&
  std::holds_alternative<ToggleAutomapAction>(*xeenMainScreenClick(307,68,MainScreen::Exploration)),"Original automap mouse bounds");
 check(!xeenMainScreenClick(308,11,MainScreen::Exploration) && !xeenMainScreenClick(236,69,MainScreen::Exploration),"Automap right/bottom exclusive");
}
using Position=std::tuple<int,int,unsigned>;
Position key(const XeenCamera &c){return {c.x,c.y,unsigned(c.direction)};}
void walk(Fixture &f,const XeenCamera &goal) {
 struct Link {Position previous;NavigationAction action;};
 const auto origin=key(f.c),target=key(goal);std::map<Position,Link> links;
 std::queue<XeenCamera> pending;pending.push(f.c);links.emplace(origin,Link{origin,NavigationAction::TurnLeft});
 constexpr NavigationAction actions[]{NavigationAction::MoveForward,NavigationAction::TurnLeft,NavigationAction::TurnRight};
 while(!pending.empty() && !links.count(target)) {
  const auto from=pending.front();pending.pop();
  for(auto action:actions) {
   auto next=from;const auto movement=XeenMovement().apply(f.w,next,action);
   if(movement!=XeenMovementResult::Moved && movement!=XeenMovementResult::Turned)continue;
   const auto cell=f.w.sampleCell(next.mapId,next.x,next.y);
   if(key(next)!=target && cell && (cell->cell->rawAttributes&kXeenAutomaticEventFlag) && xeenRegionalEvent(f.in.city,next))continue;
   if(links.emplace(key(next),Link{key(from),action}).second)pending.push(next);
  }
 }
 if(!links.count(target)) {
  std::map<unsigned,std::pair<int,int>> tiles;for(const auto &v:links){const auto [x,y,d]=v.first;tiles.emplace(f.w.sampleCell(28,x,y)->mapId.number,std::make_pair(x,y));}
  std::string detail;for(const auto &v:tiles)detail+=" tile "+std::to_string(v.first)+"="+std::to_string(v.second.first)+","+std::to_string(v.second.second);
  throw std::runtime_error("Original mapping route unreachable: "+std::to_string(f.c.x)+","+std::to_string(f.c.y)+" to "+std::to_string(goal.x)+","+std::to_string(goal.y)+"; states="+std::to_string(links.size())+detail);
 }
 std::vector<NavigationAction> route;for(auto cursor=target;cursor!=origin;cursor=links.at(cursor).previous)route.push_back(links.at(cursor).action);
 std::reverse(route.begin(),route.end());for(auto action:route){f.act(action);quiet(f);}
 check(key(f.c)==target,"Original route reaches its goal");
}
void route(Inputs &in) {
 auto source=clearedCity(in);source.camera={28,8,16,XeenDirection::North};source.characters[source.activeRosterIds[0]].skills[4]=1;
 Fixture f(in,source);check(f.w.sessionState().exploredTiles().empty(),"Restore never replays recording");
 f.act(NavigationAction::TurnRight);quiet(f);
 check(f.w.sampleCell(28,8,16)->cell->stepped && !f.w.map(f.w.sampleCell(28,8,16)->mapId).geometry.cells[8].stepped,"Turn records occupied physical cell without changing resource");
 const auto first=f.w.sessionState().exploredTiles();modal(f);
 check(first==f.w.sessionState().exploredTiles(),"Map viewing is read-only");
 walk(f,{28,8,14,XeenDirection::North});const auto town=f.w.sessionState().exploredTiles();check(town.size()>=2,"Original route crosses indoor physical tile seams");
 const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
 check(saved==XeenSaveFormat::encode(restored.snapshot()),"Explored route reload/re-save is exact");
 f.w.discardMapCache();check(f.w.sessionState().exploredTiles()==town,"Original cache reload retains masks");
 XeenCamera exit;bool found=false;
 for(const auto &r:in.city.records)if(r.line==0)for(unsigned d=0;d<4;++d) {
  XeenCamera at{28,int(r.x),int(r.y),XeenDirection(d)};
  if(at.x==15 && at.y==0 && at.direction==XeenDirection::South && xeenRegionalInteraction(in.city,at)==XeenRegionalInteraction::Event){exit=at;found=true;break;}
  if(found)break;
 }
 check(found,"Original Vertigo exit exists");walk(restored,exit);restored.act(InteractionAction{});
 if(restored.flow->presentationGeneration())restored.act(YesAction{});quiet(restored);
 check(restored.c.mapId==XeenMapIdentity(23),"Mapping route travels to mainland");
 check(restored.w.sessionState().exploredTiles().size()>town.size(),"Mainland arrival records its occupied tile");
 const auto mainland=restored.snapshot();Fixture revisit(in,mainland);
 if(revisit.c.x!=10 || revisit.c.y!=13) {
  check(revisit.c.x==10 && revisit.c.y==12,"Original exit arrival location");
  while(revisit.c.direction!=XeenDirection::North){revisit.act(NavigationAction::TurnRight);quiet(revisit);}
  revisit.act(NavigationAction::MoveForward);quiet(revisit);
 }
 while(revisit.c.direction!=XeenDirection::North){revisit.act(NavigationAction::TurnRight);quiet(revisit);}
 revisit.act(InteractionAction{});if(revisit.flow->presentationGeneration())revisit.act(YesAction{});quiet(revisit);
 check(revisit.c.mapId==XeenMapIdentity(28),"Mapping route revisits Vertigo");
 for(const auto &tile:town) {
  const auto &tiles=revisit.w.sessionState().exploredTiles();auto at=std::find_if(tiles.begin(),tiles.end(),[&](const auto &v){return v.mapId==tile.mapId;});
  check(at!=tiles.end(),"Visited tile remains after travel/actor reset");
  for(unsigned n=0;n<32;++n)check((at->cells[n]&tile.cells[n])==tile.cells[n],"Travel retains every explored cell");
 }
 modal(revisit);
}
// M69: per-map state survives the Vertigo-F3-Vertigo loop, with a save,
// encode, decode and restore at each stop. The exit script's Spawns of slots
// 50 and 51 stay inert past the 46 MOB records; an F3 defeat is kept.
XeenSaveSnapshot reload(const XeenSaveSnapshot &saved,Inputs &in,std::unique_ptr<Fixture> &next) {
 const auto bytes=XeenSaveFormat::encode(saved);
 next=std::make_unique<Fixture>(in,XeenSaveFormat::decode(bytes));
 const auto restored=next->snapshot();
 check(XeenSaveFormat::encode(restored)==bytes,"Each stop reloads and re-saves exactly");
 return restored;
}
void checkLoop(const XeenSaveSnapshot &s,const char *stop) {
 const auto &f3=s.map(23).monsters->slots.at(5);
 check(f3.lifecycle==XeenActorLifecycle::Defeated && f3.accounted && f3.x==-128 && f3.y==-128,stop);
 const auto &city=*s.map(28).monsters;
 // The exit script's Spawns of slots 50 and 51 are past the MOB count: inert, unsaved.
 check(city.originalCount==46 && city.slots.size()==46,stop);
}
void vertigoLoop(Inputs &in) {
 auto source=clearedCity(in);
 auto &orc=source.map(23).monsters->slots.at(5);orc.x=orc.y=-128;orc.lifecycle=XeenActorLifecycle::Defeated;orc.accounted=true;
 std::unique_ptr<Fixture> city;reload(source,in,city);
 XeenCamera exit;bool found=false;
 for(const auto &r:in.city.records)if(r.line==0)for(unsigned d=0;d<4 && !found;++d) {
  XeenCamera at{28,int(r.x),int(r.y),XeenDirection(d)};
  if(at.x==15 && at.y==0 && at.direction==XeenDirection::South && xeenRegionalInteraction(in.city,at)==XeenRegionalInteraction::Event){exit=at;found=true;}
 }
 check(found,"Original Vertigo exit exists");walk(*city,exit);city->act(InteractionAction{});
 if(city->flow->presentationGeneration())city->act(YesAction{});quiet(*city);
 check(city->c.mapId==XeenMapIdentity(23),"Loop reaches the mainland");
 std::unique_ptr<Fixture> mainland;const auto atF3=reload(city->snapshot(),in,mainland);
 checkLoop(atF3,"F3 save gained Vertigo script slots or lost the F3 defeat");
 auto &f=*mainland;
 if(f.c.x!=10 || f.c.y!=13) {
  check(f.c.x==10 && f.c.y==12,"Original exit arrival location");
  while(f.c.direction!=XeenDirection::North){f.act(NavigationAction::TurnRight);quiet(f);}
  f.act(NavigationAction::MoveForward);quiet(f);
 }
 while(f.c.direction!=XeenDirection::North){f.act(NavigationAction::TurnRight);quiet(f);}
 f.act(InteractionAction{});if(f.flow->presentationGeneration())f.act(YesAction{});quiet(f);
 check(f.c.mapId==XeenMapIdentity(28),"Loop returns to Vertigo");
 std::unique_ptr<Fixture> back;const auto atVertigo=reload(f.snapshot(),in,back);
 checkLoop(atVertigo,"Vertigo save gained script slots or lost the F3 defeat");
 check(back->w.sessionState().monsters(28).size()==46,"A restored Vertigo table holds only its MOB records");
}
void combatMap(Inputs &in) {
 auto source=in.base();save_test::clearExplored(source);
 source.characters[source.activeRosterIds[0]].skills[4]=1;
 Fixture f(in,source);configure(f);
 const auto pulse=[&]{f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);};
 f.act(NavigationAction::MoveForward);quiet(f);f.act(ShootAction{});quiet(f);f.act(NavigationAction::MoveForward);
 for(unsigned n=0;n<1500 && (!f.flow->encounter()->combat() || f.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady);++n)pulse();
 auto *combat=f.flow->encounter()->combat();check(combat && combat->phase()==XeenCombatPhase::PlayerReady,"Original mapping combat enters player turn");
 const auto time=*f.p.encounterContext;const auto rng=f.w.sessionState().journeyRandom();const auto masks=f.w.sessionState().exploredTiles();
 const auto camera=f.c;const auto participant=combat->participant();
 const auto characters=f.p.roster.characters();
 f.act(MapAction{});check(!f.flow->canSave(),"Combat Map blocks saving");
 for(unsigned n=0;n<12;++n)pulse();f.act(DialogKeyAction{InputKey::Escape});
 check(combat==f.flow->encounter()->combat() && combat->phase()==XeenCombatPhase::PlayerReady && combat->participant()==participant,"Map restores combat caller and turn");
 check(*f.p.encounterContext==time && f.w.sessionState().journeyRandom()==rng && f.w.sessionState().exploredTiles()==masks && key(f.c)==key(camera),"Combat Map spends no time/RNG/exploration");
 for(unsigned owner=0;owner<30;++owner)check(xeen_state::sameCharacter(f.p.roster.at(owner),characters[owner]),"Combat Map leaves roster unchanged");
 f.act(ToggleAutomapAction{});auto expected=time;expected.effects[XeenPartyEffect::AutomapOn]=!expected.effects[XeenPartyEffect::AutomapOn];
 check(*f.p.encounterContext==expected && f.w.sessionState().journeyRandom()==rng && combat->participant()==participant,"Combat automap toggles only existing switch");
 f.act(ToggleAutomapAction{});check(*f.p.encounterContext==time,"Combat automap inverse");
 for(unsigned n=0;n<2000 && f.flow->encounter()->combat();++n) {
  if(f.flow->encounter()->combat()->phase()==XeenCombatPhase::PlayerReady)f.act(RunAction{});else pulse();
 }
 check(!f.flow->encounter()->combat(),"Run returns to exploration");quiet(f);
 check(f.w.sampleCell(f.c.mapId,f.c.x,f.c.y)->cell->stepped,"Run destination records occupied cell");
 const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
 check(saved==XeenSaveFormat::encode(restored.snapshot()),"Run exploration restores exactly");
}
void serviceReturn(Inputs &in) {
 auto source=in.service();save_test::clearExplored(source);
 Fixture f(in,source);f.enter();
 check(f.w.sessionState().exploredTiles().empty(),"Service entry does not record from drawing");
 f.act(CancelInteractionAction{});quiet(f);
 check(f.w.sampleCell(f.c.mapId,f.c.x,f.c.y)->cell->stepped && f.w.sessionState().exploredTiles().size()==1,
  "Cancelled supported town service records its returned occupied cell");
 const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
 check(saved==XeenSaveFormat::encode(restored.snapshot()),"Service-return exploration exact reload");
}
}
int main(int argc,char **argv){try {
 check(argc==2,"usage: mapping-gameplay-tests <installation>");auto game=xeenTestInstallationDetector().detect(argv[1]);check(bool(game),"Missing installation");Inputs in(*game);
 std::ostringstream captured;auto *prior=std::cout.rdbuf(captured.rdbuf());
 try{controls(in);route(in);vertigoLoop(in);combatMap(in);serviceReturn(in);}catch(...){std::cout.rdbuf(prior);throw;}std::cout.rdbuf(prior);
 std::cout<<"Mapping controls, physical exploration, original travel and exact reload passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
