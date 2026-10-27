#include "app/Application.h"
#include "app/XeenAudio.h"
#include "app/XeenTitleFlow.h"
#include "app/XeenCutsceneFlow.h"
#include "formats/xeen/XeenCutsceneData.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenFontFormat.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "platform/sdl/SdlWindow.h"
#include <chrono>
#include <iostream>
namespace mmodern {
namespace {
std::uint64_t now() {
 return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
  std::chrono::steady_clock::now().time_since_epoch()).count());
}
std::optional<XeenSessionEntry> showTitle(XeenTitleFlow &flow,XeenAssetSource &assets,XeenAudio &audio) {
 audio.enterTitle(SdlWindow::presentationNanoseconds());
 assets.loadPalette(std::string(assets.uiText().scalar("TITLE_PALETTE")),XeenSceneArchive::DarksideOnly);
 SdlWindow::FrameUpdateHandler handler=[&](const PlayerAction &action){return flow.handle(action,flow.frame().presentation());};
 handler.withPresentedInput=[&](const PlayerAction &action,std::uint64_t,const auto &origin){return flow.handle(action,origin);};
 handler.displayedInput=[] {return std::optional<std::uint64_t>{1};};
 handler.acceptsFrame=[&](const auto &p){return flow.acceptsFrame(p);};
 handler.acceptsInputFrame=[&](const auto &p){return flow.acceptsInput(p);};
 handler.inputContext=[&](const auto &p){return flow.inputContext(p);};
 handler.framePresented=[&](const auto &p){flow.presented(p);};
 handler.completeInputHandoff=[&](const auto &p){flow.completeInput(p);};
 handler.frameCurrent=[&]{return flow.current();};
 handler.finished=[&]{return flow.entry().has_value();};
 handler.beginCycle=[&](auto){audio.advance(SdlWindow::presentationNanoseconds());};
 handler.failed=[&]{flow.close();};handler.closed=[&]{flow.close();};
 handler.protectAllKeys=true;handler.cursorImage=[&]{return assets.cursorImage(XeenSceneArchive::DarksideOnly);};
 handler.nextPresentationDeadline=[&]{auto next=flow.nextPresentationDeadline();const auto sound=audio.deadline();if(sound && (!next||*sound<*next))next=sound;return next;};
 handler.drawButton=[&](IndexedFrame &frame,const InputButton &button){flow.drawButton(frame,button);};
 if(!SdlWindow().showInteractive(flow.frame(),"MMModern",handler,[]{return true;},
  [&]{const auto now=SdlWindow::presentationNanoseconds();audio.advance(now);return flow.animateAt(now);}))throw std::runtime_error("Title presentation failed");
 if(flow.entry())audio.leaveTitle(SdlWindow::presentationNanoseconds());
 return flow.entry();
}
bool showCutscene(XeenCutsceneFlow &flow,XeenAssetSource &assets,XeenAudio &audio,bool controlQuit=true) {
 SdlWindow::FrameUpdateHandler h=[&](const auto &action){return flow.handle(action,flow.frame().presentation());};
 h.withPresentedInput=[&](const auto &a,auto,const auto &p){return flow.handle(a,p);};
 h.displayedInput=[] {return std::optional<std::uint64_t>{1};};h.protectAllKeys=true;h.ignoreMouse=true;
 h.cutsceneControlQuit=controlQuit;
 h.acceptsFrame=[&](const auto &p){return flow.acceptsFrame(p);};h.acceptsInputFrame=[&](const auto &p){return flow.acceptsInput(p);};
 h.inputContext=[&](const auto &p){return flow.inputContext(p);};h.framePresented=[&](const auto &p){flow.presented(p);};
 h.completeInputHandoff=[&](const auto &p){flow.completeInput(p);};h.frameCurrent=[&]{return flow.current();};h.finished=[&]{return flow.done();};
 h.failed=[&]{flow.close();};h.closed=[&]{flow.close();};
 h.cursorImage=[&]{return assets.cursorImage(XeenSceneArchive::DarksideOnly);}; // Native pointer hidden; context suppresses the game pointer.
 h.beginCycle=[&](auto){audio.advance(SdlWindow::presentationNanoseconds());};
 h.nextPresentationDeadline=[&] {
  auto next=flow.nextPresentationDeadline();auto sound=audio.deadline();if(sound && (!next || *sound<*next))next=sound;return next;
 };
 const auto ok=SdlWindow().showInteractive(flow.frame(),"MMModern",h,[]{return true;},[&]{
  audio.advance(SdlWindow::presentationNanoseconds());return flow.update();
 });
 if(!ok)throw std::runtime_error("Cutscene presentation failed");
 return flow.done();
}
XeenCutscene titleExit() {
 XeenCutscene s;XeenCutscene::Segment fade;fade.name="title fade out";fade.input=XeenCutscene::Segment::Input::Ignore;
 fade.frames=33;fade.compose=[](const auto &c)->XeenCutscene::Frame{return {XeenCutscene::Level{128-4*c.iteration}};};
 s.segments.push_back(std::move(fade));XeenCutscene::Segment idle;idle.name="title digital idle";
 idle.input=XeenCutscene::Segment::Input::Ignore;idle.frames=0;idle.untilDigitalIdle=true;idle.compose=[](const auto &)->XeenCutscene::Frame{return {};};
 s.segments.push_back(std::move(idle));return s;
}
}
int Application::run(const std::filesystem::path &gameDirectory) const {return runTitle(gameDirectory,{},true);}
int Application::runTitle(const std::filesystem::path &gameDirectory,std::optional<std::uint32_t> endingScore,bool startup) const {
 try {
  const auto installation=XeenInstallationDetector(_uiData).detect(gameDirectory);
  if(!installation) {std::cerr<<"No Xeen installation found: "<<gameDirectory.u8string()<<'\n';return 2;}
  const std::filesystem::path repository=MMODERN_REPOSITORY_ROOT;
  const auto directory=XeenSaveFile::createSlotDirectory(*installation,repository);
  const auto signature=XeenSaveFile::fingerprint(*installation);
  XeenAssetSource assets(*installation,320,200);
  if(!_program.audio)_program.audio=XeenAudio::installed(*installation,assets,SdlWindow::presentationNanoseconds());
  const XeenFontFormat font(assets.readArchiveResource("fnt",XeenSceneArchive::DarksideOnly));
  const auto path=[&](unsigned slot) {
   return XeenSaveFile::resolve(XeenSaveFile::resolve(XeenSaveFile::slotPath(directory,slot),*installation),repository);
  };
  const auto rows=[&] {
   std::array<XeenSaveFile::Slot,10> slots;
   for(unsigned i=0;i<10;++i) {
    try {slots[i]=XeenSaveFile::inspectSlot(path(i),signature);}
    catch(const std::exception &e){slots[i].state=XeenSaveFile::Slot::State::Protected;slots[i].reason=e.what();}
   }
   return slots;
  };
  std::optional<XeenCutsceneData> introData;
  const auto makeTitle=[&](XeenSoundOptions options=XeenSoundOptions{}) {
   auto services=XeenTitleFlow::original(assets,font,rows,path);services.soundOptions=options;
   services.cloudsContext=installation->hasXeen();
   services.cloudsWon=_program.cloudsWon;
   services.cloudsIntroCheck=[&] {
    try {introData=XeenCutsceneData::installed(*installation);return std::string{};}
    catch(const std::exception &e){introData.reset();return std::string(e.what());}
   };
   services.darksideIntroCheck=[&] {
    try {
     introData=XeenCutsceneData::installed(*installation);
     if(!installation->introData)throw std::runtime_error("missing INTRO.CC");
     if(!installation->hasDarkside())throw std::runtime_error("missing DARK.CC");
     return std::string{};
    }catch(const std::exception &e){
     introData.reset();const std::string error=e.what(),clouds="Clouds intro unavailable",missing="missing ";
     if(error.find(clouds)==0){std::cerr<<"Darkside intro unavailable"<<error.substr(clouds.size())<<'\n';return std::string("Darkside intro unavailable\nCheck XEEN.EXE / XEEN.CFG");}
     std::cerr<<"Darkside intro unavailable: "<<error<<'\n';
     if(error.find(missing)==0)return "Darkside intro unavailable\nMissing "+error.substr(missing.size());
     return std::string("Darkside intro unavailable\nResource error (see log)");
    }
   };return std::make_unique<XeenTitleFlow>(std::move(services));
  };
  const auto darksideScene=[&](XeenCutscene::DarksidePath path) {
   auto scene=XeenCutscene::darkside(*introData,XeenCutsceneData::captions(assets.readArchiveResource(introData->darkside.resources[XeenCutsceneData::Darkside::Caption],XeenSceneArchive::DarksideProgram)),_program.wallClock,path);
   for(auto &segment:scene.segments) if(segment.name=="seen point") {
    auto entering=segment.entering;
    segment.entering=[&,entering](const XeenCutscene::Context &context) {
     if(entering)entering(context);
     _program.darksideIntroSeen=true;
     if(_settingsFile)xeenWriteSeen(*_settingsFile,true);
    };
   }
   return scene;
  };
  if(startup) {
   auto &audio=*_program.audio;const auto clock=[] {return SdlWindow::presentationNanoseconds();};
   try {
    introData=XeenCutsceneData::installed(*installation);
    if(!installation->introData || !installation->hasDarkside())throw std::runtime_error("missing INTRO.CC or DARK.CC");
    audio.observeOptions({});audio.startProgram(clock());
    auto path=_program.darksideIntroSeen?XeenCutscene::DarksidePath::Short:XeenCutscene::DarksidePath::Full;
    XeenCutsceneFlow intro(darksideScene(path),XeenCutsceneFlow::original(assets,font,audio,clock,introData->digitalDevice>=2,XeenSceneArchive::DarksideProgram));
    if(!showCutscene(intro,assets,audio))return 0;
   }catch(const std::exception &e) {std::cerr<<"Darkside intro unavailable: "<<e.what()<<'\n';}
   audio.stopDigital(clock());audio.fadeSong(0,clock());audio.observeOptions({});audio.startProgram(clock());
   _program.resetPlayTime();
  }
  auto title=makeTitle();
  XeenSessionEntry entry;
  if(endingScore){entry.kind=XeenSessionEntry::Kind::CloudsEnding;entry.score=*endingScore;}
  bool inGameLoad=false;
  for(;;) {
   if(entry.kind==XeenSessionEntry::Kind::Title) {
    _program.innSongStarted=true;
    const auto selected=showTitle(*title,assets,*_program.audio);
    if(!selected)return 0;entry=*selected;inGameLoad=false;
   }
   if(entry.kind==XeenSessionEntry::Kind::Exit)return 0;
   if(entry.kind==XeenSessionEntry::Kind::CloudsEnding) {
    auto &audio=*_program.audio;const auto options=audio.soundOptions();
    const auto clock=[] {return SdlWindow::presentationNanoseconds();};
    audio.stopDigital(clock());audio.fadeSong(0,clock());audio.observeOptions({});
    audio.startProgram(clock()); // XEEN.EXE driver init (EXE 0x79f1).
    std::string notice;
    bool resourcesLoaded=false;
    try {
     const auto data=XeenCutsceneData::installed(*installation);
     const auto captions=XeenCutsceneData::captions(assets.readArchiveResource(data.resources[XeenCutsceneData::Caption],XeenSceneArchive::CloudsEnding));
     resourcesLoaded=true;
     XeenCutsceneFlow ending(XeenCutscene::cloudsEnding(data,captions,_program.wallClock,entry.score),
      XeenCutsceneFlow::original(assets,font,audio,clock,data.digitalDevice>=2,XeenSceneArchive::CloudsEnding));
     if(!showCutscene(ending,assets,audio)){audio.observeOptions(options);return 0;}
    }catch(const std::exception &e) {
     std::cerr<<"Clouds ending unavailable: "<<e.what()<<'\n';
     notice=resourcesLoaded?"Clouds ending unavailable\nResource error (see log)":"Clouds ending unavailable\nCheck XEEN.EXE / XEEN.CFG";
    }
    // No mixer command: the fresh DAT driver init rewrites the init levels.
    audio.stopDigital(clock());audio.fadeSong(0,clock());audio.observeOptions(options);
    audio.startProgram(clock()); // Fresh XEEN.DAT driver init (DAT 0xb646).
    title=makeTitle(options);if(!notice.empty())title->cloudsEndingFailure(notice);
    _program.resetPlayTime();entry={};continue;
   }
   if(entry.kind==XeenSessionEntry::Kind::CloudsIntro || entry.kind==XeenSessionEntry::Kind::DarksideIntro) {
    const bool darkside=entry.kind==XeenSessionEntry::Kind::DarksideIntro;
    if(darkside) {_program.darksideIntroSeen=false;if(_settingsFile)xeenWriteSeen(*_settingsFile,false);}
    auto &audio=*_program.audio;const auto options=audio.soundOptions();
    const auto clock=[] {return SdlWindow::presentationNanoseconds();};
    auto exitServices=XeenCutsceneFlow::original(assets,font,audio,clock);
    exitServices.initial=title->cutsceneExitFrame();
    exitServices.digitalBusy=[&](auto now){return !audio.titleTeardownReady(now);};
    XeenCutsceneFlow leaving(titleExit(),std::move(exitServices));
    // The DAT title-exit fade and digital wait poll no keys (0xb83e..0xb8ae); only the EXE quits on Ctrl-Q.
    if(!showCutscene(leaving,assets,audio,false))return 0;
    // EXE wrappers consult hardware, never the DAT party option bytes.
    audio.fadeSong(0,clock());audio.observeOptions({});
    audio.startProgram(clock());
    try {
     if(!introData)throw std::runtime_error(darkside?"Darkside intro unavailable: missing or different WORLD/XEEN.EXE":"Clouds intro unavailable: missing or different WORLD/XEEN.EXE");
     auto scene=darkside?darksideScene(XeenCutscene::DarksidePath::Full):
      XeenCutscene::clouds(*introData,XeenCutsceneData::firstCaption(assets.readArchiveResource(introData->resources[XeenCutsceneData::Caption],XeenSceneArchive::CloudsProgram)));
     XeenCutsceneFlow intro(std::move(scene),XeenCutsceneFlow::original(assets,font,audio,clock,introData->digitalDevice>=2,darkside?XeenSceneArchive::DarksideProgram:XeenSceneArchive::CloudsProgram));
     if(!showCutscene(intro,assets,audio)){audio.observeOptions(options);return 0;}
     audio.observeOptions(options);audio.startProgram(clock());title=makeTitle(options);
    }catch(const std::exception &e) {
     audio.stopDigital(clock());audio.fadeSong(0,clock());audio.observeOptions(options);audio.startProgram(clock());
     title=makeTitle(options);
     if(darkside) {
      std::cerr<<"Darkside intro unavailable: "<<e.what()<<'\n';
      title->darksideIntroFailure(introData?"Darkside intro unavailable\nResource error (see log)":"Darkside intro unavailable\nCheck XEEN.EXE / XEEN.CFG");
     }else title->cloudsIntroFailure(e.what());
    }
    _program.resetPlayTime();entry={};continue;
   }
   if(entry.kind!=XeenSessionEntry::Kind::New && entry.kind!=XeenSessionEntry::Kind::Load)
    throw std::logic_error("Invalid application session entry");
   const bool fresh=entry.kind==XeenSessionEntry::Kind::New;
   bool exitRequested=false;
   const auto publish=[&](const XeenSaveSnapshot &candidate,const std::function<void()> &check) {
    for(;;) {
     check();
     try {
      XeenSaveFile::writeSlot(directory,entry.slot,candidate,*installation,repository,{},check);
      check();return true;
     }catch(const std::exception &e) {
      check();std::string message=e.what();
      if(message.find("Windows error 112")!=std::string::npos || message.find("Windows error 39")!=std::string::npos)
       message=std::string(assets.uiText().scalar("NEW_SPACE"));
      title->publicationFailure(message);const auto retry=showTitle(*title,assets,*_program.audio);check();
      if(!retry){exitRequested=true;return false;}
      if(retry->kind!=XeenSessionEntry::Kind::Retry)return false;
     }
    }
   };
   XeenSessionOutcome outcome;
   const int status=gameplay(gameDirectory,{},entry.path,!fresh,
    fresh?XeenEncounterEntry::Journey:XeenEncounterEntry::Ordinary,{},
    fresh?std::optional<XeenDifficulty>{entry.difficulty}:std::nullopt,&entry,&assets,fresh?publish:InitialPublication{},&outcome);
   if(status==0 && outcome.kind==XeenSessionOutcome::Kind::CloudsEnding) {
    entry={};entry.kind=XeenSessionEntry::Kind::CloudsEnding;entry.score=outcome.score;inGameLoad=false;continue;
   }
   if(status==0 && outcome.kind==XeenSessionOutcome::Kind::Load){entry=std::move(outcome.entry);inGameLoad=true;continue;}
   if(status==0 && outcome.kind==XeenSessionOutcome::Kind::Title){title->returnToTitle();entry={};inGameLoad=false;continue;}
   outcome.kind=status==5?XeenSessionOutcome::Kind::Title:status==0?XeenSessionOutcome::Kind::Exit:XeenSessionOutcome::Kind::Failure;
   outcome.status=status;
   if(outcome.kind==XeenSessionOutcome::Kind::Title){if(exitRequested)return 0;title->cancelPublication();entry={};continue;}
   if(!fresh && status==3 && !inGameLoad){title->startupFailure("Load validation failed; see the technical diagnostic.");entry={};continue;}
   return outcome.status;
  }
 }catch(const std::exception &e){std::cerr<<"Title startup failed: "<<e.what()<<'\n';return 3;}
}
}
