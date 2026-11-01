#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "tools/XeenTestSave.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenStateEquality.h"
#include "app/XeenAudio.h"
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "app/XeenTitleFlow.h"
#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenOutdoorScene.h"
#include "formats/xeen/XeenVoc.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "tools/XeenDosExport.h"
#include "tools/XeenDosDiff.h"
#include "formats/xeen/XeenMapFormat.h"
#include <fstream>
#include <chrono>
#include <iostream>
#include <set>
#include <queue>
#include <map>
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenJumpRules.h"
#include "games/xeen/XeenTerrainRules.h"
#include "games/xeen/XeenEventTrigger.h"

using namespace training_test;
namespace mmodern {
struct XeenEventDispatchTestAccess {
 static std::string travelName(const XeenEventFlow &f,unsigned i){return f._travelUi->names.at(i);}
 static void reload(XeenEventFlow &f){f._encounter->innReturn();f._encounterFrame=f._encounter->ticket();}
 static std::optional<unsigned> fallImage(const XeenEventFlow &f){return f._fallSequence?std::optional<unsigned>{f._fallSequence->image}:std::nullopt;}
 static unsigned fallOops(const XeenEventFlow &f){return f._fallSequence?f._fallSequence->oops:0;}
 static std::optional<int> fallDamage(const XeenEventFlow &f){return f._pending && f._pending->state.pendingPresentation->request.fall?std::optional<int>{f._pending->state.pendingPresentation->request.fall->damage}:std::nullopt;}
 static bool inputPassPending(const XeenEventFlow &f){return f._encounter->_journeyCapture->inputPassPending;}
 static bool livePublication(const XeenEventFlow &f){return f._eventPublication!=nullptr;}
 static bool candidateWorldFlags(const XeenEventFlow &f){return f._transition && f._transition->world->detachedEventCandidate() && bool(f._transition->party.worldFlags);}

};
struct XeenRestTestAccess {
 static bool fatal(const XeenEventFlow &f){return f._fatal;}
 static bool explorationPassPending(const XeenEventFlow &f){return f._explorationPass;}
 static XeenCombat &combat(XeenEventFlow &f){return *f._encounter->_combat;}
 static bool death(const XeenEventFlow &f){return bool(f._death);}
 static bool laugh(const XeenEventFlow &f){return f._death && f._death->phase==XeenEventFlow::DeathContinuation::Phase::Laugh;}
 static unsigned index(const XeenEventFlow &f){return f._death->laugh;}
 static unsigned restarts(const XeenEventFlow &f){return f._death->restarts;}
 static const auto &final(const XeenEventFlow &f){return f._death->final;}
 static auto phase(const XeenEventFlow &f){return f._death->phase;}
 static void state(const XeenEventFlow &f){std::cerr<<"transition="<<bool(f._transition)<<" complete="<<(f._transition && bool(f._transition->completion))<<" handoff="<<f._handoffPending<<" pending="<<bool(f._pending)<<" arrival="<<f._arrivalPending<<" pass="<<f._explorationPass<<" fatal="<<f._fatal<<std::endl;if(f._pending)std::cerr<<"request="<<int(f._pending->state.pendingPresentation->request.kind)<<" text="<<f._pending->state.pendingPresentation->request.text<<std::endl;}

};
}
namespace {
using S=XeenLearnedSpell;
void picture(const IndexedFrame &f,const std::filesystem::path &path) {
 std::ofstream out(path,std::ios::binary);out<<"P6\n"<<f.width<<' '<<f.height<<"\n255\n";
 for(auto p:f.pixels)out.write(reinterpret_cast<const char *>(f.palette.data()+3*p),3);
 check(bool(out),"Cannot write original-data render");
}
void notice(const IndexedFrame &frame,const XeenFontFormat &font,const std::string &message) {
 const auto expected=drawXeenErrorScroll(frame,font,message);
 if(expected.pixels!=frame.pixels)throw std::runtime_error("Expected refusal notice text is not displayed: "+message);
}
struct Live {
 Inputs &in;XeenProgramState program{XeenWallClock::fixed(0)};
 XeenItemCatalogLoadResult catalogue;
 XeenWorld w;XeenPartyState p;XeenCamera c;XeenGameFlags f;XeenEventSystem events;
 std::unique_ptr<XeenEventFlow> flow;std::uint64_t now=0,cycle=0;CloudsMapComposer composer;
 XeenMapTable beforeMaps;
 std::function<void(XeenMapIdentity)> beforeCompose;
 Live(Inputs &in,const XeenSaveSnapshot &s,const XeenEventFile *eventOverride=nullptr,const XeenEventFile *destinationOverride=nullptr):in(in),catalogue(loadXeenItemCatalog(in.assets)),w(in.mapLoader(),in.objectLoader()),
  events([&in,eventOverride,destinationOverride](auto id){return XeenEventScript(eventOverride && eventOverride->mapId==id?*eventOverride:destinationOverride && destinationOverride->mapId==id?*destinationOverride:in.events.load(id));},[&](auto id){return in.texts.load(id);}) {
  in.assets.loadPalette("mm4.pal",xeenMapResources(s.camera.mapId).ambient);in.assets.loadRawFramebuffer("back.raw",xeenMapResources(s.camera.mapId).ambient);
  auto resources=in.resources();if(eventOverride)resources.loadEvents=[&in,eventOverride,destinationOverride](auto id){return eventOverride->mapId==id?*eventOverride:destinationOverride && destinationOverride->mapId==id?*destinationOverride:in.events.load(id);};
  XeenSaveState::restoreBeforeGameplay(s,resources,p,c,f,w,[](auto &,const auto &,const auto &,const auto &){});
  const auto compose=[&](XeenWorld &world,const XeenPartyState &party,const XeenCamera &camera,std::uint64_t phase) {
   if(beforeCompose)beforeCompose(camera.mapId);
   bool animated=false;IndexedFrame image;try {image=composer.compose(in.assets,world,party,camera,{party.encounterContext->year},nullptr,phase,&animated);} catch(const std::exception &e){std::cerr<<"Compose "<<unsigned(static_cast<XeenSide>(camera.mapId.side))<<':'<<camera.mapId.number<<" failed: "<<e.what()<<'\n';throw;}
   return XeenEventFlow::Composition{std::move(image),animated};
  };
  flow=std::make_unique<XeenEventFlow>(program,w,events,p,c,f,in.font,
   [&,compose](auto phase){return compose(w,p,c,phase);},XeenEventPresenter::NpcDraw{[&](auto &frame,auto id,auto phase){in.assets.drawNpc(frame,id,phase,xeenMapResources(c.mapId).ambient);}},
   [this]{return now;},XeenEventPresenter::RandomFrame{},&catalogue.catalog,
   [&,compose](auto phase,auto){return compose(w,p,c,phase);},nullptr,
   [compose](auto &world,const auto &party,const auto &camera,auto phase,auto){return compose(world,party,camera,phase);});
  flow->dialogText=&in.assets.uiText();
  flow->drawDialogSprite=[&](auto &frame,const char *name,unsigned index,int x,int y){in.assets.drawDialogSprite(frame,name,index,x,y,xeenDialogSpriteArchive(c.mapId,name));};
  flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){return xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,archive));};
  flow->drawEntranceSprite=[&](auto &frame,const char *name,unsigned index,int x,int y,int scale){in.assets.drawDialogSprite(frame,name,index,x,y,xeenMapResources(c.mapId).ambient,scale);};
  flow->loadEntranceCaption=[&](unsigned action,bool keyed){
   const auto bytes=in.assets.readArchiveResource("special.bin",xeenMapResources(c.mapId).ambient);
   const auto record=xeenEntranceCaptionRecord(action,keyed,c.mapId.side);std::size_t start=0;
   for(unsigned i=0;i<record;++i){while(bytes.at(start))++start;++start;}
   auto end=start;while(bytes.at(end))++end;return std::string(bytes.begin()+start,bytes.begin()+end);
  };
  flow->drawEntranceCaption=[&](auto &frame,const std::string &text){
   in.assets.drawDialogSprite(frame,"box.vga",0,36,189,xeenMapResources(c.mapId).ambient);
   XeenTextRenderOptions options;options.originalControls=true;options.bounds={0,0,320,200};
   frame=XeenTextRenderer(in.font).render(frame,std::string("\f35\3c\v190\t000")+text,options).pages.front();
  };
  flow->mapName=[&](auto id){return in.assets.mapName(id);};
  flow->loadSpellDescriptions=[&]{return in.assets.readArchiveResource("spldesc.bin",xeenMapResources(c.mapId).ambient);};
  flow->drawSmithArt=[&](auto &image){in.assets.drawSmith(image,xeenMapResources(c.mapId).ambient);};
  flow->drawTrainingArt=[&](auto &image){in.assets.drawTraining(image,xeenMapResources(c.mapId).ambient);};
  flow->drawTempleArt=[&](auto &image){in.assets.drawTemple(image,xeenMapResources(c.mapId).ambient);};
  flow->drawBankArt=[&](auto &image){in.assets.drawBank(image,xeenMapResources(c.mapId).ambient);};
  flow->drawGuildArt=[&](auto &image){in.assets.drawGuild(image,xeenMapResources(c.mapId).ambient);};
  flow->drawTavernArt=[&](auto &image){in.assets.drawTavern(image,xeenMapResources(c.mapId).ambient);};
  flow->tavernFoodOffer=[&](unsigned n){return in.assets.tavernFoodOffer(n,xeenMapResources(c.mapId).ambient);};
  flow->tavernString=[&](std::size_t n){return in.assets.tavernString(n,xeenMapResources(c.mapId).ambient);};
  present(flow->frame());
  beforeMaps=w.sessionState().mapStates();
 }
 void present(const IndexedFrame &frame){if(frame.presentation())flow->framePresented(frame.presentation());}
 void act(PlayerAction action){
  // The native input queue holds NPC taps until the acquired row's DOS hold
  // expires. Advancing animation first would continually arm another hold.
  if(const auto deadline=flow->nextPresentationDeadline())now=std::max(now,PresentationClock::ceilMilliseconds(*deadline));
  flow->beginCycle(++cycle);present(flow->handle(action,flow->displayedInput(),flow->frame().presentation()));
 }
 void key(unsigned key){const auto input=flow->inputContext(flow->frame().presentation()).dialog;check(bool(input),"Missing live dialog");const auto action=input->key(key);check(bool(action),"Missing live dialog key");act(*action);}
 void tick(){now+=110;flow->beginCycle(++cycle);if(auto frame=flow->updatePresentation())present(*frame);}
 void settle(){for(unsigned n=0;!flow->canSave();++n){if(n==1000){std::cerr<<"settle map="<<c.mapId.number<<" phase="<<int(flow->encounter()->state().phase())<<" pending="<<flow->encounter()->state().pending()<<" activity="<<int(w.sessionState().journeyActivity())<<" combat="<<bool(flow->encounter()->combat())<<" blocked="<<flow->blocksGameplay()<<std::endl;XeenRestTestAccess::state(*flow);throw std::runtime_error("Darkside action never settled");}
  const auto input=flow->inputContext(flow->frame().presentation()).dialog;if(input && input->anyKey)act(AcknowledgeAction{});else if(input && input->key('n'))key('n');else if(input && input->key(InputKey::F1+3))act(SelectMemberAction{3});else if(flow->encounter()->combat() && flow->encounter()->combat()->phase()==XeenCombatPhase::PlayerReady)act(AttackAction{});tick();}}
 XeenSaveSnapshot snapshot(){settle();return XeenSaveState::capture(in.signature,p,c,f,w);}
 void unchanged(const XeenSaveSnapshot &s) {
  check(xeen_state::sameMapStates(beforeMaps,w.sessionState().mapStates()),"Refusal changed map, actor or explored state");
  check(c.mapId==s.camera.mapId && c.x==s.camera.x && c.y==s.camera.y && c.direction==s.camera.direction,"Refusal relocated party");
  check(p.encounterContext==s.journey->context && p.purse==s.journey->purse && w.sessionState().journeyRandom()==s.journey->random,"Refusal changed time, money or RNG");
  check(f.values()==s.gameFlags && p.questFlags.values()==s.questFlags && p.questItems.counts()==s.questItems && w.sessionState().darksideSong()==s.darksideSong,"Refusal changed flags/counters/song");
  check(p.food==s.food && p.worldFlags==s.journey->worldFlags && p.serviceEconomy==s.journey->serviceEconomy && XeenSoundOptions(p.soundOptions)==s.soundOptions,"Refusal changed shared party/economy/options");
  for(unsigned i=0;i<30;++i)check(xeen_state::sameCharacter(p.roster.at(i),s.characters[i]),"Refusal changed a roster record");
 }
};
XeenSaveSnapshot quiet(Inputs &in,XeenSaveSnapshot s) {
 const auto id=s.camera.mapId;const auto mon=XeenMonsterFormat::parse(*in.assets.readMonsterStatistics(xeenMapResources(id).monsterTable));
 const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,id),mon);
 auto &table=s.map(id).monsters;table=XeenSaveMonsterTable{std::uint16_t(actors.size()),{}};
 for(const auto &a:actors) {if(a.lifecycle==XeenActorLifecycle::Present)table->slots.push_back({-128,-128,XeenActorLifecycle::Defeated,XeenActorStatus::Physical,true,-1});else table->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});}
 s.headCache.reset();s.soundOptions={false,false,true,false};return s;
}
XeenSaveSnapshot at(Inputs &in,XeenSaveSnapshot s,const XeenCamera &camera) {
 s.camera=camera;s.darksideSong.reset();
 if(xeenMapResources(camera.mapId).music==XeenMapMusic::DarksideRandom)s.darksideSong=static_cast<std::uint8_t>(7*xeenMapMusicRow(in.maps.loadGeometryMap(in.assets,camera.mapId).geometry));
 return quiet(in,std::move(s));
}
void arrivalMusic(Live &live,const XeenJourneyRandomState &before) {
 XeenCombatRandom expected(before);
 if(xeenMapResources(live.c.mapId).music==XeenMapMusic::DarksideRandom) {
  const auto song=7*xeenMapMusicRow(live.w.map(live.c.mapId).geometry)+expected.range(0,6);
  check(live.w.sessionState().darksideSong()==song,"Arrival picked the wrong DOS music row/column");
 }else check(!live.w.sessionState().darksideSong(),"Fixed-song arrival retained Darkside song");
 check(live.w.sessionState().journeyRandom()==expected.continuation(),"Arrival music draw count/state differs");
 XeenCombatRandom next(*live.w.sessionState().journeyRandom());
 check(next.range(1,100)==expected.range(1,100) && next.continuation()==expected.continuation(),"Crossing shifted the next Journey draw");
}
void pyramid(Inputs &in,const GameInstallation &installation,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 const std::array<XeenCamera,8> sites{{{3,5,8,XeenDirection::North},{10,10,11,XeenDirection::North},
  {11,10,6,XeenDirection::North},{16,14,8,XeenDirection::East},{23,8,10,XeenDirection::North},
  {49,7,14,XeenDirection::North},{{XeenSide::Darkside,29},26,24,XeenDirection::North},{{XeenSide::Darkside,52},2,2,XeenDirection::South}}};
 for(unsigned site=0;site<sites.size();++site)for(unsigned mode=0;mode<3;++mode) {
  std::cout<<"Pyramid site "<<site<<" audio "<<mode<<std::endl;
  const auto camera=sites[site];const XeenCamera to=site<5?XeenCamera{{XeenSide::Darkside,29},25,21,XeenDirection::North}:
   site==5?XeenCamera{{XeenSide::Darkside,52},2,2,XeenDirection::North}:site==6?XeenCamera{23,8,10,XeenDirection::North}:XeenCamera{49,7,14,XeenDirection::South};
  const auto original=in.events.load(camera.mapId);check(std::any_of(original.records.begin(),original.records.end(),[&](const auto &r){return r.x==camera.x && r.y==camera.y && r.opcode==17 && r.parameters==std::vector<std::uint8_t>{12};}),"Missing original Pyramid source");
  auto s=at(in,at(in,source,to),camera);s.soundOptions.music=mode!=1;s.characters[s.activeRosterIds[0]].skills[4]=1;
  for(unsigned bit:{255u,256u,511u})s.gameFlags[bit]=true;
  for(unsigned bit:{29u,30u,59u})s.questFlags[bit]=true;
  s.questItems[34]=7;s.questItems[35]=11;s.questItems[84]=13;
  for(unsigned owner:{0u,29u}){s.characters[owner].characterFlags[0]=true;s.characters[owner].characterFlags[31]=true;}
  s.map(to.mapId).explored.emplace();s.map(to.mapId).explored->at(0)=1;
  Live live(in,s);if(mode<2){live.program.audio=XeenAudio::installed(installation,in.assets,0,false);live.program.audio->observeOptions(s.soundOptions);}
  unsigned clears=0,publications=0,automatic=0;bool destinationPassPending=false;
  live.flow->observeEventInputClear=[&](auto address,const auto &){if(address==0xf590)++clears;};
  live.flow->reportAutomatic=[&](const auto &){++automatic;};
  live.beforeCompose=[&](auto id){if(id==to.mapId && live.c.mapId==to.mapId)
   destinationPassPending=destinationPassPending || XeenEventDispatchTestAccess::inputPassPending(*live.flow);};
  live.flow->reportManual=[&](const auto &r){if(std::holds_alternative<XeenManualEventCompleted>(r)){++publications;destinationPassPending=destinationPassPending || XeenRestTestAccess::explorationPassPending(*live.flow);}};
  const auto random=*live.w.sessionState().journeyRandom();live.act(InteractionAction{});live.settle();
  check(!automatic && !destinationPassPending && !XeenEventDispatchTestAccess::inputPassPending(*live.flow),"Pyramid ran or retained the current input's destination pass");
  check(xeen_state::sameCamera(live.c,to),"Pyramid chose the wrong destination/facing");arrivalMusic(live,random);
  check(live.p.encounterContext==s.journey->context && live.p.purse==s.journey->purse,"Pyramid charged time or money");
  check(live.f.values()==s.gameFlags && live.p.questFlags.values()==s.questFlags && live.p.questItems.counts()==s.questItems,"Pyramid mixed or erased progress banks/counters");
  for(unsigned id=0;id<30;++id)check(xeen_state::sameCharacter(live.p.roster.at(id),s.characters[id]),"Pyramid changed a character or its side");
  check(clears==1 && publications==1,"Pyramid did not settle and clear input once");
  const auto &history=live.w.sessionState().mapState(to.mapId)->explored;check(history && ((*history)[0]&1),"Revisited destination lost its history");
  const auto physical=live.w.sampleCell(to.mapId,to.x,to.y);check(bool(physical),"Pyramid arrival has no physical cell");
  const auto &stepped=live.w.sessionState().mapState(physical->mapId)->explored;
  check(stepped && ((*stepped)[(physical->y*16+physical->x)/8]&(1u<<((physical->y*16+physical->x)%8))),"Pyramid arrival missed M98 Cartographer marking");
  if(!mode)picture(live.flow->frame(),out/("pyramid-"+std::to_string(site)+".ppm"));
 }
 // Failure after destination admission, and interruption during its composition,
 // must not publish even the source's stepped mark or the music draw.
 for(unsigned mode=0;mode<2;++mode){std::cout<<"Pyramid failure "<<mode<<std::endl;auto s=at(in,source,sites[4]);Live live(in,s);
  live.beforeCompose=[&](auto id){if(id.side==XeenSide::Darkside)throw std::runtime_error(mode?"Interrupted Pyramid preparation":"Missing destination sprite");};
  live.act(InteractionAction{});live.unchanged(s);
 }
}
void travel(Inputs &in,const GameInstallation &installation,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 const auto prepare=[&](XeenSaveSnapshot s,S spell,bool item){auto &caster=s.characters[s.activeRosterIds[0]];
  caster.weapons={};caster.armor={};caster.accessories={};
  caster.characterClass=spell==S::TownPortal?XeenCharacterClass::Cleric:XeenCharacterClass::Sorcerer;caster.hasSpells=true;caster.currentSp=1000;
  caster.learnedSpells.emplace();caster.learnedSpells->fill(0);caster.currentSpell=*XeenLearnedSpellRules::slotForSpell(spell==S::TownPortal?XeenSpellCategory::Clerical:XeenSpellCategory::Wizardry,unsigned(spell));caster.learnedSpells->at(caster.currentSpell)=1;
  caster.miscellaneous={};if(item)caster.miscellaneous[0]={1,std::uint8_t(spell==S::TownPortal?55:28),3,0};return s;};
 const auto open=[](Live &live,bool item){if(item){live.act(SelectMemberAction{0});live.key('i');live.key('m');live.key('1');live.key('u');}
  else {live.act(CastSpellAction{});live.act(SelectMemberAction{0});live.key('c');}};
 constexpr int x[]{14,5,2,8,2},y[]{11,12,15,11,6};constexpr XeenDirection d[]{XeenDirection::South,XeenDirection::West,XeenDirection::East,XeenDirection::North,XeenDirection::North};
 for(auto side:{XeenSide::Clouds,XeenSide::Darkside})for(bool item:{false,true}) {
  const XeenCamera here{{side,29},14,4,XeenDirection::East};const XeenCamera other{{side==XeenSide::Clouds?XeenSide::Darkside:XeenSide::Clouds,29},5,5,XeenDirection::East};
  auto s=prepare(at(in,at(in,source,other),here),S::LloydsBeacon,item);auto &caster=s.characters[s.activeRosterIds[0]];caster.lloydMap=0;caster.lloydSide=0;
  Live defaults(in,s);open(defaults,item);const auto &c=defaults.p.roster.at(caster.rosterId);
  check(c.lloydMap==(side==XeenSide::Darkside?29:28) && c.lloydSide==unsigned(side) && c.lloydX==(side==XeenSide::Darkside?25:18) && c.lloydY==(side==XeenSide::Darkside?21:4),"Beacon default mismatch");
  if(side==XeenSide::Darkside && !item)picture(defaults.flow->frame(),out/"darkside-beacon-default.ppm");defaults.key(InputKey::Escape);defaults.settle();
  Live set(in,s);open(set,item);set.key('s');set.settle();check(set.p.roster.at(caster.rosterId).lloydSide==unsigned(side) && set.p.roster.at(caster.rosterId).lloydMap==29,"Beacon Set lost its side");
  s=set.snapshot();s=at(in,s,other);Live back(in,s);const auto random=*back.w.sessionState().journeyRandom();open(back,item);
  if(side==XeenSide::Darkside && !item)picture(back.flow->frame(),out/"cross-side-beacon.ppm");back.key('r');back.settle();
  check(xeen_state::sameCamera(back.c,here),"Cross-side Beacon failed to return");arrivalMusic(back,random);
 } for(unsigned choice=1;choice<=5;++choice)for(unsigned mode=0;mode<3;++mode)for(bool item:{false,true}) {
  const XeenCamera to{{XeenSide::Darkside,std::array<std::uint16_t,5>{4,19,22,6,98}[choice-1]},x[choice-1],y[choice-1],d[choice-1]};
  auto s=prepare(at(in,at(in,source,to),{{XeenSide::Darkside,29},14,4,XeenDirection::North}),S::TownPortal,item);s.soundOptions.music=mode!=1;
  Live live(in,s);if(mode<2){live.program.audio=XeenAudio::installed(installation,in.assets,0,false);live.program.audio->observeOptions(s.soundOptions);}
  const auto random=*live.w.sessionState().journeyRandom();std::vector<XeenMapIdentity> names;
  live.flow->mapName=[&](auto id){names.push_back(id);return in.assets.mapName(id);};open(live,item);
  check(names.size()==5,"Darkside Portal names absent");for(unsigned i=0;i<5;++i)check(names[i]==XeenMapIdentity(XeenSide::Darkside,29+2*i),"Portal named a Clouds town");
  if(choice==1 && !mode && !item)picture(live.flow->frame(),out/"darkside-portal.ppm");
  std::cout<<"Portal choice="<<choice<<" mode="<<mode<<" item="<<item<<std::endl;
  live.key('0'+choice);live.key(InputKey::Enter);
  // The fifth CD selector lands in sky map 98. Learned casting's ordinary
  // terrain suffix then falls to map 10 (the Part A DOS table, 98-88).
  // Item casting returns before that suffix. Check the load before the fall.
  if(choice==5){
   check(xeen_state::sameCamera(live.c,to),"Olympus selector did not initially land on map 98");arrivalMusic(live,random);
   live.settle();check(live.c.mapId==XeenMapIdentity(XeenSide::Darkside,item?98:10),"Original sky landing used the wrong caller suffix");
  }else{
   live.settle();check(xeen_state::sameCamera(live.c,to),"Darkside Portal landing mismatch");arrivalMusic(live,random);
  }
 }

}
void saveExport(Inputs &in,const GameInstallation &installation,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 auto s=at(in,source,{{XeenSide::Darkside,29},14,4,XeenDirection::North});
 for(unsigned bit:{0u,255u,256u,510u,511u})s.gameFlags[bit]=true;for(unsigned bit:{29u,30u,59u})s.questFlags[bit]=true;
 for(unsigned i=0;i<85;++i)s.questItems[i]=i+1;
 for(unsigned owner:{0u,29u}){s.characters[owner].characterFlags[0]=true;s.characters[owner].characterFlags[31]=true;}
 for(auto side:{XeenSide::Clouds,XeenSide::Darkside}){const XeenMapIdentity id{side,29};const auto events=in.events.load(id);unsigned record=0;while(!events.records.at(record).opcode)++record;s.map(id).events={{std::uint16_t(record),0}};}
 // DOS Load marks a Cartographer's occupied cell. Prepare an arrived witness
 // so an immediate DOS re-save changes only its name and elapsed play time.
 auto &explored=s.map(s.camera.mapId).explored;if(!explored)explored.emplace();
 const unsigned cell=s.camera.y*16+s.camera.x;explored->at(cell/8)|=1u<<(cell%8);
 Live live(in,s);const auto saved=live.snapshot();const auto encoded=XeenSaveFormat::encode(saved);Live restored(in,XeenSaveFormat::decode(encoded));
 check(XeenSaveFormat::encode(restored.snapshot())==encoded,"Two-sided witness did not re-save byte-identically");
 live.act(NavigationAction::TurnRight);restored.act(NavigationAction::TurnRight);check(XeenSaveFormat::encode(live.snapshot())==XeenSaveFormat::encode(restored.snapshot()),"Next normal action differs after Load");
 const auto pair=developer::projectDosSave(installation,saved,"M101 Darkside");const auto base=developer::dosNewGame(installation);
 const auto member=[](const auto &members,const std::string &name)->const auto &{const auto it=std::find_if(members.begin(),members.end(),[&](const auto &m){return m.name==name;});check(it!=members.end(),"Missing exported member");return it->bytes;};
 const auto &pty=member(pair.darkside,"maze.pty");const auto &original=member(base.darkside,"maze.pty");
 const auto bit=[&](unsigned offset,unsigned i){return bool(pty[offset+i/8]&(0x80u>>(i%8)));};
 for(unsigned i=0;i<512;++i)check(bit(659,i)==saved.gameFlags[i],"DOS game bank bit mismatch");
 for(unsigned i=0;i<60;++i)check(bit(739,i)==saved.questFlags[i],"DOS quest bank bit mismatch");check((pty[746]&15)==(original[746]&15),"Quest padding overwritten");
 for(unsigned i=0;i<85;++i)check(pty[747+i]==saved.questItems[i],"DOS counter mismatch");
 for(unsigned owner=0;owner<30;++owner)for(unsigned i=0;i<32;++i)check(bit(1408+4*owner,i)==saved.characters[owner].characterFlags[i],"DOS character flag mismatch");
 check(member(pair.darkside,"maze.nam")[29]==1 && member(pair.clouds,"maze.nam")[29]==1,"Darkside NAM side marker missing");
 const auto &dat=member(pair.darkside,developer::dosMapName(29,"dat"));
 check(dat.at(860+saved.camera.y*2+saved.camera.x/8)&(0x80u>>(saved.camera.x%8)),"Immediate DOS re-save witness must already mark its occupied cell");
 for(auto side:{XeenSide::Clouds,XeenSide::Darkside}){const XeenMapIdentity id{side,29};const auto &members=side==XeenSide::Clouds?pair.clouds:pair.darkside;
  const auto events=XeenEventFormat::parse(member(members,developer::dosMapName(29,"evt")));for(const auto &e:saved.map(id).events)check(events[e.record].opcode==e.opcode,"DOS same-number map history mixed sides");}
 auto tooLarge=saved;tooLarge.questItems[84]=256;bool refused=false;try{developer::projectDosSave(installation,tooLarge,"Too large");}catch(const std::invalid_argument &e){refused=std::string(e.what()).find("one byte")!=std::string::npos;}check(refused,"Unrepresentable DOS counter accepted");
 const auto save=out/"part-b-witness.mmsave";std::ofstream bytes(save,std::ios::binary);bytes.write(reinterpret_cast<const char *>(encoded.data()),encoded.size());bytes.close();
}
void resources(Inputs &in) {
 unsigned admitted=0;
 for(unsigned n=1;n<=129;++n) {
  const XeenMapIdentity id{XeenSide::Darkside,std::uint16_t(n)};
  const auto map=in.maps.loadGeometryMap(in.assets,id);const auto mob=in.maps.loadObjects(in.assets,id);const auto evt=in.events.load(id);
  check(map.identity()==id && mob.mapId==id && evt.mapId==id && mob.resourcePresent && evt.resourcePresent,"Ordinary Darkside resource identity/presence");++admitted;
 }
 check(admitted==129,"Darkside main map inventory changed");
}
void music(Inputs &in,const GameInstallation &installation,const XeenSaveSnapshot &source) {
 for(unsigned mode=0;mode<3;++mode) {
  auto s=source;s.soundOptions.music=mode!=1;Live live(in,s);
  if(mode<2)live.program.audio=XeenAudio::installed(installation,in.assets,0,false);
  const auto before=*live.w.sessionState().journeyRandom();
  check(live.w.sessionState().darksideSong()==s.darksideSong && before==*s.journey->random,"Load selected music again");
  if(live.program.audio) {
   live.program.audio->observeOptions(s.soundOptions);
   const auto name=live.program.audio->darksideMapSong(*s.darksideSong);
   check(name==in.assets.uiText().table("DARKSIDE_MUSIC").at(*s.darksideSong),"Saved song did not resolve through DOS table");
   live.program.audio->requestSong(name,XeenSceneArchive::DarksideOnly,207,0);
  }
  for(const XeenMapIdentity to: {XeenMapIdentity{XeenSide::Darkside,1},XeenMapIdentity{XeenSide::Darkside,29}}) {
   auto candidate=live.w.transitionCandidate();XeenCombatRandom expected(before);const auto roll=expected.range(0,6);
   candidate->loadEventDestination(live.c.mapId,to,true,live.f);
   check(candidate->sessionState().journeyRandom()==expected.continuation(),"Map arrival did not make exactly one music selection");
   check(candidate->sessionState().darksideSong()==std::optional<std::uint8_t>(7*xeenMapMusicRow(candidate->map(to).geometry)+roll),"Wrong DOS music row/column");
   live.unchanged(s);
  }
 }
}
void keyedEntrances(Inputs &in,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 unsigned heldKeys=0;
 for(const auto &site:xeenEntranceSites())if(site.side==XeenSide::Clouds && site.quest>=35) {
  bool found=false;
  for(const auto &e:in.events.load(site.map).records)if(e.opcode==17 && e.line==0 && e.parameters==std::vector<std::uint8_t>{std::uint8_t(site.action)}) {
   XeenCamera at{site.map,e.x,e.y,e.direction<4?XeenDirection(e.direction):XeenDirection::North};
   if(xeenEntranceSite(site.action,at)!=&site)continue;
   auto s=source;s.camera=at;s.darksideSong.reset();s.questItems[site.quest]=1;s=quiet(in,s);
   std::cout<<"Keyed entrance site "<<s.camera.x<<','<<s.camera.y<<'\n';Live live(in,s);std::string reason;live.flow->reportManual=[&](const auto &r){if(const auto *error=std::get_if<XeenEventExecutionError>(&r))reason=error->message;};
   live.act(InteractionAction{});live.settle();check(reason.empty(),reason.c_str());
   check(xeen_state::sameCamera(live.c,site.destination),"Held shared key did not enter its mixed map");found=true;break;
  }
  check(found,"Missing original Darkside-key entrance site");++heldKeys;
 }
 check(heldKeys==4,"Shared key entrance inventory changed");

}
// Original records are loaded from the installation, then isolated from their
// unrelated suffix. Both physical sides exercise the regional preflight.
// Execute unedited original records through the live detached Event boundary.
// Inert exits isolate the three rules from later milestone opcodes.
void partAEvents(Inputs &in,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 for(unsigned opcode:{51u,56u,44u}) {
  std::optional<XeenEventRecord> found;XeenMapIdentity id;
  for(unsigned map=1;map<=112 && !found;++map) {
   const XeenMapIdentity current{XeenSide::Darkside,std::uint16_t(map)};
   for(const auto &r:in.events.load(current).records)if(r.opcode==opcode &&
      (opcode!=44 || (r.parameters[0]>0 && r.parameters[0]<35))) {found=r;id=current;break;}
  }
  check(bool(found),"Missing original Part A Event");const auto original=*found;
  std::cout<<"M103 opcode "<<opcode<<" map "<<id.number<<" EVT @"<<original.fileOffset<<'\n';
  XeenEventFile isolated{id,"isolated original Part A Event",true,{original}};
  const auto end=[&](unsigned line) {
   if(line==original.line)return;
   auto r=original;r.line=line;r.opcode=18;r.parameters.clear();r.lengthField=5;isolated.records.push_back(r);
  };
  if(opcode==56)for(unsigned n=1;n<original.parameters.size();++n)end(original.parameters[n]);
  else end(original.line+1);
  // Keep the original instruction's coordinates, line, facing, offset and bytes.
  // A synthetic physical caller reaches it even when it is a logical subroutine.
  XeenEventRecord call;call.x=14;call.y=14;call.direction=4;call.line=0;call.opcode=25;call.lengthField=8;
  call.parameters={original.x,original.y,original.line};isolated.records.push_back(call);
  auto s=at(in,source,{id,14,14,original.direction<4?XeenDirection(original.direction):XeenDirection::North});
  Live live(in,s,&isolated);const auto before=live.snapshot();
  const auto a=opcode==51?live.w.effectiveObject({id,original.parameters[0]}):XeenMapEntity{};
  const auto b=opcode==51?live.w.effectiveObject({id,original.parameters[1]}):XeenMapEntity{};
  std::string failure;unsigned runs=0;XeenCombatRandom expected(*before.journey->random);
  live.flow->reportManual=[&](const auto &result){
   if(const auto *e=std::get_if<XeenEventExecutionError>(&result))failure=e->message;
   if(std::holds_alternative<XeenManualEventCompleted>(result)) {
    ++runs;
    if(opcode==56) {
     expected.range(1,original.parameters[0]);
     check(live.w.sessionState().journeyRandom()==expected.continuation(),"Original GotoRandom dispatch consumed more than one draw");
    }
   }
  };
  live.act(InteractionAction{false});auto saved=live.snapshot();check(failure.empty(),failure.c_str());
  if(opcode==51) {
   const auto afterA=live.w.effectiveObject({id,original.parameters[0]}),afterB=live.w.effectiveObject({id,original.parameters[1]});
   check(afterA.x==b.x && afterA.y==b.y && afterB.x==a.x && afterB.y==a.y && afterA.resourceId==a.resourceId && afterB.resourceId==b.resourceId,
    "Original ExchObj positions did not exchange through live publication");
  } else if(opcode==56) {
   check(runs && saved.journey->random==expected.continuation(),"Original GotoRandom did not consume exactly one Journey draw");
  } else {
   unsigned beforeCount=0,afterCount=0;
   for(auto owner:saved.activeRosterIds) {
    for(const auto &item:before.characters[owner].weapons)beforeCount+=item.id==original.parameters[0] && item.material==original.parameters[1];
    for(const auto &item:saved.characters[owner].weapons)afterCount+=item.id==original.parameters[0] && item.material==original.parameters[1];
   }
   check(runs && afterCount==beforeCount+runs,"Original enchanted weapon reward did not reach inventory once per dispatch");
  }
  const auto path=out/("m103-part-a-"+std::to_string(opcode)+".mmsave");XeenSaveFile::write(path,saved);
  const auto wire=XeenSaveFile::read(path);Live restored(in,wire,&isolated);
  check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(saved),"Part A save/load replayed or lost object/reward/RNG state");
 }
}

// Original Part B operands run through the live Flow. Inert suffixes/marker
// writes isolate dispatch decisions from unrelated later milestone mechanics.
void partBEvents(Inputs &in,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 std::optional<XeenEventRecord> numeric;XeenEventFile originalFile;
 unsigned fallRows=0,zeroRows=0;
 for(unsigned map=1;map<=112;++map) {
  const auto file=in.events.load({XeenSide::Darkside,std::uint16_t(map)});
  for(const auto &r:file.records) {
   if(r.opcode==48 && !numeric){numeric=r;originalFile=file;}
   if(r.opcode==52) {
    ++fallRows;const auto decoded=XeenEventDecoder::decode(r);
    const auto &fall=std::get<XeenEventFallToMap>(std::get<XeenDecodedEventInstruction>(decoded).operation);
    if(!r.followingByte) {
     ++zeroRows;check((map==51 && r.fileOffset==1050) || (map==89 && r.fileOffset==235) || (map==92 && r.fileOffset==274),"Unexpected end-of-resource fall");
     check(fall.damage==0,"Approved end-of-resource fall must use zero");
    }else check(fall.damage==*r.followingByte,"Original omitted damage must use next record length");
   }
  }
 }
 check(numeric && fallRows==220 && zeroRows==3,"Part B original operand inventory differs");
 const auto r=*numeric;const auto id=originalFile.mapId;
 std::cout<<"Numeric cell "<<unsigned(r.x)<<","<<unsigned(r.y)<<" params";for(auto b:r.parameters)std::cout<<' '<<unsigned(b);std::cout<<std::endl;
 std::cout<<"M103 ChooseNumeric map "<<id.number<<" EVT @"<<r.fileOffset<<" line "<<unsigned(r.line)<<std::endl;
 const auto row=[](unsigned x,unsigned y,unsigned line,unsigned opcode,std::vector<std::uint8_t> p={}) {
  XeenEventRecord e;e.x=x;e.y=y;e.line=line;e.direction=4;e.opcode=opcode;e.parameters=std::move(p);e.lengthField=5+e.parameters.size();return e;
 };
 XeenEventFile isolated{id,"isolated original numbered choice",true,{r}};
 for(unsigned choice=1;choice<=r.parameters[0];++choice) {
  const unsigned line=choice==r.parameters[0]?r.line+1:r.parameters[choice];
  isolated.records.push_back(row(r.x,r.y,line,12,{0,0,20,std::uint8_t(200+choice)}));
  isolated.records.push_back(row(r.x,r.y,line+1,18));
 }
 unsigned promptText=0;
 const auto mapText=in.texts.load(id);
 for(const auto &e:originalFile.records)if(e.x==r.x && e.y==r.y && e.line<r.line) {
  const auto decoded=XeenEventDecoder::decode(e);
  if(const auto *op=std::get_if<XeenDecodedEventInstruction>(&decoded))if(const auto *display=std::get_if<XeenEventDisplay>(&op->operation))
   if(display->textIndex<mapText.strings.size() && !mapText.strings[display->textIndex].empty())promptText=display->textIndex;
 }
 if(mapText.strings.at(promptText).empty())for(unsigned i=0;i<mapText.strings.size();++i)if(!mapText.strings[i].empty()){promptText=i;break;}
 std::cout<<"Numeric retained text "<<promptText<<" length "<<mapText.strings.at(promptText).size()<<std::endl;
 isolated.records.push_back(row(14,14,0,53,{std::uint8_t(promptText)}));
 isolated.records.push_back(row(14,14,1,24,{0,18}));
 isolated.records.push_back(row(14,14,2,25,{r.x,r.y,r.line}));
 for(unsigned choice:{1u,unsigned(r.parameters[0])}) {
  auto s=at(in,source,{id,14,14,r.direction<4?XeenDirection(r.direction):XeenDirection::North});
  Live live(in,s,&isolated);std::string failure;
  live.flow->reportManual=[&](const auto &result){if(const auto *e=std::get_if<XeenEventExecutionError>(&result))failure=e->message;};
  live.act(InteractionAction{false});
  check(!live.flow->canSave() && live.flow->inputContext(live.flow->frame().presentation()).dialog->key('1'),"Original numeric choice did not block in live Flow");
  picture(live.flow->frame(),out/"m103b-choice-normal.ppm");
  const auto before=live.flow->frame();live.act(CancelInteractionAction{});
  check(!live.flow->canSave() && live.flow->frame().pixels==before.pixels,"Escape canceled or rewrote the numeric prompt");
  if(choice==1) {const auto input=live.flow->inputContext(live.flow->frame().presentation()).dialog;const auto &hit=input->hits.front();live.act(*input->click(hit.left+1,hit.top+1));}else live.key('0'+choice);const auto saved=live.snapshot();check(failure.empty(),failure.c_str());
  for(unsigned n=1;n<=r.parameters[0];++n)check(live.f.isSet(XeenSide::Darkside,200+n)==(n==choice),"Live choice/cancel dispatch selected the wrong line");
  const auto path=out/("m103b-choice-"+std::to_string(choice)+".mmsave");XeenSaveFile::write(path,saved);
  Live restored(in,XeenSaveFile::read(path),&isolated);
  check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(saved),"Numeric choice replayed through save/load");
  if(choice==1) {
   // Render the original compact NPC window/art and this same choice with Text off.
   XeenEventPresenter presenter(in.font,[&](auto &image,auto portrait,auto frame){in.assets.drawNpc(image,portrait,frame,xeenMapResources(id).ambient);});
   presenter.originalAssets(&in.assets.uiText(),[&](auto &image,const char *name,unsigned frame,int x,int y){in.assets.drawDialogSprite(image,name,frame,x,y,xeenMapResources(id).ambient);});
   XeenPresentationRequest npc;npc.kind=XeenPresentationKind::NpcAcknowledgment;npc.npc=XeenEventNpc{0,0,0,2,0};
   for(const auto &e:originalFile.records)if(e.opcode==5){const auto decoded=XeenEventDecoder::decode(e);if(const auto *op=std::get_if<XeenDecodedEventInstruction>(&decoded))if(const auto *n=std::get_if<XeenEventNpc>(&op->operation)){npc.npc=*n;npc.npc->confirmationMode=2;break;}}
   const auto texts=in.texts.load(id);npc.title=texts.strings.at(npc.npc->titleTextIndex);npc.text=texts.strings.at(npc.npc->bodyTextIndex);npc.npcBodyHidden=true;
   presenter.present(before,npc);
   XeenPresentationRequest q;q.kind=XeenPresentationKind::NumericChoice;q.response=XeenPresentationResponseRequirement::NumericChoice;
   q.numeric=std::get<XeenEventChooseNumeric>(std::get<XeenDecodedEventInstruction>(XeenEventDecoder::decode(r)).operation);
   presenter.present(presenter.frame(),q);check(presenter.input().hits.front().top==101,"Compact original NPC choice did not move to Y=101");
   picture(presenter.frame(),out/"m103b-choice-compact.ppm");
   const auto first=presenter.frame();XeenEventPresenter::NpcTiming timing;timing.displayedFrame=1;
   const auto animated=presenter.updateNpc(timing);
   check(animated && animated->pixels!=first.pixels && presenter.input().hits.front().top==101,"Retained location art stopped animating while a compact numeric choice waits");
   presenter.finishPresentation();presenter.clear();presenter.present(before,q);
   check(presenter.input().hits.front().top==75,"Leaving compact NPC did not restore normal numeric Y");
  }
 }
 std::optional<XeenSaveSnapshot> fallBaseline;
 for(bool failFirst:{false,true}) {
 const XeenMapIdentity from{XeenSide::Darkside,8},to{XeenSide::Darkside,40};
 const auto sourceFile=in.events.load(from);const auto found=std::find_if(sourceFile.records.begin(),sourceFile.records.end(),[](const auto &e){return e.fileOffset==503;});
 check(found!=sourceFile.records.end() && found->opcode==52 && found->parameters==std::vector<std::uint8_t>({40,3,3}) && found->followingByte==7,"Map 8 @503 fall witness differs");
 XeenEventFile fallFile{from,"original map 8 scripted fall",true,{*found,row(14,14,0,25,{found->x,found->y,found->line})}};
 XeenEventFile arrival{to,"fall restart marker",true,{row(3,3,0,12,{0,0,20,203}),row(3,3,1,18)}};
 auto s=at(in,at(in,source,{to,3,3,XeenDirection::West}),{from,14,14,XeenDirection::West});
 for(auto owner:s.activeRosterIds)s.characters[owner].currentHp=100;
 s.journey->context->effects[XeenPartyEffect::PowerShield]=0;
 s.journey->context->lightAndResistances[0]=30;
 Live fall(in,s,&fallFile,&arrival);std::string failure;fall.flow->reportManual=[&](const auto &result){if(const auto *e=std::get_if<XeenEventExecutionError>(&result))failure=e->message;};
 bool threw=false,scared=false;
 if(failFirst) {
  const auto draw=fall.flow->drawDialogSprite;
  fall.flow->drawDialogSprite=[&,draw](auto &frame,const char *name,unsigned image,int x,int y) {
   if(!threw && XeenEventDispatchTestAccess::fallDamage(*fall.flow)==7) {
    if(image==4 && std::string(name).find(".fac")!=std::string::npos)scared=true;
    if(scared && std::string(name)=="global.icn" && x==8 && y==8){threw=true;throw std::runtime_error("Injected first fall image failure");}
   }
   draw(frame,name,image,x,y);
  };
 }
 fall.act(InteractionAction{false});
 if(failFirst) {
  check(threw && !XeenEventDispatchTestAccess::fallImage(*fall.flow) && !fall.flow->canSave() && fall.c.mapId==from,
   "Failed first fall composition installed an incomplete animation or published arrival");
  fall.tick();
 }
 check(XeenEventDispatchTestAccess::fallImage(*fall.flow)==0 && XeenEventDispatchTestAccess::fallDamage(*fall.flow)==7,"Live original fall did not enter M86 presentation with base damage 7");
 // Four retraces of the destination load's "Oops..." window come first.
 for(unsigned oops=0;oops<4;++oops) {
  check(XeenEventDispatchTestAccess::fallOops(*fall.flow)==4-oops && fall.c.mapId==from,"Oops window order differs");
  if(!failFirst && !oops)picture(fall.flow->frame(),out/"m103b-fall-oops.ppm");
  fall.now+=15;fall.flow->beginCycle(++fall.cycle);if(auto next=fall.flow->updatePresentation())fall.present(*next);
 }
 for(unsigned image=0;image<19;++image) {
  check(XeenEventDispatchTestAccess::fallImage(*fall.flow)==image && !fall.flow->canSave(),"Fall image order or save exclusion differs");
  check(fall.c.mapId==from && fall.p.roster.at(s.activeRosterIds[0]).currentHp==100,"Fall published arrival or damage ahead of final impact");
  if(!failFirst && (image==0 || image==14 || image==15 || image==18))picture(fall.flow->frame(),out/("m103b-fall-"+std::to_string(image)+".ppm"));
  fall.now+=15;fall.flow->beginCycle(++fall.cycle);if(auto next=fall.flow->updatePresentation()) {
   if(image<18) {
    const auto waiting=XeenEventDispatchTestAccess::fallImage(*fall.flow);
    fall.now+=15;fall.flow->beginCycle(++fall.cycle);
    check(!fall.flow->updatePresentation() && XeenEventDispatchTestAccess::fallImage(*fall.flow)==waiting,"Unacquired fall frame advanced");
   }
   fall.present(*next);
  }
 }
 const auto saved=fall.snapshot();check(failure.empty(),failure.c_str());
 if(fallBaseline)check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(*fallBaseline),"Fall presentation retry replayed load, damage or RNG");
 else fallBaseline=saved;
 check(fall.c.mapId==to && fall.c.x==3 && fall.c.y==3 && fall.c.direction==XeenDirection::West && fall.f.isSet(XeenSide::Darkside,203),"Original fall failed to preserve facing or restart destination at line zero");
 for(auto owner:s.activeRosterIds)check(fall.p.roster.at(owner).currentHp==93,"Original fall did not apply base damage 7 per member");
 const auto path=out/(failFirst?"m103b-fall-retry.mmsave":"m103b-fall.mmsave");XeenSaveFile::write(path,saved);Live restored(in,XeenSaveFile::read(path),&fallFile,&arrival);
 check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(saved),"Scripted fall replayed after exact save/load");
 }
 std::cout<<"M103 Part B original choices/cancel, 220 damage operands, M86 fall and exact round trips verified\n";
}

// Ordinary Journey rewards execute detached, then publish through the live guard.
// worldFlags is present in both owners; per-instruction publication is absent.
void enchantedLivePublication(Inputs &in,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 const XeenMapIdentity id{XeenSide::Darkside,30};
 XeenEventFile file{id,"enchanted live publication",true,{}};
 const auto row=[&](unsigned line,unsigned opcode,std::vector<std::uint8_t> parameters) {
  XeenEventRecord r;r.x=14;r.y=14;r.direction=4;r.line=line;r.opcode=opcode;
  r.parameters=std::move(parameters);r.lengthField=5+r.parameters.size();file.records.push_back(r);
 };
 row(0,1,{0});row(1,44,{12,20,0,0});row(2,44,{66,57,255,255});row(3,1,{0});row(4,18,{});
 auto s=at(in,source,{id,14,14,XeenDirection::North});Live live(in,s,&file);
 check(bool(live.p.worldFlags),"Live GiveEnchanted fixture omitted worldFlags");
 const auto before=live.snapshot();XeenCombatRandom expected(*before.journey->random);
 const auto charges=expected.range(1,8)+2;unsigned legacyStagingCalls=0,inspected=0,completed=0;
 live.flow->beforeRewardStaging=[&]{++legacyStagingCalls;};
 live.flow->reportManual=[&](const auto &result) {
  check(XeenEventDispatchTestAccess::livePublication(*live.flow),"GiveEnchanted lost the live publication guard");
  check(bool(live.p.worldFlags),"Live GiveEnchanted lost worldFlags");
  if(const auto *e=std::get_if<XeenEventExecutionError>(&result))throw std::runtime_error(e->message);
  if(const auto *pending=std::get_if<XeenEventExecutionSuspended>(&result)) {
   check(XeenEventDispatchTestAccess::candidateWorldFlags(*live.flow),"Detached reward candidate omitted worldFlags");
   if(pending->state.logicalAddress.line==4 && pending->request.kind==XeenPresentationKind::CenteredMessage) {
    ++inspected;const auto &treasure=pending->state.treasure;
    check(treasure.weapons[0].id==12 && treasure.weapons[0].material==20 && treasure.weapons[0].state==0 &&
     treasure.miscellaneous[0].material==6 && treasure.miscellaneous[0].id==57 && treasure.miscellaneous[0].state==charges,
     "Live publication staged the wrong equipment or charged Misc record");
   }
  }
  if(std::holds_alternative<XeenManualEventCompleted>(result))++completed;
 };
 live.act(InteractionAction{false});const auto saved=live.snapshot();
 check(inspected==1 && completed==1 && !legacyStagingCalls,"Ordinary reward dispatch used per-instruction publication or replayed");
 check(saved.journey->random==expected.continuation(),"Live charged Misc reward changed the shared RNG continuation");
 unsigned weapons=0,misc=0;
 for(auto owner:saved.activeRosterIds) {
  for(const auto &item:saved.characters[owner].weapons)weapons+=item.id==12 && item.material==20 && item.state==0;
  for(const auto &item:before.characters[owner].weapons)weapons-=item.id==12 && item.material==20 && item.state==0;
  for(const auto &item:saved.characters[owner].miscellaneous)misc+=item.material==6 && item.id==57 && item.state==charges;
  for(const auto &item:before.characters[owner].miscellaneous)misc-=item.material==6 && item.id==57 && item.state==charges;
 }
 check(weapons==1 && misc==1,"Live equipment and charged Misc rewards did not each publish exactly once");
 const auto path=out/"m103-enchanted-live-publication.mmsave";XeenSaveFile::write(path,saved);
 Live restored(in,XeenSaveFile::read(path),&file);
 check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(saved),"Live enchanted publication failed exact save/load");
}

void partCEvents(Inputs &in,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 const auto row=[](unsigned x,unsigned y,unsigned line,unsigned opcode,std::vector<std::uint8_t> p={}) {
  XeenEventRecord r;r.x=x;r.y=y;r.direction=4;r.line=line;r.opcode=opcode;r.parameters=std::move(p);r.lengthField=5+r.parameters.size();return r;
 };
 const auto roundTrip=[&](Live &live,const XeenEventFile &file,const std::string &name) {
  const auto saved=live.snapshot();const auto bytes=XeenSaveFormat::encode(saved);
  const auto path=out/("m103c-"+name+".mmsave");XeenSaveFile::write(path,saved);
  Live restored(in,XeenSaveFile::read(path),&file);
  check(XeenSaveFormat::encode(restored.snapshot())==bytes,"Part C exact save/load lost or replayed state");
 };
 const XeenMapIdentity id{XeenSide::Darkside,72};const auto original=in.events.load(id);
 const auto found=std::find_if(original.records.begin(),original.records.end(),[](const auto &r){return r.fileOffset==208;});
 check(found!=original.records.end() && found->opcode==55 && found->parameters==std::vector<std::uint8_t>({3,10,4,16}),"Original second password row changed");
 // Retain the original prompt's DisplayMain and password bytes. Marker suffixes
 // expose each branch without conflating it with the answers' later rewards.
 XeenEventFile password{id,"original second password branches",true,{*found}};
 for(const auto &r:original.records)if(r.x==found->x && r.y==found->y && r.line<found->line)password.records.push_back(r);
 for(unsigned n=0;n<3;++n) {
  const unsigned line=n==0?10:n==1?16:6;
  password.records.push_back(row(found->x,found->y,line,12,{0,0,20,std::uint8_t(200+n)}));
  password.records.push_back(row(found->x,found->y,line+1,18));
 }
 const auto texts=in.texts.load(id);
 for(unsigned answer=0;answer<3;++answer) {
  auto s=at(in,source,{id,found->x,found->y,XeenDirection::South});Live live(in,s,&password);
  std::string failure;unsigned completed=0;
  live.flow->reportManual=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r))failure=e->message;if(std::holds_alternative<XeenManualEventCompleted>(r))++completed;};
  live.act(InteractionAction{false});
  for(unsigned n=0;;++n) {
   const auto input=live.flow->inputContext(live.flow->frame().presentation()).dialog;if(input && input->textEntry)break;
   check(n<20 && failure.empty(),"Original password prefix did not reach text entry");live.act(AcknowledgeAction{});live.tick();
  }
  if(answer==0)picture(live.flow->frame(),out/"m103c-confirmword2-blank-title.ppm");
  live.act(TextInputAction{answer<2?texts.strings.at(answer==0?3:4):"wrong"});live.act(DialogKeyAction{13});
  live.settle();check(failure.empty(),failure.c_str());check(completed>0,"Original second password did not complete");
  for(unsigned n=0;n<3;++n)check(live.f.isSet(XeenSide::Darkside,200+n)==(n==answer),"Original second password took the wrong branch");
  roundTrip(live,password,"password-"+std::to_string(answer));
 }
 unsigned walls=0;
 for(unsigned map:{54u,55u}) {
  const XeenMapIdentity current{XeenSide::Darkside,std::uint16_t(map)};const auto file=in.events.load(current);
  const auto count=in.maps.loadObjects(in.assets,current).entities.wallItems.size();
  check(count==(map==54?21:4),"Original wall-item live count changed");
  for(const auto &r:file.records)if(r.opcode==34 && r.parameters[0]>=count) {
   check(r.parameters[0]<91 && r.parameters[1]==100 && r.parameters[2]==100,"Unused wall row changed");
   // Call the original row and run its complete original suffix. A free
   // physical cell keeps this wrapper out of all original record addresses.
   auto called=file;unsigned x=15,y=15;
   while(std::any_of(file.records.begin(),file.records.end(),[&](const auto &e){return e.x==x && e.y==y;})) {
    check(x || y,"No physical caller for original wall Event");if(x)--x;else {x=15;--y;}
   }
   called.records.push_back(row(x,y,0,25,{r.x,r.y,r.line}));called.records.push_back(row(x,y,1,18));
   auto s=at(in,source,{current,int(x),int(y),r.direction<4?XeenDirection(r.direction):XeenDirection::North});Live live(in,s,&called);
   const auto before=live.snapshot();std::vector<XeenMapEntity> items;
   for(unsigned i=0;i<count;++i)items.push_back(live.w.effectiveWallItem({current,i}));
   std::string failure;unsigned completed=0;
   live.flow->reportManual=[&](const auto &result){if(const auto *e=std::get_if<XeenEventExecutionError>(&result))failure=e->message;if(std::holds_alternative<XeenManualEventCompleted>(result))++completed;};
   live.act(InteractionAction{false});const auto saved=live.snapshot();check(failure.empty(),failure.c_str());check(completed>0,"Original wall Event did not complete");
   check(live.w.objectFile(current).entities.wallItems.size()==count &&
    saved.map(current).wallItems==before.map(current).wallItems,"Unused original wall slot changed saved or visible count");
   for(unsigned i=0;i<count;++i) {
    const auto item=live.w.effectiveWallItem({current,i});const auto &b=items[i];
    check(item.x==b.x && item.y==b.y && item.direction==b.direction && item.resourceId==b.resourceId,"Unused original wall write changed a visible item");
   }
   std::cout<<"M103 Part C map "<<map<<" MoveWallObj EVT @"<<r.fileOffset<<" completed\n";
   roundTrip(live,called,"wall-"+std::to_string(map)+"-"+std::to_string(r.fileOffset));++walls;
  }
 }
 check(walls==7,"Did not execute all seven unused-wall rows");
 const XeenMapIdentity castle{XeenSide::Darkside,29};const auto castleFile=in.events.load(castle);
 const auto branch=std::find_if(castleFile.records.begin(),castleFile.records.end(),[](const auto &r){return r.fileOffset==6240;});
 const auto spawn=std::find_if(castleFile.records.begin(),castleFile.records.end(),[](const auto &r){return r.fileOffset==6427;});
 check(branch!=castleFile.records.end() && branch->opcode==42 && branch->parameters==std::vector<std::uint8_t>({31,182}) &&
  spawn!=castleFile.records.end() && spawn->opcode==16 && spawn->parameters[0]==31,"Original map-29 negative branch/spawn changed");
 XeenEventFile negative{castle,"original map-29 spawn and negative branch",true,{*spawn,*branch}};
 negative.records.push_back(row(14,14,0,25,{spawn->x,spawn->y,spawn->line}));
 // The original Spawn falls through into unrelated logic; return from it here.
 negative.records.push_back(row(spawn->x,spawn->y,spawn->line+1,26));
 negative.records.push_back(row(14,14,1,25,{branch->x,branch->y,branch->line}));
 // Neither the caller's suffix nor the raw unsigned target may execute.
 negative.records.push_back(row(14,14,2,12,{0,0,20,203}));
 negative.records.push_back(row(branch->x,branch->y,182,12,{0,0,20,204}));
 auto s=at(in,source,{castle,14,14,branch->direction<4?XeenDirection(branch->direction):XeenDirection::North});Live live(in,s,&negative);
 check(live.w.sessionState().monsters(castle).size()==96,"Map-29 monster 31 is no longer live");
 std::string failure;unsigned completed=0;
 live.flow->reportManual=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r))failure=e->message;if(std::holds_alternative<XeenManualEventCompleted>(r))++completed;};
 live.act(InteractionAction{false});live.settle();check(failure.empty(),failure.c_str());
 const auto &monster=live.w.sessionState().monsters(castle).at(31);
 check(completed && monster.x==100 && monster.y==100 && !live.f.isSet(XeenSide::Darkside,203) && !live.f.isSet(XeenSide::Darkside,204),
  "Original signed map-29 branch executed another line or failed to publish Spawn");
 roundTrip(live,negative,"map29-negative");
 std::cout<<"M103 Part C: original passwords, all seven unused wall slots and signed map-29 branch passed exact save/load\n";
}

void characterEvents(Inputs &in,const XeenSaveSnapshot &source) {
 std::optional<XeenEventRecord> condition,give;unsigned allMemberSites=0;
 for(unsigned n=1;n<=129;++n) {
  if(n>=113 && n<=128)continue;
  const auto file=in.events.load({XeenSide::Darkside,std::uint16_t(n)});
  for(const auto &r:file.records) {
   const bool probe=r.opcode==9 && r.parameters.size()==3 && r.parameters[0]==107;
   const bool grant=r.opcode==12 && r.parameters.size()>=4 && !r.parameters[0] && r.parameters[2]==107;
   if(!probe && !grant)continue;
   if(probe && !condition)condition=r;if(grant && !give)give=r;
   for(const auto &before:file.records)if(before.x==r.x && before.y==r.y && before.line<r.line &&
      before.opcode==15 && before.parameters==std::vector<std::uint8_t>{0})++allMemberSites;
   std::cout<<"Character Event "<<n<<" @"<<r.fileOffset<<" opcode "<<unsigned(r.opcode)<<'\n';
  }
 }
 check(condition && give,"Missing original Darkside character-flag Events");
 std::cout<<"Original character-flag all-members predecessors: "<<allMemberSites<<'\n';
 Live base(in,source);
 const auto row=[](unsigned line,unsigned op,std::vector<std::uint8_t> parameters) {
  XeenEventRecord r;r.x=5;r.y=5;r.direction=4;r.line=line;r.opcode=op;r.parameters=std::move(parameters);r.lengthField=5+r.parameters.size();return r;
 };
 for(auto side:{XeenSide::Clouds,XeenSide::Darkside})for(bool reordered:{false,true})for(unsigned bit:{0u,31u})for(unsigned selector:{1u,0u}) {
  auto candidate=base.w.transitionCandidate();XeenPartyState party;candidate->copyEventParty(base.p,party);
  party.party=XeenParty::fromRosterIds(reordered?std::vector<std::uint8_t>{29,0}:std::vector<std::uint8_t>{0,29});
  const auto owners=party.party.activeRosterIds();
  for(unsigned owner:{0u,29u})party.roster.at(owner).characterFlags.fill(false);
  const XeenCamera camera{{side,29},5,5,XeenDirection::North};
  check(candidate->regionalJourney(),"Character Event fixture bypassed interpreter preflight");
  const auto run=[&](std::vector<XeenEventRecord> records) {
   XeenEventFile file{camera.mapId,"isolated original character Event",true,std::move(records)};
   const auto scripts=[&](auto){return XeenEventScript(file);};
   const auto texts=[&](auto id){XeenEventTextFile f;f.mapId=id;f.resourcePresent=true;return f;};
   XeenEventInterpreter vm;auto result=vm.begin(camera,party,XeenGameFlags{},*candidate,scripts,texts);
   for(unsigned i=0;i<32;++i) {
    auto *pending=std::get_if<XeenEventExecutionSuspended>(&result);if(!pending)break;
    auto state=pending->state;const auto response=pending->request.response==XeenPresentationResponseRequirement::Presented?
     XeenPresentationResponse(XeenPresentationResponse::Presented):XeenPresentationResponse(XeenPresentationResponse::Acknowledged);
    result=vm.resume(std::move(state),response,party,*candidate,scripts,texts);
   }
   if(const auto *error=std::get_if<XeenEventExecutionError>(&result))throw std::runtime_error(error->message);
   check(std::holds_alternative<XeenEventExecutionCompleted>(result),"Character Event did not complete");
   return std::get<XeenEventExecutionCompleted>(result).finalGameFlags;
  };
  auto grant=*give;grant.x=5;grant.y=5;grant.direction=4;grant.line=1;grant.parameters[3]=bit;
  run({row(0,15,{std::uint8_t(selector)}),grant});
  for(unsigned index=0;index<2;++index)check(party.roster.at(owners[index]).characterFlags[bit]==(!selector || selector==index+1),"Character grant followed slot instead of roster identity");
  auto probe=*condition;probe.x=5;probe.y=5;probe.direction=4;probe.line=1;probe.parameters[1]=bit;probe.parameters[2]=3;
  const auto flags=run({row(0,15,{std::uint8_t(selector)}),probe,row(2,12,{0,0,20,255}),row(3,18,{})});
  check(!flags.isSet(side,255),"Original If2 mode107 did not take its true branch");
  for(unsigned opcode:{28u,29u,38u}) {
   const auto tested=run({row(0,15,{std::uint8_t(selector)}),row(1,opcode,{107,std::uint8_t(bit),107,std::uint8_t(bit),107,std::uint8_t(bit)})});
   check(!tested.isSet(side,255),"TakeOrGiveTest mode107 preflight refused");
  }

  if(bit==31) {auto take=grant;take.parameters[0]=107;take.parameters[1]=31;take.parameters[2]=0;take.parameters[3]=0;
   run({row(0,15,{std::uint8_t(selector)}),take});
   for(auto owner:owners)check(!party.roster.at(owner).characterFlags[bit],"Character take did not clear selected roster flag");
  }
  for(auto owner:owners)party.roster.at(owner).characterFlags[bit]=false;
  const auto absent=run({row(0,15,{std::uint8_t(selector)}),probe,row(2,12,{0,0,20,255}),row(3,18,{})});
  check(absent.isSet(side,255),"Original If2 mode107 did not fall through for absent flags");
  grant.parameters[3]=32;bool refused=false;
  try{run({row(0,15,{std::uint8_t(selector)}),grant});}catch(const std::exception &e){refused=std::string(e.what()).find("Character flag index outside 0..31")!=std::string::npos;}
  check(refused,"Character bit32 runtime refusal missing");
 }
 // Publication must copy and authorize the same flag fields as the interpreter.
 for(auto side:{XeenSide::Clouds,XeenSide::Darkside}) {
  auto snapshot=source;
  if(side==XeenSide::Clouds) {
   snapshot.camera={{side,28},20,20,XeenDirection::North};snapshot.darksideSong.reset();
   XeenWorld query(in.mapLoader());const auto cell=query.sampleCell(28,20,20);check(bool(cell),"Missing publication fixture cell");
   for(unsigned d=0;d<4;++d){const auto wall=wallAt(*cell->cell,XeenDirection(d));if(wall!=1 && wall!=6 && wall!=9 && wall!=13){snapshot.camera.direction=XeenDirection(d);break;}}
  }
  snapshot=quiet(in,snapshot);const auto owner=snapshot.activeRosterIds.front();
  snapshot.characters[owner].characterFlags[0]=snapshot.characters[owner].characterFlags[31]=false;
  auto file=in.events.load(snapshot.camera.mapId);auto first=*give,second=*give,probe=*condition;
  first.line=0;first.parameters[3]=0;second.line=1;second.parameters[3]=31;probe.line=2;probe.parameters[1]=31;probe.parameters[2]=4;
  file.records={first,second,probe,row(3,12,{0,0,20,255}),row(4,18,{})};
  for(auto &r:file.records){r.x=snapshot.camera.x;r.y=snapshot.camera.y;r.direction=4;}
  Live live(in,snapshot,&file);std::string failure;
  live.flow->reportManual=[&](const auto &result){if(const auto *error=std::get_if<XeenEventExecutionError>(&result))failure=error->message;};
  live.act(InteractionAction{false});
  for(unsigned n=0;n<128 && !live.flow->canSave();++n){live.now+=110;live.flow->beginCycle(++live.cycle);if(auto frame=live.flow->updatePresentation())live.present(*frame);}
  check(failure.empty() && live.flow->canSave() && live.p.roster.at(owner).characterFlags[0] && live.p.roster.at(owner).characterFlags[31] && !live.f.isSet(side,255),"Original character flags failed live Event publication");
  check(live.p.encounterContext==snapshot.journey->context && live.p.purse==snapshot.journey->purse && live.w.sessionState().journeyRandom()==snapshot.journey->random,"Character publication changed time, currency or RNG");
 }
}
void admittedObjects(Inputs &in,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 std::set<int> remaining{29,39,48,50,59};const auto resolver=XeenObjectVisualResolver::load(in.assets);
 for(unsigned n=1;n<=129 && !remaining.empty();++n) {
  if(n>=113 && n<=128)continue;const XeenMapIdentity id{XeenSide::Darkside,std::uint16_t(n)};
  const auto mob=in.maps.loadObjects(in.assets,id);
  for(std::size_t index=0;index<mob.entities.objects.size();++index) {
   const auto &object=mob.entities.objects[index];const int sprite=object.resourceId;
   if(!object.isActive() || !remaining.count(sprite))continue;
   const auto geometry=in.maps.loadGeometryMap(in.assets,id).geometry;
   const unsigned size=geometry.isOutdoors()?16:32;bool rendered=false;
   for(unsigned direction=0;direction<4 && !rendered;++direction)for(int distance=1;distance<=4 && !rendered;++distance) {
    constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
    const int x=int(object.x)-dx[direction]*distance,y=int(object.y)-dy[direction]*distance;
    if(x<0 || y<0 || unsigned(x)>=size || unsigned(y)>=size)continue;
    auto snapshot=source;snapshot.camera={id,x,y,XeenDirection(direction)};snapshot.darksideSong=xeenMapMusicRow(geometry)*7;snapshot=quiet(in,snapshot);
    Live live(in,snapshot);bool visible=false;
    const auto contains=[&](const auto &commands){for(const auto &command:commands)if(const auto *draw=command.object())
     if(draw->visual.identity.recordIndex==index && draw->visual.resourceId==sprite)visible=true;};
    if(geometry.isOutdoors())contains(XeenOutdoorScene().build(live.w,live.c,&resolver));
    else contains(XeenIndoorScene().build(live.w,live.c,&resolver));
    if(!visible)continue;
    std::vector<XeenObjectVisual> diagnostics;const auto frame=live.composer.compose(in.assets,live.w,live.p,live.c,{live.p.encounterContext->year},&diagnostics,0);
    picture(frame,out/("darkside-object-"+std::to_string(sprite)+"-map-"+std::to_string(n)+".ppm"));
    std::cout<<"Rendered Darkside sprite "<<sprite<<" map "<<n<<" object "<<index<<" camera "<<x<<','<<y<<" direction "<<direction<<'\n';
    remaining.erase(sprite);rendered=true;
   }
  }
 }
 check(remaining.empty(),"Newly admitted Darkside object sprite lacks an original map render");
}
void forcedText(Inputs &in,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 auto s=source;s.camera.x=30;s.camera.y=20;s.soundOptions.text=false;
 for(const auto &e:in.events.load(s.camera.mapId).records)if(e.x==30 && e.y==20 && e.line==0 && e.direction<4)s.camera.direction=XeenDirection(e.direction);
 Live live(in,s);bool forced=false;
 live.flow->reportManual=[&](const auto &result){if(const auto *pending=std::get_if<XeenEventExecutionSuspended>(&result))forced=forced || pending->request.forceText;};
 live.act(InteractionAction{});check(forced && !XeenSoundOptions(live.p.soundOptions).text,"Castleview Text-off override missing or made durable");
 picture(live.flow->frame(),out/"castleview-text-off.ppm");
}
XeenSaveSnapshot combat(Inputs &in,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 const auto mob=in.maps.loadObjects(in.assets,source.camera.mapId);const auto events=in.events.load(source.camera.mapId);
 const auto statistics=XeenMonsterFormat::parse(*in.assets.readMonsterStatistics(XeenSide::Darkside));
 auto s=source;bool found=false;
 for(const auto &a:mob.entities.monsters)if(!found && a.isActive() && a.resourceId>=0 && unsigned(a.resourceId)<statistics.size() && statistics[a.resourceId].baseHp()) {
  const int x=a.x,y=a.y-1;if(x<0 || x>=32 || y<0 || y>=32)continue;
  bool occupied=false;for(const auto &other:mob.entities.monsters)occupied=occupied || (other.isActive() && other.x==x && other.y==y);
  for(const auto &event:events.records)occupied=occupied || (event.x==x && event.y==y);
  if(occupied)continue;s.camera.x=x;s.camera.y=y;s.camera.direction=XeenDirection::North;found=true;
 }
 check(found,"No original Castleview combat approach");s.maps.erase(std::remove_if(s.maps.begin(),s.maps.end(),[&](const auto &m){return m.id==s.camera.mapId;}),s.maps.end());s.headCache.reset();
 Live live(in,s);live.act(WaitAction{});
 for(unsigned n=0;n<200 && (!live.flow->encounter()->combat() || live.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady);++n) {
  live.now+=110;live.flow->beginCycle(++live.cycle);if(auto frame=live.flow->updatePresentation())live.present(*frame);
 }
 check(live.flow->encounter()->combat() && live.flow->encounter()->combat()->phase()==XeenCombatPhase::PlayerReady,"Original Darkside combat did not reach player input");
 for(const auto &a:live.w.sessionState().regionalActors(s.camera.mapId))if(a.statistics)check(a.statistics->raw==statistics[a.original.resourceId].raw,"Darkside combat used another MON bank");
 picture(live.flow->frame(),out/"castleview-combat.ppm");live.act(InteractionAction{});
 check(live.w.sessionState().darksideSong()==s.darksideSong,"Combat selected a map song again");return s;
}


void combatPortal(Inputs &in,const GameInstallation &installation,const XeenSaveSnapshot &source,bool contactMove=false) {
 check(!XeenLearnedSpellRules::supportedIn(unsigned(S::TownPortal),true) &&
     !XeenLearnedSpellRules::supportedIn(unsigned(S::LloydsBeacon),true) && !xeenItemUseSpell(28,true),
     "A previously forbidden combat travel path now requires a candidate-RNG witness");
 const auto portalSide=xeenMapResources(source.camera.mapId).townPortal;
 const auto clouds=in.maps.loadGeometryMap(in.assets,28).geometry;
 const XeenCamera destination=portalSide==XeenSide::Clouds?XeenCamera{28,int(clouds.runX),int(clouds.runY),source.camera.direction}:
  XeenCamera{{XeenSide::Darkside,4},14,11,XeenDirection::South};
 for(unsigned mode=0;mode<3;++mode) {
  auto s=at(in,source,destination);s.camera=source.camera;s.darksideSong=source.darksideSong;
  s.soundOptions={mode!=1,mode!=1,true,false};
  for(auto owner:s.activeRosterIds)s.characters[owner].miscellaneous[0]={1,55,3,0};
  Live live(in,s);
  if(mode<2){live.program.audio=XeenAudio::installed(installation,in.assets,0,false);live.program.audio->observeOptions(s.soundOptions);}
  const auto tick=[&]{live.now+=16;live.flow->beginCycle(++live.cycle);if(auto frame=live.flow->updatePresentation())live.present(*frame);};
  if(contactMove)live.act(NavigationAction::MoveForward);else live.act(WaitAction{});
  for(unsigned n=0;n<2000 && (!live.flow->encounter()->combat() || live.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady);++n)tick();
  check(live.flow->encounter()->combat() && live.flow->encounter()->combat()->phase()==XeenCombatPhase::PlayerReady,"Darkside Portal item witness did not enter Castleview combat");
  const auto contactCamera=live.c;const auto random=*live.w.sessionState().journeyRandom();
  std::cout<<"Combat Portal audio "<<mode<<std::endl;
  const auto portal=[&]{live.act(UseItemAction{});live.key('1');live.key('u');
   live.act(AcknowledgeAction{});live.key('1');live.key(InputKey::Enter);};
  if(!mode) {
   auto *fight=&XeenRestTestAccess::combat(*live.flow);const auto owner=live.p.party.activeRosterIds()[fight->participant()];
   const auto item=live.p.roster.at(owner).miscellaneous[0];bool interrupted=false;
   fight->setProbe([&]{if(fight->cast() && !fight->cast()->committed){interrupted=true;throw std::runtime_error("Interrupted Portal confirmation");}});
   portal();fight->setProbe({});
   check(interrupted && !XeenRestTestAccess::fatal(*live.flow) && fight->cast() && !fight->cast()->committed,"Portal preparation did not fail recoverably");
   check(*live.w.sessionState().journeyRandom()==random && fight->random().continuation()==random,"Uncommitted Portal advanced the combat RNG");
   for(unsigned n=0;n<3 && fight->cast();++n)live.key(InputKey::Escape);
   check(!fight->cast() && xeen_state::sameCamera(live.c,contactCamera) && xeenSameItem(live.p.roster.at(owner).miscellaneous[0],item),"Canceled Portal changed its source or item");
  }
  portal();
  check(!XeenRestTestAccess::fatal(*live.flow),"Darkside combat Portal caused an integrity failure");
  check(xeen_state::sameCamera(live.c,destination),"Darkside combat Portal did not arrive at 4:(14,11) South");
  arrivalMusic(live,random);
  for(unsigned n=0;n<2000 && !live.flow->canSave();++n) {
   check(!XeenRestTestAccess::fatal(*live.flow),"Darkside combat Portal failed during arrival settlement");
   const auto dialog=live.flow->inputContext(live.flow->frame().presentation()).dialog;
   if(dialog && dialog->anyKey)live.act(AcknowledgeAction{});
   tick();
  }
  check(live.flow->canSave() && !live.flow->encounter()->combat(),"Darkside combat Portal did not retire combat");
  arrivalMusic(live,random);
  const auto bytes=XeenSaveFormat::encode(live.snapshot());Live restored(in,XeenSaveFormat::decode(bytes));
  check(XeenSaveFormat::encode(restored.snapshot())==bytes,"Darkside combat Portal arrival did not restore exactly");
 }
}

void originalRoute(Inputs &in,const GameInstallation &installation,const std::filesystem::path &out) {
 const auto path=std::filesystem::temp_directory_path()/("mmodern-m101-route-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".mmsave");
 developer::generateTestSave(installation,"m101-pyramid-clouds",path);Live live(in,XeenSaveFile::read(path));
 std::string entryReason;live.flow->reportManual=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r))entryReason=e->message;};
 for(unsigned n=0;n<6 && live.c.mapId.side==XeenSide::Clouds;++n){live.act(InteractionAction{});live.settle();}
 if(live.c.mapId!=XeenMapIdentity(XeenSide::Darkside,29))std::cerr<<"Original crossing refusal: "<<entryReason<<" at "<<live.c.mapId.number<<" phase="<<int(live.flow->encounter()->state().phase())<<std::endl;
 check(live.c.mapId==XeenMapIdentity(XeenSide::Darkside,29),"Original preset did not cross to Castleview");
 const auto walk=[&](Live &live,int x,int y,XeenDirection facing){
  auto query=live.w.transitionCandidate();using Cell=std::pair<int,int>;std::map<Cell,std::pair<Cell,XeenDirection>> previous;
  // Plan through ordinary closed doors/grates; the live route below opens
  // them through Space and the party's thief, never through this query copy.
  for(int row=0;row<32;++row)for(int col=0;col<32;++col)for(unsigned d=0;d<4;++d){
   const XeenCamera c{live.c.mapId,col,row,XeenDirection(d)};const auto cell=query->sampleCell(c.mapId,col,row);
   if(!cell)continue;const auto wall=wallAt(*cell->cell,c.direction);
   if(wall==9 || wall==13)query->setBarrier(c,wall==9?6:1,true);
  }
  std::queue<Cell> queue;const Cell start{live.c.x,live.c.y},goal{x,y};queue.push(start);previous[start]={start,XeenDirection::North};
  const auto file=in.events.load(live.c.mapId);
  while(!queue.empty() && !previous.count(goal)){const auto from=queue.front();queue.pop();
   for(unsigned d=0;d<4;++d){XeenCamera step{live.c.mapId,from.first,from.second,XeenDirection(d)};
    if(XeenMovement().apply(*query,step,NavigationAction::MoveForward,XeenMovement::capabilities(live.p))!=XeenMovementResult::Moved || step.mapId!=live.c.mapId)continue;
    const Cell cell{step.x,step.y};if(previous.count(cell))continue;
    previous[cell]={from,XeenDirection(d)};queue.push(cell);
   }
  }
  if(!previous.count(goal))return false;
  std::vector<XeenDirection> steps;
  for(auto cell=goal;cell!=start;cell=previous.at(cell).first)steps.push_back(previous.at(cell).second);
  const auto turn=[&](XeenDirection direction){while(live.c.direction!=direction){live.act(NavigationAction::TurnRight);live.settle();}};
  for(auto it=steps.rbegin();it!=steps.rend();++it){turn(*it);
   for(unsigned tries=0;;++tries){auto probe=live.c;auto current=live.w.transitionCandidate();
    if(XeenMovement().apply(*current,probe,NavigationAction::MoveForward,XeenMovement::capabilities(live.p))==XeenMovementResult::Moved)break;
    check(tries<20,"Original route could not unlock a door");live.act(InteractionAction{});live.settle();
   }
   live.act(NavigationAction::MoveForward);live.settle();
  }
  turn(facing);check(live.c.x==x && live.c.y==y,"Original walk missed its coordinates");return true;
 };
 check(walk(live,22,24,XeenDirection::West),"Original route cannot reach Jethro");live.act(InteractionAction{});
 for(unsigned n=0;!live.flow->canSave();++n){if(n==500){XeenRestTestAccess::state(*live.flow);picture(live.flow->frame(),out/"quest-stuck.ppm");throw std::runtime_error("Castleview quest dialogue did not close");}
  const auto dialog=live.flow->inputContext(live.flow->frame().presentation()).dialog;
  if(dialog && dialog->key('y'))live.key('y');else if(dialog && dialog->anyKey)live.act(AcknowledgeAction{});live.tick();}
 check(live.p.questFlags.isSet(XeenSide::Darkside,16) && !live.p.questFlags.isSet(XeenSide::Clouds,16),"Original Castleview Event failed to set its Darkside quest flag");
 const auto checkpoint=live.snapshot();XeenSaveFile::write(path,checkpoint);Live loaded(in,XeenSaveFile::read(path));
 check(XeenSaveFormat::encode(loaded.snapshot())==XeenSaveFormat::encode(checkpoint),"Original Castleview save/load replayed work");
 // Continue the same live input route after its disk round-trip checkpoint.
 std::string refusal;bool visited=false;loaded.flow->reportManual=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r))refusal=e->message;};
 for(const auto &e:in.events.load(loaded.c.mapId).records)if(e.opcode==17 && e.line==0 && e.parameters.size()==1 && e.parameters[0]<=5 &&
     walk(loaded,e.x,e.y,e.direction<4?XeenDirection(e.direction):XeenDirection::North)) {
  std::cout<<"Original route service "<<unsigned(e.parameters[0])<<" at "<<int(e.x)<<','<<int(e.y)<<std::endl;
  loaded.act(InteractionAction{});
  for(unsigned i=0;i<512 && !(loaded.flow->inputContext(loaded.flow->frame().presentation()).dialog &&
      loaded.flow->inputContext(loaded.flow->frame().presentation()).dialog->key(InputKey::Escape));++i)loaded.tick();
  check(loaded.w.sessionState().journeyActivity()==XeenJourneyActivity::Service,"Original route did not enter the live Darkside service");
  loaded.key(InputKey::Escape);loaded.settle();visited=true;break;
 }
 check(visited && refusal.empty(),"Original Darkside service visit did not complete");
 check(walk(loaded,26,24,XeenDirection::North),"Original route cannot reach the Pyramid");loaded.act(InteractionAction{});loaded.settle();check(loaded.c.mapId==XeenMapIdentity(23),"Original route failed to return through Pyramid");
 picture(loaded.flow->frame(),out/"original-route-return.ppm");std::filesystem::remove(path);
}

void panelAndDefeat(Inputs &in,const GameInstallation &installation,XeenSaveSnapshot source,const std::filesystem::path &out,unsigned mode) {
 std::cout<<"Panel/defeat mode "<<mode<<std::endl;
 XeenProgramState program{XeenWallClock::fixed(0)};XeenEventFlow *flow=nullptr;std::uint64_t now=0;CloudsMapComposer composer;
 const auto catalogue=loadXeenItemCatalog(in.assets);XeenSessionOutcome outcome;
 if(mode<2)program.audio=XeenAudio::installed(installation,in.assets,0,false);
 source.soundOptions.music=mode!=1;source.soundOptions.efx=true;
 XeenWorld *world=nullptr;const XeenCamera *camera=nullptr;const XeenPartyState *party=nullptr;const XeenGameFlags *flags=nullptr;
 XeenGameplayServices services{program,in.resources(),[]{return XeenGameFlags{};},in.mapLoader(),in.objectLoader(),
  [&](auto id){return in.texts.load(id);},in.font,
  [&](auto &w,const auto &p,const auto &c,auto phase){bool animated=false;return XeenEventFlow::Composition{composer.compose(in.assets,w,p,c,{p.encounterContext->year},nullptr,phase,&animated),animated};},{},
  [&](auto &f,const auto &position){flow=&f;camera=&position;f.dialogText=&in.assets.uiText();
   f.drawDialogSprite=[&](auto &frame,const char *n,unsigned i,int x,int y){try{in.assets.drawDialogSprite(frame,n,i,x,y,xeenMapResources(camera->mapId).ambient);}catch(const std::exception &e){std::cerr<<"Dialog "<<n<<" frame "<<i<<": "<<e.what()<<std::endl;throw;}};
   f.drawCombatButtons=[&](auto &frame){CloudsUiComposer().drawCombatButtons(in.assets,frame,xeenMapResources(camera->mapId).ambient);};
   f.loadSampleDuration=[&](const char *n,XeenSceneArchive a){try{return xeenVocDurationNanoseconds(in.assets.readArchiveResource(n,a));}catch(const std::exception &e){std::cerr<<"Sample "<<n<<": "<<e.what()<<std::endl;throw;}};
  },{},[&](auto &w,auto &,const auto &p,auto &c,const auto &f){world=&w;party=&p;camera=&c;flags=&f;}};
 services.clock=[&]{return now;};services.outcome=&outcome;
 if(mode==4){services.journeyStart=XeenCamera{23,8,10,XeenDirection::North};
  services.resources.loadInitialContext=[&]{return XeenGameplayContextFormat::parse(in.pty);};
  services.resources.loadInitialBankBalances=[&]{return XeenCharacterFormat::parseBankBalances(in.pty);};
  services.resources.loadInitialPurse=[&]{return XeenCharacterFormat::parseMonsterPurse(in.pty);};
  services.resources.loadInitialWorldFlags=[&]{return XeenQuestFlagFormat::parseWorldFlags(in.pty);};
 }
 else services.restoreSnapshot=std::make_shared<const XeenSaveSnapshot>(source);
 services.catalog=&catalogue.catalog;services.validateEncounterSprite=[](auto,auto){};services.validateCombatSprite=[](auto,auto){};
 services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){bool animated=false;return XeenEventFlow::Composition{composer.compose(in.assets,w,p,c,{p.encounterContext->year},nullptr,phase,&animated,actor),animated};};
 services.panel=[&](const auto &base,bool combat,bool restricted,bool saveable,auto slot,const auto &name){
  auto panel=XeenTitleFlow::original(in.assets,in.font,[]{return std::array<XeenSaveFile::Slot,10>{};},[](unsigned){return std::filesystem::path("unused");});
  panel.background=base;panel.panel=true;panel.combat=combat;panel.saveRestricted=restricted;panel.saveable=saveable;panel.currentSlot=slot;panel.currentName=name;
  return std::make_unique<XeenTitleFlow>(std::move(panel));};
 services.show=[&](const auto &first,const auto &handler,const auto &,const auto &idle,const auto &){
  if(mode==5)check(XeenRestTestAccess::death(*flow) && world->sessionState().journeyRandom()==source.journey->random &&
      party->encounterContext==source.journey->context,"Restored defeat was not prepared before exposure or replayed time/RNG");
  auto visible=first;std::uint64_t cycle=0;
  const auto present=[&]{handler.framePresented(visible.presentation());handler.completeInputHandoff(visible.presentation());};present();
  const auto send=[&](const PlayerAction &a){handler.beginCycle(++cycle);const auto input=handler.displayedInput();check(bool(input),"Live Application input absent");if(auto frame=handler.withPresentedInput(a,*input,visible.presentation())){visible=*frame;present();}};
  const auto tick=[&]{now+=110;handler.beginCycle(++cycle);if(auto frame=idle()){visible=*frame;present();}};
  tick();
  if(mode==4){
   check(!world->sessionState().mapState({XeenSide::Darkside,29}),"Fresh game already has Darkside history");
   send(InteractionAction{});
   for(unsigned n=0;!flow->canSave() && n<100;++n)tick();
   check(flow->canSave() && camera->mapId==XeenMapIdentity(XeenSide::Darkside,29) && camera->x==25 && camera->y==21,
       "Fresh Application failed its first Pyramid crossing");return true;
  }
  if(mode<3){
   const auto random=world->sessionState().journeyRandom();const auto context=party->encounterContext;
   send(ControlPanelAction{});send(DialogKeyAction{in.assets.uiText().buttons("PANEL").at(8).key});
   const auto confirm=drawXeenConfirm(visible,in.font,std::string(in.assets.uiText().scalar("MR_WIZARD")),false,flow->drawDialogSprite);
   check(confirm.pixels==visible.pixels,"Mr Wizard confirmation text is not visible");if(!mode)picture(visible,out/"darkside-wizard-confirm.ppm");
   send(DialogKeyAction{'y'});check(camera->mapId==XeenMapIdentity(28) && camera->x==18 && camera->y==4 && camera->direction==XeenDirection::East,"World Mr Wizard did not return to Vertigo");
   check(!party->purse->gems && world->sessionState().journeyRandom()==random && party->encounterContext==context,"Mr Wizard changed time/RNG or retained gems");
   if(!mode)picture(visible,out/"wizard-vertigo.ppm");return true;
  }
  bool fought=false,laugh=false;std::optional<XeenJourneyRandomState> deathRandom;std::optional<XeenGameplayContext> deathContext;
  for(unsigned n=0;!handler.finished();++n){check(n<3000,"Darkside defeat recovery did not complete");
   if(!XeenRestTestAccess::death(*flow)){
    if(!flow->encounter()->combat())send(WaitAction{});
    else if(flow->encounter()->combat()->phase()==XeenCombatPhase::PlayerReady)send(BlockAction{});
    fought=fought || bool(flow->encounter()->combat());
   }else {
    if(!deathRandom){deathRandom=world->sessionState().journeyRandom();deathContext=party->encounterContext;picture(visible,out/"darkside-defeat-fire.ppm");}
    check(world->sessionState().journeyRandom()==deathRandom && party->encounterContext==deathContext,"Darkside defeat presentation advanced state");
    if(XeenRestTestAccess::laugh(*flow)){
     check(XeenRestTestAccess::index(*flow)<=9 && XeenRestTestAccess::restarts(*flow)<=2,"Darkside laugh used Clouds loop");
     const auto index=XeenRestTestAccess::index(*flow);const auto expected=drawXeenDeathLaugh(XeenRestTestAccess::final(*flow),index?std::optional<unsigned>{index-1}:std::nullopt,flow->drawDialogSprite);
     check(expected.pixels==visible.pixels,"Darkside Alamar composition mismatch");
     if(index==1 && !laugh){picture(visible,out/"darkside-defeat-alamar.ppm");laugh=true;}
    }
   }
   tick();
  }
  check((fought || mode==5) && laugh && flow->partyDeathComplete(),"Darkside defeat skipped combat or Alamar presentation");return true;
 };
 const auto result=mode==4?Application(program).playGameplay(services,source.camera,{},false,XeenEncounterEntry::Journey,7):
     Application(program).playGameplay(services,source.camera,std::filesystem::path("unused"),true);
 check(result==0,"Darkside Application scenario failed");
 if(mode==3 || mode==5){check(outcome.kind==XeenSessionOutcome::Kind::Title,"Darkside defeat did not recover to title");
  auto titleServices=XeenTitleFlow::original(in.assets,in.font,[&]{std::array<XeenSaveFile::Slot,10> slots;slots[0].state=XeenSaveFile::Slot::State::Available;slots[0].snapshot=source;return slots;},[](unsigned){return std::filesystem::path("unused");});
  XeenTitleFlow title(std::move(titleServices));title.returnToTitle();
  for(unsigned n=0;n<80;++n){title.presented(title.frame().presentation());title.completeInput(title.frame().presentation());
   if(const auto deadline=title.nextPresentationDeadline())title.animateAt(*deadline);else break;
  }
  title.presented(title.frame().presentation());title.completeInput(title.frame().presentation());
  title.handle(AcknowledgeAction{},title.frame().presentation());
  title.presented(title.frame().presentation());title.completeInput(title.frame().presentation());
  picture(title.frame(),out/"darkside-defeat-title.ppm");
  check(bool(title.handle(DialogKeyAction{'l'},title.frame().presentation())) && title.screen()==XeenTitleFlow::Screen::LoadSlots,
      "Darkside defeat title did not accept Load");
 }
}

void pyramidCombatAudio(Inputs &in,const GameInstallation &installation,const std::filesystem::path &out) {
 for(const char *name:{"pow0.voc","pow1.voc","pow2.voc","pow3.voc","pow4.voc","pow5.voc","miss.voc","scream.voc","unnh.voc"})
  check(in.assets.readArchiveResource(name,XeenSceneArchive::Clouds)==in.assets.readArchiveResource(name,XeenSceneArchive::DarksideOnly),(std::string("Gameplay VOC differs between sides: ")+name).c_str());
 const auto path=std::filesystem::temp_directory_path()/("mmodern-m101-combat-audio-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".mmsave");
 developer::generateTestSave(installation,"m101-pyramid-clouds",path);const auto source=XeenSaveFile::read(path);std::filesystem::remove(path);
 check(in.assets.hasSceneResource("gremlin.voc",XeenSceneArchive::DarksideOnly) &&
     !in.assets.hasSceneResource("gremlin.voc",XeenSceneArchive::Clouds),"Original goblin voice no longer distinguishes the archive banks");
 std::optional<XeenSaveSnapshot> baseline;
 for(unsigned mode=0;mode<3;++mode){
  // CD dialogue animation owns separate draws; isolate attack VOC output.
  auto snapshot=source;snapshot.soundOptions.efx=mode!=1;snapshot.soundOptions.speech=false;Live live(in,snapshot);
  unsigned goblinVoices=0;bool wrongArchive=false;
  if(mode<2){live.program.audio=XeenAudio::installed(installation,in.assets,0,false);live.program.audio->observeOptions(snapshot.soundOptions);
   live.program.audio->digitalSink([&](const auto &name,auto archive){if(name=="gremlin.voc"){++goblinVoices;wrongArchive=wrongArchive || archive!=XeenSceneArchive::Darkside;}});
  }
  bool fought=false;unsigned blocks=0;
  const auto settle=[&]{for(unsigned n=0;!live.flow->canSave();++n){check(n<3000,"Pyramid goblin/audio route never settled");
   if(const auto *combat=live.flow->encounter()->combat()){
    fought=true;if(combat->phase()==XeenCombatPhase::PlayerReady){
     if(blocks++<6)live.act(BlockAction{});else live.act(AttackAction{});
    }
   }
   const auto input=live.flow->inputContext(live.flow->frame().presentation()).dialog;
   if(input && input->anyKey)live.act(AcknowledgeAction{});else if(input && input->key('n'))live.key('n');
   live.now+=16;live.flow->beginCycle(++live.cycle);if(auto frame=live.flow->updatePresentation())live.present(*frame);
  }};
  const auto act=[&](PlayerAction action){live.act(action);settle();};
  act(InteractionAction{});check(live.c.mapId==XeenMapIdentity(XeenSide::Darkside,29),"Goblin witness failed to cross the Pyramid");
  act(NavigationAction::TurnRight);act(NavigationAction::MoveForward);act(NavigationAction::TurnLeft);
  for(unsigned n=0;n<3;++n)act(NavigationAction::MoveForward);
  for(unsigned n=0;n<20;++n)act(WaitAction{});
  check(fought && live.c.mapId==XeenMapIdentity(XeenSide::Darkside,29),"Original goblin witness never entered combat");
  check(!wrongArchive && (mode==0?goblinVoices>0:goblinVoices==0),"Goblin voice did not honor its Darkside archive or Efx gate");
  auto after=live.snapshot();after.soundOptions=source.soundOptions;
  if(baseline){if(XeenSaveFormat::encode(after)!=XeenSaveFormat::encode(*baseline)){
    XeenSaveFile::write(out/"m101-voice-baseline.mmsave",*baseline);XeenSaveFile::write(out/"m101-voice-after.mmsave",after);
   }check(XeenSaveFormat::encode(after)==XeenSaveFormat::encode(*baseline),"Attack voice routing changed gameplay state/time/RNG");}
  else baseline=std::move(after);
 }
}

// M105: the three Events M104's refusal sweep left, with live input.
// Darkstone Tower's four heads give world flags 125-128; flag 128 is DOS's
// Clouds quest flag 0 (Part 0). The Dungeon of Death's book is two
// DisplayLarge lines written back to back in window 11.
void mixedEvents(Inputs &in,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 using Side=XeenSide;using D=XeenDirection;
 const XeenMapIdentity tower{Side::Darkside,116},death{Side::Darkside,121};
 const std::array<std::tuple<int,int,unsigned>,4> heads{{{4,6,125},{4,10,126},{10,10,127},{10,6,128}}};
 for(const auto &[x,y,flag]:heads) {
  const auto s=at(in,source,{tower,x,y,D::North});Live f(in,s);
  check(flag==128?!f.p.questFlags.isSet(Side::Clouds,0):!f.p.worldFlags->values[flag],"Head preset already holds its note");
  f.act(InteractionAction{});
  if(flag==128)picture(f.flow->frame(),out/"m105-head-16.ppm");
  f.settle();
  for(unsigned n=0;n<128;++n)check(f.p.worldFlags->values[n]==(n==flag || s.journey->worldFlags->values[n]),"Head gave the wrong world flag");
  check(f.p.questFlags.isSet(Side::Clouds,0)==(flag==128) && !f.p.questFlags.isSet(Side::Darkside,0),"Head 16 did not set DOS's flag-128 bit");
  check(xeenQuestDisplay(f.p).notes[flag],"Head note missing from Auto Notes");
  const auto saved=f.snapshot();Live restored(in,saved);
  check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(saved) && xeenQuestDisplay(restored.p).notes[flag],"Head note lost or replayed on reload");
 }
 const auto texts=in.texts.load(death);
 check(texts.largeResourcePresent && texts.largeStrings.size()>189,"Dungeon of Death large text missing");
 // Independent oracle: the two lines written back to back are one stream.
 XeenTextRenderOptions o;o.originalControls=true;o.stopAtBottom=true;
 o.bounds={16,16,216,132};o.windowBounds={8,8,224,140};o.x=16;o.y=16;o.drawWindow=true;
 const auto window=[](const IndexedFrame &f) {std::vector<std::uint8_t> r;for(int y=8;y<140;++y)r.insert(r.end(),f.pixels.begin()+y*f.width+8,f.pixels.begin()+y*f.width+224);return r;};
 std::vector<std::vector<std::uint8_t>> runs[2];
 for(const bool text:{true,false}) {
  auto s=at(in,source,{death,21,1,D::North});s.soundOptions.text=text;
  Live f(in,s);std::vector<IndexedFrame> pages;
  f.act(InteractionAction{});
  for(unsigned n=0;n<200;++n) {
   const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;
   if(!input)break;
   if(!input->anyKey) {check(bool(input->key(InputKey::F1)),"Dungeon book expected WhoWill");f.act(SelectMemberAction{0});f.tick();continue;}
   pages.push_back(f.flow->frame());f.act(AcknowledgeAction{});f.tick();
  }
  f.settle();
  std::vector<IndexedFrame> oracle;std::string rest=texts.largeStrings[5]+texts.largeStrings[189];
  std::size_t boundary=0,written=0;auto options=o;
  for(IndexedFrame base=pages.at(0);;) {
   const auto prefix=oracle.empty()?std::string():std::string("\r");
   const auto page=XeenTextRenderer(in.font).render(base,prefix+rest,options);
   oracle.push_back(page.pages.front());
   const auto consumed=std::min(rest.size(),page.pageSourceEnds.back()-prefix.size());
   written+=consumed;if(!boundary && written>texts.largeStrings[5].size())boundary=oracle.size();
   if(consumed==rest.size())break;
   rest=rest.substr(consumed);base=oracle.back();options.drawWindow=false;
   options.size=page.writeSize;options.colorIndex=page.writeColor;options.alignment=page.writeAlignment;
  }
  if(pages.size()!=oracle.size())std::cerr<<"Book pages live="<<pages.size()<<" oracle="<<oracle.size()<<'\n';
  check(pages.size()==oracle.size() && pages.size()>2 && boundary>1,"Dungeon book page count differs from the back-to-back writer");
  for(std::size_t n=0;n<pages.size();++n) {
   if(window(pages[n])!=window(oracle[n])) {picture(pages[n],out/"m105-book-live.ppm");picture(oracle[n],out/"m105-book-oracle.ppm");
    std::cerr<<"Book page "<<n<<" differs\n";}
   check(window(pages[n])==window(oracle[n]),"Dungeon book page differs from DOS's writer");
   runs[text].push_back(window(pages[n]));
  }
  if(text) {picture(pages.front(),out/"m105-book-first.ppm");picture(pages[boundary-1],out/"m105-book-join.ppm");picture(pages.back(),out/"m105-book-last.ppm");
   std::cout<<"Dungeon book: "<<pages.size()<<" pages, line 189 joins page "<<boundary<<'\n';}
  check(!f.flow->presenter().hasEventWindows(),"Dungeon book left window 11 open after the run");
  f.unchanged(s);
  const auto saved=f.snapshot();Live restored(in,saved);
  check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(saved),"Dungeon book reload replayed state");
 }
 check(runs[0]==runs[1],"Text off changed the Dungeon book");
 // Window 11 holding DisplayMain's text, then a bottom-window text: DOS's
 // writer cursor would sit in window 12, which is not modeled.
 const XeenCamera c{death,21,1,D::North};
 const auto row=[&](unsigned line,unsigned op,std::vector<std::uint8_t> p) {
  XeenEventRecord r;r.x=c.x;r.y=c.y;r.direction=4;r.line=line;r.opcode=op;r.parameters=std::move(p);r.lengthField=5+r.parameters.size();return r;
 };
 XeenEventFile script{death,"DisplayLarge after window 12",true,{row(0,53,{3}),row(1,41,{3}),row(2,50,{5})}};
 const auto s=at(in,source,c);Live f(in,s,&script);std::string reason;
 f.flow->reportManual=[&](const auto &result){if(const auto *error=std::get_if<XeenEventExecutionError>(&result))reason=error->message;};
 f.act(InteractionAction{false});for(unsigned n=0;n<8 && reason.empty();++n){const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;if(input && input->anyKey)f.act(AcknowledgeAction{});f.tick();}
 check(reason.find("not supported yet")!=std::string::npos,"DisplayLarge after another window was not refused");
 notice(f.flow->frame(),in.font,"This Event is not supported yet");f.settle();f.unchanged(s);
 std::cout<<"M105 Darkstone heads, Dungeon of Death book and DisplayLarge refusal passed\n";
}

// Original maps and live input; synthetic Event rows only exercise pending states
// the four original FlipWorld routes cannot leave behind (Part 0 evidence).
void mixedTravel(Inputs &in,const GameInstallation &installation,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 using D=XeenDirection;using Side=XeenSide;using R=XeenLearnedSpellRules;
 const auto row=[](const XeenCamera &c,unsigned line,unsigned op,std::vector<std::uint8_t> p={}) {
  XeenEventRecord r;r.x=c.x;r.y=c.y;r.direction=4;r.line=line;r.opcode=op;r.parameters=std::move(p);r.lengthField=5+r.parameters.size();return r;
 };
 const auto audio=[&](Live &f,unsigned mode) {if(mode<2){f.program.audio=XeenAudio::installed(installation,in.assets,0,false);f.program.audio->observeOptions(f.p.soundOptions);}};
 const auto cast=[](Live &f,S spell) {f.act(CastSpellAction{});f.act(SelectMemberAction{0});f.key('c');};
 const auto mage=[](XeenSaveSnapshot &s,S spell) {s.journey->context->effects[XeenPartyEffect::Levitate]=200;auto &c=s.characters[s.activeRosterIds[0]];c.weapons={};c.armor={};c.accessories={};c.characterClass=spell==S::TownPortal?XeenCharacterClass::Cleric:XeenCharacterClass::Sorcerer;c.hasSpells=true;c.currentSp=1000;c.permanentLevel=50;c.learnedSpells.emplace();c.learnedSpells->fill(0);c.currentSpell=*R::slotForSpell(spell==S::TownPortal?XeenSpellCategory::Clerical:XeenSpellCategory::Wizardry,unsigned(spell));c.learnedSpells->at(c.currentSpell)=1;};
 const auto saved=[&](const XeenCamera &c) {return at(in,source,c);};
 const auto exact=[&](Live &f) {const auto s=f.snapshot();Live restored(in,s);check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(s),"Mixed travel exact restore replayed state");};
 for(unsigned side=0;side<2;++side) {const auto decoded=XeenEventDecoder::decode(row({28,5,5,D::North},0,59,{std::uint8_t(side)}));check(std::get<XeenEventFlipWorld>(std::get<XeenDecodedEventInstruction>(decoded).operation).side==side,"FlipWorld operand decoded as a toggle");}
 for(const auto &parameters:std::vector<std::vector<std::uint8_t>>{{},{0,1},{2}})check(std::holds_alternative<XeenEventDecodeError>(XeenEventDecoder::decode(row({28,5,5,D::North},0,59,parameters))),"Invalid FlipWorld operand admitted");
 // All original keyed entrance rows, both outcomes and all audio states.
 for(const auto &site:xeenEntranceSites())if(site.side==Side::Clouds && site.quest>=35) {
  const auto events=in.events.load(site.map);
  const auto e=std::find_if(events.records.begin(),events.records.end(),[&](const auto &r){return r.opcode==17 && r.line==0 && r.parameters==std::vector<std::uint8_t>{std::uint8_t(site.action)};});
  check(e!=events.records.end(),"Missing original cross-side entrance");
  const XeenCamera start{site.map,e->x,e->y,e->direction<4?D(e->direction):D::North};
  for(bool keyed:{false,true})for(unsigned mode=0;mode<3;++mode) {
   std::cout<<"Entrance "<<site.map<<" key="<<keyed<<" audio="<<mode<<std::endl;
   auto s=at(in,saved(site.destination),start);s.questItems[site.quest]=keyed?256:0;s.soundOptions.music=mode!=1;s.soundOptions.efx=mode==0;s.journey->context->lightAndResistances[0]=200;
   Live f(in,s);audio(f,mode);unsigned loads=0,settlements=0;
   f.flow->observeEventInputClear=[&](auto pc,const auto &){if(pc==0xf590)++loads;};
   f.flow->reportManual=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r))throw std::runtime_error(e->message);if(std::holds_alternative<XeenManualEventCompleted>(r))++settlements;};
   f.act(InteractionAction{false});f.settle();
   check(xeen_state::sameCamera(f.c,keyed?site.destination:start) && loads==1 && settlements==1,"Keyed entrance outcome/load/settlement differs");
   check(f.w.pendingLoadSide()==f.c.mapId.side && !XeenEventDispatchTestAccess::inputPassPending(*f.flow),"Entrance did not reset pending side/input");
   check(f.p.encounterContext->minutes==s.journey->context->minutes && f.p.encounterContext->day==s.journey->context->day && f.p.encounterContext->year==s.journey->context->year,"Entrance charged game/service time");
   if(keyed && mode==2)picture(f.flow->frame(),out/("m104b-entrance-"+std::to_string(site.map)+".ppm"));
   exact(f);
   if(keyed) {
    const auto backEvents=in.events.load(site.destination.mapId);
    const auto flip=std::find_if(backEvents.records.begin(),backEvents.records.end(),[](const auto &r){return r.opcode==59;});
    check(flip!=backEvents.records.end(),"Keyed entrance has no original return");
    auto returning=at(in,f.snapshot(),{site.destination.mapId,flip->x,flip->y,D::South});
    Live back(in,returning);audio(back,mode);const auto random=*back.w.sessionState().journeyRandom();
    unsigned clears=0;back.flow->observeEventInputClear=[&](auto pc,const auto &){if(pc==0xf590)++clears;};
    back.act(InteractionAction{false});back.act(YesAction{});back.settle();
    const auto n=site.destination.mapId.number;
    const XeenCamera expected=n==113?XeenCamera{7,11,0,D::South}:n==117?XeenCamera{13,10,5,D::East}:n==121?XeenCamera{19,3,4,D::South}:XeenCamera{2,11,9,D::South};
    check(xeen_state::sameCamera(back.c,expected) && clears==1,"Original mixed return/input clear differs");
    arrivalMusic(back,random);exact(back);
   }
  }
 }
 // The unused Darkside map-19 row is a table rule, never an invented live route.
 const auto *unused=xeenEntranceSite(9,{{Side::Darkside,19},3,4,D::North});
 XeenQuestItems::Counts keys{};keys[50]=1;
 check(unused && xeenEntranceDestination(*unused,{{Side::Darkside,19},3,4,D::North},XeenQuestItems(keys)).mapId==XeenMapIdentity(Side::Darkside,121),"Unused generic entrance row changed");
 // Original Portal names and every target from each Portal selector.
 for(unsigned map:{125u,128u})for(unsigned town=1;town<=5;++town)for(unsigned mode=0;mode<3;++mode) {
  std::cout<<"Portal "<<map<<" town="<<town<<" audio="<<mode<<std::endl;
  const Side side=map==125?Side::Clouds:Side::Darkside;
  constexpr unsigned maps[]{4,19,22,6,98};constexpr int xs[]{14,5,2,8,2},ys[]{11,12,15,11,6};constexpr D ds[]{D::South,D::West,D::East,D::North,D::North};
  XeenCamera to=map==125?XeenCamera{27+town,0,0,D::West}:XeenCamera{{side,std::uint16_t(maps[town-1])},xs[town-1],ys[town-1],ds[town-1]};
  if(map==125) {const auto g=in.maps.loadGeometryMap(in.assets,to.mapId).geometry;to.x=std::int8_t(g.runX);to.y=std::int8_t(g.runY);}
  auto s=at(in,saved(to),{{Side::Darkside,std::uint16_t(map)},map==125?5:3,map==125?9:13,D::West});mage(s,S::TownPortal);s.soundOptions.music=mode!=1;
  Live f(in,s);audio(f,mode);const auto inputGeneration=f.w.inputMapLoadGeneration();
  cast(f,S::TownPortal);
  for(unsigned i=1;i<=5;++i)check(XeenEventDispatchTestAccess::travelName(*f.flow,i-1)==in.assets.mapName(R::townPortalMap(side,i)),"Portal dialog names/selector differ");
  const auto random=*f.w.sessionState().journeyRandom();f.key('0'+town);f.key(InputKey::Enter);f.settle();
  check(xeen_state::sameCamera(f.c,to) && f.w.inputMapLoadGeneration()==inputGeneration+1,"Mixed Portal arrival/input clear differs");arrivalMusic(f,random);
  check(f.p.encounterContext->minutes==s.journey->context->minutes+(f.w.map(to.mapId).geometry.isOutdoors()?10:1),"Portal elapsed time differs");exact(f);
 }
 // Beacon Set/default/Return retains the actual side, including fixed-song maps.
 for(unsigned n=113;n<=128;++n) {
  const XeenCamera start{{Side::Darkside,std::uint16_t(n)},n==128?3:5,n==128?13:5,D::North};
  auto s=saved(start);s.gameFlags[256+118]=true;mage(s,S::LloydsBeacon);auto &caster=s.characters[s.activeRosterIds[0]];caster.lloydMap=0;
  Live f(in,s);cast(f,S::LloydsBeacon);check(f.p.roster.at(s.activeRosterIds[0]).lloydMap==29 && f.p.roster.at(s.activeRosterIds[0]).lloydSide==1,"Mixed Beacon default used Portal/art side");f.key('s');f.settle();
  const auto &c=f.p.roster.at(s.activeRosterIds[0]);check(c.lloydSide==1 && c.lloydMap==n && c.lloydX==start.x && c.lloydY==start.y,"Mixed Beacon Set stored the art/Portal side");
  const auto random=*f.w.sessionState().journeyRandom();const auto generation=f.w.inputMapLoadGeneration();
  cast(f,S::LloydsBeacon);f.key('r');f.settle();check(xeen_state::sameCamera(f.c,start) && f.w.inputMapLoadGeneration()==generation && f.w.sessionState().journeyRandom()==random,"Same-map Beacon reloaded/drew music");
  // Pure admission probes cover restrictions for every original map.
  const auto &g=f.w.map(start.mapId).geometry;
  check(!R::travel(f.w,f.c,S::TownPortal,0,nullptr,false).failed && !R::travel(f.w,f.c,S::LloydsBeacon,0,&c,false).failed,"Mixed Portal/Beacon restriction");
  if(g.flags&0x1000)check(R::travel(f.w,f.c,S::Teleport,1,nullptr,false).failed,"Mixed Teleport restriction differs");
  const auto expected=xeenTerrainFallDestination(start);
  check(expected.mapId==(n==128?XeenMapIdentity(8):XeenMapIdentity(Side::Darkside,29)) && expected.x==(n==128?11:25) && expected.y==(n==128?15:21) && expected.direction==D::North,"Mixed terrain fall table differs");
  check(!xeenTerrainArrival(10,false,false,false,start,true).falling,"Flag118 did not suppress mixed fall");
 }
 // The initial Beacon dialog installs the actual-side default. Return from the
 // other side must reload the mixed map, with its own music selector.
 for(unsigned map:{125u,128u})for(unsigned mode=0;mode<3;++mode) {
  const XeenCamera target{{Side::Darkside,std::uint16_t(map)},map==125?5:3,map==125?9:13,D::East};
  auto s=at(in,saved(target),{28,20,20,D::East});mage(s,S::LloydsBeacon);s.soundOptions.music=mode!=1;
  auto &c=s.characters[s.activeRosterIds[0]];c.lloydMap=map;c.lloydSide=1;c.lloydX=target.x;c.lloydY=target.y;
  Live f(in,s);audio(f,mode);const auto random=*f.w.sessionState().journeyRandom();cast(f,S::LloydsBeacon);f.key('r');f.settle();
  check(xeen_state::sameCamera(f.c,target) && f.w.pendingLoadSide()==Side::Darkside,"Cross-side mixed Beacon Return failed");arrivalMusic(f,random);exact(f);
 }
 // Every mixed map's Teleport flag and Jump wall checks through live casting.
 for(unsigned n=113;n<=128;++n) {
  const XeenMapIdentity id{Side::Darkside,std::uint16_t(n)};XeenWorld query(in.mapLoader(),in.objectLoader());const auto &g=query.map(id).geometry;const auto events=in.events.load(id);
  std::optional<XeenCamera> from;XeenCamera to;
  for(int y=1;y<14 && !from;++y)for(int x=1;x<14 && !from;++x)for(unsigned d=0;d<4 && !from;++d) {
   const XeenCamera c{id,x,y,D(d)};const auto jump=xeenJumpPreflight(query,c);
   if(jump.admission!=XeenJumpAdmission::Ready)continue;
   if(std::any_of(events.records.begin(),events.records.end(),[&](const auto &r){return (r.x==x && r.y==y) || (r.x==jump.destination.x && r.y==jump.destination.y);}))continue;
   from=c;to=jump.destination;
  }
  check(bool(from),"Mixed map has no original open Jump corridor");
  for(S spell:{S::Jump,S::Teleport}) {
   auto s=saved(*from);s.gameFlags[256+118]=true;mage(s,spell);Live f(in,s);const auto random=*f.w.sessionState().journeyRandom();cast(f,spell);
   const bool blocked=spell==S::Teleport && (g.flags&0x1000);
   if(spell==S::Teleport && !blocked){f.key('2');f.key(InputKey::Enter);}f.settle();
   check(xeen_state::sameCamera(f.c,blocked?*from:to),"Mixed Teleport/Jump live restriction/destination differs");
   check(f.p.encounterContext->minutes==s.journey->context->minutes+1 && f.w.sessionState().journeyRandom()==random,"Same-map spell elapsed time/RNG differs");
  }
 }
 // No mixed EVT contains FallToMap. Synthetic rows therefore prove its generic
 // table selection, damage/Oops presentation and flag118 without inventing a route.
 unsigned terrainWitnesses=0;
 for(unsigned n=113;n<=128;++n) {
  const XeenCamera start{{Side::Darkside,std::uint16_t(n)},5,5,D::West};const auto expected=xeenTerrainFallDestination(start);
  std::optional<std::vector<std::uint8_t>> fallBaseline;
  for(bool suppress:{false,true})for(unsigned mode=0;mode<(suppress?1u:3u);++mode) {
   auto s=at(in,saved(expected),start);s.gameFlags[256+118]=suppress;s.journey->context->effects[XeenPartyEffect::PowerShield]=0;
   s.soundOptions.music=mode!=1;
   // A suppressed Fall restarts at line zero too. Guard this synthetic probe
   // once, as a real conditional script would, rather than constructing a loop.
   s.gameFlags[256+200]=false;
   XeenEventFile script{start.mapId,"mixed scripted fall table",true,{row(start,0,9,{20,200,3}),row(start,1,12,{0,0,20,200}),row(start,2,52,{0,0,0,7}),row(start,3,18)}};
   Live f(in,s,&script);audio(f,mode);std::string reason;f.flow->reportManual=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r))reason=e->message;};
   f.act(InteractionAction{false});if(!suppress)check(XeenEventDispatchTestAccess::fallOops(*f.flow)==4,"Scripted mixed fall omitted Oops");f.settle();
   check(reason.empty(),reason.c_str());check(xeen_state::sameCamera(f.c,suppress?start:expected),"Mixed scripted fall table/flag118 differs");
   if(!suppress)check(f.p.roster.at(s.activeRosterIds[0]).currentHp<s.characters[s.activeRosterIds[0]].currentHp,"Scripted mixed fall omitted damage");
   if(!suppress) {auto final=f.snapshot();final.soundOptions=s.soundOptions;final.soundOptions.music=true;const auto bytes=XeenSaveFormat::encode(final);
    if(fallBaseline)check(bytes==*fallBaseline,"Mixed fall audio changed Journey draws/time/damage");else fallBaseline=bytes;}
  }
  const auto g=in.maps.loadGeometryMap(in.assets,start.mapId).geometry;const auto events=in.events.load(start.mapId);
  for(unsigned cell=0;cell<256;++cell) {
   const auto type=g.surfaceTypes[g.cells[cell].surfaceIndex];if(type!=10 && type!=13)continue;
   if(std::any_of(events.records.begin(),events.records.end(),[&](const auto &r){return r.x==cell%16 && r.y==cell/16;}))continue;
   const XeenCamera floor{start.mapId,int(cell%16),int(cell/16),D::West};
   for(bool suppress:{false,true}) {
    auto s=at(in,saved(expected),floor);s.gameFlags[256+118]=suppress;s.journey->context->effects[XeenPartyEffect::Levitate]=0;s.journey->context->effects[XeenPartyEffect::PowerShield]=0;
    Live f(in,s);f.act(WaitAction{});f.settle();check(xeen_state::sameCamera(f.c,suppress?floor:expected),"Original mixed terrain fall/flag118 differs");
   }
   ++terrainWitnesses;break;
  }
 }
 check(terrainWitnesses>0,"No original mixed terrain-fall witness");
 // Traced pending states are synthetic, running through the ordinary Event owner.
 const XeenCamera start{{Side::Darkside,29},14,4,D::North};
 XeenEventFile flip{start.mapId,"pending-side cases",true,{row(start,0,59,{0}),row(start,1,12,{0,0,20,255}),row(start,2,24,{0,0}),row(start,3,7,{29,14,4})}};
 auto s=saved(start);mage(s,S::LloydsBeacon);auto &c=s.characters[s.activeRosterIds[0]];c.lloydMap=29;c.lloydX=14;c.lloydY=4;c.lloydSide=1;
 Live pending(in,s,&flip);const auto random=*pending.w.sessionState().journeyRandom();const auto generation=pending.w.inputMapLoadGeneration();
 std::string pendingError;pending.flow->reportManual=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r))pendingError=e->message;};
 pending.act(InteractionAction{false});pending.settle();
 check(pendingError.empty(),pendingError.c_str());
 check(pending.w.pendingLoadSide()==Side::Clouds,"FlipWorld pending side was not published");
 check(pending.c.mapId==start.mapId,"Same-number pending teleport changed logical camera");
 check(pending.f.isSet(Side::Darkside,255) && pending.f.isSet(Side::Clouds,255)==s.gameFlags[255],"FlipWorld changed logical flag bank");
 check(pending.w.sessionState().journeyRandom()==random && pending.w.inputMapLoadGeneration()==generation,"Same-number Event teleport loaded/drew music");
 bool refused=false;try{XeenSaveState::capture(in.signature,pending.p,pending.c,pending.f,pending.w);}catch(const std::logic_error &e){refused=std::string(e.what()).find("not supported yet")!=std::string::npos;}
 check(refused && pending.flow->canSave(),"Pending Save was not safely refused");
 XeenGameplayServices saveServices{pending.program,{}, {},{}, {},{},in.font,{},{},{},{}};
 bool wrote=false;refused=false;
 try {xeenSaveGameplay(saveServices,pending.w,pending.p,pending.c,pending.f,*pending.flow,out/"pending-must-not-save.mmsave",
  [](auto &,const auto &,const auto &,const auto &){throw std::runtime_error("Pending Save reached preflight");},nullptr,
  [&](const auto &,const auto &){wrote=true;});}
 catch(const std::logic_error &e){refused=std::string(e.what()).find("not supported yet")!=std::string::npos;}
 check(refused && !wrote && pending.flow->canSave() && pending.w.pendingLoadSide()==Side::Clouds,"Gameplay Save failed to preserve the running pending state");
 cast(pending,S::LloydsBeacon);pending.key('r');pending.settle();
 check(pending.w.pendingLoadSide()==Side::Darkside && pending.w.inputMapLoadGeneration()==generation,"Same-map Beacon failed to overwrite pending side without loading");
 // A refused destination discards the complete pending-side candidate.
 auto bad=flip;bad.records.back()=row(start,3,7,{200,1,1});Live rejected(in,s,&bad);std::string reason;
 rejected.flow->reportManual=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r))reason=e->message;};
 rejected.act(InteractionAction{false});rejected.settle();check(reason.find("not supported yet")!=std::string::npos && rejected.w.pendingLoadSide()==Side::Darkside,"Refused transition published pending side");rejected.unchanged(s);
 // The next real same-map reload uses P and resets it on the destination side.
 Live reload(in,s,&flip);reload.act(InteractionAction{false});reload.settle();XeenEventDispatchTestAccess::reload(*reload.flow);
 check(reload.c.mapId==XeenMapIdentity(29) && reload.w.pendingLoadSide()==Side::Clouds,"Reload ignored pending side");
 // Roster filtering reads P while lodging still uses the current map number.
 XeenPartyState roster;for(unsigned i=0;i<2;++i){auto &c=roster.roster.at(i);c.name="Lodged";c.savedMaze=29;c.xeenSide=i;}
 check(xeenInnList(roster,start.mapId,false,Side::Clouds)==std::vector<std::uint8_t>{0} && xeenInnList(roster,start.mapId,false,Side::Darkside)==std::vector<std::uint8_t>{1},"Roster did not filter on pending side");
 // Accepted Portal and entrance outcomes explicitly overwrite P.
 {
  auto portalSave=s;mage(portalSave,S::TownPortal);portalSave=at(in,portalSave,{{Side::Darkside,4},14,11,D::South});portalSave=at(in,portalSave,start);
  Live f(in,portalSave,&flip);f.act(InteractionAction{false});f.settle();check(f.w.pendingLoadSide()==Side::Clouds,"Portal precondition lost pending flip");
  cast(f,S::TownPortal);f.key('1');f.key(InputKey::Enter);f.settle();check(f.c.mapId==XeenMapIdentity(Side::Darkside,4) && f.w.pendingLoadSide()==Side::Darkside,"Portal did not overwrite pending side");
 }
 for(bool keyed:{false,true}) {
  const auto original=in.events.load(2);const auto e=std::find_if(original.records.begin(),original.records.end(),[](const auto &r){return r.opcode==17 && r.parameters==std::vector<std::uint8_t>{11};});check(e!=original.records.end(),"Missing Sphinx entrance");
  const XeenCamera from{2,e->x,e->y,e->direction<4?D(e->direction):D::North};auto entrance=*e;entrance.line=1;
  XeenEventFile script{from.mapId,"pending entrance override",true,{row(from,0,59,{std::uint8_t(keyed?0:1)}),entrance}};
  auto save=at(in,saved({{Side::Darkside,125},7,6,D::North}),from);save.questItems[51]=keyed?1:0;
  Live f(in,save,&script);f.act(InteractionAction{false});f.settle();check(f.w.pendingLoadSide()==(keyed?Side::Darkside:Side::Clouds) && f.c.mapId==(keyed?XeenMapIdentity(Side::Darkside,125):XeenMapIdentity(2)),"Entrance did not overwrite pending side");
 }
 // A subsequent scripted fall uses P, retaining the nonzero-map facing.
 {
  auto save=at(in,saved({29,5,5,D::North}),start);
  XeenEventFile script{start.mapId,"pending fall",true,{row(start,0,59,{0}),row(start,1,52,{29,5,5,7})}};
  Live f(in,save,&script);f.act(InteractionAction{false});f.settle();check(xeen_state::sameCamera(f.c,{29,5,5,D::North}) && f.w.pendingLoadSide()==Side::Clouds,"Scripted fall ignored pending side/facing");
 }
 // Original outdoor neighbor numbers are resolved on the current map; the
 // next main-map load selects that number from the pending archive.
 {
  XeenWorld query(in.mapLoader(),in.objectLoader());std::optional<XeenCamera> from;XeenCamera to;
  for(unsigned map=1;map<=24 && !from;++map)for(unsigned d=0;d<4 && !from;++d)for(int cell=1;cell<15 && !from;++cell) {
   XeenCamera c{{Side::Darkside,std::uint16_t(map)},d==1?15:d==3?0:cell,d==0?15:d==2?0:cell,D(d)};if(!query.map(c.mapId).geometry.isOutdoors())continue;
   auto target=c;XeenMovement::Capabilities skills;skills.swimming=skills.walkOnWater=skills.mountaineer=skills.pathfinder=true;
   if(XeenMovement().apply(query,target,NavigationAction::MoveForward,skills)!=XeenMovementResult::Moved || target.mapId==c.mapId)continue;
   target.mapId={Side::Clouds,target.mapId.number};
   try {
    const auto a=query.sampleCell(c.mapId,c.x,c.y),b=query.sampleCell(target.mapId,target.x,target.y);if(!a || !b)continue;
    const auto safe=[](auto cell){const auto t=cell->geometry->surfaceTypes[cell->cell->surfaceIndex];return t!=10 && t!=13 && t!=15;};if(!safe(a) || !safe(b))continue;
    const auto events=in.events.load(target.mapId);if(std::any_of(events.records.begin(),events.records.end(),[&](const auto &r){return r.x==target.x && r.y==target.y;}))continue;
    from=c;to=target;
   }catch(const std::runtime_error &){}
  }
  check(bool(from),"No original cross-archive boundary witness");auto save=at(in,saved(to),*from);for(auto owner:save.activeRosterIds)save.characters[owner].skills.fill(1);
  XeenEventFile script{from->mapId,"pending boundary",true,{row(*from,0,59,{0}),row(*from,1,24,{0,0}),row(*from,2,18)}};
  Live f(in,save,&script);f.act(InteractionAction{false});f.settle();const auto random=*f.w.sessionState().journeyRandom();f.act(NavigationAction::MoveForward);f.settle();
  check(xeen_state::sameCamera(f.c,to) && f.w.pendingLoadSide()==Side::Clouds,"Boundary ignored pending side");arrivalMusic(f,random);
 }
 mixedEvents(in,source,out);
 std::cout<<"M104 Part B entrances, returns, Portal, Beacon and pending-side cases passed\n";
}

void mixedApplicationAudio(Inputs &in,const GameInstallation &installation,const XeenSaveSnapshot &s,unsigned mode) {
 XeenProgramState program{XeenWallClock::fixed(0)};if(mode<2)program.audio=XeenAudio::installed(installation,in.assets,0,false);
 CloudsMapComposer composer;std::uint64_t now=0;XeenWorld *world=nullptr;const XeenPartyState *party=nullptr;const XeenCamera *camera=nullptr;unsigned shows=0,validated=0,expectedValidation=0;XeenEventFlow *flow=nullptr;
 XeenGameplayServices services{program,in.resources(),[]{return XeenGameFlags{};},in.mapLoader(),in.objectLoader(),
  [&](auto id){return in.texts.load(id);},in.font,
  [&](auto &w,const auto &p,const auto &c,auto phase){bool animated=false;return XeenEventFlow::Composition{composer.compose(in.assets,w,p,c,{p.encounterContext->year},nullptr,phase,&animated),animated};},{},
  [&](auto &f,const auto &c){flow=&f;f.dialogText=&in.assets.uiText();f.drawDialogSprite=[&,id=c.mapId](auto &frame,const char *name,unsigned index,int x,int y){in.assets.drawDialogSprite(frame,name,index,x,y,xeenDialogSpriteArchive(id,name));};},
  {},[&](auto &w,auto &,const auto &p,auto &c,const auto &){world=&w;party=&p;camera=&c;}};
 services.clock=[&]{return now;};services.restoreSnapshot=std::make_shared<const XeenSaveSnapshot>(s);
 const auto catalogue=loadXeenItemCatalog(in.assets);services.catalog=&catalogue.catalog;
 services.validateEncounterSprite=[&](auto image,auto archive){++validated;in.assets.validateNormalMonster(image,archive);};
 services.validateCombatSprite=[&](auto image,auto archive){++validated;in.assets.validateAttackMonster(image,archive);};
 services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){bool animated=false;return XeenEventFlow::Composition{composer.compose(in.assets,w,p,c,{p.encounterContext->year},nullptr,phase,&animated,actor),animated};};
 services.show=[&](const auto &,const auto &,const auto &,const auto &idle,const auto &){
  ++shows;check(world && party && camera && camera->mapId==s.camera.mapId,"Mixed Application camera absent");
  for(const auto &actor:world->sessionState().monsters(camera->mapId))if(actor.statistics)expectedValidation+=2;
  flow->prepareJourneySprites();
  for(unsigned repeat=0;repeat<3;++repeat){now+=1000;static_cast<void>(idle());}
  check(world->sessionState().journeyRandom()==s.journey->random && party->encounterContext==s.journey->context &&
   world->sessionState().darksideSong()==s.darksideSong,"Application audio/idle replayed mixed load work");return true;
 };
 check(Application(program).playGameplay(services,s.camera,std::filesystem::path("unused"),true)==0 && shows==1 && validated==expectedValidation,
  "Mixed Application audio startup/preflight failed");
}

// M104 Part A: root-selected resource families, load draws, exact restores and
// live input. The expected table is independent of the production resolver.
void mixedMaps(Inputs &in,const GameInstallation &installation,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 using A=XeenSceneArchive;using Side=XeenSide;
 struct Expected {bool cloudsMonsters,cloudsObjects,cloudsPictures,cloudsScenery;unsigned missing;bool hide,day;};
 const std::array<Expected,16> expected{{
  {false,true,false,false,6,false,false},{false,true,false,false,6,false,false},
  {false,true,false,false,6,false,false},{false,true,false,false,6,false,false},
  {true,true,false,false,6,false,false},{true,true,false,false,6,false,false},
  {true,true,false,false,6,false,false},{true,true,false,false,6,false,false},
  {false,false,false,false,0,false,false},{false,false,false,false,0,false,false},
  {false,false,false,false,0,false,false},{true,true,false,false,0,false,false},
  {false,true,true,true,6,false,false},{false,true,true,true,6,false,false},
  {false,true,true,true,0,false,false},{false,true,false,false,6,true,true}}};
 const std::set<unsigned> render{113,117,121,124,125,127,128};
 const auto art=[](bool clouds){return clouds?A::Clouds:A::Darkside;};
 const auto table=[](bool clouds){return clouds?Side::Clouds:Side::Darkside;};
 // Ordinary map choices must remain identical, including the existing sky exceptions.
 for(auto side:{Side::Clouds,Side::Darkside})for(unsigned n=1;n<=129;++n) {
  if(side==Side::Darkside && n>=113 && n<=128)continue;
  const auto r=xeenMapResources({side,static_cast<std::uint16_t>(n)});const auto a=art(side==Side::Clouds);
  check(r.mapArchive==side && r.monsterTable==side && r.animationTable==side && r.pictureMetadata==side &&
   r.monsterArt==a && r.objectArt==a && r.pictures==a && r.walls==a && r.sky==a && r.ground==a && r.tiles==a &&
   r.surfaces==a && r.voices==a && r.ambient==a && r.townPortal==side,"Ordinary map selector changed");
 }
 for(unsigned n=113;n<=128;++n) {
  const XeenMapIdentity id{Side::Darkside,static_cast<std::uint16_t>(n)};const auto &e=expected[n-113];const auto r=xeenMapResources(id);
  std::cout<<"M104 map "<<n<<std::endl;
  check(r.mapArchive==Side::Darkside && r.monsterTable==table(e.cloudsMonsters) &&
   r.monsterArt==art(e.cloudsMonsters) && r.objectArt==art(e.cloudsObjects) &&
   r.animationTable==table(e.cloudsObjects) && r.pictures==art(e.cloudsPictures) && r.pictureMetadata==table(e.cloudsPictures) &&
   r.walls==art(e.cloudsScenery) && r.sky==art(e.cloudsScenery) && r.ground==art(e.cloudsScenery) &&
   r.tiles==art(e.cloudsScenery) && r.surfaces==art(e.cloudsScenery) && r.missingCellSlot==e.missing &&
   r.hideHorizon==e.hide && r.alwaysDay==e.day && r.voices==A::Darkside && r.ambient==A::Darkside &&
   r.townPortal==(n==128?Side::Darkside:Side::Clouds) &&
   r.music==(e.cloudsScenery?XeenMapMusic::Clouds:XeenMapMusic::DarksideRandom) &&
   r.musicArchive==(e.cloudsScenery?A::Clouds:A::DarksideOnly),"Mixed root selector mismatch");
  check(xeenMonsterArt(id,91)==A::Clouds && xeenMonsterArt(id,0)==r.monsterArt,"Species 91 changed another selector");
  check(xeenDialogSpriteArchive(id,"dung.til")==r.tiles && xeenDialogSpriteArchive(id,"global.icn")==r.ambient,"Minimap/map tile used ambient UI archive");
  const auto &map=in.maps.loadGeometryMap(in.assets,id);const auto mob=in.maps.loadObjects(in.assets,id);
  const auto events=in.events.load(id);const auto texts=in.texts.load(id);
  check(map.identity()==id && mob.mapId==id && events.mapId==id && texts.mapId==id &&
   mob.resourcePresent && events.resourcePresent && texts.resourcePresent && !in.assets.mapName(id).empty(),"Logical mixed resources missing");
  check(!map.geometry.isOutdoors() && !(map.geometry.flags2&0x900),"Mixed geometry or Portal/Beacon restrictions changed");
  const auto mon=XeenMonsterFormat::parse(*in.assets.readMonsterStatistics(table(e.cloudsMonsters)));
  const auto meta=XeenCloudsVisualMetadata::parse(*in.assets.readObjectVisualMetadata(table(e.cloudsObjects)));
  auto resolver=XeenObjectVisualResolver::load(in.assets);
  for(unsigned i=0;i<mob.entities.objects.size();++i)if(mob.entities.objects[i].hasResource()) {
   const auto visual=resolver.resolve(mob,i,XeenDirection::North,0);const auto &entry=meta.at(mob.entities.objects[i].resourceId);
   const auto facing=(4-mob.entities.objects[i].direction)%4;
   check(visual.initialFrame==entry.initialFrames[facing] && visual.endFrame==entry.frameLimits[facing],"Mixed animation table mismatch");
  }
  const auto presetPath=out/("preset-"+std::to_string(n)+".mmsave");std::filesystem::remove(presetPath);
  developer::generateTestSave(installation,"m104-map-"+std::to_string(n),presetPath);
  const auto preset=XeenSaveFile::read(presetPath);check(preset.camera.mapId==id,"Mixed preset lost map identity");
  // All three output states: audio enabled, muted, and no audio owner.
  for(unsigned mode=0;mode<3;++mode) {
   auto s=at(in,source,preset.camera);s.soundOptions.music=mode!=1;Live live(in,s);
   const auto before=*live.w.sessionState().journeyRandom();
   const auto admitted=live.w.prepareEventTransition(preset.camera);check(bool(admitted),"Production mixed admission refused");
   check(!XeenLearnedSpellRules::travel(live.w,live.c,S::TownPortal,0,nullptr,false).failed &&
    !XeenLearnedSpellRules::travel(live.w,live.c,S::LloydsBeacon,0,&live.p.roster.at(s.activeRosterIds[0]),false).failed,"Mixed spell restrictions disagree");
   for(const auto &actor:live.w.sessionState().monsters(id))if(actor.statistics)
    check(actor.statistics->raw==mon.at(actor.original.resourceId).raw,"Mixed actors bound to wrong MON");
   Live origin(in,quiet(in,source));auto candidate=origin.w.transitionCandidate();XeenCombatRandom random(before);
   const auto load=[&](XeenMapIdentity from,XeenMapIdentity to) {
    candidate->loadEventDestination(from,to,true,live.f);
    const bool draws=xeenMapResources(to).music==XeenMapMusic::DarksideRandom;
    std::optional<std::uint8_t> song;
    if(draws)song=static_cast<std::uint8_t>(7*xeenMapMusicRow(candidate->map(to).geometry)+random.range(0,6));
    check(candidate->sessionState().darksideSong()==song && candidate->sessionState().journeyRandom()==random.continuation(),"Map load song/draw order mismatch");
    XeenCombatRandom actual(*candidate->sessionState().journeyRandom()),next(random.continuation());
    check(actual.range(1,100000)==next.range(1,100000),"Next Journey draw shifted");
   };
   load(origin.c.mapId,id); // First entry stages the original MOB with its own MON.
   load(id,{Side::Darkside,29});load({Side::Darkside,29},id); // Actual leave and revisit.
   load(id,id); // Same-map reload, which still draws on a drawing map.
   const auto retained=*candidate->sessionState().journeyRandom();
   for(unsigned repeat=0;repeat<3;++repeat) {
    static_cast<void>(xeenMapResources(id));check(bool(candidate->prepareEventTransition(preset.camera)),"Mixed preflight failed");
    live.composer.compose(in.assets,*candidate,live.p,preset.camera,{610},nullptr,repeat);
   }
   if(mode<2) {
    auto audio=XeenAudio::installed(installation,in.assets,0,false);audio->observeOptions(s.soundOptions);
    if(e.cloudsScenery)check(audio->mapSong(map.geometry)=="dungeon.m","Sphinx did not use Clouds dungeon.m");
    else {
     const auto ordinal=*candidate->sessionState().darksideSong();const auto name=audio->darksideMapSong(ordinal);
     check(ordinal/7==(n==124?5u:3u) && name==in.assets.uiText().table("DARKSIDE_MUSIC").at(ordinal),"Mixed music row/song mismatch");
    }
    for(unsigned repeat=0;repeat<3;++repeat){audio->requestMap(id,map.geometry,candidate->sessionState().darksideSong(),repeat*1000);audio->advance(repeat*1000+900);}
   }
   check(candidate->sessionState().journeyRandom()==retained && live.w.sessionState().journeyRandom()==before,"Lookup/preflight/render/audio consumed Journey RNG");
   mixedApplicationAudio(in,installation,s,mode);
   const auto bytes=XeenSaveFormat::encode(live.snapshot());Live restored(in,XeenSaveFormat::decode(bytes));
   check(XeenSaveFormat::encode(restored.snapshot())==bytes && restored.w.sessionState().journeyRandom()==before,"Mixed exact restore consumed RNG/time/rewards");
  }
  if(n==113 || n==117 || n==125) {
   // A real walk in unchanged geometry. No Event, actor or wall is removed.
   auto s=at(in,source,preset.camera);XeenWorld query(in.mapLoader(),in.objectLoader());bool found=false;
   for(int y=1;y<15 && !found;++y)for(int x=1;x<15 && !found;++x)for(unsigned d=0;d<4 && !found;++d) {
    const XeenCamera c{id,x,y,XeenDirection(d)};auto destination=c;const auto moved=XeenMovement().apply(query,destination,NavigationAction::MoveForward);
    if(moved!=XeenMovementResult::Moved || destination.mapId!=id)continue;
    if(std::any_of(events.records.begin(),events.records.end(),[&](const auto &e){return (e.x==x && e.y==y) || (e.x==destination.x && e.y==destination.y);}))continue;
    const auto cell=query.sampleCell(id,destination.x,destination.y);
    if(!cell || map.geometry.surfaceTypes[cell->cell->surfaceIndex]==10 || map.geometry.surfaceTypes[cell->cell->surfaceIndex]==13)continue;
    s.camera=c;Live live(in,s);const auto time=live.p.encounterContext->minutes;
    live.act(NavigationAction::MoveForward);live.settle();
    check(xeen_state::sameCamera(live.c,destination) && live.p.encounterContext->minutes!=time,"Mixed live walk did not settle time/position");
    found=true;
   }
   check(found,"Mixed map has no ordinary walking witness");
  }
  if(n==113 || n==117 || n==121 || n==125) {
   const auto flip=std::find_if(events.records.begin(),events.records.end(),[](const auto &r){return r.opcode==59;});
   check(flip!=events.records.end(),"Original mixed FlipWorld absent");
   auto s=at(in,source,{id,flip->x,flip->y,XeenDirection::South});
   Live live(in,s);std::string refusal;
   live.flow->reportManual=[&](const auto &result){if(const auto *error=std::get_if<XeenEventExecutionError>(&result))refusal=error->message;};
   live.act(InteractionAction{false});live.act(YesAction{});live.settle();
   std::cout<<"Flip "<<n<<": "<<refusal<<std::endl;
   const XeenCamera expected=n==113?XeenCamera{7,11,0,XeenDirection::South}:n==117?XeenCamera{13,10,5,XeenDirection::East}:
    n==121?XeenCamera{19,3,4,XeenDirection::South}:XeenCamera{2,11,9,XeenDirection::South};
   check(refusal.empty() && xeen_state::sameCamera(live.c,expected),"Original FlipWorld return failed");
   check(live.w.pendingLoadSide()==XeenSide::Clouds,"FlipWorld return retained the wrong pending side");
  }
  if(n==113 || n==124 || n==125) {
   auto s=preset;bool found=false;XeenWorld query(in.mapLoader(),in.objectLoader());
   for(const auto &a:mob.entities.monsters)if(!found && a.isActive())for(unsigned d=0;d<4 && !found;++d) {
    constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
    const int x=a.x-dx[d],y=a.y-dy[d];if(x<0 || x>=32 || y<0 || y>=32)continue;
    if(std::any_of(mob.entities.monsters.begin(),mob.entities.monsters.end(),[&](const auto &m){return m.isActive() && m.x==x && m.y==y;}))continue;
    if(std::any_of(events.records.begin(),events.records.end(),[&](const auto &e){return (e.x==x && e.y==y) || (e.x==a.x && e.y==a.y);} ))continue;
    const auto startCell=query.sampleCell(id,x,y),targetCell=query.sampleCell(id,a.x,a.y);
    if(!startCell || !targetCell)continue;
    const auto safe=[&](const auto &cell){const auto type=map.geometry.surfaceTypes[cell->cell->surfaceIndex];return type!=10 && type!=13 && type!=15;};
    if(!safe(startCell) || !safe(targetCell))continue;
    XeenCamera c{id,x,y,XeenDirection(d)};auto moved=c;
    if(XeenMovement().apply(query,moved,NavigationAction::MoveForward)!=XeenMovementResult::Moved)continue;
    s.camera=c;s.headCache.reset();Live live(in,s);bool fought=false;
    std::cout<<"Fight candidate "<<n<<" ("<<x<<','<<y<<") target "<<a.resourceId<<" at "<<a.x<<','<<a.y<<std::endl;
    live.act(NavigationAction::MoveForward);
    for(unsigned tick=0;tick<200 && !fought;++tick) {
      fought=live.flow->encounter()->combat() && live.flow->encounter()->combat()->phase()==XeenCombatPhase::PlayerReady;
      if(!fought)live.tick();
    }
    if(!fought) {XeenRestTestAccess::state(*live.flow);std::cout<<"fight camera "<<live.c.mapId<<' '<<live.c.x<<','<<live.c.y<<std::endl;}
    check(fought,"Original mixed contact did not reach PlayerReady");
    for(const auto &actor:live.w.sessionState().monsters(id))if(actor.statistics)
     check(actor.statistics->raw==mon.at(actor.original.resourceId).raw,"Mixed live fight used the logical-side table");
    // Exercise a round through presented input; a fight is not a save boundary.
    for(unsigned turn=0;turn<12;++turn) {
     const auto *combat=live.flow->encounter()->combat();
     if(combat && combat->phase()==XeenCombatPhase::PlayerReady) {
      if(turn==0)live.act(AttackAction{});else live.act(BlockAction{});
     }
     live.tick();
    }
    check(!(live.w.sessionState().journeyRandom()==s.journey->random) &&
     !(live.p.encounterContext==s.journey->context),"Mixed live fight did not advance RNG/time");
    combatPortal(in,installation,s,true);
    found=true;std::cout<<"Fight "<<n<<" from ("<<x<<','<<y<<") "<<d<<std::endl;
   }
   check(found,"No original mixed-map fight reached live input");
  }
  if(render.count(n)) {
   // Original positions and actors, facing the most populated composed view.
   auto s=preset;s.journey->context->lightAndResistances[0]=200;XeenWorld query(in.mapLoader(),in.objectLoader());int best=-1;
   for(int y=1;y<15;++y)for(int x=1;x<15;++x)for(unsigned d=0;d<4;++d) {
    if(std::any_of(mob.entities.monsters.begin(),mob.entities.monsters.end(),[&](const auto &a){return a.isActive() && a.x==x && a.y==y;}))continue;
    const auto cell=query.sampleCell(id,x,y);if(!cell)continue;
    const auto type=map.geometry.surfaceTypes[cell->cell->surfaceIndex];if(type==10 || type==15)continue;
    const XeenCamera c{id,x,y,XeenDirection(d)};query.sceneActors(id,[&]{return mon;});
    const auto commands=XeenIndoorScene().build(query,c,&resolver,nullptr,0);
    int score=0;for(const auto &command:commands)score+=command.actor()?100:command.object()?30:command.wallItem()?10:0;
    if(score>best){best=score;s.camera=c;}
   }
   s.headCache.reset();Live live(in,s);picture(live.flow->frame(),out/("mixed-"+std::to_string(n)+".ppm"));
   std::cout<<"Frame "<<n<<" ("<<live.c.x<<','<<live.c.y<<") "<<unsigned(XeenDirection(live.c.direction))<<" score="<<best<<'\n';
  }
 }
 std::cout<<"M104: all 16 selectors/admissions/presets, boundary cases, load/revisit/reload music, audio gates and exact restores passed\n";
}

}
int main(int argc,char **argv) {
 try {
  check(argc==3 || (argc==4 && (std::string(argv[3])=="partb" || std::string(argv[3])=="partc" || std::string(argv[3])=="mixed" || std::string(argv[3])=="mixed-travel")),"usage: darkside-tests <installation> <output-directory> [partb|partc]");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Missing original installation");
  const std::filesystem::path out=std::string(argv[2])=="temporary"?std::filesystem::temp_directory_path()/"mmodern-m104-frames":std::filesystem::path(argv[2]);std::filesystem::create_directories(out);const auto save=std::filesystem::temp_directory_path()/("mmodern-m101-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".mmsave");
  std::cout<<"Preset: "<<save<<'\n';
  developer::generateTestSave(*installation,"m101-castleview",save);Inputs in(*installation);const auto preset=XeenSaveFile::read(save);
  check(preset.camera.mapId==XeenMapIdentity(XeenSide::Darkside,29),"Wrong Castleview preset");
  if(argc==4 && std::string(argv[3])=="partc"){partCEvents(in,quiet(in,preset),out);return 0;}
  if(argc==4 && std::string(argv[3])=="mixed-travel"){mixedTravel(in,*installation,preset,out);return 0;}
  if(argc==4 && std::string(argv[3])=="mixed"){mixedMaps(in,*installation,preset,out);return 0;}
  partBEvents(in,quiet(in,preset),out);if(argc==4)return 0;
  partCEvents(in,quiet(in,preset),out);
  enchantedLivePublication(in,quiet(in,preset),out);partAEvents(in,quiet(in,preset),out);characterEvents(in,quiet(in,preset));resources(in);std::cout<<"Resources verified\n";const auto s=quiet(in,preset);music(in,*installation,s);std::cout<<"Music verified\n";keyedEntrances(in,s,out);forcedText(in,s,out);
  pyramid(in,*installation,s,out);saveExport(in,*installation,s,out);originalRoute(in,*installation,out);pyramidCombatAudio(in,*installation,out);
  panelAndDefeat(in,*installation,s,out,4);
  for(unsigned mode=0;mode<3;++mode)panelAndDefeat(in,*installation,s,out,mode);
  admittedObjects(in,s,out);auto incoming=combat(in,preset,out);combatPortal(in,*installation,incoming);for(auto owner:incoming.activeRosterIds)incoming.characters[owner].currentHp=1;
  XeenSaveFile::write(out/"darkside-defeat.mmsave",incoming);
  panelAndDefeat(in,*installation,incoming,out,3);
  auto terminal=incoming;for(auto owner:terminal.activeRosterIds){terminal.characters[owner].currentHp=0;terminal.characters[owner].conditions[unsigned(XeenCondition::Unconscious)]=1;}
  panelAndDefeat(in,*installation,terminal,out,5);travel(in,*installation,s,out);
  Live castleview(in,s);picture(castleview.flow->frame(),out/"castleview.ppm");
  auto outdoors=s;outdoors.camera={{XeenSide::Darkside,1},8,8,XeenDirection::North};outdoors.darksideSong=0;outdoors=quiet(in,outdoors);
  Live landscape(in,outdoors);picture(landscape.flow->frame(),out/"darkside-outdoors.ppm");
  std::cout<<"113 ordinary Darkside maps, Pyramid and side travel, exact save/DOS export, Wizard/defeat and shared-key entrances passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<"Darkside tests: "<<e.what()<<'\n';return 1;}
}
