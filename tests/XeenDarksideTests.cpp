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
#include <fstream>
#include <chrono>
#include <iostream>
#include <set>

using namespace training_test;
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
 Live(Inputs &in,const XeenSaveSnapshot &s,const XeenEventFile *eventOverride=nullptr):in(in),catalogue(loadXeenItemCatalog(in.assets)),w(in.mapLoader(),in.objectLoader()),
  events([&in,eventOverride](auto id){return XeenEventScript(eventOverride && eventOverride->mapId==id?*eventOverride:in.events.load(id));},[&](auto id){return in.texts.load(id);}) {
  in.assets.loadPalette("mm4.pal",xeenPictureArchive(s.camera.mapId));in.assets.loadRawFramebuffer("back.raw",xeenPictureArchive(s.camera.mapId));
  auto resources=in.resources();if(eventOverride)resources.loadEvents=[&in,eventOverride](auto id){return eventOverride->mapId==id?*eventOverride:in.events.load(id);};
  XeenSaveState::restoreBeforeGameplay(s,resources,p,c,f,w,[](auto &,const auto &,const auto &,const auto &){});
  const auto compose=[&](XeenWorld &world,const XeenPartyState &party,const XeenCamera &camera,std::uint64_t phase) {
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
  flow->mapName=[&](auto id){return in.assets.mapName(id);};
  flow->loadSpellDescriptions=[&]{return in.assets.readArchiveResource("spldesc.bin",xeenPictureArchive(c.mapId));};
  present(flow->frame());
  beforeMaps=w.sessionState().mapStates();
 }
 void present(const IndexedFrame &frame){if(frame.presentation())flow->framePresented(frame.presentation());}
 void act(PlayerAction action){flow->beginCycle(++cycle);present(flow->handle(action,flow->displayedInput(),flow->frame().presentation()));}
 void key(unsigned key){const auto input=flow->inputContext(flow->frame().presentation()).dialog;check(bool(input),"Missing live dialog");const auto action=input->key(key);check(bool(action),"Missing live dialog key");act(*action);}
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
 for(const auto &a:actors)table->slots.push_back({-128,-128,XeenActorLifecycle::Defeated,XeenActorStatus::Physical,true,-1});
 s.headCache.reset();s.soundOptions={false,false,true,false};return s;
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
 const auto events=in.events.load({XeenSide::Darkside,29});std::set<unsigned> actions;
 for(const auto &e:events.records)if(e.opcode==17 && e.line==0 && e.parameters.size()==1 && (e.parameters[0]<=5 || e.parameters[0]==12) && !actions.count(e.parameters[0])) {
  auto s=source;s.camera.x=e.x;s.camera.y=e.y;s.camera.direction=e.direction<4?XeenDirection(e.direction):XeenDirection::North;
  std::cout<<"Refusal site "<<s.camera.x<<','<<s.camera.y<<'\n';Live live(in,s);std::string reason;live.flow->reportManual=[&](const auto &result){if(const auto *error=std::get_if<XeenEventExecutionError>(&result))reason=error->message;};
  live.act(InteractionAction{});check(reason.find("not supported yet")!=std::string::npos,"Deferred town/Pyramid input did not refuse clearly");live.unchanged(s);actions.insert(e.parameters[0]);
 }
 check(actions.count(12) && actions.size()==7,"Castleview service/Pyramid refusal inventory incomplete");
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
 for(auto spell:{S::LloydsBeacon,S::TownPortal})for(bool item:{false,true}) {
  auto s=source;auto &caster=s.characters[s.activeRosterIds.front()];const auto category=spell==S::TownPortal?XeenSpellCategory::Clerical:XeenSpellCategory::Wizardry;
  caster.weapons={};caster.armor={};caster.accessories={};caster.miscellaneous={};
  caster.characterClass=spell==S::TownPortal?XeenCharacterClass::Cleric:XeenCharacterClass::Sorcerer;caster.hasSpells=true;caster.currentSp=1000;
  caster.learnedSpells.emplace();caster.learnedSpells->fill(0);caster.currentSpell=*XeenLearnedSpellRules::slotForSpell(category,unsigned(spell));caster.learnedSpells->at(caster.currentSpell)=1;
  if(item)caster.miscellaneous[0]={1,static_cast<std::uint8_t>(spell==S::LloydsBeacon?28:55),3,0};
  std::cout<<"Spell refusal "<<unsigned(spell)<<" item "<<item<<'\n';Live live(in,s);
  if(item){live.act(SelectMemberAction{0});live.key('i');live.key('m');live.key('1');live.key('u');}
  else {live.act(CastSpellAction{});live.act(SelectMemberAction{0});live.key('c');}
  live.unchanged(s);notice(live.flow->frame(),in.font,item?"Item use: not supported yet":"Spell not supported yet");
  picture(live.flow->frame(),out/(std::string(spell==S::TownPortal?"portal-refusal":"beacon-refusal")+(item?"-item.ppm":".ppm")));
 }
 auto dead=source;for(auto owner:dead.activeRosterIds){dead.characters[owner].currentHp=0;dead.characters[owner].conditions[13]=1;}
 Live defeated(in,dead);defeated.act(AcknowledgeAction{});defeated.unchanged(dead);notice(defeated.flow->frame(),in.font,"Darkside defeat recovery: not supported yet");check(!defeated.flow->partyDeathComplete(),"Darkside defeat ran Clouds recovery");picture(defeated.flow->frame(),out/"defeat-refusal.ppm");
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
void wizardPanel(Inputs &in,const GameInstallation &installation,const XeenSaveSnapshot &source,const std::filesystem::path &save,const std::filesystem::path &out,unsigned mode) {
 XeenProgramState program{XeenWallClock::fixed(0)};XeenEventFlow *flow=nullptr;std::uint64_t now=0;CloudsMapComposer composer;
 const auto catalogue=loadXeenItemCatalog(in.assets);
 if(mode<2)program.audio=XeenAudio::installed(installation,in.assets,0,false);
 const XeenPartyState *party=nullptr;XeenWorld *world=nullptr;XeenCamera *camera=nullptr;const XeenGameFlags *flags=nullptr;
 XeenGameplayServices s{program,in.resources(),[]{return XeenGameFlags{};},in.mapLoader(),in.objectLoader(),
  [&](auto id){return in.texts.load(id);},in.font,
  [&](auto &w,const auto &p,const auto &c,auto phase){bool animated=false;auto frame=composer.compose(in.assets,w,p,c,{p.encounterContext->year},nullptr,phase,&animated);return XeenEventFlow::Composition{std::move(frame),animated};},{},
  [&](auto &f,const auto &){flow=&f;f.dialogText=&in.assets.uiText();f.drawCombatButtons=[&](auto &frame){CloudsUiComposer().drawCombatButtons(in.assets,frame,xeenPictureArchive(source.camera.mapId));};},{},
  [&](auto &w,auto &,const auto &p,auto &c,const auto &f){world=&w;party=&p;camera=&c;flags=&f;}};
 XeenSessionOutcome outcome;s.outcome=&outcome;
 auto loaded=source;loaded.soundOptions.music=mode!=1;
 s.restoreSnapshot=std::make_shared<XeenSaveSnapshot>(loaded);s.developerSavePath=save;s.clock=[&]{return now;};
 s.validateEncounterSprite=[](auto){};s.validateCombatSprite=[](auto){};
 s.catalog=&catalogue.catalog;
 s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){bool animated=false;auto frame=composer.compose(in.assets,w,p,c,{p.encounterContext->year},nullptr,phase,&animated,actor);return XeenEventFlow::Composition{std::move(frame),animated};};
 s.panel=[&](const auto &base,bool combat,bool restricted,bool saveable,auto slot,const auto &name) {
  XeenTitleFlow::Services panel{in.assets.uiText(),in.font,base,{},{},[&](auto &f,const char *n,unsigned i,int x,int y){in.assets.drawDialogSprite(f,n,i,x,y,xeenPictureArchive(source.camera.mapId));},
   [&]{std::array<XeenSaveFile::Slot,10> slots{};slots[0].state=XeenSaveFile::Slot::State::Available;slots[0].snapshot=source;slots[0].snapshot->name="Review Load";return slots;},[&](unsigned){return save;}};
  panel.panel=true;panel.combat=combat;panel.saveRestricted=restricted;panel.saveable=saveable;panel.currentSlot=slot;panel.currentName=name;
  return std::make_unique<XeenTitleFlow>(std::move(panel));
 };
 s.show=[&](const auto &first,const auto &handler,const auto &,const auto &idle,const auto &) {
  auto visible=first;std::uint64_t cycle=0;
  if(program.audio)check(program.audio->currentName()==program.audio->darksideMapSong(*source.darksideSong),"Production Load did not request the saved song");
  const auto present=[&]{handler.framePresented(visible.presentation());handler.completeInputHandoff(visible.presentation());};present();
  handler.beginCycle(++cycle);if(auto next=idle())visible=*next;present();
  if(mode==4) {
   bool refused=false,fought=false;
   for(unsigned n=0;n<2000 && !refused;++n) {
    const auto context=handler.inputContext(visible.presentation());
    if(n==0 || context.readyForAction || (flow->encounter()->combat() && flow->encounter()->combat()->phase()==XeenCombatPhase::PlayerReady)) {
     handler.beginCycle(++cycle);const PlayerAction action=!flow->encounter()->combat()?PlayerAction{WaitAction{}}:PlayerAction{BlockAction{}};
     if(auto next=handler.withDisplayedInput(action,*handler.displayedInput()))visible=*next;present();
    }
    now+=110;handler.beginCycle(++cycle);if(auto next=idle())visible=*next;present();
    fought=fought || bool(flow->encounter()->combat());
    refused=drawXeenErrorScroll(visible,in.font,"Darkside defeat recovery: not supported yet").pixels==visible.pixels;
   }
   if(!refused || !fought || flow->encounter()->state().reason()!=XeenEncounterStop::Defeat) {
    std::cerr<<"Defeat probe refused="<<refused<<" fought="<<fought<<" phase="<<(flow->encounter()->combat()?int(flow->encounter()->combat()->phase()):-1)<<" reason="<<int(flow->encounter()->state().reason())<<" hp=";
    for(auto owner:party->party.activeRosterIds())std::cerr<<party->roster.at(owner).currentHp<<',';std::cerr<<'\n';
    picture(visible,out/"combat-defeat-probe.ppm");
   }
   check(refused && fought && flow->encounter()->state().reason()==XeenEncounterStop::Defeat,"Original combat did not reach Darkside defeat refusal");
   picture(visible,out/"combat-defeat-refusal.ppm");
  }
  XeenRestoreGuard guard(*world,*party,*camera,*flags);
  const auto send=[&](const PlayerAction &a){handler.beginCycle(++cycle);const auto input=handler.displayedInput();check(bool(input),"Panel input missing");const auto next=handler.withPresentedInput(a,*input,visible.presentation());check(bool(next),"Panel did not respond");visible=*next;present();guard.check();};
  if(mode==3 || mode==4) {
   notice(visible,in.font,"Darkside defeat recovery: not supported yet");
   send(ControlPanelAction{});
   if(mode==4) {
    send(DialogKeyAction{in.assets.uiText().buttons("PANEL").at(3).key});notice(visible,in.font,"not supported yet");send(AcknowledgeAction{});
    send(DialogKeyAction{in.assets.uiText().buttons("PANEL").at(5).key});picture(visible,out/"combat-defeat-quit.ppm");send(DialogKeyAction{'n'});
   }
   send(DialogKeyAction{in.assets.uiText().buttons("PANEL").at(2).key});
   const auto input=handler.inputContext(visible.presentation()).dialog;
   check(input && input->key('1'),"Load slots unreachable after refused defeat");
   picture(visible,out/(mode==4?"combat-defeat-load-slots.ppm":"defeat-load-slots.ppm"));
   if(mode==4) {send(DialogKeyAction{'1'});send(DialogKeyAction{InputKey::Enter});check(handler.finished(),"Refused-defeat Load did not finish");return true;}
   send(CancelInteractionAction{});
   send(DialogKeyAction{in.assets.uiText().buttons("PANEL").at(5).key});
   picture(visible,out/"defeat-quit.ppm");send(DialogKeyAction{'y'});return true;
  }
  send(ControlPanelAction{});send(DialogKeyAction{in.assets.uiText().buttons("PANEL").at(8).key});send(DialogKeyAction{'y'});
  check(handler.inputContext(visible.presentation()).dialog->anyKey,"Darkside Mr Wizard refusal not visible");
  check(camera->mapId==source.camera.mapId && party->purse==source.journey->purse && world->sessionState().journeyRandom()==source.journey->random,"Mr Wizard changed state");
  notice(visible,in.font,"not supported yet");picture(visible,out/"wizard-refusal.ppm");return true;
 };
 check(Application(program).playGameplay(s,source.camera,save,true)==0,"Darkside control panel scenario");
 if(mode==4)check(outcome.kind==XeenSessionOutcome::Kind::Load,"Refused-defeat Load outcome missing");
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
  for(unsigned mode=0;mode<3;++mode)wizardPanel(in,*installation,s,save,out,mode);
  auto dead=s;for(auto owner:dead.activeRosterIds){dead.characters[owner].currentHp=0;dead.characters[owner].conditions[13]=1;}
  wizardPanel(in,*installation,dead,save,out,3);
  admittedObjects(in,s,out);auto incoming=combat(in,preset,out);
  for(auto owner:incoming.activeRosterIds){incoming.characters[owner].currentHp=1;}
  wizardPanel(in,*installation,incoming,save,out,4);
  Live castleview(in,s);picture(castleview.flow->frame(),out/"castleview.ppm");
  auto outdoors=s;outdoors.camera={{XeenSide::Darkside,1},8,8,XeenDirection::North};outdoors.darksideSong=0;outdoors=quiet(in,outdoors);
  Live landscape(in,outdoors);picture(landscape.flow->frame(),out/"darkside-outdoors.ppm");
  std::cout<<"113 ordinary Darkside maps, saved-song restore, arrival RNG, original rendering and Part A live refusals passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<"Darkside tests: "<<e.what()<<'\n';return 1;}
}
