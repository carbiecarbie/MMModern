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
#define TEXT_SYMBOL "_ZNK7mmodern16XeenTextRenderer6renderERKNS_12IndexedFrameERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERKNS_21XeenTextRenderOptionsE"
extern "C" XeenTextRenderResult realTitleText(const XeenTextRenderer *,const IndexedFrame &,const std::string &,const XeenTextRenderOptions &) asm("__real_" TEXT_SYMBOL);
extern "C" XeenTextRenderResult wrappedTitleText(const XeenTextRenderer *,const IndexedFrame &,const std::string &,const XeenTextRenderOptions &) asm("__wrap_" TEXT_SYMBOL);
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
 bool panelMode=false,failPreflight=false,failWrite=false;
 bool failNotice=false;
 unsigned noticeAttempts=0,writes=0;
 const XeenDosText *text=nullptr;
 const XeenFontFormat *font=nullptr;
 std::string notice,saveAsName="Panel Name";
 SdlWindow::FrameUpdateHandler retired;
 SdlWindow::IdleFrameHandler retiredIdle;
 IndexedFrame::Presentation retiredFrame;
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
bool panelGameplay(const IndexedFrame &first,const SdlWindow::FrameUpdateHandler &handler,
 const SdlWindow::IdleFrameHandler &idle,const std::function<std::string()> &status) {
 IndexedFrame visible=first;
 const auto present=[&] {
  check(handler.acceptsFrame(visible.presentation()),"panel returned stale frame");
  handler.framePresented(visible.presentation());handler.completeInputHandoff(visible.presentation());
 };
 present();
 auto capture=[&] {
  auto snapshot=XeenSaveState::capture(state.signature,*state.party,*state.camera,*state.flags,*state.world);
  snapshot.name="Gameplay comparison";return XeenSaveFormat::encode(snapshot);
 };
 const auto send=[&](const PlayerAction &action) {
  const auto next=handler.withPresentedInput(action,*handler.displayedInput(),visible.presentation());
  if(next)visible=*next;present();
 };
 const auto key=[&](unsigned key) {
  const auto context=handler.inputContext(visible.presentation());
  check(context.readyForAction && context.dialog,"panel modal input");
  const auto action=context.dialog->key(key);check(bool(action),"panel dialog key");send(*action);
 };
 const auto expectNotice=[&](const std::string &name) {
  state.notice=xeenDialogFormat(state.text->scalar("SAVED_NOTICE"),{name});state.noticeAttempts=0;
 };
 const auto checkNotice=[&] {
  check(state.noticeAttempts==1,"real SAVED_NOTICE was not presented exactly once");
  XeenTextRenderOptions options;options.originalControls=true;options.drawWindow=true;
  options.windowBounds={99,59,237,141};options.bounds={107,67,229,133};options.x=107;options.y=67;
  options.stopAtBottom=true;
  XeenTextRenderer renderer(*state.font);
  const auto expected=realTitleText(&renderer,first,state.notice,options);
  check(expected.diagnostics.empty() && expected.pages.size()==1 && expected.pages.front().pixels==visible.pixels,
   "SAVED_NOTICE differs from DOS window 21/raw resource controls");
  check(handler.inputContext(visible.presentation()).dialog->anyKey,"saved notice must wait for acknowledgment");
  preview(visible,"saved-"+std::to_string(state.name.size())+"-"+std::to_string(state.saveAsName.size()));
 };
 const auto failConfirmation=[&](unsigned code) {
  state.failNotice=true;const auto writes=state.writes;key(code);
  check(!state.failNotice && state.noticeAttempts==1 && state.writes==writes+1 &&
   status().find("Game saved; confirmation unavailable")!=std::string::npos,
   "post-write presentation failure was reported/retried as a save failure");
  key(27);
 };
 if(state.loading) {
  auto snapshot=XeenSaveState::capture(state.signature,*state.party,*state.camera,*state.flags,*state.world);snapshot.name=state.name;
  check(XeenSaveFormat::encode(snapshot)==state.saved && state.seeds==0,"in-game Load changed state/time/RNG or reran initialization");
  send(ControlPanelAction{});expectNotice(state.name);key('s');checkNotice();key(27);
  check(XeenSaveFormat::encode(XeenSaveFile::read(state.slot))==state.saved,"immediate in-game Load/re-save differs");
  key('q');key('y');check(handler.finished(),"panel Quit did not finish native session");state.shown=true;return true;
 }
 const auto before=capture();
 const auto originalSlot=state.slot;const auto originalWire=XeenSaveFormat::encode(XeenSaveFile::read(originalSlot));
 send(ControlPanelAction{});preview(visible,"native-panel");
 const auto retained=visible.presentation();
 key('e');key(27);key('w');key('n');key('w');key('y');key(27);key('q');key('n');
 check(!handler.withPresentedInput(DialogKeyAction{'q'},*handler.displayedInput(),retained),"stale panel input accepted");
 for(unsigned i=0;i<10;++i)idle();check(capture()==before,"panel idle/deferred/decline changed gameplay");
 expectNotice(state.name);key('s');checkNotice();key(27);
 key('a');key('3');key(13);key(27); // Name cancel: no file.
 const auto third=XeenSaveFile::slotPath(originalSlot.parent_path(),2);
 check(!fs::exists(third),"cancelled Save As wrote a slot");
 key(13);send(TextInputAction{state.saveAsName});
 state.failWrite=true;key(13);key(27); // Failed write returns to the list.
 check(!fs::exists(third) && XeenSaveFormat::encode(XeenSaveFile::read(originalSlot))==originalWire,"failed Save As changed target/current slot");
 key(13);send(TextInputAction{state.saveAsName});expectNotice(state.saveAsName);key(13);checkNotice();key(27);
 check(fs::exists(third),"Save As did not publish");
 const auto saved=XeenSaveFormat::encode(XeenSaveFile::read(third));
 // A Save As confirmation failure follows a committed overwrite; dismissal
 // must still return to the chooser without a second write or a slot rollback.
 key(13);key('y');send(TextInputAction{state.saveAsName});expectNotice(state.saveAsName);failConfirmation(13);
 check(handler.inputContext(visible.presentation()).dialog->hits.size()==16 &&
  XeenSaveFormat::encode(XeenSaveFile::read(third))==saved,"Save As presentation failure lost committed file/list return");
 key(27);expectNotice(state.saveAsName);key('s');checkNotice();key(27);
 check(XeenSaveFormat::encode(XeenSaveFile::read(third))==saved && XeenSaveFormat::encode(XeenSaveFile::read(originalSlot))==originalWire,"direct Save targeted wrong slot or changed state");
 key('l');key(27);check(capture()==before,"cancelled Load changed retained session");
 key('l');key('3');state.failPreflight=true;key(13);key(27);
 check(capture()==before && !handler.finished(),"failed scratch Load tore down/changed retained session");
 key(27);expectNotice(state.saveAsName);failConfirmation('s');
 check(XeenSaveFormat::encode(XeenSaveFile::read(third))==saved,"failed confirmation/Load changed current slot or committed file");
 // Dismissal still returns to the panel, with the committed slot current.
 expectNotice(state.saveAsName);key('s');checkNotice();key(27);
 key('l');key('3');key(13);check(handler.finished() && capture()==before,"successful Load did not return exact retained outcome");
 state.saved=saved;state.name=state.saveAsName;state.loading=true;state.shown=false;state.seeds=0;
 state.retired=handler;state.retiredIdle=idle;state.retiredFrame=visible.presentation();return true;
}
}
extern "C" XeenTextRenderResult wrappedTitleText(const XeenTextRenderer *renderer,const IndexedFrame &base,
 const std::string &text,const XeenTextRenderOptions &options) {
 if(state.enabled && state.panelMode && !state.notice.empty() && text==state.notice) {
  ++state.noticeAttempts;
  check(options.windowBounds.left==99 && options.windowBounds.top==59 && options.windowBounds.right==237 &&
   options.windowBounds.bottom==141 && options.bounds.left==107 && options.bounds.top==67 &&
   options.bounds.right==229 && options.bounds.bottom==133 && options.stopAtBottom,"saved confirmation differs from DOS window/newline bounds");
  if(state.failNotice){state.failNotice=false;throw std::runtime_error("Synthetic confirmation presentation failure");}
 }
 return realTitleText(renderer,base,text,options);
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
 state.font=&services.font;
 const auto configure=services.configureFlow;
 services.configureFlow=[configure](auto &flow,const auto &position){if(configure)configure(flow,position);state.text=&flow.dosText();};
 services.sampleJourneySeed=[] {++state.seeds;return 1u;};
 if(state.panelMode && !resume) {
  const auto compose=services.composeEncounter;
  services.composeEncounter=[compose](auto &w,const auto &p,const auto &c,auto phase,auto actor) {
   if(state.failPreflight && &w!=state.world){state.failPreflight=false;throw std::runtime_error("Synthetic scratch Load failure");}
   return compose(w,p,c,phase,actor);
  };
  const auto writer=services.writeManaged;
  services.writeManaged=[writer](unsigned slot,const auto &snapshot,const auto &checkSource) {
   ++state.writes;
   checkSource();if(state.failWrite){state.failWrite=false;throw std::runtime_error("Synthetic panel write failure");}
   writer(slot,snapshot,checkSource);
  };
 }
 if(state.panelMode)services.show=[](const auto &first,const auto &handler,const auto &,const auto &idle,const auto &status) {
  return panelGameplay(first,handler,idle,status);
 };
 if(resume) {
  if(state.panelMode) {
   check(!state.world && !state.party && !state.camera && !state.flags,"old owners retained across outer Load");
   check(!state.retired.frameCurrent() && !state.retired.acceptsFrame(state.retiredFrame) &&
    !state.retired.acceptsInputFrame(state.retiredFrame) && !state.retired.displayedInput() &&
    !state.retired.withPresentedInput(SaveGameAction{},1,state.retiredFrame) &&
    !state.retired.withDisplayedInput(ControlPanelAction{},1) && !state.retiredIdle(),"retired callbacks touched destroyed session");
   state.retired.framePresented(state.retiredFrame);state.retired.completeInputHandoff(state.retiredFrame);
   state.retired.beginCycle(1000);state.retired.closed();state.retired.failed();
  }
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
 state.panelMode=true;
 for(const auto &name:std::vector<std::string>{"A","Twenty CharacterName",std::string(20,'W')}) {
  state.loading=false;state.shown=false;state.seeds=0;state.name=name;state.saveAsName=name;
  state.notice.clear();state.writes=0;
  check(Application().run(game)==0 && state.loading && state.shown,"panel Save/Save As/Load real saved notices");
  fs::remove(XeenSaveFile::slotPath(state.slot.parent_path(),2));
  fs::remove(XeenSaveFile::slotPath(state.slot.parent_path(),8));
 }
 state.panelMode=false;
}
void titlePreview(const IndexedFrame &frame,const std::string &label){preview(frame,label);}
