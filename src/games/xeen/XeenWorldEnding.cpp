#include "games/xeen/XeenCutscene.h"
#include "games/xeen/XeenDialogView.h"

namespace mmodern {
// Original World CD EXE 0xfbc9..0x11864; calibrated rules and capture in
// docs/rules/cutscenes.md#world-ending-presentation. Positions, counts and
// waits describe the program; names and texts are read from the installed EXE.
// There are no voices, caption records or random draws in this ending.
XeenCutscene XeenCutscene::worldEnding(const XeenCutsceneData &data,std::uint32_t score) {
 using A=Audio;using I=Segment::Input;using R=Segment::Reset;
 auto d=std::make_shared<const XeenCutsceneData>(data);
 const auto &w=d->worldEnding;
 XeenCutscene scene;scene.timedControlQuit=true;
 const auto name=[d](const char *s)->const std::string &{return d->worldEnding.resource(s);};
 const auto sprite=[name](const char *s,unsigned frame,int x,int y,unsigned flags=0)->Sprite{return {name(s),frame,x,y,0,flags};};
 // Flag 0x4000 draws keep rows 17..bottom (0x1509c..0x150ba); the bottom
 // word is 185 from 0xfc51/0x107fe and 118 from 0x108f0.
 const auto clipped=[sprite](const char *s,unsigned frame,int x,int y,unsigned bottom) {auto v=sprite(s,frame,x,y);v.top=17;v.bottom=bottom;return v;};
 const auto add=[&](std::string n,Frame enter,std::function<Frame(const Context &)> compose,unsigned frames=1,unsigned ticks=0)->Segment & {
  scene.segments.push_back({});auto &s=scene.segments.back();s.name=std::move(n);s.enter=std::move(enter);s.compose=std::move(compose);
  s.frames=frames;s.ticks=ticks;s.input=frames==1 && !ticks?I::Unpolled:I::Ignore;s.audioBeforeFlip=true;return s;
 };
 const auto none=[](const Context &)->Frame{return {};};
 // Palette fades only change the DAC, so they show `slot`: by default the
 // back buffer at entry, or an image stored before a later load.
 const auto fade=[&](std::string n,bool in,Frame enter={},const char *slot=nullptr) {
  if(!slot){enter.push_back(Store{"fade"});slot="fade";}
  const std::string shown=slot;
  auto &s=add(std::move(n),std::move(enter),[=](const Context &c)->Frame{return {Restore{shown},Level{in?c.iteration*4:128-c.iteration*4}};},33);
  s.reset=R::Entry;s.input=I::Unpolled;
 };
 // Fade step 0x81 (0x14a97..0x14aa3) writes the full palette once, still
 // showing the image displayed before the new background was loaded.
 const auto instant=[&](std::string n,Frame enter) {add(std::move(n),std::move(enter),[](const Context &)->Frame{return {Restore{"visible"},Level{128}};});};
 const auto hold=[&](std::string n,unsigned ticks) {add(std::move(n),{},none,1,ticks).continuousKeyPoll=true;};
 const auto audio=[name](const char *s,XeenSceneArchive archive=XeenSceneArchive::DarksideProgram)->Frame {
  return {Archive{archive},A{A::RetainDigital,name(s)},A{A::Digital,name(s)},Archive{XeenSceneArchive::DarksideProgram}};
 };
 const auto song=[name](const char *s)->Frame{return {A{A::Song,name(s)}};};
 const auto append=[](Frame &f,Frame tail){f.insert(f.end(),tail.begin(),tail.end());};
 const auto prepare=[name](std::initializer_list<const char *> names){Frame f;for(auto n:names)f.push_back(Prepare{name(n)});return f;};
 // Captions 0xfb54 (colors 38/39) and 0xfadf (5/11): shadow in window 28
 // (2,157), then text at (1,156), 318x61/60, inside an 8-pixel margin
 // (EXE window table cs:0x1ea, setter 0x1d51f); the screen ends at 200.
 const auto caption=[d](Frame &f,unsigned index,bool last=false) {
  const auto &text=d->worldEnding.captions.at(index);const auto &formats=d->worldEnding.formats;const unsigned first=last?0:2;
  f.push_back(Text{xeenDialogFormat(formats[first],{text}),10,165,10,165,312,200});
  f.push_back(Text{xeenDialogFormat(formats[first+1],{text}),9,164,9,164,311,200});
 };
 const auto songWait=[&](std::string n) {
  auto &s=add(std::move(n),{},none,0,0);s.input=I::Unpolled;s.songFadeWait=true;
  s.finished=[](const Context &c){return !c.songFading;};
 };

 // 0xfbc9..0xfc45: the first elect.voc load only reserves the shared sound
 // buffer, as ido2.voc does in the Darkside ending; nothing plays it.
 // The DAT title faded out before the EXE started, so the room starts dark.
 Frame opening{Level{0}};append(opening,song("outday3.m"));
 pharaohPages(scene,d,std::move(opening),"World opening",{w.opening});
 // 0xfc48..0xfd0c: song fade 223 and every load precede the fade-out of the
 // displayed page, which already uses the new sky palette.
 Frame sky{Store{"visible"},A{A::FadeSong,{},223},Raw{name("skymain.raw")},Store{"background"},Raw{name("twrsky1.raw")},
  Store{"page0"},Store{"page1"},Palette{name("skymain.pal")}};
 append(sky,prepare({"sc02.eg2","tower1.eg2","tower2.eg2","sc3a.eg2","sc3b1.eg2","sc3b2.eg2"}));
 fade("opening page fade out",false,std::move(sky),"visible");
 songWait("opening song fade");
 // 0xfd4f draws the caption before the restore at 0xfd59: invisible, but it
 // leaves the EXE text state centered for the visible draw at 0xfd64.
 add("sky",song("nwblksmt.m"),[=](const Context &){Frame f;caption(f,0);f.push_back(Restore{"background"});caption(f,0);return f;});
 fade("sky fade in",true);hold("sky hold",60);
 add("dragon approaches",{},[=](const Context &c) {
  Frame f;if(c.iteration==9 || c.iteration==22 || c.iteration==33 || c.iteration==44)f=audio("whoosh.voc");
  f.push_back(Restore{"background"});f.push_back(sprite("sc02.eg2",c.iteration,0,0));caption(f,0);return f;
 },50,2);
 add("dragon lands",{},[=](const Context &c)->Frame {
  return {HorizontalMerge{"page0","page1",0},sprite("tower1.eg2",0,0,0,0x800),sprite("sc3a.eg2",c.iteration,91,86,0x800)};
 },40,2);
 // 0xfe68..0xff7d: x advances by 5 with one tick per frame; the merge
 // offset steps from 0 to 320, then down by two.
 const auto merge=[](unsigned i){return i?322-2*i:0u;};
 add("crowd scroll",{},[=](const Context &c) {
  const auto i=c.iteration;const int x=int(5*i);const auto people=i>33?i-33:0;
  Frame f{HorizontalMerge{"page0","page1",merge(i)},sprite("tower1.eg2",0,x,0,0x800),sprite("sc3a.eg2",(40+i)%57,x+91,86,0x800),
   sprite("tower2.eg2",0,x-320,0,0x800),sprite("tower2.eg2",1,x-160,0,0x800),
   sprite(people/30?"sc3b2.eg2":"sc3b1.eg2",people%30,x-277,65,0x800)};
  if(x>120)caption(f,1);return f;
 },64,1);
 // 0xff80..0x10012: the remaining crowd frames keep the last merge offset.
 add("crowd gathers",{},[=](const Context &c) {
  const auto people=31+c.iteration;
  Frame f{HorizontalMerge{"page0","page1",merge(64)},sprite("tower2.eg2",0,0,0,0x800),sprite("tower2.eg2",1,160,0,0x800),
   sprite(people/30?"sc3b2.eg2":"sc3b1.eg2",people%30,43,65,0x800)};
  caption(f,1);return f;
 },29,2);
 // 0x10015..0x102b5: CD animations, absent from the reference.
 Frame cd1{Store{"visible"},Palette{name("cd10001.pal")}};
 for(unsigned i=0;i<4;++i)cd1.push_back(Prepare{w.cd(1,i)});
 append(cd1,{Raw{name("cd10001.raw")},Store{"background"}});
 instant("CD 1 palette",std::move(cd1));
 add("CD 1 background",{},[](const Context &)->Frame{return {Restore{"background"}};});
 add("CD 1",{},[=](const Context &c)->Frame{return {Restore{"background"},Sprite{d->worldEnding.cd(1,c.iteration/16),c.iteration%16}};},59,2);
 Frame cd2{Store{"visible"},Palette{name("cd2orig.pal")}};
 for(unsigned i=0;i<35;++i)cd2.push_back(Prepare{w.cd(2,i)});
 append(cd2,{Raw{name("cd2orig.raw")},Store{"background"}});
 instant("CD 2 palette",std::move(cd2));
 add("CD 2",{},[=](const Context &c)->Frame{return {Restore{"background"},Sprite{d->worldEnding.cd(2,c.iteration/8),c.iteration%8}};},273,2);
 instant("CD 3 palette",{Store{"visible"},Prepare{name("cd3i2.eg2")},Palette{name("cd30001.pal")},Raw{name("cd30001.raw")},Store{"cd3"}});
 // 0x1022f..0x102b0: each fourth frame loads the next member, then draws
 // over the previous frames without restoring the background.
 add("CD 3",{},[=](const Context &c) {
  Frame f;if(!c.iteration)f.push_back(Restore{"cd3"});
  const auto member=d->worldEnding.cd(3,c.iteration/4);if(c.iteration%4==0)f.push_back(Prepare{member});
  f.push_back(Sprite{member,c.iteration%4,101,0});return f;
 },268,2);
 fade("CD 3 fade out",false);
 // 0x102be..0x10445: the ceremony room fills.
 Frame crowd{Palette{name("skymain.pal")}};
 append(crowd,prepare({"sc06.eg2","sc14.eg2","sc13.eg2","sc17.eg2","cube.eg2","hands.eg2","sc20a.eg2","sc20b.eg2","sc20c.eg2","sc20d.eg2","sc22a.eg2","sc22b.eg2","sc10.eg2","staff.eg2"}));
 append(crowd,{Raw{name("foura.raw")},Store{"background"}});
 add("crowd room",std::move(crowd),[=](const Context &)->Frame{return {sprite("sc06.eg2",0,26,75)};});
 fade("crowd fade in",true);
 add("crowd sits",{},[=](const Context &c)->Frame{return {Restore{"background"},sprite("sc06.eg2",c.iteration,26,75)};},26,3);
 fade("crowd fade out",false);
 // Each Pharaoh stage publishes its background, then fades it in; the
 // caption drawn after that publication first appears with frame 0.
 const auto pharaoh=[&](const char *stage,bool withCaption) {
  add(std::string(stage)+" background",{Raw{name("eg140001.raw")},Store{"background"}},none);
  fade(std::string(stage)+" fade in",true);
  add(stage,{},[=](const Context &c){Frame f{Restore{"background"},sprite("sc14.eg2",c.iteration%15,141,63)};if(withCaption)caption(f,2);return f;},30,3);
 };
 pharaoh("Dragon Pharaoh",true);
 add("Queen Kalindra",{Raw{name("eg100001.raw")},Store{"page0"},Store{"background"}},[=](const Context &c) {
  Frame f{Restore{"background"},sprite("sc13.eg2",c.iteration%6,26,21)};caption(f,3);return f;
 },18,2);
 add("Queen lifts the Cube",{},[=](const Context &) {Frame f{Restore{"background"},sprite("sc13.eg2",5,26,21),Store{"background"}};caption(f,3);return f;});
 // 0x105a0..0x1063a and 0x1063a..0x106c2: all 17 sc17 frames, cast
 // replayed whenever the sound is idle.
 const auto cube=[=](const Context &c,int y,unsigned frame) {
  Frame f;if(!c.digitalBusy)f=audio("cast.voc");
  append(f,{Restore{"background"},clipped("sc17.eg2",0,33,y,185),clipped("sc17.eg2",frame,33,y,185)});caption(f,3);return f;
 };
 add("Cube rises",{},[=](const Context &c){return cube(c,185-3*int(c.iteration),c.iteration%17);},39,2);
 add("Cube shines",{},[=](const Context &c){return cube(c,68,c.iteration);},17,2);
 add("Cube raised",{},[=](const Context &) {Frame f{Restore{"background"},clipped("sc17.eg2",0,33,68,185)};caption(f,3);return f;});
 fade("Cube fade out",false);
 pharaoh("Pharaoh watches",false);
 add("Prince Roland",{HorizontalMerge{"page0","page1",0},Store{"background"}},[=](const Context &c) {
  Frame f{Restore{"background"},sprite("sc10.eg2",c.iteration%5,26,21)};caption(f,4);return f;
 },15,2);
 add("Roland raises the Sceptre",{},[=](const Context &) {Frame f{Restore{"background"}};caption(f,4);return f;});
 add("Sceptre rises",{},[=](const Context &c) {
  Frame f{Restore{"background"},clipped("staff.eg2",0,196,185-6*int(c.iteration),185)};caption(f,4);return f;
 },29,2);
 hold("Sceptre hold",30);fade("Sceptre fade out",false);
 pharaoh("Pharaoh presides",false);
 // 0x108f0..0x10d30: the table; cube bottom clip 118.
 const auto table=[=](unsigned hands,unsigned frame,int y,bool showHands) {
  Frame f{Restore{"background"},sprite("sc20a.eg2",0,26,55)};if(showHands)f.push_back(sprite("hands.eg2",hands,58,17));
  f.push_back(clipped("cube.eg2",frame,101,y,118));return f;
 };
 auto &tableFirst=add("table",{Raw{name("tablmain.raw")},Store{"background"},Store{"page1"}},[=](const Context &)->Frame {
  Frame f{sprite("sc20a.eg2",0,26,55),sprite("hands.eg2",0,58,17),clipped("cube.eg2",0,101,11,118)};return f;
 });
 tableFirst.afterHold=[=](const Context &)->Frame{return audio("cast.voc");};
 const auto cast=[=](const Context &c){return c.digitalBusy?Frame{}:audio("cast.voc");};
 add("Cube placed",{},[=](const Context &c){auto f=cast(c);append(f,table(0,c.iteration,11,true));caption(f,5);return f;},5,2);
 add("hands turn",{},[=](const Context &c){auto f=cast(c);append(f,table(c.iteration,c.iteration%5,11,true));caption(f,5);return f;},16,2);
 add("Cube lowers",{},[=](const Context &c){auto f=cast(c);append(f,table(0,c.iteration%5,11+int(c.iteration),false));caption(f,5);return f;},71,2);
 // 0x10b71..0x10bb0: stop, then click and padspell started back to back.
 Frame activate{A{A::StopDigital}};append(activate,audio("click.voc"));append(activate,audio("padspell.voc"));
 add("table activates",std::move(activate),[=](const Context &c) {
  Frame f;if(c.iteration==10)f=audio("padspell.voc");
  f.push_back(Restore{"background"});f.push_back(sprite(c.iteration/7?"sc20b.eg2":"sc20a.eg2",c.iteration%7,26,55));caption(f,5);return f;
 },14,2);
 add("table glows",{},[=](const Context &c) {
  Frame f;if(c.iteration==10)f=audio("padspell.voc");
  f.push_back(Restore{"background"});f.push_back(sprite("sc20c.eg2",c.iteration,26,55));caption(f,5);return f;
 },17,3);
 // 0x10c95: one more padspell before the last six frames.
 add("table closes",audio("padspell.voc"),[=](const Context &c){Frame f{Restore{"background"},sprite("sc20d.eg2",c.iteration,26,55)};caption(f,5);return f;},6,4);
 add("table rests",{},[=](const Context &){Frame f{Restore{"background"}};caption(f,5);return f;});
 hold("table hold",30);
 add("Sceptre placed",{HorizontalMerge{"page0","page1",320},Store{"background"}},[=](const Context &c) {
  Frame f;const auto i=c.iteration;
  if(i==3)f=audio("click.voc");if(i==5 || i==15)f=audio("elect.voc",XeenSceneArchive::DarksideProgramDark);if(i==22)f=audio("explosio.voc");
  f.push_back(Restore{"background"});f.push_back(sprite(i/20?"sc22b.eg2":"sc22a.eg2",i%20,112,17));caption(f,6);return f;
 },35,3);
 Frame light=prepare({"sc23a.eg2","sc23b.eg2","sc23c.eg2","sc23d.eg2","sc23e.eg2","sc23f.eg2","sc23g.eg2","sc23h.eg2","sc24.eg2"});
 fade("altar fade out",false,std::move(light));
 // 0x10f05..0x10fc5: the light, drawn without restoring the background.
 add("light background",{Raw{name("eg23prt2.raw")},Store{"background"}},[=](const Context &)->Frame{return {sprite("sc23a.eg2",0,0,0)};});
 fade("light fade in",true);
 add("light",{},[=](const Context &c) {
  const auto i=c.iteration;Frame f;if(i==2 || i==15 || i==25 || i==33 || i==41)f=audio("gascompr.voc");
  const char *pages[]{"sc23a.eg2","sc23b.eg2","sc23c.eg2","sc23d.eg2","sc23e.eg2","sc23f.eg2","sc23g.eg2","sc23h.eg2"};
  f.push_back(sprite(pages[i/8],i%8,0,0));return f;
 },61,4);
 add("beam background",{Raw{name("skymain.raw")},Store{"background"}},[=](const Context &)->Frame{return {sprite("sc24.eg2",0,103,6)};});
 add("beam",{},[=](const Context &c) {
  const auto i=c.iteration;Frame f;if(i==2 || i==15 || i==21)f=audio("photon.voc",XeenSceneArchive::DarksideProgramDark);
  f.push_back(Restore{"background"});f.push_back(sprite("sc24.eg2",i,103,6));return f;
 },35,3);
 add("beam fades",{},[=](const Context &c)->Frame{return {Restore{"background"},sprite("sc24.eg2",20+c.iteration,103,6)};},15,3);
 fade("beam fade out",false,prepare({"sc25.eg2","sc261a.eg2","sc261b.eg2","sc262.eg2","sc263.eg2","sc264.eg2","sc27.eg2"}));
 add("power lines background",{Raw{name("eg250001.raw")},Palette{name("eg250001.pal")}},none);
 fade("power lines fade in",true);
 add("power lines",audio("comet.voc"),[=](const Context &c) {
  Frame f;if(c.iteration==28)f=audio("click.voc");else if(!c.digitalBusy)f=audio("comet.voc");
  f.push_back(sprite("sc25.eg2",c.iteration,0,0));return f;
 },52,2);
 // 0x1121b..0x112e3: no fade-out. Step 0x81 follows the publication within
 // the same retrace (the capture shows no old-palette frame), unlike the CD
 // scenes, which write the palette before publishing.
 add("collectors background",{A{A::StopDigital},Raw{name("blank.raw")},Palette{name("skymain.pal")}},[=](const Context &)->Frame {
  return {sprite("sc261a.eg2",0,7,4),sprite("sc262.eg2",0,86,4),sprite("sc263.eg2",0,164,4),sprite("sc264.eg2",0,242,4)};
 });
 add("collectors",{},[=](const Context &c) {
  // 0x112ec..0x11450: the four counters and their EXE clamps.
  unsigned left=0,a=0,b=0,e=0;
  for(unsigned i=0;i<c.iteration;++i) {
   left=(left+1)%28;a=(a+1)%9;b=(b+1)%19;e=(e+1)%10;
   if(i>12 && left<13)left=13;
   if(i<23)a=0;else if(i>26 && a<5)a=5;
   if(i<34)b=0;else if(i>43 && b<12)b=12;
   if(i<53)e=0;else if(i>57 && e<5)e=5;
  }
  Frame f{sprite(left/14?"sc261b.eg2":"sc261a.eg2",left%14,7,4),sprite("sc262.eg2",a,86,4),sprite("sc263.eg2",b,164,4),sprite("sc264.eg2",e,242,4)};
  const auto i=c.iteration;if(i==10 || i==28 || i==43 || i==56)append(f,audio("photon.voc",XeenSceneArchive::DarksideProgramDark));
  return f;
 },78,2);
 // Final helper 0x11468..0x11864.
 fade("collectors fade out",false);
 add("arcs background",{Raw{name("eg270001.raw")},Palette{name("eg250001.pal")}},none);
 fade("arcs fade in",true);
 add("arcs",{},[=](const Context &c) {
  Frame f;if(!c.digitalBusy)f=audio("comet.voc");if(c.iteration==19 || c.iteration==60)append(f,audio("click.voc"));
  f.push_back(sprite("sc27.eg2",c.iteration,0,0));return f;
 },89,2);
 fade("arcs fade out",false,{A{A::StopDigital}});
 Frame globe=prepare({"sc28a.eg2","sc28b1.eg2","sc28c.eg2","sc28d.eg2","sc28e.eg2","sc28f.eg2","sc28g.eg2","sc28h.eg2","sc28i.eg2","sc28j.eg2","sc28k.eg2","sc28l.eg2","sc28m.eg2","sc28n.eg2","sc30.eg2"});
 append(globe,{Raw{name("eg280001.raw")},Store{"background"}});
 add("globe background",std::move(globe),none);
 fade("globe fade in",true);
 add("globe",{},[=](const Context &c) {
  const char *pages[]{"sc28a.eg2","sc28b1.eg2","sc28c.eg2","sc28d.eg2","sc28e.eg2","sc28f.eg2","sc28g.eg2","sc28h.eg2","sc28i.eg2","sc28j.eg2","sc28k.eg2","sc28l.eg2","sc28m.eg2","sc28n.eg2"};
  Frame f;if(!c.digitalBusy && c.iteration>98)f=audio("rumble.voc");
  f.push_back(sprite(pages[c.iteration/10],c.iteration%10,52,15));return f;
 },138,2);
 // 0x11702..0x1175d: white flash, ten unpolled ticks, then the globe fades
 // out in its own palette.
 fade("flash",true,{A{A::StopDigital},Palette{name("white.pal")}});
 auto &flash=add("flash hold",audio("explosio.voc"),none,1,10);flash.input=I::Unpolled;
 fade("flash fade out",false,{Palette{name("eg250001.pal")}});
 auto &unite=add("Xeen united",{},[=](const Context &c) {
  Frame f;if(c.iteration/4==17)f.push_back(A{A::FadeSong,{},207});
  f.push_back(sprite("sc30.eg2",c.iteration%4,0,0));caption(f,7,true);return f;
 },80,2);
 unite.fadeAfterFirst=4;unite.firstFadeOrder=Segment::FirstFadeOrder::AfterPublish;
 fade("united fade out",false);
 songWait("final song fade");
 // 0x117e9..0x11854: the shared Darkside first-page format; the bonus page
 // exists only for the Goober states, which no DOS path sends.
 pharaohPages(scene,d,song("outday3.m"),"World score",{endingScore(d->darksideEnding.scorePages[0],score)});
 // 0x11458: 0x6f22(1,0) returns without a parchment, as Darkside's 0xfad3.
 add("World ending shutdown",{A{A::StopDigital},A{A::FadeSong,{},0}},[](const Context &)->Frame{return {Level{0}};});
 scene.closing=scene.segments.size();return scene;
}
}
