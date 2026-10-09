#include "XeenExpeditionTestSupport.h"
#include "XeenRegionalSaveGameplayTestSupport.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "games/xeen/XeenOutdoorScene.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
using namespace mmodern;
using combat_test::check;
using combat_test::Phase;
using combat_test::Work;
struct Harness:regional_save_test::Fixture {unsigned saves=0;auto services(){auto s=Fixture::services();s.observeSaveStage=[&](auto){++saves;};return s;}};
namespace fs=std::filesystem;
namespace {
std::vector<std::uint8_t> diskBytes(const fs::path &path){std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
auto services(Harness &h){auto s=h.services();s.objects=[](auto id){auto o=regional_test::objects(id);if(id==XeenMapIdentity(23))o.entities.monsters[16].resourceId=9;return o;};s.resources.loadMonsterStatistics=[]{auto m=regional_test::statistics();m[9]=expedition_fixture::monsters()[9];return m;};return s;}
void press(Harness &h,const SdlWindow::FrameUpdateHandler &handler,const PlayerAction &a) {
 handler.beginCycle(++h.cycle);handler.withDisplayedInput(a,*handler.displayedInput());
 check(handler.frameCurrent(),"current Application input frame");handler.framePresented(h.flow->frame().presentation());
}
void tick(Harness &h,const SdlWindow::FrameUpdateHandler &handler,const SdlWindow::IdleFrameHandler &idle) {
 h.now+=100;handler.beginCycle(++h.cycle);idle();check(handler.frameCurrent(),"current automatic frame");handler.framePresented(h.flow->frame().presentation());
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
 auto grouped=initial;grouped.camera={23,4,14,XeenDirection::East};
 for(auto &a:grouped.journey->actors){if(a.id.recordIndex==9||a.id.recordIndex==16){a.x=5;a.y=14;a.hp=a.id.recordIndex==16?30:20;a.activated=true;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;}else{a.x=a.y=-128;a.hp=0;a.activated=false;a.accounted=true;a.lifecycle=XeenActorLifecycle::Defeated;}}
 XeenSaveFile::write(path,grouped);
 // Recomposition failure follows a real publication, and cannot rerun it.
 {Harness h;auto s=services(h);bool injected=false;
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){handler.framePresented(h.flow->frame().presentation());press(h,handler,NavigationAction::MoveForward);
  const auto old=*handler.displayedInput();auto *c=h.flow->encounter()->combat();check(c&&c->contacts()[1],"mixed production attachment");
  const auto rng=h.world->sessionState().journeyRandom()->count;
  h.flow->beforeEncounterFrameCopy=[&]{if(!injected&&h.world->sessionState().journeyRandom()->count>rng){injected=true;check(!h.flow->canSave(),"capture closed after publication before handoff");throw std::runtime_error("postpublication frame fault");}};
  for(unsigned n=0;!injected&&n<100;++n){if(c->phase()==Phase::PlayerReady)press(h,handler,InteractionAction{});else tick(h,handler,idle);}
  check(injected&&h.flow->encounter()->combat()->phase()!=Phase::Failed,"one guarded rebuild retains result");
  const auto after=*h.world->sessionState().journeyRandom();std::vector<XeenActor> actors=h.world->sessionState().actors();handler.withDisplayedInput(InteractionAction{},old);
  check(*h.world->sessionState().journeyRandom()==after&&xeen_state::sameActor(actors[9],h.world->sessionState().actors()[9]),"stale frame cannot replay published attack");return true;};
 check(Application().playGameplay(s,{},path,true)==0,"production postpublication rebuild");}
 // A second composition failure closes capture while retaining the published RNG/result.
 {Harness h;auto s=services(h);bool failed=false;std::uint64_t initialCount=0;
 const auto compose=s.composeEncounter;s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto ordinary,auto appearance){
  if(h.flow&&h.flow->encounter()->combat()&&w.sessionState().journeyRandom()->count>initialCount)throw std::runtime_error("persistent postpublication composition fault");
  return compose(w,p,c,ordinary,appearance);};
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){handler.framePresented(h.flow->frame().presentation());initialCount=h.world->sessionState().journeyRandom()->count;press(h,handler,NavigationAction::MoveForward);
  const auto old=*handler.displayedInput();
  for(unsigned n=0;!failed&&n<100;++n){try{if(h.flow->encounter()->combat()->phase()==Phase::PlayerReady)press(h,handler,InteractionAction{});else tick(h,handler,idle);}catch(const std::exception &){failed=true;}}
  check(failed&&h.world->sessionState().journeyRandom()->count>initialCount&&!h.flow->canSave(),"fatal composition retains published cursor and closes capture");
  check(h.flow->encounter()->notice().find("FAILED:")!=std::string::npos,"stopped state does not advertise ready controls");
  const auto after=*h.world->sessionState().journeyRandom();const auto stages=h.saves;
  handler.withDisplayedInput(SaveGameAction{},old);handler.withDisplayedInput(InteractionAction{},old);
  check(h.saves==stages&&*h.world->sessionState().journeyRandom()==after,"failed frame cannot save or replay");return true;};
 check(Application().playGameplay(s,{},path,true)==0,"production fatal frame closure");}
 // Real SDL poll batches and held keys at a two-target contact.
 {Harness h;auto s=services(h);unsigned stage=0,loops=0,stable=0,queuedAttacks=0;std::uint64_t rng=0;int participant=0;
 const auto key=[](SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0){SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.timestamp=SDL_GetTicks()+1;e.key.repeat=repeat;check(SDL_PushEvent(&e)==1,"SDL target key queue");};
 s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  auto native=handler;
  native.withPresentedInput=[&](const auto &action,auto token,const auto &origin){
   const auto generation=h.flow->encounter()->combat()?h.flow->encounter()->combat()->result().generation:0;
   auto frame=handler.withPresentedInput(action,token,origin);
   if(std::holds_alternative<AttackAction>(action) && h.flow->encounter()->combat()->result().generation!=generation)++queuedAttacks;
   return frame;
  };
  auto driver=[&]()->std::optional<IndexedFrame>{check(++loops<100,"bounded SDL target controls");if(h.flow->encounter()->combat()&&h.flow->encounter()->combat()->pending()!=Work::None)h.now+=100;auto frame=idle();if(frame){stable=0;return frame;}if(++stable<2)return frame;stable=0;
   auto *c=h.flow->encounter()->combat();
   switch(stage++){
   case 0:key(SDLK_UP);key(SDLK_UP,SDL_KEYUP);break;
   case 1:check(c&&c->phase()==Phase::PlayerReady,"SDL automatic attachment");rng=h.world->sessionState().journeyRandom()->count;participant=c->participant();key(SDLK_2);key(SDLK_a);key(SDLK_a,SDL_KEYUP);key(SDLK_F9);key(SDLK_F9,SDL_KEYUP);break;
   case 2:if(queuedAttacks!=1 || c->phase()!=Phase::PlayerReady){--stage;break;}
    check(c->selectedTarget()==XeenMonsterIdentity{23,16}&&h.world->sessionState().journeyRandom()->count>rng&&h.saves==0,"SDL selection queues Space once and refuses F9");
    rng=h.world->sessionState().journeyRandom()->count;participant=c->participant();key(SDLK_1);key(SDLK_1,SDL_KEYUP);break;
   case 3:check(c->selectedTarget()==XeenMonsterIdentity{23,9},"SDL fresh target row");key(SDLK_2);key(SDLK_2,SDL_KEYDOWN,1);break;
   case 4:check(c->selectedTarget()==XeenMonsterIdentity{23,9}&&h.world->sessionState().journeyRandom()->count==rng,"SDL held/repeated target cannot select replacement");key(SDLK_2,SDL_KEYUP);break;
   case 5:key(SDLK_2);key(SDLK_2,SDL_KEYUP);break;
   default:check(c->selectedTarget()==XeenMonsterIdentity{23,16}&&c->participant()==participant,"released fresh target accepted without turn");{SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}break;
   }return frame;};
  return SdlWindow().showInteractive(first,"M30B target controls",native,escape,driver,status);};
 check(Application().playGameplay(s,{},path,true)==0&&stage>=7,"real SDL target generation safety");}
}
void deathControl(const fs::path &path) {
 auto saved=XeenSaveFile::read(path);
 for(auto &a:saved.journey->actors)if(a.id.recordIndex!=16){a.hp=0;a.x=a.y=-128;a.activated=false;a.accounted=true;a.lifecycle=XeenActorLifecycle::Defeated;}
 for(auto owner:kXeenCombatOwners){auto &c=saved.characters[owner];c.currentHp=0;c.conditions[12]=1;}
 auto &cleric=saved.characters[1];cleric.currentHp=1;cleric.conditions[12]=0;cleric.permanentLevel=1;cleric.temporaryLevel=0;cleric.endurance={0,0};
 XeenSaveFile::write(path,saved);const auto before=diskBytes(path);Harness h;auto s=services(h);
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){handler.framePresented(h.flow->frame().presentation());press(h,handler,NavigationAction::MoveForward);
  for(unsigned n=0;n<1000&&h.flow->encounter()->combat()->phase()!=Phase::Defeat;++n){auto *c=h.flow->encounter()->combat();check(c->phase()!=Phase::Failed&&c->phase()!=Phase::SupportStopped,"death fixture remains supported");if(c->phase()==Phase::PlayerReady)press(h,handler,BlockAction{});else tick(h,handler,idle);}
  check(h.flow->encounter()->combat()->phase()==Phase::Defeat&&h.party->roster.at(1).conditions[13]&&h.flow->encounter()->notice().find("Dead")!=std::string::npos,"production injury sets and presents death");
  check(h.flow->encounter()->combatObservation().armorCount&&h.flow->encounter()->notice().find("armor broken 2")!=std::string::npos,"production lethal injury presents armor breakage");
  const auto stages=h.saves;handler.withDisplayedInput(SaveGameAction{},*handler.displayedInput());check(!h.flow->canSave()&&stages==h.saves&&diskBytes(path)==before,"death cannot save or recover");return true;};
 check(Application().playGameplay(s,{},path,true)==0,"synthetic successor death production control");
}
}
int main(){try{
 projection();const auto dir=fs::current_path()/"expedition-gameplay-tests";fs::create_directories(dir);const auto path=dir/"regional.mmsave";
 auto initial=regional_test::snapshot();initial.journey->actors[16].hp=30;initial.journey->random=XeenJourneyRandomState{1,1,0};const auto bytes=combat_test::chr();initial.characters=XeenPartyLoader().loadFromResources(bytes,combat_test::pty()).roster.characters();for(unsigned i=0;i<30;++i){initial.characters[i].learnedSpells=XeenCharacterFormat::parseLearnedSpells(bytes,i);initial.journey->supplements[i]={static_cast<std::uint8_t>(i),XeenCharacterFormat::parseCombatInputs(bytes,i,true,true,true)};}XeenSaveFile::write(path,initial);controls(path);deathControl(path);
 std::cout<<"Regional group projection, input, publication and death controls passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
