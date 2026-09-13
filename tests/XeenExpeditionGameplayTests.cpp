#include "XeenExpeditionTestSupport.h"
#include "XeenRestoreReplayProbe.h"
#include "XeenVisualRemoveTestSupport.h"

#include "games/xeen/XeenOutdoorScene.h"
#include "games/xeen/XeenStateEquality.h"
#include <iostream>
#define SDL_MAIN_HANDLED
#include <SDL.h>
using namespace combat_gameplay_test;
namespace fs=std::filesystem;
namespace {
Bytes diskBytes(const fs::path &path){std::ifstream in(path,std::ios::binary);return Bytes(std::istreambuf_iterator<char>(in),{});}
XeenGameplayServices services(Harness &h) {
 auto s=h.services();
 s.texts=[&h](XeenMapIdentity id) {
  if (!h.assets) return XeenEventTextFile{id,"synthetic.txt",true,{"Synthetic discovery","","","Synthetic bones"}};
  return XeenEventTextLoader([&h](const std::string &name)->std::optional<Bytes> {
   if (!h.assets->hasArchiveResource(name)) return {};
   return h.assets->readArchiveResource(name);
  }).load(id);
 };
 if(!h.assets){s.maps=[](auto){return expedition_fixture::terrain();};s.objects=[](auto){return expedition_fixture::objects();};
 s.resources.loadEvents=[](auto){return expedition_fixture::events();};s.resources.loadMonsterStatistics=[]{return expedition_fixture::monsters();};}
 return s;
}
void press(Harness &h,const SdlWindow::FrameUpdateHandler &handler,const PlayerAction &a) {
 handler.beginCycle(++h.cycle);handler.withDisplayedInput(a,*handler.displayedInput());
 check(handler.frameCurrent(),"current Application input frame");handler.framePresented();h.visibleScene();
}
void tick(Harness &h,const SdlWindow::FrameUpdateHandler &handler,const SdlWindow::IdleFrameHandler &idle) {
 h.now+=100;handler.beginCycle(++h.cycle);idle();check(handler.frameCurrent(),"current automatic frame");handler.framePresented();h.visibleScene();
}
void collect(Harness &h,const SdlWindow::FrameUpdateHandler &handler,const fs::path &path) {
 const auto baseline=XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world);
 const auto wire=XeenSaveFormat::encode(baseline);
 const auto reconstruct=[&] {
  const auto frame=h.flow->frame();const auto pending=h.flow->presentationGeneration();
  for(unsigned cache=0;cache<5;++cache) {
   const auto maps=h.mapCalls,mobs=h.mobCalls;
   if(cache==0||cache==4)h.world->discardMapCache();
   if(cache==1||cache==4)h.eventSystem->discardScriptCache();
   if(cache==2||cache==4)h.eventSystem->discardTextCache();
   if(h.assets&&(cache==3||cache==4))h.assets->discardSpriteCache();
   h.flow->refresh(true);check(!h.flow->canSave(),"cache frame requires new presentation");handler.framePresented();
   check(h.flow->frame().pixels==frame.pixels&&h.flow->presentationGeneration()==pending,"individual and combined caches retain objective frame and continuation");
   if(cache==1||cache==4)check(h.eventSystem->cachedScriptCount()>0,"EVT cache really reconstructed");
   if(cache==2||cache==4)check(h.eventSystem->cachedTextCount()>0,"text cache really reconstructed");
   if(h.assets&&(cache==0||cache==4))check(h.mapCalls>maps&&h.mobCalls>mobs,"original scene/MOB caches really reloaded");
   if(h.assets&&(cache==3||cache==4))check(h.assets->cachedSpriteCount()>0,"original sprite cache really reloaded");
  }
 };
 unsigned instructions=0;
 h.flow->reportManual=[&](const auto &r) { if (const auto *c=std::get_if<XeenManualEventCompleted>(&r)) instructions=c->instructionCount; if(const auto *e=std::get_if<XeenEventExecutionError>(&r)) std::cerr<<"Objective error: "<<e->message<<"\n"; };
 press(h,handler,InteractionAction{});
 check(h.flow->canCancelInteraction()&&!h.flow->canSave(),"Journey original WhoWill owns unsaveable Event");
 visual_remove_test::save(h.flow->frame(),path.string()+"-who.bmp");
 reconstruct();
 const auto gen=*handler.displayedInput();
 const auto saves=h.saves;
 for (const PlayerAction &a:std::vector<PlayerAction>{NavigationAction::MoveForward,WaitAction{},InspectInventoryAction{},BlockAction{},SaveGameAction{}})
  press(h,handler,a);
 check(h.saves==saves&&h.party->questItems.counts()==baseline.questItems,"modal incompatible controls cannot publish/save");
 press(h,handler,CancelInteractionAction{});
 check(instructions==1&&h.flow->canSave()&&XeenSaveFormat::encode(XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world))==wire,"cancel exactly one instruction and no durable mutation");
 handler.withDisplayedInput(InteractionAction{},gen);
 check(!h.flow->canCancelInteraction(),"stale cancel-to-retry refused");
 press(h,handler,InteractionAction{});
 std::size_t selected=4;
 if(!h.party->party.member(h.party->roster,selected).canAct()) { selected=1; while(selected<h.party->party.size()&&!h.party->party.member(h.party->roster,selected).canAct())++selected; }
 check(selected<h.party->party.size(),"connected live nonfirst WhoWill recipient");
 press(h,handler,SelectMemberAction{selected});
 check(!h.flow->canCancelInteraction()&&!h.flow->canSave()&&h.party->questItems.counts()==baseline.questItems,"eligible nonfirst discovery precedes grant");
 visual_remove_test::save(h.flow->frame(),path.string()+"-ack.bmp");
 reconstruct();
 check(h.party->questItems.counts()==baseline.questItems&&!h.flow->canSave(),"reconstruction retains acknowledgment without mutation");
 press(h,handler,AcknowledgeAction{});
 check(h.flow->canSave()&&instructions==10,"ten original instructions and presented quiet success");
 auto expected=baseline;++expected.questItems[18];expected.disabledObjects.push_back({20,1});
 for(unsigned i=1;i<=5;++i)expected.disabledEvents.push_back({20,i});
 check(XeenSaveFormat::encode(XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world))==XeenSaveFormat::encode(expected),"only checked counter and exact Remove effects; expedition consequences unchanged");
 visual_remove_test::save(h.flow->frame(),path.string()+"-collected.bmp");
 reconstruct();
 press(h,handler,InteractionAction{});
 check(instructions==5&&h.party->questItems.counts()==expected.questItems,"repeat executes five effective None without grant");
 h.flow->reportManual={};
}
void projection() {
 const auto stats=expedition_fixture::monsters();
 for(unsigned count=1;count<=3;++count) {
  std::vector<XeenActor> actors;
  for(unsigned i=0;i<count;++i){XeenActor a;a.id={20,i};a.x=4;a.y=14;a.hp=30;a.lifecycle=XeenActorLifecycle::Present;a.statistics=stats[9];actors.push_back(a);}
  for(unsigned selected=0;selected<count;++selected){XeenMonsterAppearance appearance{XeenMonsterSpriteKind::Attack,2};appearance.identity=actors[selected].id;
   auto commands=XeenOutdoorScene::actorCommands(actors,{20,4,14,XeenDirection::East},appearance);
   check(commands.size()==count,"one command per selected slot");
   for(unsigned i=0;i<count;++i){const auto &c=commands[i];check(c.actor()->identity==actors[i].id&&c.actor()->selectedSlot==int(i),"original identity slot");
    check((c.originalOrder==121)==(i==selected)&&((c.actor()->kind==XeenMonsterSpriteKind::Attack)==(i==selected)),"only responsible identity ATT order121");
    check(c.x==(count==2?(i==0?31:-36):(i==0?-5:i==1?-67:58))&&c.y==2&&c.drawOptions().bottomClipped,"pair/triple original anchors");}
   actors[selected].lifecycle=XeenActorLifecycle::Defeated;actors[selected].x=actors[selected].y=-128;
   commands=XeenOutdoorScene::actorCommands(actors,{20,4,14,XeenDirection::East},appearance);
   for(const auto &c:commands)check(c.actor()->kind==XeenMonsterSpriteKind::Normal,"dead identity cannot transfer ATT");
   actors[selected].lifecycle=XeenActorLifecycle::Present;actors[selected].x=4;actors[selected].y=14;
  }
 }
 // Classifier queries every admitted distance and lateral group; no actor-per-placement duplication.
 std::array<bool,26> seen{};
 for(unsigned count=1;count<=3;++count)for(int x=1;x<=7;++x)for(int y=11;y<=17;++y){std::vector<XeenActor> actors;
  for(unsigned i=0;i<count;++i){XeenActor a;a.id={20,i};a.x=x;a.y=y;a.hp=30;a.lifecycle=XeenActorLifecycle::Present;a.statistics=stats[9];actors.push_back(a);}
  const auto commands=XeenOutdoorScene::actorCommands(actors,{20,4,14,XeenDirection::East},0);
  constexpr int orders[]{118,112,115,94,92,93,75,73,74,52,50,51,90,91,69,71,44,47,48,49,70,72,42,45,43,46};
  constexpr int xs[]{-5,-67,58,-7,-38,25,-8,-24,9,-9,-17,-1,-112,98,-65,49,-34,16,-58,40,-85,65,-41,-16,-26,23};
  constexpr int pairs[]{31,-36,58,8,-23,25,0,-16,9,-5,-13,-1,-112,98,-65,49,-27,20,-58,40,-85,65,-37,-12,-26,23};
  constexpr int query[]{2,2,2,7,7,7,14,14,14,27,27,27,5,9,12,16,25,29,23,31,12,16,25,29,25,29};
  for(const auto &c:commands){const auto slot=c.actor()->selectedSlot;seen[slot]=true;const auto q=query[slot];const auto scale=q==2?0:q==5||q==7||q==9?8:q==12||q==14||q==16?12:14;const auto y=scale==0?2:scale==8?34:scale==12?53:59;
   check(c.originalOrder==orders[slot]&&c.x==(commands.size()==2?pairs[slot]:xs[slot])&&c.y==y&&c.actor()->scaleIndex==scale&&c.sampleIndex==q&&c.drawOptions().sceneClipped,"literal all26 MON orders/anchors/scales/query and 1/2/3 arrangement");}
 }
 check(std::all_of(seen.begin(),seen.end(),[](bool v){return v;}),"all26 selected projection slots exercised");
}
void controls(const fs::path &path) {
 const auto initial=XeenSaveFile::read(path);
 for(unsigned variant=0;variant<4;++variant){auto bad=initial;
  if(variant==0)--bad.journey->actors[0].hp;
  if(variant==1)bad.disabledEvents.push_back({20,999});
  if(variant==2)bad.disabledObjects.push_back({20,999});
  if(variant==3)bad.journey->actors[3].activated=false;
  XeenSaveFile::write(path,bad);Harness rejected;auto s=services(rejected);bool shown=false;s.show=[&](const auto &,const auto &,const auto &,const auto &,const auto &){shown=true;return true;};
  check(Application().playGameplay(s,{},path,true)==3&&!shown&&!rejected.flow,"malformed successor fails production startup before first frame without fallback");
 }
 XeenSaveFile::write(path,initial);
 for(unsigned variant=0;variant<3;++variant){Harness rejected;auto s=services(rejected);unsigned providers=0;s.resources.loadInitialParty=[&]{++providers;return combat_test::party();};
  const int result=Application().playGameplay(s,{},path,true,variant==2?XeenEncounterEntry::Journey:XeenEncounterEntry::Ordinary,variant==0?std::optional<std::uint32_t>{1}:std::nullopt,variant==1?std::optional<std::uint16_t>{2}:std::nullopt);
  check(result==3&&providers==0&&!rejected.flow,"load rejects seed/content/fresh-entry overrides");
 }

 for(unsigned facing=0;facing<4;++facing){auto saved=initial;saved.camera={20,5,14,static_cast<XeenDirection>(facing)};
  for(auto &a:saved.journey->actors){a.hp=0;a.x=a.y=-128;a.activated=false;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
  XeenSaveFile::write(path,saved);Harness h;auto s=services(h);unsigned dispatched=0;
  s.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &){handler.framePresented();h.flow->reportManual=[&](const auto &){++dispatched;};
   collect(h,handler,path);return true;};
  check(Application().playGameplay(s,{},path,true)==0,"all-facing production restore/collection");
 }
 // Objective phases through the real SDL poll loop; queued keys cannot cross modal frames.
 {Harness h;auto s=services(h);unsigned stage=0,stable=0,loops=0;
 const auto key=[](SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0){SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.timestamp=SDL_GetTicks()+1;e.key.repeat=repeat;check(SDL_PushEvent(&e)==1,"SDL objective key queue");};
 const auto tap=[&](SDL_Keycode code){key(code);key(code,SDL_KEYUP);};
 s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  auto driver=[&]()->std::optional<IndexedFrame>{
   check(++loops<120,"bounded SDL objective schedule");auto frame=idle();if(frame){stable=0;return frame;}if(++stable<2)return frame;stable=0;
   switch(stage++){
   case 0:tap(SDLK_SPACE);tap(SDLK_F9);break;
   case 1:check(h.flow->canCancelInteraction()&&h.saves==0,"SDL WhoWill holds F9");tap(SDLK_ESCAPE);tap(SDLK_SPACE);break;
   case 2:check(h.flow->canSave()&&!h.flow->presentationGeneration(),"SDL cancel batch cannot retry");tap(SDLK_SPACE);break;
   case 3:check(h.flow->canCancelInteraction(),"SDL fresh retry");key(SDLK_F2);tap(SDLK_SPACE);tap(SDLK_F9);break;
   case 4:check(!h.flow->canCancelInteraction()&&h.flow->presentationGeneration()&&h.party->questItems.counts()[18]==0&&h.saves==0,"SDL selecting F-key batch cannot acknowledge/save");key(SDLK_F2,SDL_KEYDOWN,1);break;
   case 5:check(h.party->questItems.counts()[18]==0,"held/repeated selection cannot acknowledge");key(SDLK_F2,SDL_KEYUP);tap(SDLK_SPACE);tap(SDLK_F9);break;
   case 6:check(h.flow->canSave()&&h.party->questItems.counts()[18]==1&&h.saves==0,"SDL acknowledgment-to-save batch closed");tap(SDLK_F9);break;
   default:check(h.saves==3&&XeenSaveFile::read(path).questItems[18]==1,"SDL fresh F9 persists collection");{SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}break;
   }return frame;};
  return SdlWindow().showInteractive(first,"M31 objective modal controls",handler,escape,driver,status);
 };
 check(Application().playGameplay(s,{},path,true)==0&&stage>=8,"real SDL objective authority");
 }
 auto grouped=initial;grouped.camera={20,4,14,XeenDirection::East};
 for(auto &a:grouped.journey->actors){if(a.id.recordIndex==9||a.id.recordIndex==25){a.x=5;a.y=14;a.activated=true;}else{a.x=a.y=-128;a.hp=0;a.activated=false;a.accounted=true;a.lifecycle=XeenActorLifecycle::Defeated;}}
 XeenSaveFile::write(path,grouped);
 // Recomposition failure follows a real publication, and cannot rerun it.
 {Harness h;auto s=services(h);bool injected=false;
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){handler.framePresented();press(h,handler,NavigationAction::MoveForward);
  const auto old=*handler.displayedInput();auto *c=h.flow->encounter()->combat();check(c&&c->contacts()[1],"mixed production attachment");
  const auto rng=h.world->sessionState().journeyRandom()->count;
  h.flow->beforeEncounterFrameCopy=[&]{if(!injected&&h.world->sessionState().journeyRandom()->count>rng){injected=true;check(!h.flow->canSave(),"capture closed after publication before handoff");throw std::runtime_error("postpublication frame fault");}};
  for(unsigned n=0;!injected&&n<100;++n){if(c->phase()==Phase::PlayerReady)press(h,handler,InteractionAction{});else tick(h,handler,idle);}
  check(injected&&h.flow->encounter()->combat()->phase()!=Phase::Failed,"one guarded rebuild retains result");
  const auto after=*h.world->sessionState().journeyRandom();auto actors=h.world->sessionState().actors();handler.withDisplayedInput(InteractionAction{},old);
  check(*h.world->sessionState().journeyRandom()==after&&xeen_state::sameActor(actors[9],h.world->sessionState().actors()[9]),"stale frame cannot replay published attack");return true;};
 check(Application().playGameplay(s,{},path,true)==0,"production postpublication rebuild");}
 // A second composition failure closes capture while retaining the published RNG/result.
 {Harness h;auto s=services(h);bool failed=false;std::uint64_t initialCount=0;
 const auto compose=s.composeEncounter;s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto ordinary,auto appearance){
  if(h.flow&&h.flow->encounter()->combat()&&w.sessionState().journeyRandom()->count>initialCount)throw std::runtime_error("persistent postpublication composition fault");
  return compose(w,p,c,ordinary,appearance);};
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){handler.framePresented();initialCount=h.world->sessionState().journeyRandom()->count;press(h,handler,NavigationAction::MoveForward);
  const auto old=*handler.displayedInput();
  for(unsigned n=0;!failed&&n<100;++n){try{if(h.flow->encounter()->combat()->phase()==Phase::PlayerReady)press(h,handler,InteractionAction{});else tick(h,handler,idle);}catch(const std::exception &){failed=true;}}
  check(failed&&h.world->sessionState().journeyRandom()->count>initialCount&&!h.flow->canSave(),"fatal composition retains published cursor and closes capture");
  check(h.flow->encounter()->notice().find("STOPPED")!=std::string::npos,"stopped state does not advertise ready controls");
  const auto after=*h.world->sessionState().journeyRandom();const auto stages=h.saves;
  handler.withDisplayedInput(SaveGameAction{},old);handler.withDisplayedInput(InteractionAction{},old);
  check(h.saves==stages&&*h.world->sessionState().journeyRandom()==after,"failed frame cannot save or replay");return true;};
 check(Application().playGameplay(s,{},path,true)==0,"production fatal frame closure");}
 // Real SDL poll batches and held keys at a two-target contact.
 {Harness h;auto s=services(h);unsigned stage=0,loops=0,stable=0;std::uint64_t rng=0;int participant=0;
 const auto key=[](SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0){SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.timestamp=SDL_GetTicks()+1;e.key.repeat=repeat;check(SDL_PushEvent(&e)==1,"SDL target key queue");};
 s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  auto driver=[&]()->std::optional<IndexedFrame>{check(++loops<100,"bounded SDL target controls");if(h.flow->encounter()->combat()&&h.flow->encounter()->combat()->pending()!=Work::None)h.now+=100;auto frame=idle();if(frame){stable=0;return frame;}if(++stable<2)return frame;stable=0;
   auto *c=h.flow->encounter()->combat();
   switch(stage++){
   case 0:key(SDLK_w);key(SDLK_w,SDL_KEYUP);break;
   case 1:check(c&&c->phase()==Phase::PlayerReady,"SDL automatic attachment");rng=h.world->sessionState().journeyRandom()->count;participant=c->participant();key(SDLK_2);key(SDLK_SPACE);key(SDLK_SPACE,SDL_KEYUP);key(SDLK_F9);key(SDLK_F9,SDL_KEYUP);break;
   case 2:check(c->selectedTarget()==XeenMonsterIdentity{20,25}&&h.world->sessionState().journeyRandom()->count==rng&&c->participant()==participant&&h.saves==0,"SDL fixed selection batch rejects Space/F9");key(SDLK_1);key(SDLK_1,SDL_KEYUP);break;
   case 3:check(c->selectedTarget()==XeenMonsterIdentity{20,9},"SDL fresh target row");key(SDLK_2);key(SDLK_2,SDL_KEYDOWN,1);break;
   case 4:check(c->selectedTarget()==XeenMonsterIdentity{20,9}&&h.world->sessionState().journeyRandom()->count==rng,"SDL held/repeated target cannot select replacement");key(SDLK_2,SDL_KEYUP);break;
   case 5:key(SDLK_2);key(SDLK_2,SDL_KEYUP);break;
   default:check(c->selectedTarget()==XeenMonsterIdentity{20,25}&&c->participant()==participant,"released fresh target accepted without turn");{SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}break;
   }return frame;};
  return SdlWindow().showInteractive(first,"M30B target controls",handler,escape,driver,status);};
 check(Application().playGameplay(s,{},path,true)==0&&stage>=7,"real SDL target generation safety");}
}

void deathControl(const fs::path &path) {
 auto saved=XeenSaveFile::read(path);
 for(auto &a:saved.journey->actors)if(a.id.recordIndex!=25){a.hp=0;a.x=a.y=-128;a.activated=false;a.accounted=true;a.lifecycle=XeenActorLifecycle::Defeated;}
 for(auto owner:kXeenCombatOwners){auto &c=saved.characters[owner];c.currentHp=0;c.conditions[12]=1;}
 auto &cleric=saved.characters[1];cleric.currentHp=1;cleric.conditions[12]=0;cleric.permanentLevel=1;cleric.temporaryLevel=0;cleric.endurance={0,0};
 XeenSaveFile::write(path,saved);const auto before=diskBytes(path);Harness h;auto s=services(h);
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){handler.framePresented();press(h,handler,NavigationAction::MoveForward);
  for(unsigned n=0;n<1000&&h.flow->encounter()->combat()->phase()!=Phase::Defeat;++n){auto *c=h.flow->encounter()->combat();check(c->phase()!=Phase::Failed&&c->phase()!=Phase::SupportStopped,"death fixture remains supported");if(c->phase()==Phase::PlayerReady)press(h,handler,BlockAction{});else tick(h,handler,idle);}
  check(h.flow->encounter()->combat()->phase()==Phase::Defeat&&h.party->roster.at(1).conditions[13]&&h.flow->encounter()->notice().find("Dead")!=std::string::npos,"production injury sets and presents death");
  check(h.flow->encounter()->combatObservation().armorCount&&h.flow->encounter()->notice().find("armor broken:")!=std::string::npos,"production lethal injury presents armor breakage");
  const auto stages=h.saves;handler.withDisplayedInput(SaveGameAction{},*handler.displayedInput());check(!h.flow->canSave()&&stages==h.saves&&diskBytes(path)==before,"death cannot save or recover");return true;};
 check(Application().playGameplay(s,{},path,true)==0,"synthetic successor death production control");
}

void unfavorable(const fs::path &game,const fs::path &path,unsigned seed=3) {
 Harness h(game);auto s=services(h);bool injury=false,broken=false,disease=false,dead=false;
 std::ofstream requests(path.string()+".rng");
 replay_test::observeDraw=[&](auto lo,auto hi,auto result,auto state){requests<<lo<<','<<hi<<','<<(result?std::to_string(*result):"reject")<<','<<state.state<<','<<state.count<<'\n';};
 struct ClearDraw { ~ClearDraw(){replay_test::observeDraw={};} } clearDraw;
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){handler.framePresented();
  press(h,handler,InspectInventoryAction{});
  for(unsigned member:{4u,5u}){press(h,handler,SelectMemberAction{member});press(h,handler,SelectInventorySlotAction{0});press(h,handler,EquipmentInventoryAction{});}
  press(h,handler,InspectInventoryAction{});press(h,handler,NavigationAction::MoveForward);
  while(!h.flow->encounter()->combat()&&h.flow->encounter()->state().pending())tick(h,handler,idle);
  for(unsigned n=0;n<15000;++n){auto *c=h.flow->encounter()->combat();check(c,"unfavorable contact remains active");
   if(c->phase()==Phase::Defeat)break;
   check(c->phase()!=Phase::Failed&&c->phase()!=Phase::SupportStopped,"unfavorable production must not support-stop");
   if(c->phase()==Phase::PlayerReady)press(h,handler,c->participant()>=4?PlayerAction{InteractionAction{}}:PlayerAction{BlockAction{}});else tick(h,handler,idle);
   const auto &r=h.flow->encounter()->combatObservation();if(r.armorCount&&!broken){broken=true;check(h.flow->encounter()->notice().find("armor broken:")!=std::string::npos,"readable breakage result");visual_remove_test::save(h.flow->frame(),path.string()+"-breakage.bmp");}
   for(auto owner:kXeenCombatOwners){const auto &p=h.party->roster.at(owner);injury=injury||p.conditions[12]||p.conditions[13];disease=disease||p.conditions[4];dead=dead||p.conditions[13];}
  }
  std::cout<<"Unfavorable minute="<<h.party->encounterContext->minutes<<" phase="<<unsigned(h.flow->encounter()->combat()->phase())<<" injury="<<injury<<" broken="<<broken<<" disease="<<disease<<" dead="<<dead<<"\n";
  check(h.flow->encounter()->combat()->phase()==Phase::Defeat&&injury&&broken&&disease,"real accumulated Disease/injury/breakage and total defeat");
  visual_remove_test::save(h.flow->frame(),path.string()+"-defeat.bmp");
  const auto stages=h.saves;press(h,handler,SaveGameAction{});check(h.saves==stages&&!h.flow->canSave()&&!fs::exists(path),"defeat cannot save or recover");return true;
 };
 check(Application().playGameplay(s,{},path,false,XeenEncounterEntry::Journey,seed,2)==0,"original unfavorable production control");
}

void route(const std::optional<fs::path> &game,const fs::path &path,unsigned schedule,unsigned start,unsigned stop,bool restart) {
 Harness h(game);auto s=services(h);const bool original=bool(game);
 std::ofstream requests(path.string()+".rng",restart?std::ios::app:std::ios::trunc);
 replay_test::observeDraw=[&](auto lo,auto hi,auto result,auto state){requests<<lo<<','<<hi<<','<<(result?std::to_string(*result):"reject")<<','<<state.state<<','<<state.count<<'\n';};
 struct ClearDraw { ~ClearDraw(){replay_test::observeDraw={};} } clearDraw;
 if(original){const char *names[]{"008.mon","008.att","009.mon","009.att"};const unsigned sizes[]{35946,22004,22076,14892};for(unsigned i=0;i<4;++i)check(h.assets->readArchiveResource(names[i]).size()==sizes[i],"original MON/ATT exact resource sizes");}

 unsigned expectedReplay=replay_test::unexpected;
 if(restart){++replay_test::depth;s.resources.loadInitialParty=[]()->XeenPartyState{throw std::runtime_error("initial party replay");};s.resources.loadInitialCharacters=[]()->Bytes{throw std::runtime_error("CHR replay");};s.resources.loadInitialContext=[]()->XeenGameplayContext{throw std::runtime_error("preparation replay");};}
 auto observer=s.observeGameplay;s.observeGameplay=[&](auto &w,auto &e,const auto &p,auto &c,const auto &f){observer(w,e,p,c,f);if(restart){--replay_test::depth;check(replay_test::unexpected==expectedReplay,"startup restores with zero gameplay replay");}};
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
  check(!h.flow->canSave(),"first-frame capture closed");handler.framePresented();check(h.flow->canSave(),"matching first frame opens quiet");
  if(original&&!restart){
   visual_remove_test::save(h.flow->frame(),path.string()+"-entry.bmp");
   const auto classified=XeenOutdoorScene::actorCommands(h.world->sessionState().actors(),*h.camera,0);check(!classified.empty(),"forest entry has selected actor independent of visibility");
   const auto full=CloudsMapComposer().compose(*h.assets,*h.world,*h.party,*h.camera,{610},nullptr,0,nullptr,XeenMonsterAppearance{0});
   const auto terrain=CloudsMapComposer().compose(*h.assets,*h.world,*h.party,*h.camera,{610},nullptr,0);
   unsigned changed=0;for(int y=8;y<135;++y)for(int x=8;x<223;++x)if(full.pixels[y*320+x]!=terrain.pixels[y*320+x])++changed;
   check(changed==0,"original entry forest occludes selected actor without removing threat");
   const auto state=*h.world->sessionState().journeyRandom();h.assets->discardSpriteCache();
   const auto rebuilt=CloudsMapComposer().compose(*h.assets,*h.world,*h.party,*h.camera,{610},nullptr,0,nullptr,XeenMonsterAppearance{0});
   check(rebuilt.pixels==full.pixels&&*h.world->sessionState().journeyRandom()==state,"original sprite/cache rebuild is deterministic and draw-free");
  }
  const auto savedState=[&]{return XeenSaveFormat::encode(XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world));};
  const auto save=[&]{const auto before=savedState();auto stages=h.saves;press(h,handler,SaveGameAction{});check(h.saves==stages+3&&diskBytes(path)==before,"real Application F9 writes exact current values");};
  if(restart)check(savedState()==diskBytes(path),"startup current state exact, no replay");
  unsigned ends=0;bool triple=false;
  const auto settle=[&]{
   if(original){const auto rows=h.flow->encounter()->combat()->contacts();const auto minute=h.party->encounterContext->minutes;
    if(schedule<2&&h.camera->x==1)check(rows[0]==XeenMonsterIdentity{20,25}&&!rows[1]&&minute==490,"P1 solo25 contact490");
    else if(schedule<2&&h.camera->x==4)check(rows[0]==XeenMonsterIdentity{20,9}&&!rows[1]&&minute==521,"P1 solo9 contact521");
    else if(schedule==1)check(rows[0]==XeenMonsterIdentity{20,17}&&rows[1]==XeenMonsterIdentity{20,18}&&!rows[2]&&minute==562,"P1 pair17/18 contact562");
    std::cout<<"production contact "<<h.camera->x<<','<<h.camera->y<<" minute "<<minute<<'\n';
   }
   bool mixed=false;
   for(unsigned n=0;h.flow->encounter()->combat()&&n<3000;++n){auto *c=h.flow->encounter()->combat();const auto rows=c->contacts();
    // The retained entry move admits record 9 before the first player action.
    if(original&&schedule>=2&&!mixed&&rows[0]==XeenMonsterIdentity{20,9}&&rows[1]==XeenMonsterIdentity{20,25}){check(h.camera->x==5&&h.camera->y==14&&h.party->encounterContext->minutes==530,"P1 mixed9/25 after retained entry move");mixed=true;std::cout<<"production mixed9/25 minute530\n";}
    if(original){const auto appearance=h.flow->encounter()->appearance();if(appearance.kind==XeenMonsterSpriteKind::Attack){check(bool(appearance.identity),"production ATT always identity-bound");auto commands=XeenOutdoorScene::actorCommands(h.world->sessionState().actors(),*h.camera,appearance);for(const auto &command:commands)if(command.actor()->kind==XeenMonsterSpriteKind::Attack)check(command.actor()->identity==*appearance.identity&&command.originalOrder==121,"production only responsible identity ATT");}}

    if(original&&schedule==3&&rows[0]==XeenMonsterIdentity{20,17}&&rows[1]==XeenMonsterIdentity{20,18}&&rows[2]==XeenMonsterIdentity{20,25}&&!triple){check(h.party->encounterContext->minutes==533&&h.world->sessionState().actors()[25].hp==17,"P1 seed78 minute533 damaged25 triple");triple=true;if(original)visual_remove_test::save(h.flow->frame(),path.string()+"-triple.bmp");}
    if(c->phase()==Phase::PlayerReady){
     const auto old=*handler.displayedInput(), rng=h.world->sessionState().journeyRandom()->count;const auto turn=c->participant();
     unsigned occupied=0;for(auto id:rows)if(id)++occupied;
     // Every displayed row is an identity-bound intent; selection consumes no turn/RNG.
     for(unsigned row=0;row<occupied;++row){press(h,handler,SelectInventorySlotAction{row});check(c->selectedTarget()==rows[row]&&c->participant()==turn&&h.world->sessionState().journeyRandom()->count==rng,"production row selection no turn or RNG");}
     press(h,handler,SelectInventorySlotAction{0});
     handler.withDisplayedInput(InteractionAction{},old);check(c->participant()==turn&&h.world->sessionState().journeyRandom()->count==rng,"stale selection batch cannot attack replacement");
     const auto gen=*handler.displayedInput();handler.withDisplayedInput(SelectInventorySlotAction{8},gen);check(c->participant()==turn&&h.world->sessionState().journeyRandom()->count==rng,"empty row refuses");handler.framePresented();
     press(h,handler,InteractionAction{});
    } else {check(c->phase()!=Phase::Failed&&c->phase()!=Phase::SupportStopped&&c->phase()!=Phase::Defeat,"connected combat failure");tick(h,handler,idle);}
    if(h.flow->encounter()->combat()){auto stages=h.saves;handler.withDisplayedInput(SaveGameAction{},*handler.displayedInput());check(h.saves==stages,"busy combat F9 refuses all providers");}
   }
   check(!h.flow->encounter()->combat(),"automatic End/retirement completed");
   if(original&&schedule>=2)check(mixed,"retained entry move mixed checkpoint observed");
   check(h.flow->canSave(),"retired matching frame quiet");++ends;
   std::cout<<"production End "<<h.party->encounterContext->minutes<<" RNG "<<h.world->sessionState().journeyRandom()->count<<'\n';
  };
  const auto action=[&](PlayerAction a){press(h,handler,a);if(h.flow->encounter()->combat())settle();
   if(schedule<2)for(unsigned n=0;h.flow->encounter()->state().pending()&&n<20;++n){tick(h,handler,idle);if(h.flow->encounter()->combat())settle();}
  };
  if(stop==0){save();return true;}
  for(unsigned i=start;i<5;++i){action(NavigationAction::MoveForward);if(original&&schedule<2&&i==0)check(h.party->encounterContext->minutes==491,"P1 first25 End491");if(original&&schedule<2&&i==3)check(h.party->encounterContext->minutes==522,"P1 successive9 End522");if(stop==i+1){save();return true;}}
  if(start!=6){
   if(start<=5){
   if(schedule==1)for(unsigned i=0;i<3;++i)action(WaitAction{});
   if(original&&schedule==1)visual_remove_test::save(h.flow->frame(),path.string()+"-condition.bmp");
   if(original&&schedule==1)check(h.flow->encounter()->notice().find("Rebecca 5/18\nSP21/18 D3")!=std::string::npos&&h.party->encounterContext->minutes==565&&h.party->roster.at(1).currentHp==5&&h.party->roster.at(1).conditions[4]==3,"P1 pair End565 RebeccaHP5 D3");
   if(original&&schedule==2)check(h.party->encounterContext->minutes==532&&h.party->roster.at(1).currentHp==12&&h.party->roster.at(1).conditions[4]==2,"P1 mixed End532 RebeccaHP12 D2");
   if(original&&schedule==3)check(h.party->encounterContext->minutes==536&&triple,"P1 joined End536");
   action(NavigationAction::TurnLeft);
   }
   if(stop==7){save();return true;}
   if(start!=8 && !h.flow->encounter()->state().pending()) collect(h,handler,path);
   if(start==8){unsigned count=0;h.flow->reportManual=[&](const auto &r){if(const auto *c=std::get_if<XeenManualEventCompleted>(&r))count=c->instructionCount;};press(h,handler,InteractionAction{});check(count==5,"postcollection restart repeat has five None only");h.flow->reportManual={};}
   if(stop==8){save();return true;}
   action(NavigationAction::TurnLeft);for(unsigned i=0;i<5;++i)action(NavigationAction::MoveForward);
   for(unsigned n=0;h.flow->encounter()->state().pending()&&n<20;++n)tick(h,handler,idle);
   check(h.camera->x==0&&h.camera->y==14&&h.camera->direction==XeenDirection::West,"survivor return endpoint");
   if(original&&schedule<2)check(h.party->encounterContext->minutes==(schedule?615:582),"P1 return615/582");
   if(original&&(schedule==0||schedule==2))for(auto id:{17,18})check(h.world->sessionState().actors()[id].activated&&h.world->sessionState().actors()[id].x==(schedule==0?8:7)&&h.world->sessionState().actors()[id].hp==30,"activated17/18 survivors retained on return");
   if(stop==6){save();return true;}
  }
  // Further inventory/equipment and navigation after return/restart, with exact current values.
  const auto hp=h.party->roster.at(6).currentHp,sp=h.party->roster.at(6).currentSp;
  press(h,handler,InspectInventoryAction{});auto stages=h.saves;press(h,handler,SaveGameAction{});check(stages==h.saves,"inventory F9 does no I/O");
  for(const PlayerAction &a:std::vector<PlayerAction>{SelectMemberAction{5},NavigationAction::TurnRight,NavigationAction::TurnRight,SelectInventorySlotAction{1},EquipmentInventoryAction{},InspectInventoryAction{}})press(h,handler,a);
  check(h.party->roster.at(6).currentHp==hp&&h.party->roster.at(6).currentSp==sp&&h.party->roster.at(6).accessories[1].frame==8,"postrestart equipment changes item only, currentHP/SP exact");
  action(NavigationAction::TurnRight);action(NavigationAction::TurnRight);action(NavigationAction::MoveForward);
  while(h.flow->encounter()->state().pending())tick(h,handler,idle);
  save();return true;
 };
 const int result=Application().playGameplay(s,{},path,restart,restart?XeenEncounterEntry::Ordinary:XeenEncounterEntry::Journey,restart?std::optional<std::uint32_t>{}:schedule==3?78u:1u,restart?std::optional<std::uint16_t>{}:2);
 check(result==0,"Application expedition route");
}
}
int main(int argc,char **argv){try{
 if((argc==4||argc==5)&&std::string(argv[1])=="--unfavorable"){unfavorable(fs::path(argv[2]),fs::absolute(argv[3]),argc==5?std::stoul(argv[4]):3);return 0;}
 if(argc==8&&std::string(argv[1])=="--child"){route(fs::path(argv[2]),fs::absolute(argv[3]),std::stoul(argv[4]),std::stoul(argv[5]),std::stoul(argv[6]),std::string(argv[7])=="load");return 0;}
 projection();
 const auto dir=fs::temp_directory_path()/("mmodern-m30b-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));fs::create_directories(dir);
 if(argc==2){const fs::path game=fs::absolute(argv[1]);for(unsigned schedule=0;schedule<4;++schedule){auto direct=dir/("direct"+std::to_string(schedule)+".mmsave");route(game,direct,schedule,0,99,false);
  for(unsigned checkpoint:{0u,1u,6u,7u,8u}){if(schedule>=2&&(checkpoint==1||checkpoint>=7))continue;auto continued=dir/("continued"+std::to_string(schedule)+"-"+std::to_string(checkpoint)+".mmsave");
   const auto child=[&](unsigned start,unsigned stop,bool resume,const std::string &label){auto result=child_test::launch(fs::absolute(argv[0]),{L"--child",game.wstring(),continued.wstring(),std::to_wstring(schedule),std::to_wstring(start),std::to_wstring(stop),resume?L"load":L"fresh"},dir/(label+".log"));if(result.exit)std::cerr<<result.output;check(result.exit==0,"separate process production route");std::cout<<label<<" PID="<<result.pid<<" passed\n";};
   const auto label=std::to_string(schedule)+"-"+std::to_string(checkpoint);child(0,checkpoint,false,"producer"+label);child(checkpoint,99,true,"consumer"+label);
   check(diskBytes(direct)==diskBytes(continued),"uninterrupted versus separate-process restart exact complete wire");
   check(diskBytes(direct.string()+".rng")==diskBytes(continued.string()+".rng"),"restart preserves every inclusive RNG request/result/state/count");
  }
 }
  unfavorable(game,dir/"unfavorable.mmsave");
  const auto cli=fs::absolute(argv[0]).parent_path()/"mmodern.exe";const auto saved=dir/"direct0.mmsave";
  for(bool resume:{false,true}){const auto before=diskBytes(saved);const auto args=resume?std::vector<std::wstring>{L"--load-game",game.wstring(),saved.wstring()}:std::vector<std::wstring>{L"--journey-expedition",L"--combat-seed",L"1",game.wstring(),L"--save-file",saved.wstring()};auto result=child_test::launch(cli,args,dir/(resume?"product-load.log":"product-entry.log"),true,true);if(result.exit)std::cerr<<result.output;check(result.exit==0&&diskBytes(saved)==before,"uninstrumented expedition CLI/SDL startup inventory exit does not rewrite save");}
 } else {const auto path=dir/"synthetic.mmsave";route({},path,0,0,0,false);controls(path);deathControl(path);}
 std::cout<<"M31 production collection/projection/route/restart controls passed. Evidence "<<dir.u8string()<<'\n';return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
