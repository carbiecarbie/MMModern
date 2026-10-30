#include "SdlMouseTestSupport.h"
#include "games/xeen/XeenMapView.h"
// One original-data Application/SDL path for both New difficulties and title
// Load. Only seed sampling and the Local AppData test directory are substituted.
#include "app/Application.h"
#include "app/XeenAudio.h"
#include "core/PresentationClock.h"
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
#include "TestTreeRemoval.h"
#include "formats/xeen/XeenCutsceneData.h"
#include <fstream>
#include <sstream>
#include <iostream>
using namespace mmodern;
namespace fs=std::filesystem;
using save_test::check;
#define PLAY_SYMBOL "_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjE"
#define SHOW_SYMBOL "_ZNK7mmodern9SdlWindow15showInteractiveERKNS_12IndexedFrameERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERKNS0_18FrameUpdateHandlerERKSt8functionIFbvEERKSF_IFSt8optionalIS1_EvEERKSF_IFS9_vEE"
#define DIRECTORY_SYMBOL "_ZN7mmodern12XeenSaveFile19createSlotDirectoryERKNS_16GameInstallationERKNSt10filesystem7__cxx114pathERKSt8optionalIS6_E"
#define TEXT_SYMBOL "_ZNK7mmodern16XeenTextRenderer6renderERKNS_12IndexedFrameERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERKNS_21XeenTextRenderOptionsE"
#define DATA_SYMBOL "_ZN7mmodern16XeenCutsceneData9installedERKNS_16GameInstallationE"
extern "C" XeenCutsceneData realEndingData(const GameInstallation &) asm("__real_" DATA_SYMBOL);
extern "C" XeenCutsceneData wrappedEndingData(const GameInstallation &) asm("__wrap_" DATA_SYMBOL);
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
extern "C" std::uint64_t realTitleClock() asm("__real__ZN7mmodern9SdlWindow23presentationNanosecondsEv");
extern "C" std::uint64_t wrappedTitleClock() asm("__wrap__ZN7mmodern9SdlWindow23presentationNanosecondsEv");
namespace mmodern {
struct XeenTemplePriceStateTestAccess {
 static void seed(XeenTemplePriceState &prices,unsigned map) {
  prices.checkPublication(prices.revision());prices.publish(prices.prepare(XeenMapIdentity(map)));
 }
};
}
namespace {
struct State {
 XeenProgramState *program=nullptr;
 bool enabled=false,loading=false,mouse=false,shown=false;
 bool introsOnly=false;
 XeenTemplePriceState *templePrices=nullptr;XeenTempleSurcharges templeRow;
 bool panelMode=false,failPreflight=false,failWrite=false;
 bool retainedIdle=false;
 bool endingMode=false,endingReturned=false,endingNotice=false,endingPlayed=false;
 bool endingReplay=false;unsigned endingExeFailure=0;
 bool endingOriginal=false;
 bool startupMode=false,startupMissing=false,startupFailure=false,startupEscaped=false,startupAfterSeen=false;
 bool startupDigitalNotice=false;unsigned startupExeStarts=0;
 std::uint64_t startupEscape=0,startupDuration=0;
 bool introMode=false,introDark=false,introFailure=false,introMissingExe=false,introNotice=false,introCloudsNotice=false;unsigned introWindow=0,introStarts=0;std::uint64_t introNow=17000000,introSkip=0;
 std::uint32_t wallNow=0;
 std::optional<std::uint64_t> nameCursorNow;
 bool failNotice=false;
 unsigned noticeAttempts=0,writes=0;
 const XeenDosText *text=nullptr;
 const XeenFontFormat *font=nullptr;
 XeenDialogSpriteDraw drawMapSprite;
 std::function<std::string(XeenMapIdentity)> mapName;
 std::string notice,saveAsName="Panel Name";
 SdlWindow::FrameUpdateHandler retired;
 SdlWindow::IdleFrameHandler retiredIdle;
 IndexedFrame::Presentation retiredFrame;
 unsigned seeds=0;
 XeenDifficulty difficulty=XeenDifficulty::Adventurer;
 fs::path base,slot,darksideSettings;
 std::vector<std::uint8_t> saved;
 XeenWorld *world=nullptr;const XeenPartyState *party=nullptr;
 const XeenCamera *camera=nullptr;const XeenGameFlags *flags=nullptr;
 XeenSaveResourceSignature signature;
 std::string name="Native Case !~";
} state;
bool introApplication(const IndexedFrame &first,const SdlWindow::FrameUpdateHandler &handler,const SdlWindow::IdleFrameHandler &idle) {
 const auto window=++state.introWindow;check(window<=4,"intro did not return to one fresh title");
 const bool failureReturn=state.introFailure && window==3;
 check(handler.cutsceneControlQuit==(window==3 && !failureReturn),"Ctrl-Q scope leaked between intro and DAT title");
 if(window==4 || failureReturn)check(state.program->playTimeMark==state.wallNow,"EXE hand-off did not reset the fresh DAT mark");
 IndexedFrame visible=first;
 const auto present=[&] {check(handler.acceptsFrame(visible.presentation()),"intro stale publication");handler.framePresented(visible.presentation());handler.completeInputHandoff(visible.presentation());};
 present();unsigned guard=0;
 const auto tick=[&] {
  check(++guard<100000,"Application intro stalled");
  ++state.wallNow;
  const auto deadline=handler.nextPresentationDeadline();if(deadline && *deadline>state.introNow)state.introNow=*deadline;
  handler.beginCycle(guard);if(auto next=idle()){visible=*next;present();}
 };
 const auto key=[&](unsigned code) {
  const auto context=handler.inputContext(visible.presentation());check(context.dialog && context.readyForAction,"intro input not acquired");
  const auto a=context.dialog->key(code);check(bool(a),"intro key missing");
  if(auto next=handler.withPresentedInput(*a,1,visible.presentation())){visible=*next;present();}
 };
 if(window==1 || window==4 || failureReturn) {
  while(!handler.inputContext(visible.presentation()).readyForAction)tick();
  check(handler.inputContext(visible.presentation()).dialog->hits.empty(),"fresh intro return exposed menu");
  if(window==4 || failureReturn) {
   check(state.program->audio->loadedName()=="newbrigh.m" && state.program->audio->soundOptions()==XeenSoundOptions{false,false,true,false},"intro return song/options");
   if(failureReturn){check(state.introNotice,"missing Darkside failure notice");key(13);check(handler.inputContext(visible.presentation()).dialog->hits.empty(),"failure notice did not return to fresh title");}
   key(32);check(handler.inputContext(visible.presentation()).dialog->hits.size()==4,"fresh title key did not reveal menu");
   handler.closed();state.shown=true;return true;
  }
  key(27);key('o');
  state.program->audio->observeOptions({false,false,true,false});
  state.program->audio->digitalSink([](const auto &,auto){++state.introStarts;});key(state.introDark?'d':'c');
  if(state.introMissingExe) {
   check(!handler.finished() && state.introNotice && !state.introCloudsNotice,"missing EXE notice did not name Darkside");
   key(13);handler.closed();state.shown=true;return true;
  }
  check(handler.finished(),"Application did not select intro");return true;
 }
 if(window==3 && state.introDark) {
  check(!state.program->darksideIntroSeen,"D did not clear seen before playback");
  if(!state.darksideSettings.empty())check(!xeenReadSettings(state.darksideSettings).darksideIntroSeen,"D did not persist cleared state before playback");
 }
 const auto start=state.introNow;bool escaped=false;
 while(!handler.finished()) {
  if(window==3 && state.introNow-start>15000000000ULL && state.introNow-start<120000000000ULL)check(state.program->audio->loadedName()==(state.introDark?"bigtheme.m":"mm4theme.m"),"EXE song suppressed by party settings or stale song identity");
  if(window==3 && !state.introDark && !escaped && state.introNow-start>=state.introSkip && handler.inputContext(visible.presentation()).readyForAction) {
   check(handler.inputContext(visible.presentation()).hideCursor && !handler.inputContext(visible.presentation()).dialog->click(160,100),"intro cursor/click policy");
   key(27);escaped=true;
  }
  tick();
 }
 check(window!=3 || escaped || (state.introDark && state.introNow-start>130000000000ULL),"intro finished before requested route");return true;
}
void quit(){SDL_Event event{};event.type=SDL_QUIT;check(SDL_PushEvent(&event)==1,"title native quit");}
void key(unsigned code,std::vector<SDL_Event> &pending) {
 SDL_Event event{};event.type=SDL_KEYDOWN;event.key.keysym.sym=code;event.key.keysym.scancode=SDL_GetScancodeFromKey(code);
 pending.push_back(event);event.type=SDL_KEYUP;pending.push_back(event);
}
extern "C" std::uint64_t wrappedTitleClock(){
 if(state.enabled && state.introsOnly)check(state.introMode || state.startupMode,"Parallel title intros reached the real presentation clock");
 return state.nameCursorNow?*state.nameCursorNow:(state.introMode || state.startupMode || (state.endingMode && (state.endingReturned || state.endingReplay)))?state.introNow:realTitleClock();
}
extern "C" XeenCutsceneData wrappedEndingData(const GameInstallation &installation) {
 if(state.enabled && state.startupMode && state.startupMissing)throw std::runtime_error("missing EXE test");
 if(state.enabled && state.introMode && state.introDark && state.introMissingExe)
  throw std::runtime_error("Clouds intro unavailable: missing or different WORLD/XEEN.EXE");
 if(state.enabled && state.introMode && state.introDark && state.introFailure) {
  auto data=realEndingData(installation);data.darkside.resources[XeenCutsceneData::Darkside::Caption]="missing-m93-caption.bin";return data;
 }
 if(!state.enabled || !state.endingMode || !state.endingExeFailure)return realEndingData(installation);
 if(state.endingExeFailure==1) {
  auto missing=installation;missing.uiModule->physicalPath=state.base/"missing-exe"/"XEEN.DAT";return realEndingData(missing);
 }
 std::ifstream file(installation.uiModule->physicalPath.parent_path()/"XEEN.EXE",std::ios::binary);
 std::vector<std::uint8_t> exe((std::istreambuf_iterator<char>(file)),{});check(!exe.empty(),"EXE corruption fixture");exe.back()^=1;
 auto in=installation.configuration->open();std::vector<std::uint8_t> cfg(in->size());check(in->read(cfg.data(),cfg.size())==cfg.size(),"CFG read");
 return XeenCutsceneData::parse(exe,cfg);
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
void click(const DialogInput &dialog,unsigned value,std::vector<SDL_Event> &pending) {
 const DialogHit *hit=nullptr;for(const auto &candidate:dialog.hits)if(candidate.key==value){hit=&candidate;break;}
 check(hit,"native title click target");
 SDL_Event event{};event.type=SDL_MOUSEBUTTONDOWN;event.button.button=SDL_BUTTON_LEFT;
 sdl_mouse_test::position(event.button,(hit->left+hit->right)/2,(hit->top+hit->bottom)/2);
 pending.push_back(event);event.type=SDL_MOUSEBUTTONUP;pending.push_back(event);
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
  const auto input=handler.displayedInput();check(bool(input),"panel input ticket");
  const auto next=handler.withPresentedInput(action,*input,visible.presentation());
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
  // Title-menu play keeps the window title free of save diagnostics.
  check(!state.failNotice && state.noticeAttempts==1 && state.writes==writes+1 && status()=="MMModern",
   "post-write presentation failure was reported/retried as a save failure");
  key(27);
 };
 if(state.loading) {
  check(state.program->audio->retainedCdCue().has_value() && state.program->audio->cdBusy(SdlWindow::presentationNanoseconds()),
   "Same-process panel Load discarded surviving CD playback/operands");
  auto snapshot=XeenSaveState::capture(state.signature,*state.party,*state.camera,*state.flags,*state.world);snapshot.name=state.name;
  check(XeenSaveFormat::encode(snapshot)==state.saved && state.seeds==0,"in-game Load changed state/time/RNG or reran initialization");
  handler.beginCycle(1);if(auto next=idle())visible=*next;present(); // Load entry pass.
   send(ControlPanelAction{});expectNotice(state.name);key('s');checkNotice();key(27);
  check(XeenSaveFormat::encode(XeenSaveFile::read(state.slot))==state.saved,"immediate in-game Load/re-save differs");
  // Mr Wizard closes the panel and moves the party to Vertigo's run position.
  key('w');key('y');
  check(!handler.inputContext(visible.presentation()).dialog && state.camera->mapId==XeenMapIdentity(28) &&
   state.camera->direction==XeenDirection::East && state.party->purse->gems==0,"panel Mr Wizard did not move the party");
  for(unsigned i=0;i<10;++i)if(auto next=idle()){visible=*next;present();}
  send(ControlPanelAction{});
  key('q');key('y');check(handler.finished(),"panel Quit did not finish native session");
  check(!state.program->audio->cdBusy(SdlWindow::presentationNanoseconds()),"Confirmed Quit did not stop CD");state.shown=true;return true;
 }
 const auto readyBegan=SDL_GetTicks();std::uint64_t readyCycle=0;
 while(!handler.inputContext(visible.presentation()).readyForAction) {
  check(SDL_GetTicks()-readyBegan<2000,"Panel witness never reached the ordinary input boundary");
  handler.beginCycle(++readyCycle);SDL_Delay(1);
  if(handler.inputContext(visible.presentation()).readyForAction)break;
  if(auto next=idle()){visible=*next;present();}
 }
 const auto before=capture();
 check(!state.program->audio->retainedCdCue() && !state.program->audio->cdBusy(SdlWindow::presentationNanoseconds()),
  "New session replayed CD from template or fresh process");
 const auto originalSlot=state.slot;const auto originalWire=XeenSaveFormat::encode(XeenSaveFile::read(originalSlot));
 send(ControlPanelAction{});preview(visible,"native-panel");
 const auto retained=visible.presentation();
 key('e');key('e');key('w');key('n');key('q');key('n');
 const auto input=handler.displayedInput();check(bool(input),"panel input ticket after cancellation");
 check(!handler.withPresentedInput(DialogKeyAction{'q'},*input,retained),"stale panel input accepted");
 for(unsigned i=0;i<10;++i)idle();check(capture()==before,"panel idle/deferred/decline changed gameplay");
 expectNotice(state.name);key('s');checkNotice();key(27);
 key('a');key('3');key(13);
 {
  // With sound detached, the in-game handler must schedule the name cursor
  // itself. This exercises Application's panel deadline, not just TitleFlow.
  struct CursorClockScope {
   std::shared_ptr<XeenAudio> audio=std::exchange(state.program->audio,{});
   ~CursorClockScope(){state.nameCursorNow.reset();state.program->audio=std::move(audio);}
  } held;
  state.nameCursorNow=realTitleClock();
  check(!idle(),"first Save As cursor poll should only arm its wait");
  const auto deadline=handler.nextPresentationDeadline();
  check(deadline && *deadline>*state.nameCursorNow,"in-game Save As name cursor has no scheduled deadline");
  const auto pixels=visible.pixels;const auto context=handler.inputContext(visible.presentation()).contextId;
  state.nameCursorNow=*deadline-1;check(!idle(),"Save As cursor blinked before its deadline");
  state.nameCursorNow=*deadline;const auto blink=idle();
  check(blink && blink->pixels!=pixels,"Save As cursor did not blink at its scheduled deadline");
  visible=*blink;present();
  check(handler.inputContext(visible.presentation()).contextId==context,"cursor blink changed the in-game input context");
  while(realTitleClock()<*state.nameCursorNow)SDL_Delay(1);
 }
 key(27); // Name cancel: no file.
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
 state.program->audio->retainCdCue({19,370,2161});
 state.program->audio->startCd(SdlWindow::presentationNanoseconds()); // Model surviving NPC playback before the Load handoff.
 key('l');key('3');key(13);check(handler.finished() && capture()==before,"successful Load did not return exact retained outcome");
 state.saved=saved;state.name=state.saveAsName;state.loading=true;state.shown=false;state.seeds=0;
 state.retired=handler;state.retiredIdle=idle;state.retiredFrame=visible.presentation();return true;
}
}
extern "C" XeenTextRenderResult wrappedTitleText(const XeenTextRenderer *renderer,const IndexedFrame &base,
 const std::string &text,const XeenTextRenderOptions &options) {
 if(state.enabled && state.startupMode && text.find("Digital device ")!=std::string::npos && text.find("not supported yet")!=std::string::npos)state.startupDigitalNotice=true;
 if(state.enabled && state.introMode && text.find("Darkside intro unavailable")!=std::string::npos)state.introNotice=true;
 if(state.enabled && state.introMode && text.find("Clouds intro unavailable")!=std::string::npos)state.introCloudsNotice=true;
 if(state.enabled && state.endingMode && text.find("Clouds ending unavailable")!=std::string::npos)state.endingNotice=true;
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
 if(state.endingMode) {
  check(!state.endingReplay,"V executed gameplay or the ending tail");
  if(state.endingOriginal) {
   auto services=original;services.clock=[] {return state.introNow/1000000;};services.presentationClock=[] {return state.introNow;};
   services.compose=[](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{IndexedFrame{320,200,std::vector<std::uint8_t>(64000)},false};};
   services.composeEncounter=[](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{IndexedFrame{320,200,std::vector<std::uint8_t>(64000)},false};};
   services.show=[&](auto visible,const auto &h,const auto &,const auto &idle,const auto &) {
    const auto present=[&]{if(h.finished())return;h.framePresented(visible.presentation());h.completeInputHandoff(visible.presentation());};present();bool sent=false;
    for(unsigned n=1;n<1000 && !h.finished();++n) {
     state.introNow+=125000000;h.beginCycle(n);if(auto f=idle()){visible=*f;present();}if(h.finished())break;
     const auto c=h.inputContext(visible.presentation());if(!c.readyForAction)continue;
     std::optional<PlayerAction> a;if(!sent){sent=true;a=InteractionAction{};}else if(c.dialog && c.dialog->anyKey)a=AcknowledgeAction{};
     if(a)if(auto f=h.withPresentedInput(*a,*h.displayedInput(),visible.presentation())){visible=*f;present();}
    }
    check(h.finished(),"Original Sixth Mirror did not finish");return true;
   };
   const auto result=realTitlePlay(app,services,camera,target,resume,entry,seed);
   check(original.outcome && original.outcome->kind==XeenSessionOutcome::Kind::CloudsEnding,"Original mirror did not produce the presentation outcome");
   // Gameplay close rebased audio onto the real window clock; the wrapped
   // window clock switches to introNow below, so restart it from real time.
   // Keeping the session's fake time fails under load once real time has
   // passed it ("Audio presentation clock moved backwards").
   state.introNow=realTitleClock();state.endingReturned=true;return result;
  }
  check(original.outcome,"Outer gameplay wrapper must retain the outcome sink");
  *original.outcome={XeenSessionOutcome::Kind::CloudsEnding,0,{},71001722};
  state.wallNow+=100;state.introNow=realTitleClock();state.endingReturned=true;return 0;
 }
 check(resume==state.loading && target.has_value() && !seed,"title session entry");
 check(&original.program.templePrices==state.templePrices && original.program.templePrices.row()==state.templeRow &&
  original.program.templePrices.revision()==1,"Title/New Game/panel Load replaced or reset process Temple prices");
 state.slot=*target;state.signature=original.resources.signature;
 auto services=original;
 if(!resume && !state.panelMode) {
  const auto load=services.resources.loadInitialParty;
  services.resources.loadInitialParty=[load]{auto p=load();p.playTimeSeconds=888;p.cloudsCompleted=true;return p;};
 }
 state.font=&services.font;
 const auto configure=services.configureFlow;
 services.configureFlow=[configure](auto &flow,const auto &position){if(configure)configure(flow,position);state.text=&flow.dosText();state.drawMapSprite=flow.drawDialogSprite;state.mapName=flow.mapName;};
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
 const auto result=realTitlePlay(app,services,camera,target,resume,entry,seed);
 check(original.program.templePrices.row()==state.templeRow && original.program.templePrices.revision()==1,"Game initialization or Load changed process Temple prices");
 return result;
}
extern "C" bool wrappedTitleShow(const SdlWindow *window,const IndexedFrame &first,const std::string &label,
 const SdlWindow::FrameUpdateHandler &handler,const std::function<bool()> &escape,const SdlWindow::IdleFrameHandler &idle,
 const std::function<std::string()> &status) {
 if(!state.enabled)return realTitleShow(window,first,label,handler,escape,idle,status);
 if(state.startupMode) {
  if(!handler.cutsceneControlQuit) {handler.closed();state.shown=true;return true;}
  ++state.startupExeStarts;
  if(state.startupFailure){handler.failed();handler.closed();return false;}
  IndexedFrame visible=first;unsigned guard=0;const auto start=state.introNow;bool escaped=false;
  const auto present=[&]{handler.framePresented(visible.presentation());handler.completeInputHandoff(visible.presentation());};present();
  while(!handler.finished()) {
   check(++guard<100000,"startup stalled");
   if(state.startupEscape && !escaped && state.introNow-start>=state.startupEscape && (!state.startupAfterSeen || state.program->darksideIntroSeen)) {
    const auto context=handler.inputContext(visible.presentation());
    if(context.dialog && context.readyForAction)if(auto action=context.dialog->key(27)) {
     if(auto next=handler.withPresentedInput(*action,1,visible.presentation())){visible=*next;present();}escaped=true;
    }
   }
   const auto deadline=handler.nextPresentationDeadline();state.introNow=deadline && *deadline>state.introNow?*deadline:state.introNow+17000000;
   handler.beginCycle(guard);if(auto next=idle()){visible=*next;present();}
  }
  state.startupDuration=state.introNow-start;state.startupEscaped=escaped;return true;
 }
 if(state.introMode)return introApplication(first,handler,idle);
 check(!state.introsOnly,"Parallel title intros reached the native window path");
 if(state.endingMode && state.endingReplay && !state.endingReturned) {
  IndexedFrame visible=first;unsigned cycles=0;
  const auto present=[&]{handler.framePresented(visible.presentation());handler.completeInputHandoff(visible.presentation());};present();
  const auto key=[&](unsigned k){
   while(!handler.inputContext(visible.presentation()).readyForAction){check(++cycles<10000,"V title did not acquire input");
    auto next=handler.nextPresentationDeadline();state.introNow=next?std::max(state.introNow,*next):state.introNow+17000000;handler.beginCycle(cycles);if(auto f=idle()){visible=*f;present();}}
   const auto action=handler.inputContext(visible.presentation()).dialog->key(k);check(bool(action),"V not visible after won store");
   if(auto f=handler.withPresentedInput(*action,1,visible.presentation())){visible=*f;present();}
  };
  key(27);key('o');key('v');state.endingReturned=true;return true;
 }
 if(state.endingMode && state.endingReturned) {
  if(handler.cutsceneControlQuit) {
   check(handler.cutsceneControlQuit,"Clouds ending did not enable EXE Ctrl-Q");
   check(!state.endingPlayed,"Ending program repeated before the title notice");
   IndexedFrame visible=first;unsigned cycles=0;const auto began=state.introNow;
   const auto present=[&]{handler.framePresented(visible.presentation());handler.completeInputHandoff(visible.presentation());};present();
   while(!handler.finished()) {
    check(++cycles<100000,"C1 Application ending stalled");
    const auto context=handler.inputContext(visible.presentation());
    if(context.readyForAction) {
     check(context.hideCursor && !context.dialog->click(160,100),"Ending admitted a mouse click");
     for(unsigned code:{27u,32u})if(auto action=context.dialog->key(code))handler.withPresentedInput(*action,1,visible.presentation());
    }
    const auto deadline=handler.nextPresentationDeadline();check(bool(deadline),"C1 Application ending lost its deadline");
    state.introNow=std::max(state.introNow,*deadline);++state.wallNow;
    handler.beginCycle(cycles);if(auto next=idle()){visible=*next;present();}
   }
   check(state.introNow-began>50000000000ULL,"Ending input bypassed stages 1 and 2");
   state.endingPlayed=true;return true;
  }
  check(state.endingExeFailure?state.endingNotice:state.endingPlayed,"Ending did not reach the expected fresh title");
  check(!handler.cutsceneControlQuit,"Ending Ctrl-Q leaked into fresh DAT title");
  check(state.program->playTimeMark==state.wallNow,"Ending return did not reset the DAT play-time mark");
  IndexedFrame visible=first;
  const auto present=[&]{handler.framePresented(visible.presentation());handler.completeInputHandoff(visible.presentation());};present();
  unsigned cycle=0;
  while(!handler.inputContext(visible.presentation()).readyForAction) {
   check(++cycle<10000,"Ending notice title did not acquire input");
   const auto deadline=handler.nextPresentationDeadline();
   state.introNow=deadline && *deadline>state.introNow?*deadline:state.introNow+17000000;
   handler.beginCycle(cycle);
   if(auto next=idle()){visible=*next;present();}
  }
  if(state.endingNotice) {
   const auto action=handler.inputContext(visible.presentation()).dialog->key(InputKey::Enter);check(bool(action),"Ending notice missing acknowledgement");
   if(auto next=handler.withPresentedInput(*action,1,visible.presentation())){visible=*next;present();}
  }
  check(handler.inputContext(visible.presentation()).dialog->hits.empty(),"Ending must return to a fresh World title with its menu hidden");
  handler.closed();state.shown=true;return true;
 }
 const bool title=!status; // Only gameplay supplies a status line.
 check(!handler.cutsceneControlQuit,"DAT title/gameplay enabled EXE Ctrl-Q");
 auto native=handler;IndexedFrame::Presentation presented;unsigned step=0;
 std::uint64_t heldAudioTick=0;
 std::vector<SDL_Event> pendingEvents;
 native.beginCycle=[&](auto cycle){
  if(handler.beginCycle)handler.beginCycle(cycle);
  if(title && state.program) {
   if(cycle==2)heldAudioTick=state.program->audio->ticks();
   if(cycle==8)check(state.program->audio->ticks()>heldAudioTick,"held title mouse stopped audio publication");
   if(cycle==1 || cycle==8) {
    // Middle mouse has no title action, but holds the native release wait
    // and suppresses visual idle. Audio must still advance each loop cycle.
    SDL_Event held{};held.type=cycle==1?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
    held.button.button=SDL_BUTTON_MIDDLE;sdl_mouse_test::position(held.button,0,0);check(SDL_PushEvent(&held)==1,"held title mouse queue");
   }
  }
  // The script plans its next physical input during idle. Send it only after
  // that iteration's upload/acquisition completes, with a fresh SDL timestamp.
  for(auto &event:pendingEvents)check(SDL_PushEvent(&event)==1,"native title input after acquisition");
  pendingEvents.clear();
 };
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
   const auto raw=state.loading?state.saved:XeenSaveFormat::encode(XeenSaveFile::read(state.slot));
   auto expectedArrival=XeenSaveFormat::decode(raw);
   const auto cell=state.world->sampleCell(state.camera->mapId,state.camera->x,state.camera->y);
   check(cell && cell->cell->stepped,"New/Load arrival with Cartographer must mark its cell");
   auto &bits=expectedArrival.map(cell->mapId).explored;if(!bits)bits.emplace();
   const unsigned index=cell->y*16+cell->x;(*bits)[index/8]|=1u<<(index%8);
   check(wire==XeenSaveFormat::encode(expectedArrival),"First frame changed more than its occupied-cell mark");
   if(state.loading) {
    check(state.seeds==0,"title Load ran initialization");
    check(!state.program->audio->cdBusy(SdlWindow::presentationNanoseconds()) &&
     state.program->audio->retainedCdCue().has_value()==state.retainedIdle,
     "Title Load replayed passive speech or changed process cue retention");
   }
   else {
    check(snapshot.journey->context->difficulty==state.difficulty && state.seeds==1,"New sampled seed/difficulty");
    check(snapshot.journey->playTimeSeconds==37 && !snapshot.journey->cloudsCompleted && state.program->playTimeMark==47,"New must zero saved time/completion and include title time before its first snapshot");
    check(raw==wire,"New save must contain its prepared arrival frame state");state.saved=raw;
   }
   preview(*frame,state.loading?"load-first":"new-first");
   if(std::getenv("MMODERN_TITLE_PREVIEW"))preview(drawXeenMap(*state.world,*state.camera,*frame,*state.font,*state.text,
    state.drawMapSprite,0,state.mapName(state.camera->mapId)),state.loading?"load-map":"new-map");
   state.shown=true;
  }
 };
 const auto nativeIdle=[&]() -> std::optional<IndexedFrame> {
  if(std::chrono::steady_clock::now()-began>std::chrono::seconds(15))throw std::runtime_error("Native title route stalled at step="+std::to_string(step)+" mouse="+std::to_string(state.mouse)+" loading="+std::to_string(state.loading));
  if(!title){const auto next=idle?idle():std::nullopt;if(state.shown)quit();return next;}
  if(!presented)return idle?idle():std::nullopt;
  const auto context=handler.inputContext(presented);
  // Deadline-driven waiting may draw a cursor on every idle call. Plan the
  // next physical input from the already acquired frame before that redraw.
  if(!context.readyForAction || !context.dialog || emitted==context.contextId ||
      (context.dialog->textEntry && !SDL_IsTextInputActive()))return idle?idle():std::nullopt;
  const std::optional<IndexedFrame> next;
  emitted=context.contextId;preview(*presented,"title-"+std::to_string(step));
  if(step==0){key(27,pendingEvents);++step;return next;} // Original background -> menu.
  if(state.loading) {
   const unsigned route[]{'l','9',13};check(step<=3,"unexpected title Load screen");
   const unsigned code=route[step++-1];if(state.mouse)click(*context.dialog,code==13?'s':code,pendingEvents);else key(code,pendingEvents);return next;
  }
  // Cancel name once, return to the menu, then complete a fresh New.
  const unsigned route[]{'s','9',13,27,27,'s','9',13,0,13,'a'};
  check(step<=11,"unexpected title New screen");unsigned code=route[step++-1];
  if(code==0) {
   check(context.dialog->textEntry,"native name input mode");SDL_Event event{};event.type=SDL_TEXTINPUT;
   std::strcpy(event.text.text,state.name.c_str());pendingEvents.push_back(event);
  }else {
   if(step==12)code=state.difficulty==XeenDifficulty::Warrior?'w':'a';
   unsigned target=code;
   if(code==13 && !context.dialog->textEntry)target='s';
   const bool hit=std::any_of(context.dialog->hits.begin(),context.dialog->hits.end(),[&](const auto &h){return h.key==target;});
   if(state.mouse && hit)click(*context.dialog,target,pendingEvents);else key(code,pendingEvents);
  }
  return next;
 };
 return realTitleShow(window,first,label,native,escape,nativeIdle,status);
}
int runWithoutStartup(const Application &app,const char *game) {return app.runTitleForTest(game);}
// Four whole Clouds ending playbacks: CPU-bound on the fake clock, after the
// New/Load routes that create the slot they load.
void endingControls(const char *game,const char *endingPreset) {
 // A transport control crosses gameplay(), unlike direct playGameplay tests.
 // Gameplay ending content itself is exercised by the original Lord/Mirror test.
 for(bool developer:{false,true}) {
  XeenSaveFile::write(state.slot,XeenSaveFormat::decode(state.saved),{},true);
  state.wallNow=10;XeenProgramState endingProgram{XeenWallClock([]{return state.wallNow;})};state.program=&endingProgram;
  state.endingMode=true;state.endingReturned=state.endingNotice=state.endingPlayed=state.shown=false;
  state.loading=true;state.retainedIdle=false;state.templePrices=&endingProgram.templePrices;
  const auto result=developer?Application(endingProgram).loadGame(game,state.slot):runWithoutStartup(Application(endingProgram),game);
  check(result==0 && state.endingReturned && state.endingPlayed && !state.endingNotice && state.shown,"CloudsEnding program/title handoff for player and developer entries");
  state.endingMode=false;
 }
 // V reads the won store, plays the same program, and leaves save/store bytes intact.
 const auto readFile=[](const fs::path &p){std::ifstream f(p,std::ios::binary);return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(f)),{});};
 const auto settings=state.base/"ending-settings.ini";{std::ofstream f(settings);f<<"won=1\n";}
 for(bool replay:{false,true})for(unsigned failure=0;failure<3;++failure) {
  if(!replay && !failure)continue; // Both successful gameplay entry modes above.
  XeenSaveFile::write(state.slot,XeenSaveFormat::decode(state.saved),{},true);const auto saved=readFile(state.slot),won=readFile(settings);
  state.wallNow=10;XeenProgramState p{XeenWallClock([]{return state.wallNow;})};state.program=&p;
  state.endingMode=true;state.endingReplay=replay;state.endingExeFailure=failure;state.endingReturned=state.endingNotice=state.endingPlayed=state.shown=false;
  state.introNow=realTitleClock();state.loading=true;state.templePrices=&p.templePrices;
  const auto result=replay?runWithoutStartup(Application(p,{},settings),game):Application(p,{},settings).loadGame(game,state.slot);
  check(result==0 && state.shown && state.endingNotice==bool(failure) && state.endingPlayed==!failure,"V/ending missing or different EXE path");
  check(readFile(state.slot)==saved && readFile(settings)==won && p.cloudsWon,"V changed saved state, play time or won store");
 }
 state.endingMode=state.endingReplay=false;state.endingExeFailure=0;
 if(endingPreset) {
  // Explicit local witness, reusing the maintainer's current-format preset without writing it.
  fs::copy_file(fs::path(endingPreset),state.slot,fs::copy_options::overwrite_existing);
  state.wallNow=10;XeenProgramState p{XeenWallClock([]{return state.wallNow;})};state.program=&p;
  state.endingMode=state.endingOriginal=true;state.endingReturned=state.endingNotice=state.endingPlayed=state.shown=false;
  state.introNow=realTitleClock();state.templePrices=&p.templePrices;
  const auto originalSettings=state.base/"original-ending-settings.ini";
  check(Application(p,{},originalSettings).loadGame(game,state.slot)==0 && state.endingPlayed && state.shown && !state.endingNotice && p.cloudsWon,"Original preset to fresh World title");
  check(XeenSaveFile::read(state.slot).journey->cloudsCompleted && xeenReadWon(originalSettings),"Original ending did not save the win");
  state.endingMode=state.endingOriginal=false;
 }
}
void titleApplicationControls(const char *game,const char *endingPreset,const std::string &part) {
 state.enabled=true;
 state.introsOnly=part=="--intros";
 const bool native=part!="--intros",intros=part!="--serial" && part!="--endings";
 state.base=fs::temp_directory_path()/("mmodern-m54-title-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
 fs::create_directory(state.base);
 struct Cleanup {~Cleanup(){state.enabled=false;removeTestTree(state.base);}} cleanup;
 if(native)for(auto difficulty:{XeenDifficulty::Adventurer,XeenDifficulty::Warrior}) {
  state.wallNow=10;XeenProgramState programState{XeenWallClock([]{return state.wallNow;})};state.wallNow=47;
  state.program=&programState;
  XeenTemplePriceStateTestAccess::seed(programState.templePrices,difficulty==XeenDifficulty::Adventurer?30:49);
  state.templePrices=&programState.templePrices;state.templeRow=programState.templePrices.row();
  state.difficulty=difficulty;state.mouse=difficulty==XeenDifficulty::Warrior;
  state.loading=false;state.shown=false;state.seeds=0;
  check(runWithoutStartup(Application(programState),game)==0 && state.shown,"native title/New Application loop");
  const auto replayNow=SdlWindow::presentationNanoseconds();
  programState.audio->retainCdCue({19,370,2161});
  check(programState.audio->startCd(replayNow),"Passive speech before title Load did not play");
  programState.audio->advance(replayNow+18213333334ULL);
  programState.audio->rebaseClock(SdlWindow::presentationNanoseconds());
  state.retainedIdle=true;
  state.loading=true;state.shown=false;state.seeds=0;
  check(runWithoutStartup(Application(programState),game)==0 && state.shown,"native title/Load Application loop");
  // The first Load deliberately removes its source after validation.
  XeenSaveFile::write(state.slot,XeenSaveFormat::decode(state.saved),{},true);
  XeenProgramState freshProgram{mmodern::XeenWallClock::fixed(0)};
  XeenTemplePriceStateTestAccess::seed(freshProgram.templePrices,difficulty==XeenDifficulty::Adventurer?30:49);
  state.program=&freshProgram;state.templePrices=&freshProgram.templePrices;
  state.retainedIdle=false;state.shown=false;state.seeds=0;
  check(runWithoutStartup(Application(freshProgram),game)==0 && state.shown,"fresh program title/Load Application loop");
 }
 // CTest runs these playbacks as xeen_title_flow_endings: they use the fake
 // clock only, so they need not wait for the serial xeen_title_flow.
 if(part.empty() || part=="--endings")endingControls(game,endingPreset);
 if(part=="--endings")return;
 if(intros) {
 // Player startup uses the same presenter on a fake clock, with private settings.
 for(unsigned route=0;route<4;++route) {
  const auto file=state.base/("startup-"+std::to_string(route)+".ini");
  {std::ofstream out(file);out<<"won=1\ndarkside_intro_seen="<<(route==1?1:0)<<"\n";}
  XeenProgramState p{XeenWallClock::fixed(0)};state.program=&p;state.startupMode=true;state.shown=false;
  state.introNow=17000000;state.startupAfterSeen=route==3;state.startupEscape=route==2?30000000000ULL:route==3?1:0;
  check(Application(p,{},file).run(game)==0 && state.shown,"startup returns to fresh title");
  const auto settings=xeenReadSettings(file);
  if(!(settings.won && settings.darksideIntroSeen==(route!=2)))throw std::runtime_error("startup seen persistence route="+std::to_string(route)+" duration="+std::to_string(state.startupDuration)+" seen="+std::to_string(settings.darksideIntroSeen));
  if(route>=2)check(state.startupEscaped,"startup Escape was not delivered");
  if(route<2)check(route==0?state.startupDuration>120000000000ULL:state.startupDuration<40000000000ULL,"startup full/short route");
  state.startupMode=false;
 }
 for(unsigned id:{3u,4u,9u}) {
  const auto settings=state.base/("unsupported-digital-"+std::to_string(id)+".ini");
  const auto cfg=state.base/("unsupported-digital-"+std::to_string(id)+".cfg");
  std::vector<std::uint8_t> bytes(29);bytes[0]=id;bytes[2]=0x20;bytes[3]=2;bytes[4]=3;bytes[6]=0x20;bytes[7]=2;bytes[0x14]=1;bytes[0x18]=1;
  {std::ofstream out(cfg,std::ios::binary);out.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());}
  XeenProgramState p{XeenWallClock::fixed(0)};state.program=&p;state.startupMode=true;state.shown=false;
  state.startupDigitalNotice=false;state.startupExeStarts=0;state.introNow=17000000;
  std::ostringstream diagnostic;auto *old=std::cerr.rdbuf(diagnostic.rdbuf());
  const auto result=Application(p,{},settings,cfg).run(game);std::cerr.rdbuf(old);
  const auto text="Digital device "+std::to_string(id)+": not supported yet\n";
  const auto first=diagnostic.str().find(text);
  check(result==0 && state.shown && !state.startupDigitalNotice && state.startupExeStarts==0 && !p.darksideIntroSeen &&
        first!=std::string::npos && diagnostic.str().find(text,first+text.size())==std::string::npos,
        "Unsupported EXE digital id did not report once on stderr and return to title");
  state.startupMode=false;
 }
 {
  const auto file=state.base/"startup-missing.ini";XeenProgramState p{XeenWallClock::fixed(0)};state.program=&p;
  state.startupMode=state.startupMissing=true;state.shown=false;std::ostringstream diagnostic;auto *old=std::cerr.rdbuf(diagnostic.rdbuf());
  const auto result=Application(p,{},file).run(game);std::cerr.rdbuf(old);
  check(result==0 && state.shown && !p.darksideIntroSeen && diagnostic.str().find("Darkside intro unavailable")!=std::string::npos,"missing EXE startup diagnostic/title");
  state.startupMode=state.startupMissing=false;
 }
 {
  const auto file=state.base/"startup-failure.ini";XeenProgramState p{XeenWallClock::fixed(0)};state.program=&p;
  state.startupMode=state.startupFailure=true;state.shown=false;std::ostringstream diagnostic;auto *old=std::cerr.rdbuf(diagnostic.rdbuf());
  const auto result=Application(p,{},file).run(game);std::cerr.rdbuf(old);
  check(result==0 && state.shown && diagnostic.str().find("Darkside intro unavailable")!=std::string::npos,"startup playback failure diagnostic/title");
  state.startupMode=state.startupFailure=false;
 }
 }
 if(native) {
 // The original Load controls remove slot 9 after validation. Our transport
 // control retains it; restore the same empty-slot setup for the New cases.
 fs::remove(state.slot);
 state.panelMode=true;
 for(const auto &name:std::vector<std::string>{"A","Twenty CharacterName",std::string(20,'W')}) {
  XeenProgramState programState{mmodern::XeenWallClock::fixed(0)};
  state.program=&programState;
  XeenTemplePriceStateTestAccess::seed(programState.templePrices,name=="A"?30:49);
  state.templePrices=&programState.templePrices;state.templeRow=programState.templePrices.row();
  state.loading=false;state.shown=false;state.seeds=0;state.name=name;state.saveAsName=name;
  state.notice.clear();state.writes=0;
  check(runWithoutStartup(Application(programState),game)==0 && state.loading && state.shown,"panel Save/Save As/Load real saved notices");
  fs::remove(XeenSaveFile::slotPath(state.slot.parent_path(),2));
  fs::remove(XeenSaveFile::slotPath(state.slot.parent_path(),8));
 }
 state.panelMode=false;
 }
 if(intros) {
 for(const auto skip:{1000000000ULL,30000000000ULL,60000000000ULL}) {
  state.wallNow=10;XeenProgramState programState{XeenWallClock([]{return state.wallNow;})};state.program=&programState;state.introMode=true;state.introNow=17000000;
  state.introWindow=0;state.introStarts=0;state.introSkip=skip;state.shown=false;
  check(runWithoutStartup(Application(programState),game)==0 && state.shown && state.introWindow==4,"Clouds Escape/Application fresh title route");
  if(skip==60000000000ULL)check(state.introStarts>2,"party switches suppressed EXE speech");
  state.introMode=false;
 }
 {
  state.wallNow=10;XeenProgramState programState{XeenWallClock([]{return state.wallNow;})};state.program=&programState;
  state.introMode=true;state.introDark=true;state.introNow=17000000;state.introWindow=0;state.introStarts=0;state.shown=false;
  const auto settings=state.base/"darkside-d.ini";state.darksideSettings=settings;{std::ofstream out(settings);out<<"won=1\ndarkside_intro_seen=1\n";}
  check(runWithoutStartup(Application(programState,{},settings),game)==0 && state.shown && state.introWindow==4 && state.introStarts>20,"Darkside full Application/fresh title route");
  check(programState.darksideIntroSeen && xeenReadSettings(settings).won && xeenReadSettings(settings).darksideIntroSeen,"D did not restore seen at the store point");
  state.introMode=false;state.introDark=false;state.darksideSettings.clear();
 }
 {
  state.wallNow=10;XeenProgramState programState{XeenWallClock([]{return state.wallNow;})};state.program=&programState;
  state.introMode=true;state.introDark=true;state.introFailure=true;state.introNotice=false;state.introNow=17000000;state.introWindow=0;state.shown=false;
  check(runWithoutStartup(Application(programState),game)==0 && state.shown && state.introNotice && state.introWindow==3,"Darkside failure did not return to fresh title with notice");
  state.introMode=false;state.introDark=false;state.introFailure=false;
 }
 {
  state.wallNow=10;XeenProgramState programState{XeenWallClock([]{return state.wallNow;})};state.program=&programState;
  state.introMode=true;state.introDark=true;state.introMissingExe=true;state.introNotice=false;state.introCloudsNotice=false;state.introNow=17000000;state.introWindow=0;state.shown=false;
  check(runWithoutStartup(Application(programState),game)==0 && state.shown && state.introNotice && state.introWindow==1,"missing EXE did not show Darkside notice");
  state.introMode=false;state.introDark=false;state.introMissingExe=false;
 }
 }
}
void titlePreview(const IndexedFrame &frame,const std::string &label){preview(frame,label);}
