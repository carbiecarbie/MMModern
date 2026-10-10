#include "app/Application.h"
#include "app/XeenTitleFlow.h"
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
std::optional<XeenSessionEntry> showTitle(XeenTitleFlow &flow,XeenAssetSource &assets) {
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
 handler.failed=[&]{flow.close();};handler.closed=[&]{flow.close();};
 handler.protectAllKeys=true;handler.cursorImage=[&]{return assets.cursorImage(XeenSceneArchive::DarksideOnly);};
 handler.drawButton=[&](IndexedFrame &frame,const InputButton &button){flow.drawButton(frame,button);};
 if(!SdlWindow().showInteractive(flow.frame(),"MMModern - World of Xeen",handler,[]{return true;},
  [&]{return flow.animate(now());}))throw std::runtime_error("Title presentation failed");
 return flow.entry();
}
}
int Application::run(const std::filesystem::path &gameDirectory) const {
 try {
  const auto installation=XeenInstallationDetector(_uiData).detect(gameDirectory);
  if(!installation) {std::cerr<<"No Xeen installation found: "<<gameDirectory.u8string()<<'\n';return 2;}
  const std::filesystem::path repository=MMODERN_REPOSITORY_ROOT;
  const auto directory=XeenSaveFile::createSlotDirectory(*installation,repository);
  const auto signature=XeenSaveFile::fingerprint(*installation);
  XeenAssetSource assets(*installation,320,200);
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
  XeenTitleFlow title(XeenTitleFlow::original(assets,font,rows,path));
  XeenSessionEntry entry;
  bool inGameLoad=false;
  for(;;) {
   if(entry.kind==XeenSessionEntry::Kind::Title) {
    const auto selected=showTitle(title,assets);
    if(!selected)return 0;entry=*selected;inGameLoad=false;
   }
   if(entry.kind==XeenSessionEntry::Kind::Exit)return 0;
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
      title.publicationFailure(message);const auto retry=showTitle(title,assets);check();
      if(!retry){exitRequested=true;return false;}
      if(retry->kind!=XeenSessionEntry::Kind::Retry)return false;
     }
    }
   };
   XeenSessionOutcome outcome;
   const int status=gameplay(gameDirectory,{},entry.path,!fresh,
    fresh?XeenEncounterEntry::Journey:XeenEncounterEntry::Ordinary,{},
    fresh?std::optional<XeenDifficulty>{entry.difficulty}:std::nullopt,&entry,&assets,fresh?publish:InitialPublication{},&outcome);
   if(status==0 && outcome.kind==XeenSessionOutcome::Kind::Load){entry=std::move(outcome.entry);inGameLoad=true;continue;}
   outcome.kind=status==5?XeenSessionOutcome::Kind::Title:status==0?XeenSessionOutcome::Kind::Exit:XeenSessionOutcome::Kind::Failure;
   outcome.status=status;
   if(outcome.kind==XeenSessionOutcome::Kind::Title){if(exitRequested)return 0;title.cancelPublication();entry={};continue;}
   if(!fresh && status==3 && !inGameLoad){title.startupFailure("Load validation failed; see the technical diagnostic.");entry={};continue;}
   return outcome.status;
  }
 }catch(const std::exception &e){std::cerr<<"Title startup failed: "<<e.what()<<'\n';return 3;}
}
}
