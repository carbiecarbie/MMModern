// One original-data Application/SDL path for both New difficulties and title
// Load. Only seed sampling and the Local AppData test directory are substituted.
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "platform/XeenSaveFile.h"
#include "XeenSaveTestSupport.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <cstdlib>
using namespace mmodern;
namespace fs=std::filesystem;
using save_test::check;
#define PLAY_SYMBOL "_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE"
#define SHOW_SYMBOL "_ZNK7mmodern9SdlWindow15showInteractiveERKNS_12IndexedFrameERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERKNS0_18FrameUpdateHandlerERKSt8functionIFbvEERKSF_IFSt8optionalIS1_EvEERKSF_IFS9_vEE"
#define DIRECTORY_SYMBOL "_ZN7mmodern12XeenSaveFile19createSlotDirectoryERKNS_16GameInstallationERKNSt10filesystem7__cxx114pathERKSt8optionalIS6_E"
extern "C" int realTitlePlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>) asm("__real_" PLAY_SYMBOL);
extern "C" int wrappedTitlePlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>) asm("__wrap_" PLAY_SYMBOL);
extern "C" bool realTitleShow(const SdlWindow *,const IndexedFrame &,const std::string &,const SdlWindow::FrameUpdateHandler &,
 const std::function<bool()> &,const SdlWindow::IdleFrameHandler &,const std::function<std::string()> &) asm("__real_" SHOW_SYMBOL);
extern "C" bool wrappedTitleShow(const SdlWindow *,const IndexedFrame &,const std::string &,const SdlWindow::FrameUpdateHandler &,
 const std::function<bool()> &,const SdlWindow::IdleFrameHandler &,const std::function<std::string()> &) asm("__wrap_" SHOW_SYMBOL);
extern "C" fs::path realTitleDirectory(const GameInstallation &,const fs::path &,const std::optional<fs::path> &) asm("__real_" DIRECTORY_SYMBOL);
extern "C" fs::path wrappedTitleDirectory(const GameInstallation &,const fs::path &,const std::optional<fs::path> &) asm("__wrap_" DIRECTORY_SYMBOL);
namespace {
struct State {
 bool enabled=false,loading=false,mouse=false,shown=false;
 unsigned seeds=0;
 XeenDifficulty difficulty=XeenDifficulty::Adventurer;
 fs::path base,slot;
 std::vector<std::uint8_t> saved;
 XeenWorld *world=nullptr;const XeenPartyState *party=nullptr;
 const XeenCamera *camera=nullptr;const XeenGameFlags *flags=nullptr;
 XeenSaveResourceSignature signature;
 std::string name="Native Case !~";
} state;
void quit(){SDL_Event event{};event.type=SDL_QUIT;check(SDL_PushEvent(&event)==1,"title native quit");}
void key(unsigned code) {
 SDL_Event event{};event.type=SDL_KEYDOWN;event.key.keysym.sym=code;event.key.keysym.scancode=SDL_GetScancodeFromKey(code);
 check(SDL_PushEvent(&event)==1,"title native key down");event.type=SDL_KEYUP;check(SDL_PushEvent(&event)==1,"title native key up");
}
void preview(const IndexedFrame &frame,const std::string &label) {
 const char *directory=std::getenv("MMODERN_TITLE_PREVIEW");if(!directory)return;
 auto *surface=SDL_CreateRGBSurfaceWithFormat(0,320,200,32,SDL_PIXELFORMAT_ARGB8888);check(surface,"title preview surface");
 for(unsigned y=0;y<200;++y)for(unsigned x=0;x<320;++x) {
  const auto index=frame.pixels[y*320+x]*3;
  reinterpret_cast<Uint32 *>(static_cast<Uint8 *>(surface->pixels)+y*surface->pitch)[x]=SDL_MapRGB(surface->format,frame.palette[index],frame.palette[index+1],frame.palette[index+2]);
 }
 const auto path=fs::path(directory)/(label+".bmp");const auto result=SDL_SaveBMP(surface,path.string().c_str());SDL_FreeSurface(surface);check(result==0,"title preview write");
}
void click(const DialogInput &dialog,unsigned value) {
 const DialogHit *hit=nullptr;for(const auto &candidate:dialog.hits)if(candidate.key==value){hit=&candidate;break;}
 check(hit,"native title click target");
 SDL_Window *window=nullptr;for(unsigned id=1;id<100 && !window;++id)window=SDL_GetWindowFromID(id);check(window,"native title window");
 SDL_Event event{};event.type=SDL_MOUSEBUTTONDOWN;event.button.button=SDL_BUTTON_LEFT;event.button.windowID=SDL_GetWindowID(window);
 event.button.x=(hit->left+hit->right-1)*3/2;event.button.y=(hit->top+hit->bottom-1)*3/2;
 check(SDL_PushEvent(&event)==1,"native title click");
}
}
extern "C" fs::path wrappedTitleDirectory(const GameInstallation &installation,const fs::path &repository,const std::optional<fs::path> &base) {
 return realTitleDirectory(installation,repository,state.enabled?std::optional<fs::path>{state.base}:base);
}
extern "C" int wrappedTitlePlay(const Application *app,const XeenGameplayServices &original,XeenCamera camera,
 const std::optional<fs::path> &target,bool resume,XeenEncounterEntry entry,std::optional<std::uint32_t> seed) {
 if(!state.enabled)return realTitlePlay(app,original,camera,target,resume,entry,seed);
 check(resume==state.loading && target.has_value() && !seed,"title session entry");
 state.slot=*target;state.signature=original.resources.signature;
 auto services=original;
 services.sampleJourneySeed=[] {++state.seeds;return 1u;};
 if(resume) {
  check(original.restoreSnapshot && !original.originalStart && !original.publishInitial,"title Load must retain snapshot");
  check(XeenSaveFormat::encode(*original.restoreSnapshot)==state.saved,"title chooser candidate differs from named save");
  check(fs::remove(*target),"remove test slot after candidate validation"); // No reread may occur after this point.
 } else check(original.originalStart==state.difficulty && original.publishInitial && !fs::exists(*target),"New published before initialization");
 const auto observe=services.observeGameplay;
 services.observeGameplay=[observe](auto &world,auto &events,const auto &party,auto &camera,const auto &flags) {
  if(observe)observe(world,events,party,camera,flags);
  state.world=&world;state.party=&party;state.camera=&camera;state.flags=&flags;
 };
 struct Clear {~Clear(){state.world=nullptr;state.party=nullptr;state.camera=nullptr;state.flags=nullptr;}} clear;
 return realTitlePlay(app,services,camera,target,resume,entry,seed);
}
extern "C" bool wrappedTitleShow(const SdlWindow *window,const IndexedFrame &first,const std::string &label,
 const SdlWindow::FrameUpdateHandler &handler,const std::function<bool()> &escape,const SdlWindow::IdleFrameHandler &idle,
 const std::function<std::string()> &status) {
 if(!state.enabled)return realTitleShow(window,first,label,handler,escape,idle,status);
 const bool title=label=="MMModern - World of Xeen";
 auto native=handler;IndexedFrame::Presentation presented;unsigned step=0;
 std::optional<std::uint64_t> emitted;
 const auto began=std::chrono::steady_clock::now();
 native.completeInputHandoff=[&](const auto &frame){handler.completeInputHandoff(frame);presented=frame;};
 const auto originalPresented=handler.framePresented;
 native.framePresented=[&](const auto &frame) {
  originalPresented(frame);
  if(!title && !state.shown) {
   check(state.world && state.party && state.camera && state.flags,"title native gameplay owners");
   auto snapshot=XeenSaveState::capture(state.signature,*state.party,*state.camera,*state.flags,*state.world);snapshot.name=state.name;
   const auto wire=XeenSaveFormat::encode(snapshot);
   if(state.loading)check(wire==state.saved && state.seeds==0,"title Load changed state/RNG or ran initialization");
   else {
    check(snapshot.journey->context->difficulty==state.difficulty && state.seeds==1,"New sampled seed/difficulty");
    check(XeenSaveFormat::encode(XeenSaveFile::read(state.slot))==wire,"New write did not precede first presented state");state.saved=wire;
   }
   preview(*frame,state.loading?"load-first":"new-first");state.shown=true;
  }
 };
 const auto nativeIdle=[&]() -> std::optional<IndexedFrame> {
  if(std::chrono::steady_clock::now()-began>std::chrono::seconds(15))throw std::runtime_error("Native title route stalled");
  const auto next=idle?idle():std::nullopt;
  if(!title){if(state.shown)quit();return next;}
  if(next || !presented)return next;
  const auto context=handler.inputContext(presented);
  if(!context.readyForAction || !context.dialog || emitted==context.contextId)return next;
  emitted=context.contextId;preview(*presented,"title-"+std::to_string(step));
  if(step==0){key(27);++step;return next;} // Original background -> menu.
  if(state.loading) {
   const unsigned route[]{'l','9',13};check(step<=3,"unexpected title Load screen");
   const unsigned code=route[step++-1];if(state.mouse)click(*context.dialog,code==13?'s':code);else key(code);return next;
  }
  // Cancel name once, return to the menu, then complete a fresh New.
  const unsigned route[]{'s','9',13,27,27,'s','9',13,0,13,'a'};
  check(step<=11,"unexpected title New screen");unsigned code=route[step++-1];
  if(code==0) {
   check(context.dialog->textEntry,"native name input mode");SDL_Event event{};event.type=SDL_TEXTINPUT;
   std::strcpy(event.text.text,state.name.c_str());check(SDL_PushEvent(&event)==1,"native case-preserving text");
  }else {
   if(step==12)code=state.difficulty==XeenDifficulty::Warrior?'w':'a';
   unsigned target=code;
   if(code==13 && !context.dialog->textEntry)target='s';
   const bool hit=std::any_of(context.dialog->hits.begin(),context.dialog->hits.end(),[&](const auto &h){return h.key==target;});
   if(state.mouse && hit)click(*context.dialog,target);else key(code);
  }
  return next;
 };
 return realTitleShow(window,first,label,native,escape,nativeIdle,status);
}
void titleApplicationControls(const char *game) {
 state.enabled=true;
 state.base=fs::temp_directory_path()/("mmodern-m54-title-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
 fs::create_directory(state.base);
 struct Cleanup {~Cleanup(){state.enabled=false;std::error_code error;fs::remove_all(state.base,error);}} cleanup;
 for(auto difficulty:{XeenDifficulty::Adventurer,XeenDifficulty::Warrior}) {
  state.difficulty=difficulty;state.mouse=difficulty==XeenDifficulty::Warrior;
  state.loading=false;state.shown=false;state.seeds=0;
  check(Application().run(game)==0 && state.shown,"native title/New Application loop");
  state.loading=true;state.shown=false;state.seeds=0;
  check(Application().run(game)==0 && state.shown,"native title/Load Application loop");
 }
}
void titlePreview(const IndexedFrame &frame,const std::string &label){preview(frame,label);}
