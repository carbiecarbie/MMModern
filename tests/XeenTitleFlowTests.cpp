#include "app/XeenTitleFlow.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "XeenSaveTestSupport.h"
#include <iostream>
using namespace mmodern;
using save_test::check;
void titleApplicationControls(const char *);
void titlePreview(const IndexedFrame &,const std::string &);
namespace {
using Screen=XeenTitleFlow::Screen;
struct Driver {
 XeenTitleFlow flow;std::uint64_t now=1;
 explicit Driver(XeenTitleFlow::Services services):flow(std::move(services)){present();fade();}
 void present(){flow.presented(flow.frame().presentation());flow.completeInput(flow.frame().presentation());}
 void fade(){flow.animate(now);now+=1000;flow.animate(now);present();}
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
  d.click(it->left,it->top);check(d.flow.screen()==Screen::Notice && !d.flow.entry(),"deferred control must refuse");d.key(27);
 }
 d.key('w');check(d.flow.screen()==Screen::Wizard,"Mr Wizard confirmation first");d.key('n');
 d.key('w');d.key('y');check(d.flow.screen()==Screen::Notice && !d.flow.entry(),"rescue must refuse after Yes");d.key(27);
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
 for(unsigned i=0;i<7;++i) {
  check(!input.animate(149+i*50),"cursor advanced before wait tick");
  check(input.animate(150+i*50) && input.cursor()==glyphs[i],"cursor glyph/cadence");
 }
 check(input.animate(700) && input.cursor()==124,"delayed fixed-clock cursor ticks");
 check(input.type("Case !~ ",font),"cursor input");input.keyRedraw();
 check(input.cursor()==126 && input.backspace() && input.value()=="Case !~","typed key redraw cursor phase/input");
 input.keyRedraw();check(input.cursor()==127 && !input.animate(700) && !input.animate(749),"backspace redraw did not restart wait cadence");
 check(input.animate(750) && input.cursor()==126,"first wait tick after key redraw");
}
void animationResume(XeenTitleFlow::Services source) {
 // An off-dialog pixel identifies the exact animation phase independently of
 // fonts, menu text or palette fades, without adding a production test accessor.
 for(unsigned i=0;i<source.animation.size();++i)source.animation[i].pixels[0]=i;
 for(const auto screen:{Screen::NewSlots,Screen::LoadSlots,Screen::Name,Screen::Difficulty,
   Screen::Overwrite,Screen::PublicationFailure,Screen::Credits}) {
  Driver d(source);d.key(27);d.flow.animate(d.now);d.now+=200;d.flow.animate(d.now);d.present();
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
  else d.key(27);
  if(d.flow.screen()==Screen::NewSlots)d.key(27);
  check(d.flow.screen()==Screen::Menu && d.flow.frame().pixels[0]==phase,"modal visit changed retained phase");
  check(!d.flow.animate(d.now),"animation resumed by consuming modal time");
  if(screen==Screen::Credits){d.now+=1000;d.flow.animate(d.now);d.present();}
  d.now+=200;check(bool(d.flow.animate(d.now)),"resumed animation did not tick");d.present();
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
 check(load.flow.screen()==Screen::Credits,"DAT-index Credits mapping");load.key(27);load.fade();load.key('k');load.key('c');load.key(13);
 check(load.flow.entry() && load.flow.entry()->kind==XeenSessionEntry::Kind::Load,"DAT-index title Load mapping");
 Driver p(services(true));
 for(unsigned key:{'a','b','g','h'}){p.key(key);check(p.flow.screen()==Screen::Notice,"DAT-index deferred panel mapping");p.key(27);}
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
 panelControls(assets,font);
 animationResume(services());mappedHotkeys(*installation,assets,font);
 Driver d(services());
 check(d.flow.screen()==Screen::Background,"plain title starts with animated background");d.key(27);
 const auto menu=d.flow.frame().pixels;const auto old=d.flow.frame().presentation();
 d.flow.animate(d.now);d.now+=200;d.flow.animate(d.now);d.present();check(d.flow.frame().pixels!=menu,"title animation phase did not advance");
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
 d.fade();d.key('c');d.key(27);check(d.flow.screen()==Screen::Menu,"credits Escape did not return immediately");d.fade();
 d.key('o');check(d.flow.screen()==Screen::Other,"Other Options");
 check(d.flow.inputContext(d.flow.frame().presentation()).dialog->hits.size()==2,"locked endings exposed buttons");
 d.key('d');check(d.flow.screen()==Screen::Notice,"deferred intro did not report refusal");d.key(27);d.key(27);
 d.click(start.left,start.top);check(d.flow.screen()==Screen::NewSlots,"New mouse/key parity");
 const auto chooser=d.flow.inputContext(d.flow.frame().presentation()).dialog;
 for(unsigned i=0;i<10;++i)check(chooser->key(i==9?'0':'1'+i).has_value(),"ten numbered slots");
 const auto &up=chooser->hits[0],&bar=chooser->hits[13];
 check(up.button && up.button->pressedFrame()==1 && !bar.button,"DOS arrow/bar feedback mapping");
 const auto unchanged=d.flow.frame().presentation();
 check(!d.flow.handle(*chooser->click(up.left,up.top),unchanged) && !d.flow.handle(*chooser->click(bar.left,bar.top),unchanged),"arrow/bar changed selection");
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
 slots={};Driver empty(services());empty.key(27);empty.key('l');check(empty.flow.screen()==Screen::Notice,"no-saves notice");empty.key(27);
 empty.key('s');empty.key('0');empty.key(13);empty.type(std::string(20,' '));empty.key(13);
 check(empty.flow.screen()==Screen::Name && !empty.flow.entry(),"all-space name was accepted");
 empty.type("  Case");empty.type(" + internal spaces  ");empty.type("ignored");empty.key(13);empty.key('a');
 check(empty.flow.entry()->name=="Case + internal spac" && empty.flow.entry()->slot==9,"leading space rejection/name byte limit/case-preserving round trip");
 const auto retired=empty.flow.frame().presentation();empty.flow.close();
 check(!empty.flow.current() && !empty.flow.acceptsFrame(retired) && !empty.flow.handle(DialogKeyAction{'s'},retired),"closed title callbacks retained authority");
 }
 titleApplicationControls(argv[1]);
 std::cout<<"Original title, four credits pages, chooser, New/Load, mouse/key/cancel and stale frames passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
