#include "XeenRegionalSaveGameplayTestSupport.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>
using namespace gameplay_test;
namespace fs=std::filesystem;
int main(){try{
 const auto dir=fs::current_path()/"save-sdl-tests";fs::create_directories(dir);const auto path=dir/"session.mmsave";
 for(bool quitFirst:{false,true}){
  fs::remove(path);regional_save_test::Fixture f;
  f.saved.camera={23,8,2,XeenDirection::North};XeenSaveFile::write(path,f.saved);
  auto services=f.services();regional_save_test::phirna(services);unsigned saves=0,refused=0;bool titleSeen=false;
  services.show=[&](const auto &first,const auto &handle,const auto &escape,const auto &idle,const auto &status){
   std::atomic<bool> finished{false};std::exception_ptr senderError;
   std::thread sender([&]{try{
    const auto push=[&](SDL_Event e){for(unsigned i=0;i<100&&!finished;++i){std::this_thread::sleep_for(std::chrono::milliseconds(20));if(SDL_PushEvent(&e)==1)return;}throw std::runtime_error("SDL push failed");};
    if(!quitFirst){for(auto step:std::vector<std::pair<SDL_Keycode,Uint8>>{{SDLK_F9,1},{SDLK_F9,0},{SDLK_SPACE,0},{SDLK_F9,0},{SDLK_F9,1},{SDLK_9,0},{SDLK_F9,0},{SDLK_ESCAPE,0},{SDLK_F9,0}}){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=step.first;e.key.repeat=step.second;push(e);if(!step.second){e.type=SDL_KEYUP;push(e);}std::this_thread::sleep_for(std::chrono::milliseconds(40));}}
    SDL_Event quit{};quit.type=SDL_QUIT;push(quit);
    SDL_Event late{};late.type=SDL_KEYDOWN;late.key.keysym.sym=SDLK_F9;SDL_PushEvent(&late);
   }catch(...){senderError=std::current_exception();SDL_Event q{};q.type=SDL_QUIT;SDL_PushEvent(&q);}});
   auto observed=handle;
   observed.withPresentedInput=[&](const PlayerAction &action,std::uint64_t token,const IndexedFrame::Presentation &presentation){
    const auto gen=f.flow->presentationGeneration();const auto frame=f.flow->frame().pixels;
    auto next=handle.withPresentedInput(action,token,presentation);
    if(std::holds_alternative<SaveGameAction>(action)){
     if(f.flow->blocksGameplay()){++refused;check(gen==f.flow->presentationGeneration()&&frame==f.flow->frame().pixels,"SDL F9 advanced pending state");}
     else ++saves;
    }return next;
   };
   const bool result=SdlWindow().showInteractive(first,"Save SDL test",observed,escape,[&]{for(Uint32 id=1;id<20;++id)if(auto *window=SDL_GetWindowFromID(id))if(std::string(SDL_GetWindowTitle(window)).find("Saved")!=std::string::npos)titleSeen=true;return idle();},status);
   finished=true;sender.join();if(senderError)std::rethrow_exception(senderError);return result;
  };
  check(Application().playGameplay(services,{},path,true)==0,"SDL Application failed");
  check(saves==(quitFirst?0U:2U)&&refused==(quitFirst?0U:2U),"F9 repeats/quit ordering");
  sameSnapshot(f.saved,XeenSaveFile::read(path));
  if(!quitFirst){check(titleSeen,"native window title not updated");check(XeenSaveFile::read(path).questItems[17]==0,"SDL save changed gameplay");}
 }
 std::cout<<"Production SDL F9, pending refusal, title and quit ordering passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
