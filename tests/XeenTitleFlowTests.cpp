#include "app/XeenTitleFlow.h"
#include "app/XeenCutsceneFlow.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "XeenSaveTestSupport.h"
#include "platform/sdl/SdlWindow.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <chrono>
#include <iostream>
#include <memory>
using namespace mmodern;
using save_test::check;
void titleApplicationControls(const char *);
void titlePreview(const IndexedFrame &,const std::string &);
namespace {
using Screen=XeenTitleFlow::Screen;
void cutsceneMouse() {
 std::uint64_t now=0;XeenCutscene scene;XeenCutscene::Segment s;s.name="mouse ignored";s.frames=4;s.ticks=2;
 s.compose=[](const auto &)->XeenCutscene::Frame{return {};};scene.segments.push_back(s);
 XeenCutsceneFlow::Services providers;providers.clock=[&]{return now;};providers.read=[](const auto &){return std::vector<std::uint8_t>{};};
 providers.draw=[](auto &,const auto &){};providers.audio=[](const auto &,auto){};providers.digitalBusy=[](auto){return false;};
 XeenCutsceneFlow flow(std::move(scene),providers);unsigned cycles=0;bool injected=false;
 SdlWindow::FrameUpdateHandler h=[](const auto &)->std::optional<IndexedFrame>{throw std::runtime_error("ignored mouse became an action");};
 h.ignoreMouse=true;h.protectAllKeys=true;h.presentationClock=[&]{return now;};h.finished=[&]{return flow.done();};
 h.displayedInput=[] {return std::optional<std::uint64_t>{1};};
 h.acceptsFrame=[&](const auto &p){return flow.acceptsFrame(p);};h.acceptsInputFrame=[&](const auto &p){return flow.acceptsInput(p);};
 h.frameCurrent=[&]{return flow.current();};h.inputContext=[&](const auto &p){return flow.inputContext(p);};
 h.framePresented=[&](const auto &p){flow.presented(p);};h.completeInputHandoff=[&](const auto &p){flow.completeInput(p);};
 h.beginCycle=[&](auto) {
  check(++cycles<100,"held mouse stopped cutscene publication");
  if(!injected){injected=true;SDL_Event e{};e.type=SDL_MOUSEBUTTONDOWN;e.button.button=SDL_BUTTON_LEFT;e.button.x=160;e.button.y=100;e.button.timestamp=SDL_GetTicks();check(SDL_PushEvent(&e)==1,"cutscene mouse injection");}
 };
 check(SdlWindow().showInteractive(flow.frame(),"Cutscene ignores held mouse",h,[]{return true;},[&]{now+=20000000;return flow.update();}) && flow.done(),"native cutscene mouse policy");
}
struct Driver {
 // Several simultaneous ten-save chooser fixtures exceed the Windows Debug
 // stack after v9 adds roster awards. Keep these test owners on the heap.
 std::unique_ptr<XeenTitleFlow> owner;
 XeenTitleFlow &flow;std::uint64_t now=1;
 explicit Driver(XeenTitleFlow::Services services):owner(std::make_unique<XeenTitleFlow>(std::move(services))),flow(*owner){present();fade();}
 void present(){flow.presented(flow.frame().presentation());flow.completeInput(flow.frame().presentation());}
 void fade(){
  flow.animate(now);
  for(unsigned i=0;i<65 && !flow.inputContext(flow.frame().presentation()).readyForAction;++i){now+=20;flow.animate(now);present();}
  check(flow.inputContext(flow.frame().presentation()).readyForAction,"fade did not publish all levels");
 }
 void key(unsigned value) {
  const auto action=flow.inputContext(flow.frame().presentation()).dialog->key(value);
  check(bool(action),"missing title key mapping");flow.handle(*action,flow.frame().presentation());if(flow.current())present();
 }
 void click(int x,int y) {
  const auto action=flow.inputContext(flow.frame().presentation()).dialog->click(x,y);
  check(bool(action),"missing title hit mapping");flow.handle(*action,flow.frame().presentation());present();
 }
 void type(std::string value){flow.handle(TextInputAction{std::move(value)},flow.frame().presentation());present();}
};
// A mouse press sampled in the bounded A batch must retain A through release,
// including when cosmetic B was just acquired. It cannot be relabeled as B.
void cosmeticMouseRelease(const XeenTitleFlow::Services &services) {
 for(unsigned mode=0;mode<3;++mode) {
  const bool held=mode==1,close=mode==2;
  Driver d(services);d.key(27);const auto origin=d.flow.frame().presentation();
  bool injected=false,releaseNextCycle=false;unsigned delivered=0,loops=0;
  const auto mouse=[&](Uint32 type) {
   const auto dialog=d.flow.inputContext(origin).dialog;
   const auto hit=std::find_if(dialog->hits.begin(),dialog->hits.end(),[](const auto &h){return h.key=='s';});
   check(hit!=dialog->hits.end(),"cosmetic mouse Start hit");
   SDL_Event event{};event.type=type;event.button.button=SDL_BUTTON_LEFT;
   event.button.x=hit->left;event.button.y=hit->top;event.button.timestamp=SDL_GetTicks();
   check(SDL_PeepEvents(&event,1,SDL_ADDEVENT,0,0)==1,"cosmetic mouse queue");
  };
  SdlWindow::FrameUpdateHandler h=[](const auto &)->std::optional<IndexedFrame>{throw std::runtime_error("unversioned cosmetic mouse");};
  h.protectAllKeys=true;h.displayedInput=[] {return std::optional<std::uint64_t>{1};};
  h.acceptsFrame=[&](const auto &p){return d.flow.acceptsFrame(p);};
  h.acceptsInputFrame=[&](const auto &p){return d.flow.acceptsInput(p);};
  h.frameCurrent=[&]{return d.flow.current();};h.inputContext=[&](const auto &p){return d.flow.inputContext(p);};
  h.withPresentedInput=[&](const auto &action,auto,const auto &p){
   check(p==origin && d.flow.acceptsInput(origin),"cosmetic mouse borrowed new origin");
   ++delivered;return d.flow.handle(action,p);
  };
  h.framePresented=[&](const auto &p){d.flow.presented(p);if(p!=origin && !injected){
   injected=true;mouse(SDL_MOUSEBUTTONDOWN);if(held)releaseNextCycle=true;else mouse(SDL_MOUSEBUTTONUP);
   if(close){SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);}
  }};
  h.completeInputHandoff=[&](const auto &p){d.flow.completeInput(p);};
  h.beginCycle=[&](auto){if(releaseNextCycle){releaseNextCycle=false;mouse(SDL_MOUSEBUTTONUP);}};
  h.drawButton=[&](auto &frame,const auto &button){d.flow.drawButton(frame,button);};
  const bool ok=SdlWindow().showInteractive(d.flow.frame(),"Cosmetic mouse origin through release",h,[]{return true;},[&]()->std::optional<IndexedFrame>{
   check(++loops<8,"cosmetic mouse was lost at input handoff");
   if(d.flow.screen()==Screen::NewSlots){check(delivered==1,"cosmetic click repeated");SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return {};}
   d.now+=220;return d.flow.animate(d.now);
  });
  check(ok && injected && delivered==(close?0u:1u),"cosmetic mouse release did not retain acquired origin or crossed close");
  if(close)check(d.flow.screen()==Screen::Menu,"queued close applied a mouse response");
 }
}
void panelControls(XeenAssetSource &assets,const XeenFontFormat &font) {
 assets.loadPalette("dark.pal",XeenSceneArchive::DarksideOnly);
 std::array<XeenSaveFile::Slot,10> rows;
 auto snapshot=save_test::currentWireSnapshot();snapshot.name="Existing";
 rows[0]={XeenSaveFile::Slot::State::Available,snapshot,{}};
 rows[9]={XeenSaveFile::Slot::State::Protected,{},"Synthetic protected slot"};
 const auto services=[&](bool combat=false,bool restricted=false) {
  XeenTitleFlow::Services s{assets.uiText(),font,assets.snapshot(),{}, {},
   [&](auto &frame,const char *name,unsigned index,int x,int y){assets.drawDialogSprite(frame,name,index,x,y);},
   [&]{return rows;},[](unsigned slot){return std::filesystem::path("panel-test")/std::to_string(slot);}};
  s.panel=true;s.combat=combat;s.saveRestricted=restricted;s.currentSlot=0;s.currentName="Native Case !~";return s;
 };
 Driver d(services());check(d.flow.screen()==Screen::Panel,"panel entry");
 auto longName=services();longName.currentName=std::string(20,'W');Driver longest(std::move(longName));
 check(longest.flow.screen()==Screen::Panel,"long current name must stop at original window bottom");
 // Normal newline termination is original behavior; an absolute cursor control
 // that tries to draw outside the window must still trip the layout guard.
 bool overflow=false;
 try {longest.flow.panelResult("\v999X",true);}
 catch(const std::runtime_error &e){overflow=std::string(e.what()).find("Original dialog layout overflow")!=std::string::npos;}
 check(overflow,"saved confirmation disabled control/glyph overflow validation");
 titlePreview(d.flow.frame(),"panel");
 const auto hits=d.flow.inputContext(d.flow.frame().presentation()).dialog;
 check(hits->hits.size()==9,"DOS panel requires nine buttons");
 for(unsigned key:{'e','m','p','t'}) {
  const auto it=std::find_if(hits->hits.begin(),hits->hits.end(),[&](const auto &h){return h.key==key;});
  check(it!=hits->hits.end() && it->button && it->button->pressedFrame()==1,"panel pressed frame");
  d.click(it->left,it->top);check(d.flow.screen()==Screen::Panel && !d.flow.entry(),"sound toggle must remain on panel");
 }
 check(d.flow.soundOptions()==XeenSoundOptions{false,false,true,false},"Speech off forces Text on, including Text toggle redraw");
 d.key('p');d.key('t');check(d.flow.soundOptions()==XeenSoundOptions{false,false,false,true},"Text off allowed with Speech on");
 d.key('w');check(d.flow.screen()==Screen::Wizard,"Mr Wizard confirmation first");d.key('n');
 d.key('w');d.key('y');check(d.flow.entry() && d.flow.entry()->kind==XeenSessionEntry::Kind::MrWizard,"Mr Wizard request after Yes");
 d.flow.panelFailure("not supported yet");d.present();check(d.flow.screen()==Screen::Notice && !d.flow.entry(),"refused Mr Wizard notice");d.key(27);
 {Driver c(services(true));c.key('w');c.key('y');
  check(c.flow.entry() && c.flow.entry()->kind==XeenSessionEntry::Kind::MrWizard,"combat Mr Wizard request after Yes");}
 d.key('q');d.key(27);check(d.flow.screen()==Screen::Panel,"Quit cancel");
 d.key('a');check(d.flow.screen()==Screen::SaveSlots,"Save As chooser");
 d.key('1');d.key(13);check(d.flow.screen()==Screen::Overwrite,"Save As overwrite before name");
 d.key(27);check(d.flow.screen()==Screen::SaveSlots,"overwrite Escape to list");
 d.key(13);d.key('y');d.key(27);check(d.flow.screen()==Screen::SaveSlots,"Save As name Escape to list");
 d.key('3');d.key(13);d.type("Discarded");d.key(27);
 d.key('4');d.key(13);d.key(13);
 check(d.flow.screen()==Screen::Name,"Save As retained a cancelled name on another slot");
 d.key(27);d.key('3');d.key(13);d.type("New Name");d.key(13);
 check(d.flow.entry() && d.flow.entry()->kind==XeenSessionEntry::Kind::Save && d.flow.entry()->slot==2 && d.flow.entry()->name=="New Name","Save As request");
 d.flow.panelCurrent(2,"New Name");d.flow.panelResult(xeenDialogFormat(assets.uiText().scalar("SAVED_NOTICE"),{"New Name"}),true);d.present();
 check(d.flow.screen()==Screen::SavedNotice,"saved confirmation must use its original window");d.key(27);
 check(d.flow.screen()==Screen::SaveSlots,"success must return to list");d.key(27);d.key('s');
 check(d.flow.entry()->slot==2 && d.flow.entry()->name=="New Name","Save As sets current direct Save slot");
 d.flow.panelResult("Synthetic failure");d.present();d.key(27);d.key('l');d.key('0');
 check(d.flow.screen()==Screen::Notice && !d.flow.entry(),"protected Load row");d.key(27);d.key(27);
 d.key('q');d.key('y');check(d.flow.entry()->kind==XeenSessionEntry::Kind::Exit,"Quit confirmation outcome");
 for(bool restricted:{false,true}) {
  Driver refusal(services(!restricted,restricted));
  for(unsigned code:{'s','a'}){refusal.key(code);check(refusal.flow.screen()==Screen::Notice,"save refusal");refusal.key(27);}
  refusal.key('l');check(refusal.flow.screen()==(restricted?Screen::LoadSlots:Screen::Notice),"map restriction applies to Save only");
 }
 auto developer=services();developer.currentSlot.reset();developer.currentName.clear();Driver dev(std::move(developer));dev.key('s');
 check(dev.flow.screen()==Screen::SaveSlots,"developer Save must select managed slot");dev.key(27);
 const auto retired=dev.flow.frame().presentation();dev.key(27);check(!dev.flow.current() && !dev.flow.acceptsInput(retired),"panel Escape invalidates input");
 Driver cursorFlow(services());cursorFlow.key('a');cursorFlow.key('3');cursorFlow.key(13);
 const auto cursorPixels=[&](unsigned glyph) {
  XeenTextRenderOptions o;o.originalControls=true;o.drawWindow=true;o.windowBounds={52,149,268,198};
  o.bounds={60,157,260,190};o.x=60;o.y=157;XeenTextRenderer renderer(font);
  const auto prompt=renderer.render(assets.snapshot(),std::string(assets.uiText().scalar("NAME_PROMPT")),o);
  o.drawWindow=false;o.x=prompt.writeX;o.y=prompt.writeY;o.size=prompt.writeSize;o.colorIndex=prompt.writeColor;
  return renderer.render(prompt.pages.front(),std::string(1,char(glyph)),o).pages.front().pixels;
 };
 check(cursorFlow.flow.frame().pixels==cursorPixels(124),"name entry first visible cursor glyph");
 cursorFlow.type(" ");check(cursorFlow.flow.frame().pixels==cursorPixels(126),"rejected leading space did not redraw/advance cursor");
 cursorFlow.key(8);check(cursorFlow.flow.frame().pixels==cursorPixels(127),"empty backspace did not redraw/advance cursor");
 XeenTextInput input;input.begin();check(input.cursor()==124 && !input.animate(100),"cursor first visible glyph/wait");
 const unsigned glyphs[]{126,127,126,124,32,124,126};
 for(unsigned glyph:glyphs) {
  const auto deadline=*input.deadline();
  check(!input.animate(deadline-1),"cursor advanced before counter increment");
  check(input.animate(deadline) && input.cursor()==glyph,"cursor glyph/cadence");
 }
 check(input.animate(700) && input.cursor()==127,"late cursor wake skipped a glyph");
 check(input.type("Case !~ ",font),"cursor input");input.keyRedraw();
 check(input.cursor()==126 && input.backspace() && input.value()=="Case !~","typed key redraw cursor phase/input");
 input.keyRedraw();check(input.cursor()==124 && !input.animate(700),"backspace redraw did not restart wait cadence");
 const auto deadline=*input.deadline();check(!input.animate(deadline-1),"cursor early reset edge");
 check(input.animate(deadline) && input.cursor()==32,"first wait tick after key redraw");

}
void animationResume(XeenTitleFlow::Services source) {
 // An off-dialog pixel identifies the exact animation phase independently of
 // fonts, menu text or palette fades, without adding a production test accessor.
 for(unsigned i=0;i<source.animation.size();++i)source.animation[i].pixels[0]=i;
 for(const auto screen:{Screen::NewSlots,Screen::LoadSlots,Screen::Name,Screen::Difficulty,
   Screen::Overwrite,Screen::PublicationFailure,Screen::Credits}) {
  Driver d(source);d.key(27);d.flow.animate(d.now);d.now+=220;d.flow.animate(d.now);d.present();
  const auto phase=d.flow.frame().pixels[0];
  if(screen==Screen::Credits)d.key('c');
  else if(screen==Screen::LoadSlots)d.key('l');
  else {
   d.key('s');
   if(screen==Screen::Overwrite){d.key('1');d.key(13);}
   else if(screen!=Screen::NewSlots) {
    d.key('3');d.key(13);
    if(screen!=Screen::Name){d.type("Temporary");d.key(13);}
    if(screen==Screen::PublicationFailure){d.key('a');d.flow.publicationFailure("Synthetic failure");d.present();}
   }
  }
  check(d.flow.screen()==screen,"animation modal setup");
  d.now+=10000;check(!d.flow.animate(d.now),"modal screen animated");
  if(screen==Screen::PublicationFailure){d.flow.cancelPublication();d.present();}
  else if(screen==Screen::Credits)for(unsigned page=0;page<4;++page)d.key(27);
  else d.key(27);
  if(d.flow.screen()==Screen::NewSlots)d.key(27);
  check(d.flow.screen()==Screen::Menu && d.flow.frame().pixels[0]==phase,"modal visit changed retained phase");
  check(!d.flow.animate(d.now),"animation resumed by consuming modal time");
  if(screen==Screen::Credits)d.fade();
  d.now+=220;check(bool(d.flow.animate(d.now)),"resumed animation did not tick");d.present();
  check(d.flow.frame().pixels[0]==(phase+1)%source.animation.size(),"animation did not resume at phase+1");
 }
}
void mappedHotkeys(const GameInstallation &installation,XeenAssetSource &assets,const XeenFontFormat &font) {
 auto stream=installation.uiModule->open();std::vector<std::uint8_t> bytes(stream->size());
 check(stream->read(bytes.data(),bytes.size())==bytes.size(),"hotkey DAT short read");
 for(const auto &layout:XeenDosText::buttonLayouts())for(unsigned i=0;i<layout.count;++i)
  bytes[layout.key+i]=std::string_view(layout.name)=="TITLE"?"jkuv"[i]:std::string_view(layout.name)=="DIFFICULTY"?"zx"[i]:'a'+i;
 bytes[XeenDosText::kOtherButtonKeyOffsets[0]]='b';bytes[XeenDosText::kOtherButtonKeyOffsets[1]]='h';
 const XeenDosText mapped(bytes);
 std::array<XeenSaveFile::Slot,10> rows;auto snapshot=save_test::currentWireSnapshot();snapshot.name="Existing";
 rows[0]={XeenSaveFile::Slot::State::Available,snapshot,{}};
 const auto original=XeenTitleFlow::original(assets,font,[&]{return rows;},[](unsigned i){return std::filesystem::path(std::to_string(i));});
 const auto services=[&](bool panel=false) {
  XeenTitleFlow::Services s{mapped,font,original.background,original.animation,original.credits,original.draw,original.slots,original.path};
  s.panel=panel;s.currentSlot=0;s.currentName="Existing";return s;
 };
 Driver d(services());d.key(27);d.key('j');d.key('e');d.key('p');d.type("Mapped");d.key(13);d.key('x');
 check(d.flow.entry() && d.flow.entry()->slot==2 && d.flow.entry()->difficulty==XeenDifficulty::Warrior,"DAT-index New/chooser/difficulty mapping");
 Driver load(services());load.key(27);load.key('v');load.key('b');
 check(load.flow.screen()==Screen::Notice,"DAT-index Other Options mapping");load.key(27);load.key(27);load.key('u');
 check(load.flow.screen()==Screen::Credits,"DAT-index Credits mapping");for(unsigned page=0;page<4;++page)load.key(27);load.fade();load.key('k');load.key('c');load.key(13);
 check(load.flow.entry() && load.flow.entry()->kind==XeenSessionEntry::Kind::Load,"DAT-index title Load mapping");
 Driver p(services(true));
 for(unsigned key:{'a','b','g','h'}){p.key(key);check(p.flow.screen()==Screen::Panel,"DAT-index sound panel mapping");}
 p.key('f');check(p.flow.screen()==Screen::Quit,"DAT-index Quit mapping");p.key('n');
 p.key('i');check(p.flow.screen()==Screen::Wizard,"DAT-index Wizard mapping");p.key('n');
 p.key('d');check(p.flow.entry() && p.flow.entry()->kind==XeenSessionEntry::Kind::Save,"DAT-index direct Save mapping");
 p.flow.panelResult("Synthetic failure");p.present();p.key(27);p.key('e');
 check(p.flow.screen()==Screen::SaveSlots,"DAT-index Save As mapping");p.key('m');p.key('c');
 check(p.flow.screen()==Screen::LoadSlots,"DAT-index chooser Exit/panel Load mapping");
}
}
int main(int argc,char **argv){try {
 check(argc==2,"Title flow test requires original CD source");
 {
 const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"missing CD installation");
 XeenAssetSource assets(*installation,320,200);const XeenFontFormat font(assets.readArchiveResource("fnt",XeenSceneArchive::DarksideOnly));
 std::array<XeenSaveFile::Slot,10> slots;
 auto saved=save_test::currentWireSnapshot();saved.name="Retained Game";
 slots[0].state=XeenSaveFile::Slot::State::Available;slots[0].snapshot=saved;
 slots[5].state=XeenSaveFile::Slot::State::Protected;slots[5].reason="Synthetic protected target";
 const auto services=[&]{return XeenTitleFlow::original(assets,font,[&]{return slots;},[](unsigned slot){return std::filesystem::path("test-slots")/(std::to_string(slot)+".mmsave");});};
 // Section wall times go to stderr, so a slow run shows where it went.
 auto sectionBegan=std::chrono::steady_clock::now();
 const auto section=[&](const char *name) {
  const auto now=std::chrono::steady_clock::now();
  std::cerr<<name<<": "<<std::chrono::duration_cast<std::chrono::milliseconds>(now-sectionBegan).count()<<" ms\n";sectionBegan=now;
 };
 panelControls(assets,font);section("panelControls");
 animationResume(services());section("animationResume");cosmeticMouseRelease(services());section("cosmeticMouseRelease");
 mappedHotkeys(*installation,assets,font);section("mappedHotkeys");
 Driver d(services());
 check(d.flow.screen()==Screen::Background,"plain title starts with animated background");d.key(27);
 const auto menu=d.flow.frame().pixels;const auto old=d.flow.frame().presentation();
 d.flow.animate(d.now);d.now+=220;d.flow.animate(d.now);d.present();check(d.flow.frame().pixels!=menu,"title animation phase did not advance");
 d.key(27);check(d.flow.screen()==Screen::Background,"title Escape hid menu");
 check(!d.flow.handle(DialogKeyAction{'s'},old),"stale title frame opened New");
 d.key(27);check(d.flow.screen()==Screen::Menu,"background Escape must reopen menu");
 const auto input=d.flow.inputContext(d.flow.frame().presentation()).dialog;
 const auto &start=input->hits.at(0);
 check(!input->click(start.left-1,start.top) && input->click(start.left,start.top) &&
  input->click(start.right-1,start.bottom-1) && !input->click(start.right,start.bottom),"title hit edges");
 d.key('c');check(d.flow.screen()==Screen::Credits,"credits entry");
 for(unsigned page=0;page<4;++page) {
  titlePreview(d.flow.frame(),"credits-"+std::to_string(page+1));
  const auto before=d.flow.frame().pixels;d.now+=10000;check(!d.flow.animate(d.now) && d.flow.frame().pixels==before,"credits advanced automatically");
  d.key('x');check(d.flow.screen()==(page==3?Screen::Menu:Screen::Credits),"credits ordered four-page return");
 }
 // DOS 0x0bb62..0x0bccd: Escape advances exactly one page, including intermediate
 // pages; only the fourth page returns to the menu.
 const auto credits=services();
 d.fade();d.key('c');
 for(unsigned page=0;page<4;++page) {
  check(d.flow.screen()==Screen::Credits && d.flow.frame().pixels==credits.credits[page].pixels,"Escape credits page identity");
  d.key(27);
 }
 check(d.flow.screen()==Screen::Menu,"credits Escape did not return after the fourth page");d.fade();
 // The right mouse button reaches the same page with the same single-page step.
 d.key('c');
 for(unsigned page=0;page<4;++page) {
  check(d.flow.screen()==Screen::Credits && d.flow.frame().pixels==credits.credits[page].pixels,"right-click credits page identity");
  const auto action=d.flow.inputContext(d.flow.frame().presentation()).dialog->click(0,0);
  check(bool(action),"credits right click mapping");d.flow.handle(*action,d.flow.frame().presentation());if(d.flow.current())d.present();
 }
 check(d.flow.screen()==Screen::Menu,"credits right click did not return after the fourth page");d.fade();
 {
  // DOS 0x0bee7..0x0bf8d: a 13-phase idle loop. Phase 0 restores the background and draws no
  // world0 frame 0; phases 1..12 draw sheet phase/5, local frame phase%5.
  const auto source=services();
  check(source.animation.size()==13 && source.animation[0].pixels!=source.background.pixels,"world0 frame 0 is not distinguishable from the background");
  Driver idle(services());idle.key(27);idle.key(27);check(idle.flow.screen()==Screen::Background,"idle title after menu Escape");
  idle.flow.animate(idle.now);
  int previous=-1;unsigned backgrounds=0;
  for(unsigned step=0;step<26;++step) {
   idle.now+=220;check(bool(idle.flow.animate(idle.now)),"idle title did not tick");idle.present();
   int phase=-1;
   for(unsigned p=0;p<13;++p)if(idle.flow.frame().pixels==(p==0?source.background:source.animation[p]).pixels)phase=int(p);
   check(phase>=0,"idle title frame is neither the background nor an animation phase");
   if(previous>=0)check(phase==(previous+1)%13,"idle title phase order");
   backgrounds+=phase==0;previous=phase;
  }
  check(backgrounds==2,"phase 0 is the plain background once per 13-phase cycle");
 }
 d.key('o');check(d.flow.screen()==Screen::Other,"Other Options");
 check(d.flow.inputContext(d.flow.frame().presentation()).dialog->hits.size()==2,"locked endings exposed buttons");
 d.key('d');check(d.flow.screen()==Screen::Notice,"deferred intro did not report refusal");d.key(27);d.key(27);
 {
  auto introServices=services();introServices.cloudsIntroCheck=[] {return std::string{};};Driver intro(introServices);
  intro.key(27);intro.key('o');intro.key('c');
  check(intro.flow.entry() && intro.flow.entry()->kind==XeenSessionEntry::Kind::CloudsIntro,"Clouds Other Options route");
  intro.flow.returnToTitle();intro.present();intro.fade();
  check(intro.flow.screen()==Screen::Background && !intro.flow.entry(),"intro return resumed Other Options or revealed menu");
  intro.key(32);check(intro.flow.screen()==Screen::Menu,"fresh title did not wait for new key");
  introServices.cloudsIntroCheck=[] {return std::string("Clouds intro unavailable: missing or different WORLD/XEEN.EXE");};Driver missing(introServices);
  missing.key(27);missing.key('o');missing.key('c');check(missing.flow.screen()==Screen::Notice && !missing.flow.entry(),"missing/different EXE notice");
  introServices.cloudsContext=false;unsigned checked=0;introServices.cloudsIntroCheck=[&] {++checked;return std::string{};};Driver noContext(introServices);
  noContext.key(27);noContext.key('o');noContext.key('c');check(!checked && !noContext.flow.entry(),"Clouds route ignored DAT context");
 }
 d.click(start.left,start.top);check(d.flow.screen()==Screen::NewSlots,"New mouse/key parity");
 const auto chooser=d.flow.inputContext(d.flow.frame().presentation()).dialog;
 for(unsigned i=0;i<10;++i)check(chooser->key(i==9?'0':'1'+i).has_value(),"ten numbered slots");
 const auto &up=chooser->hits[0],&bar=chooser->hits[13];
 check(up.button && up.button->pressedFrame()==1 && !bar.button,"DOS arrow/bar feedback mapping");
 const auto unchanged=d.flow.frame().presentation();
 const auto upAction=chooser->click(up.left,up.top),barAction=chooser->click(bar.left,bar.top);
 check(upAction && barAction,"missing chooser arrow/bar click mapping");
 check(!d.flow.handle(*upAction,unchanged) && !d.flow.handle(*barAction,unchanged),"arrow/bar changed selection");
 d.key('6');check(d.flow.screen()==Screen::Notice && !d.flow.entry(),"protected row not explained/disabled");d.key(27);
 d.key('3');check(d.flow.screen()==Screen::NewSlots && !d.flow.entry(),"number key confirmed slot prematurely");
 const auto &row=assets.uiText().buttons("CHOOSER")[4];bool green=false;
 for(unsigned y=row.y;y<row.y+row.height;++y)for(unsigned x=row.x;x<row.x+row.width;++x) {
  const auto p=d.flow.frame().pixels[y*320+x]*3;const auto &palette=d.flow.frame().palette;
  green=green || (palette[p+1]>palette[p] && palette[p+1]>palette[p+2]);
 }
 check(green,"DOS selected slot must be green");
 d.key(13);check(d.flow.screen()==Screen::Name,"empty New slot must precede name");
 d.key(13);check(d.flow.screen()==Screen::Name,"empty name confirmed");
 d.type(std::string(1,char(127)));check(d.flow.screen()==Screen::Name,"invalid code changed name screen");
 d.type("Discarded");d.key(27);d.key('4');d.key(13);d.key(13);
 check(d.flow.screen()==Screen::Name && !d.flow.entry(),"New retained a cancelled name on another slot");
 d.key(27);d.key('3');d.key(13);
 d.type("Case !~ ");d.key(8);d.key(13);check(d.flow.screen()==Screen::Difficulty,"name before difficulty");
 d.key(27);check(d.flow.screen()==Screen::NewSlots,"difficulty cancel must return to slots");
 d.key('1');d.key(13);check(d.flow.screen()==Screen::Overwrite,"occupied New requires overwrite before name");
 d.key('n');check(d.flow.screen()==Screen::NewSlots,"overwrite decline route");d.key(13);d.key('y');
 check(d.flow.screen()==Screen::Name,"overwrite acceptance route");d.key(27);check(d.flow.screen()==Screen::NewSlots,"name cancel route");
 d.key('3');d.key(13);d.type("X");d.key(13);d.key('w');
 check(d.flow.entry() && d.flow.entry()->name=="X" && d.flow.entry()->kind==XeenSessionEntry::Kind::New &&
  d.flow.entry()->slot==2 && d.flow.entry()->difficulty==XeenDifficulty::Warrior,"New entry values");
 d.flow.publicationFailure("Synthetic write failure");d.present();d.key(27);
 check(d.flow.entry()->kind==XeenSessionEntry::Kind::CancelNew,"failed New cancel route");
 Driver load(services());load.key(27);load.key('l');load.key('1');check(!load.flow.entry(),"title number key loaded prematurely");load.key(13);
 check(load.flow.entry() && load.flow.entry()->kind==XeenSessionEntry::Kind::Load &&
  load.flow.entry()->snapshot->name==saved.name,"title Load immutable candidate");
 slots[0].snapshot->name="Changed disk row";check(load.flow.entry()->snapshot->name==saved.name,"Load candidate borrowed mutable chooser row");
 const auto backup=slots[0];slots[0].snapshot->activeRosterIds.clear();
 Driver emptyLevel(services());emptyLevel.key(27);emptyLevel.key('l');
 slots[0].snapshot->activeRosterIds={0};slots[0].snapshot->characters[0].permanentLevel=0;slots[0].snapshot->characters[0].temporaryLevel=0;
 Driver zeroLevel(services());zeroLevel.key(27);zeroLevel.key('l');
 check(emptyLevel.flow.frame().pixels==zeroLevel.flow.frame().pixels,"empty-party slot shows level 0");
 slots[0].snapshot->characters[0].permanentLevel=1;
 Driver oneLevel(services());oneLevel.key(27);oneLevel.key('l');
 check(emptyLevel.flow.frame().pixels!=oneLevel.flow.frame().pixels,"slot level label responds to nonzero membership");slots[0]=backup;
 slots={};Driver empty(services());empty.key(27);empty.key('l');check(empty.flow.screen()==Screen::Notice,"no-saves notice");empty.key(27);
 empty.key('s');empty.key('0');empty.key(13);empty.type(std::string(20,' '));empty.key(13);
 check(empty.flow.screen()==Screen::Name && !empty.flow.entry(),"all-space name was accepted");
 empty.type("  Case");empty.type(" + internal spaces  ");empty.type("ignored");empty.key(13);empty.key('a');
 check(empty.flow.entry()->name=="Case + internal spac" && empty.flow.entry()->slot==9,"leading space rejection/name byte limit/case-preserving round trip");
 const auto retired=empty.flow.frame().presentation();empty.flow.close();
 check(!empty.flow.current() && !empty.flow.acceptsFrame(retired) && !empty.flow.handle(DialogKeyAction{'s'},retired),"closed title callbacks retained authority");
 section("titleFlow");
 }
 cutsceneMouse();
 const auto nativeBegan=std::chrono::steady_clock::now();titleApplicationControls(argv[1]);
 std::cerr<<"titleApplicationControls: "<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-nativeBegan).count()<<" ms\n";
 std::cout<<"Original title, four credits pages, chooser, New/Load, mouse/key/cancel and stale frames passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
