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

using namespace training_test;
namespace mmodern {
struct XeenEventDispatchTestAccess {
 static bool inputPassPending(const XeenEventFlow &f){return f._encounter->_journeyCapture->inputPassPending;}
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
 Live(Inputs &in,const XeenSaveSnapshot &s,const XeenEventFile *eventOverride=nullptr):in(in),catalogue(loadXeenItemCatalog(in.assets)),w(in.mapLoader(),in.objectLoader()),
  events([&in,eventOverride](auto id){return XeenEventScript(eventOverride && eventOverride->mapId==id?*eventOverride:in.events.load(id));},[&](auto id){return in.texts.load(id);}) {
  in.assets.loadPalette("mm4.pal",xeenPictureArchive(s.camera.mapId));in.assets.loadRawFramebuffer("back.raw",xeenPictureArchive(s.camera.mapId));
  auto resources=in.resources();if(eventOverride)resources.loadEvents=[&in,eventOverride](auto id){return eventOverride->mapId==id?*eventOverride:in.events.load(id);};
  XeenSaveState::restoreBeforeGameplay(s,resources,p,c,f,w,[](auto &,const auto &,const auto &,const auto &){});
  const auto compose=[&](XeenWorld &world,const XeenPartyState &party,const XeenCamera &camera,std::uint64_t phase) {
   if(beforeCompose)beforeCompose(camera.mapId);
   bool animated=false;IndexedFrame image;try {image=composer.compose(in.assets,world,party,camera,{party.encounterContext->year},nullptr,phase,&animated);} catch(const std::exception &e){std::cerr<<"Compose "<<unsigned(static_cast<XeenSide>(camera.mapId.side))<<':'<<camera.mapId.number<<" failed: "<<e.what()<<'\n';throw;}
   return XeenEventFlow::Composition{std::move(image),animated};
  };
  flow=std::make_unique<XeenEventFlow>(program,w,events,p,c,f,in.font,
   [&,compose](auto phase){return compose(w,p,c,phase);},XeenEventPresenter::NpcDraw{[&](auto &frame,auto id,auto phase){in.assets.drawNpc(frame,id,phase,xeenPictureArchive(c.mapId));}},
   [this]{return now;},XeenEventPresenter::RandomFrame{},&catalogue.catalog,
   [&,compose](auto phase,auto){return compose(w,p,c,phase);},nullptr,
   [compose](auto &world,const auto &party,const auto &camera,auto phase,auto){return compose(world,party,camera,phase);});
  flow->dialogText=&in.assets.uiText();
  flow->drawDialogSprite=[&](auto &frame,const char *name,unsigned index,int x,int y){in.assets.drawDialogSprite(frame,name,index,x,y,xeenPictureArchive(c.mapId));};
  flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){return xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,archive));};
  flow->mapName=[&](auto id){return in.assets.mapName(id);};
  flow->loadSpellDescriptions=[&]{return in.assets.readArchiveResource("spldesc.bin",xeenPictureArchive(c.mapId));};
  flow->drawSmithArt=[&](auto &image){in.assets.drawSmith(image,xeenPictureArchive(c.mapId));};
  flow->drawTrainingArt=[&](auto &image){in.assets.drawTraining(image,xeenPictureArchive(c.mapId));};
  flow->drawTempleArt=[&](auto &image){in.assets.drawTemple(image,xeenPictureArchive(c.mapId));};
  flow->drawBankArt=[&](auto &image){in.assets.drawBank(image,xeenPictureArchive(c.mapId));};
  flow->drawGuildArt=[&](auto &image){in.assets.drawGuild(image,xeenPictureArchive(c.mapId));};
  flow->drawTavernArt=[&](auto &image){in.assets.drawTavern(image,xeenPictureArchive(c.mapId));};
  flow->tavernFoodOffer=[&](unsigned n){return in.assets.tavernFoodOffer(n,xeenPictureArchive(c.mapId));};
  flow->tavernString=[&](std::size_t n){return in.assets.tavernString(n,xeenPictureArchive(c.mapId));};
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
 const auto id=s.camera.mapId;const auto mon=XeenMonsterFormat::parse(*in.assets.readMonsterStatistics(id.side));
 const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,id),mon);
 auto &table=s.map(id).monsters;table=XeenSaveMonsterTable{std::uint16_t(actors.size()),{}};
 for(const auto &a:actors) {if(a.lifecycle==XeenActorLifecycle::Present)table->slots.push_back({-128,-128,XeenActorLifecycle::Defeated,XeenActorStatus::Physical,true,-1});else table->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});}
 s.headCache.reset();s.soundOptions={false,false,true,false};return s;
}
XeenSaveSnapshot at(Inputs &in,XeenSaveSnapshot s,const XeenCamera &camera) {
 s.camera=camera;s.darksideSong.reset();
 if(camera.mapId.side==XeenSide::Darkside)s.darksideSong=static_cast<std::uint8_t>(7*xeenMapMusicRow(in.maps.loadGeometryMap(in.assets,camera.mapId).geometry));
 return quiet(in,std::move(s));
}
void arrivalMusic(Live &live,const XeenJourneyRandomState &before) {
 XeenCombatRandom expected(before);
 if(live.c.mapId.side==XeenSide::Darkside) {
  const auto song=7*xeenMapMusicRow(live.w.map(live.c.mapId).geometry)+expected.range(0,6);
  check(live.w.sessionState().darksideSong()==song,"Arrival picked the wrong DOS music row/column");
 }else check(!live.w.sessionState().darksideSong(),"Clouds arrival retained Darkside song");
 check(live.w.sessionState().journeyRandom()==expected.continuation(),"Arrival music draw count/state differs");
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
  if(n>=113 && n<=128) {bool refused=false;try{in.maps.loadGeometryMap(in.assets,id);}catch(const std::invalid_argument &e){refused=std::string(e.what()).find("not supported yet")!=std::string::npos;}check(refused,"Mixed map admitted");continue;}
  const auto map=in.maps.loadGeometryMap(in.assets,id);const auto mob=in.maps.loadObjects(in.assets,id);const auto evt=in.events.load(id);
  check(map.identity()==id && mob.mapId==id && evt.mapId==id && mob.resourcePresent && evt.resourcePresent,"Ordinary Darkside resource identity/presence");++admitted;
 }
 check(admitted==113,"Ordinary Darkside main map inventory changed");
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
void refusals(Inputs &in,const XeenSaveSnapshot &source,const std::filesystem::path &out) {
 unsigned deferredKeys=0;
 for(const auto &site:xeenEntranceSites())if(site.quest>=35) {
  bool found=false;
  for(const auto &e:in.events.load(site.map).records)if(e.opcode==17 && e.line==0 && e.parameters==std::vector<std::uint8_t>{std::uint8_t(site.action)}) {
   XeenCamera at{site.map,e.x,e.y,e.direction<4?XeenDirection(e.direction):XeenDirection::North};
   if(xeenEntranceSite(site.action,at)!=&site)continue;
   auto s=source;s.camera=at;s.darksideSong.reset();s.questItems[site.quest]=1;s=quiet(in,s);
   std::cout<<"Refusal site "<<s.camera.x<<','<<s.camera.y<<'\n';Live live(in,s);std::string reason;live.flow->reportManual=[&](const auto &r){if(const auto *error=std::get_if<XeenEventExecutionError>(&r))reason=error->message;};
   live.act(InteractionAction{});check(reason.find("not supported yet")!=std::string::npos,"Held Darkside key did not refuse before its scene");live.unchanged(s);found=true;break;
  }
  check(found,"Missing original Darkside-key entrance site");++deferredKeys;
 }
 check(deferredKeys==4,"Deferred key entrance inventory changed");

}
// Original records are loaded from the installation, then isolated from their
// unrelated suffix. Both physical sides exercise the regional preflight.
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


void combatPortal(Inputs &in,const GameInstallation &installation,const XeenSaveSnapshot &source) {
 check(!XeenLearnedSpellRules::supportedIn(unsigned(S::TownPortal),true) &&
     !XeenLearnedSpellRules::supportedIn(unsigned(S::LloydsBeacon),true) && !xeenItemUseSpell(28,true),
     "A previously forbidden combat travel path now requires a candidate-RNG witness");
 const XeenCamera destination{{XeenSide::Darkside,4},14,11,XeenDirection::South};
 for(unsigned mode=0;mode<3;++mode) {
  auto s=at(in,source,destination);s.camera=source.camera;s.darksideSong=source.darksideSong;
  s.soundOptions={mode!=1,mode!=1,true,false};
  for(auto owner:s.activeRosterIds)s.characters[owner].miscellaneous[0]={1,55,3,0};
  Live live(in,s);
  if(mode<2){live.program.audio=XeenAudio::installed(installation,in.assets,0,false);live.program.audio->observeOptions(s.soundOptions);}
  const auto tick=[&]{live.now+=16;live.flow->beginCycle(++live.cycle);if(auto frame=live.flow->updatePresentation())live.present(*frame);};
  live.act(WaitAction{});
  for(unsigned n=0;n<2000 && (!live.flow->encounter()->combat() || live.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady);++n)tick();
  check(live.flow->encounter()->combat() && live.flow->encounter()->combat()->phase()==XeenCombatPhase::PlayerReady,"Darkside Portal item witness did not enter Castleview combat");
  const auto random=*live.w.sessionState().journeyRandom();
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
   check(!fight->cast() && xeen_state::sameCamera(live.c,source.camera) && xeenSameItem(live.p.roster.at(owner).miscellaneous[0],item),"Canceled Portal changed its source or item");
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
   f.drawDialogSprite=[&](auto &frame,const char *n,unsigned i,int x,int y){try{in.assets.drawDialogSprite(frame,n,i,x,y,xeenPictureArchive(camera->mapId));}catch(const std::exception &e){std::cerr<<"Dialog "<<n<<" frame "<<i<<": "<<e.what()<<std::endl;throw;}};
   f.drawCombatButtons=[&](auto &frame){CloudsUiComposer().drawCombatButtons(in.assets,frame,xeenPictureArchive(camera->mapId));};
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
 services.catalog=&catalogue.catalog;services.validateEncounterSprite=[](auto){};services.validateCombatSprite=[](auto){};
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

}
int main(int argc,char **argv) {
 try {
  check(argc==3,"usage: darkside-tests <installation> <output-directory>");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Missing original installation");
  const std::filesystem::path out=argv[2];std::filesystem::create_directories(out);const auto save=std::filesystem::temp_directory_path()/("mmodern-m101-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".mmsave");
  std::cout<<"Preset: "<<save<<'\n';
  developer::generateTestSave(*installation,"m101-castleview",save);Inputs in(*installation);const auto preset=XeenSaveFile::read(save);
  check(preset.camera.mapId==XeenMapIdentity(XeenSide::Darkside,29),"Wrong Castleview preset");
  characterEvents(in,quiet(in,preset));resources(in);std::cout<<"Resources verified\n";const auto s=quiet(in,preset);music(in,*installation,s);std::cout<<"Music verified\n";refusals(in,s,out);forcedText(in,s,out);
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
  std::cout<<"113 ordinary Darkside maps, Pyramid and side travel, exact save/DOS export, Wizard/defeat and deferred-entrance refusals passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<"Darkside tests: "<<e.what()<<'\n';return 1;}
}
