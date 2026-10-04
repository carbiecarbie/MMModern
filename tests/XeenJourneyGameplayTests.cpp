#include "XeenCombatGameplayTestSupport.h"
#include "XeenRegionalSaveGameplayTestSupport.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "XeenCosmeticHandoffTestSupport.h"
#include <iostream>
using namespace mmodern;
using gameplay_test::check;
namespace fs=std::filesystem;
namespace {
std::vector<std::uint8_t> diskBytes(const fs::path &path){std::ifstream in(path,std::ios::binary);check(bool(in),"disk evidence read");return {std::istreambuf_iterator<char>(in),{}};}
struct Harness:regional_save_test::Fixture {
 unsigned saves=0;
 auto services(){auto s=Fixture::services();s.observeSaveStage=[&](auto){++saves;};return s;}
};
void quietCosmeticInput() {
 Harness h;auto services=h.services();bool inject=false;unsigned issued=0,settled=0,loops=0,stage=0;
 std::optional<std::uint64_t> semantic;
 const auto compose=services.composeEncounter;
 services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor){
  auto frame=compose(w,p,c,phase,actor);
  if(inject){inject=false;++issued;
   // Sample an early key while a purely cosmetic Quiet frame is being built.
   SDL_Event down{};down.type=SDL_KEYDOWN;down.key.keysym.sym=SDLK_RIGHT;
   check(SDL_PushEvent(&down)==1,"Queue early key during Quiet recomposition");
   SDL_Event up=down;up.type=SDL_KEYUP;check(SDL_PushEvent(&up)==1,"Queue release");
   SDL_Delay(5); // Distinguish sampling from the later frame handoff timestamp.
  }
  return frame;
 };
 services.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  const auto driver=[&]()->std::optional<IndexedFrame>{
   check(++loops<80,"Quiet input control terminates");
   if(stage==0) {
    check(h.flow->canSave(),"Quiet cosmetic starting authority");
    if(settled==3){SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return {};}
    semantic=handler.displayedInput();inject=true;stage=1;h.now+=100;return idle();
   }
   if(stage==1) {
    if(!h.flow->canSave())return idle();
    check(handler.displayedInput()!=semantic && issued==settled+1 &&
        h.party->encounterContext->ctr24==2*settled+1,"Live A input during cosmetic preparation must execute under A");
    SDL_Event down{};down.type=SDL_KEYDOWN;down.key.keysym.sym=SDLK_RIGHT;
    check(SDL_PushEvent(&down)==1,"Fresh key after Quiet acquisition");
    down.type=SDL_KEYUP;check(SDL_PushEvent(&down)==1,"Fresh Quiet release");stage=2;return {};
   }
   if(h.flow->canSave()) {
    check(h.party->encounterContext->ctr24==2*issued,"Fresh Quiet movement was discarded after acquisition");
    settled=issued;stage=0;return {};
   }
   h.now+=100;return idle();
  };
  return SdlWindow().showInteractive(first,"Quiet cosmetic input",handler,escape,driver,status);
 };
 check(Application().playGameplay(services,xeenJourneyContent().entry,{},false,XeenEncounterEntry::Journey,56)==0 && settled==3,"Three consecutive fresh Quiet commands through real SDL");
}
void cosmeticOmissions() {
 for(bool inventory:{false,true})for(unsigned mode=0;mode<6;++mode) {
  Harness h;auto services=h.services();cosmetic_handoff_test::changingPixels(services);
  services.show=[&](const auto &,const auto &handler,const auto &escape,const auto &idle,const auto &status){
   handler.framePresented(h.flow->frame().presentation());
   if(inventory){h.flow->drawDialogSprite=[](auto &frame,const char *,unsigned id,int x,int y){frame.pixels[y*320+x]=id;};handler.beginCycle(++h.cycle);handler.withDisplayedInput(SelectMemberAction{0},*handler.displayedInput());handler.framePresented(h.flow->frame().presentation());}
   const auto unchanged=[&]{check(h.flow->inventoryOpen()==inventory && h.camera->direction==XeenDirection::West,"Omission preserves inventory lease and navigation");};
   const auto accepted=[&]{check(inventory?!h.flow->inventoryOpen():h.camera->direction==XeenDirection::North,"Fresh input accepted once after correct cosmetic upload");};
   const PlayerAction action=inventory?PlayerAction{DialogKeyAction{InputKey::Escape}}:PlayerAction{NavigationAction::TurnRight};
   return cosmetic_handoff_test::exercise(h,handler,idle,escape,status,mode,inventory?SDLK_ESCAPE:SDLK_RIGHT,action,unchanged,accepted);
  };
  check(Application().playGameplay(services,xeenJourneyContent().entry,{},false,XeenEncounterEntry::Journey,56)==0,"Quiet/inventory cosmetic handoff result");
 }
}
void sdlBoundaries() {
 for(bool lost:{false,true}) {
  Harness h;auto s=h.services();unsigned stage=0,stable=0,loops=0,noEvents=0;bool done=false,omitted=false;std::vector<SDL_Event> pendingKeys;
  s.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status) {
   const auto key=[&](SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0){SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.repeat=repeat;pendingKeys.push_back(e);};
   auto native=handler;
   native.beginCycle=[&](auto cycle){handler.beginCycle(cycle);for(auto &event:pendingKeys)check(SDL_PushEvent(&event)==1,"fresh Journey key");pendingKeys.clear();};
   h.flow->reportManual=[&](const auto &){++noEvents;};
   const auto driver=[&]()->std::optional<IndexedFrame>{
    check(++loops<150,"bounded Journey SDL input test");
    if(omitted){check(!h.flow->canSave()&&!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"Unpresented forced refresh remains closed");SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return {};}
    auto frame=idle();if(frame){stable=0;return frame;}if(++stable<2)return frame;stable=0;
    switch(stage++) {
    case 0:key(SDLK_F1);key(SDLK_F1,SDL_KEYUP);key(SDLK_RIGHT);key(SDLK_RIGHT,SDL_KEYDOWN,1);key(SDLK_F9);key(SDLK_F9,SDL_KEYUP);key(SDLK_SPACE);key(SDLK_SPACE,SDL_KEYUP);break;
    case 1:check(h.camera->direction==XeenDirection::West&&h.party->encounterContext->ctr24==0&&h.flow->inventoryOpen()&&h.saves==0&&noEvents==0,"strict inventory opening flushes queued navigation/Space");key(SDLK_ESCAPE);key(SDLK_ESCAPE,SDL_KEYUP);key(SDLK_RIGHT);break;
    case 2:check(h.camera->direction==XeenDirection::West && !h.flow->inventoryOpen(),"held navigation cannot repeat across inventory context");key(SDLK_RIGHT,SDL_KEYUP);break;
    case 3:key(SDLK_RIGHT);key(SDLK_RIGHT,SDL_KEYUP);break;
    case 4:check(h.camera->direction==XeenDirection::North&&h.party->encounterContext->ctr24==1,"released fresh navigation accepted");key(SDLK_F1);break;
    case 5:check(h.flow->inventoryOpen()&&!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"SDL modal capture lease");key(SDLK_F1);break;
    case 6:check(h.flow->inventoryOpen(),"held F1 cannot cross sheet boundary");key(SDLK_F1,SDL_KEYUP);break;
    case 7:key(SDLK_ESCAPE);key(SDLK_ESCAPE,SDL_KEYUP);break;
    default:check(!h.flow->inventoryOpen()&&h.flow->canSave(),"matching closed inventory handoff");done=true;
     if(lost){h.flow->refresh(true);omitted=true;return std::nullopt;}
     {SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}break;
    }
    return frame;
   };
   const bool ok=SdlWindow().showInteractive(first,"Journey SDL boundary",native,escape,driver,status);
   check(done&&ok&&!h.flow->canSave()&&!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"SDL lost-upload/shutdown closes Journey");
   return ok;
  };
  check(Application().playGameplay(s,xeenJourneyContent().entry,{},false,XeenEncounterEntry::Journey,56)==0,"Journey SDL boundary result");
 }
}
void press(Harness &h,const SdlWindow::FrameUpdateHandler &handler,const PlayerAction &a) {
 handler.beginCycle(++h.cycle);
 handler.withDisplayedInput(a,*handler.displayedInput());
 check(handler.frameCurrent(),"current production frame");handler.framePresented(h.flow->frame().presentation());
}
void tick(Harness &h,const SdlWindow::FrameUpdateHandler &handler,const SdlWindow::IdleFrameHandler &idle) {
 h.now+=100;handler.beginCycle(++h.cycle);idle();check(handler.frameCurrent(),"current idle frame");handler.framePresented(h.flow->frame().presentation());
}
void boundaries(const fs::path &dir) {
 for(unsigned mode=0;mode<11;++mode) {
  if(mode>=6&&mode<=8)continue;
  Harness h;auto s=h.services();std::optional<XeenSaveSnapshot> before;const auto path=dir/("boundary-"+std::to_string(mode)+".mmsave");
  XeenSaveFile::write(path,save_test::sample());const auto old=diskBytes(path);
  bool armed=false,injected=false;unsigned attempts=0,samples=0;
  s.sampleJourneySeed=[&]{++samples;return mode==10?0u:56u;};

  const auto compose=s.composeEncounter;
  s.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor) {
   if(armed&&mode==0&&++attempts==1){injected=true;throw std::runtime_error("isolated composition fault");}
   return compose(w,p,c,phase,actor);
  };
  s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &) {
   check(samples==1,"default seed sampled once");handler.framePresented(h.flow->frame().presentation());before=h.capture();armed=true;
   if(mode==0||mode==1) {
    if(mode==1)h.flow->beforeEncounterFrameCopy=[&]{if(!injected){injected=true;throw std::runtime_error("frame copy fault");}};
    const auto generation=*handler.displayedInput();
    handler.withDisplayedInput(InspectInventoryAction{},generation);
    check(injected&&!h.flow->canSave()&&!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"recovered modal frame capture closed");
    handler.withDisplayedInput(SaveGameAction{},generation);check(h.saves==0,"stale F9 before recovered upload");
    handler.framePresented(h.flow->frame().presentation());check(h.flow->inventoryOpen(),"recovery kept modal state");
    check(!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"modal world capture lease");
    press(h,handler,CancelInteractionAction{});check(h.flow->canSave(),"matching close handoff opens capture");save_test::sameSnapshot(*before,h.capture());
   } else if(mode==2) {
    unsigned reports=0;h.flow->reportManual=[&](const auto &){++reports;throw std::runtime_error("no-event report fault");};
    press(h,handler,InteractionAction{});check(reports==1&&h.flow->canSave(),"no-event bounded recovery without replay");save_test::sameSnapshot(*before,h.capture());
   } else if(mode==3) {
    const auto original=h.party->roster.at(0).currentHp;
    const_cast<XeenPartyState*>(h.party)->roster.at(0).currentHp++;
    check(!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"direct capture latches unauthorized mutation");
    const_cast<XeenPartyState*>(h.party)->roster.at(0).currentHp=original;
    check(!XeenSaveState::canCapture(*h.party,*h.camera,*h.world)&&!h.flow->canSave(),"exact restoration cannot revive authority");
    handler.withDisplayedInput(SaveGameAction{},*handler.displayedInput());check(h.saves==0,"integrity F9 before providers");return false;
   } else if(mode==4||mode==5) {
    h.flow->refresh(true);if(mode==4)handler.failed();else handler.closed();
    check(!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"lost upload / shutdown closure");
    handler.withDisplayedInput(SaveGameAction{},*handler.displayedInput());check(h.saves==0,"closed F9");return false;
   } else if(mode==9) {
    // Save callbacks are observed through the supplied service function below.
    press(h,handler,SaveGameAction{});
    check(injected&&diskBytes(path)==old&&h.flow->canSave(),"failed detached save preserves target and live authority");
   }
   return true;
  };
  s.observeSaveStage=[&](auto stage){++h.saves;if(mode==9&&stage==XeenGameplayServices::SaveStage::Preflight){injected=true;throw std::runtime_error("detached preflight failure");}};
  const int expected=mode>=3&&mode<=5?4:0;
  check(Application().playGameplay(s,xeenJourneyContent().entry,path,false,XeenEncounterEntry::Journey,{})==expected,"Journey boundary matrix");
 }
}
}
int main(int argc,char **argv){try{
 if(argc==2&&std::string(argv[1])=="sdl"){quietCosmeticInput();cosmeticOmissions();sdlBoundaries();}
 else {const auto dir=fs::current_path()/"journey-gameplay-tests";fs::create_directories(dir);boundaries(dir);}
 std::cout<<"Regional Journey production and SDL boundary controls passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
