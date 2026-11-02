#include "games/xeen/XeenCutscene.h"
#include "games/xeen/XeenDialogView.h"
#include <algorithm>
#include <cstdio>

namespace mmodern {
// Original World CD EXE 0xd64e..0xfade; calibrated rules and capture in
// docs/rules/cutscenes.md. Positions/counts describe the program, while all
// art, text, animation tables and random seeds are read from the installation.
XeenCutscene XeenCutscene::darksideEnding(const XeenCutsceneData &data,const XeenCutsceneData::Captions &captions,XeenWallClock clock,std::uint32_t score) {
 using A=Audio;using I=Segment::Input;using R=Segment::Reset;using D=XeenCutsceneData::Darkside;
 auto d=std::make_shared<XeenCutsceneData>(data);
 auto lines=std::make_shared<XeenCutsceneData::Captions>(captions);
 auto random=std::make_shared<XeenPresentationRandom>(std::move(clock),data.ending.randomWords);
 struct State {unsigned cursor=0,phase=0;std::uint64_t captionTick=0;bool captionEnabled=true;std::string caption,strip;};
 auto state=std::make_shared<State>();XeenCutscene scene;scene.timedControlQuit=true;auto observed=scene.observations;
 const bool digital=data.digitalDevice>=2;
 const auto name=[d](const char *s)->const std::string &{return d->darksideEnding.resource(s);};
 const auto sprite=[name](const char *s,unsigned frame=0,int x=0,int y=0,unsigned flags=0)->Sprite{return {name(s),frame,x,y,0,flags};};
 Frame pending;
 const auto add=[&](std::string n,Frame enter,std::function<Frame(const Context &)> compose,unsigned frames=1,unsigned ticks=0)->Segment & {
  if(!pending.empty()){enter.insert(enter.begin(),pending.begin(),pending.end());pending.clear();}
  scene.segments.push_back({});auto &s=scene.segments.back();s.name=std::move(n);s.enter=std::move(enter);s.compose=std::move(compose);
  s.frames=frames;s.ticks=ticks;s.input=frames==1 && !ticks?I::Unpolled:I::Ignore;s.audioBeforeFlip=true;return s;
 };
 const auto fade=[&](std::string n,bool in,Frame enter={}) {
  enter.push_back(Store{"fade"});auto &s=add(std::move(n),std::move(enter),[=](const Context &c)->Frame {
   return {Restore{"fade"},Level{in?c.iteration*4:128-c.iteration*4}};
  },33);s.reset=R::Entry;s.input=I::Unpolled;
 };
 const auto audio=[name](const char *s,XeenSceneArchive archive=XeenSceneArchive::DarksideProgram)->Frame {
  return {Archive{archive},A{A::RetainDigital,name(s)},A{A::Digital,name(s)},Archive{XeenSceneArchive::DarksideProgram}};
 };
 const auto song=[name](const char *s,XeenSceneArchive archive=XeenSceneArchive::DarksideProgram)->Frame {
  return {Archive{archive},A{A::Song,name(s)},Archive{XeenSceneArchive::DarksideProgram}};
 };
 const auto append=[](Frame &f,Frame tail){f.insert(f.end(),tail.begin(),tail.end());};
 const auto select=[=](unsigned record,const Context &c,bool enabled=true) {
  state->caption=lines->record(record,1000);state->cursor=(digital || state->caption.empty())?0:1;state->captionTick=c.tick;state->strip.clear();state->captionEnabled=enabled;
  observed->captionRecords.push_back(record);
 };
 const auto caption=[=](Frame &f,const Context &c) {
  if(state->caption.empty()){state->cursor=0;return;}
  if(digital || !state->captionEnabled)return;
  if(state->cursor && c.tick-state->captionTick>=2) {
   state->captionTick=c.tick;state->cursor=(state->cursor+1)%state->caption.size();
   const auto begin=state->cursor>40?state->cursor-40:0;
   state->strip=xeenDialogFormat(d->captionFormat,{state->caption.substr(begin,state->cursor-begin)});
   observed->captionAdvances.push_back(c.tick);
  }
  observed->captionWidths.push_back(std::min(40u,state->cursor));
  f.push_back(sprite("box.vga",0,36,189));f.push_back(Text{state->strip});
 };
 const auto idle=[=](const Context &c){return !c.digitalBusy && !state->cursor;};
 const auto tail=[&](std::string n,bool voice=true,unsigned minimum=0)->Segment & {
  auto &s=add(std::move(n),{},[=](const Context &c){Frame f;caption(f,c);return f;},0,0);s.input=I::Unpolled;
  s.reset=R::Entry;s.finished=[=](const Context &c){return c.elapsed>=minimum && (!voice || !c.digitalBusy) && !state->cursor;};return s;
 };
 const auto background=[&](const char *stage,const char *raw,bool publish=true) {
  if(publish)add(stage,{Raw{name(raw)},Store{"background"}},[](const Context &)->Frame{return {};});
  else append(pending,{Raw{name(raw)},Store{"background"}});
 };
 const auto split=[&](const char *stage,const char *saved) {
  Frame f{Raw{name("scene4.raw")},Store{"left"},Raw{name("scene4-1.raw")},Store{"right"}};
  if(saved){f.push_back(Raw{name(saved)});f.push_back(Store{"background"});}
  (void)stage;append(pending,std::move(f));
 };
 const auto loop=[&](std::string stage,const std::vector<const char *> &resources,unsigned divisor,unsigned count,unsigned ticks,int x=0,int y=0)->Segment & {
  std::vector<std::string> names;Frame prepare;
  for(auto r:resources){names.push_back(name(r));prepare.push_back(Prepare{name(r)});}
  return add(std::move(stage),std::move(prepare),[=](const Context &c)->Frame {
   return {Restore{"background"},Sprite{names.at(c.iteration/divisor),c.iteration%divisor,x,y}};
  },count,ticks);
 };
 Frame init= song("dngon3.m",XeenSceneArchive::DarksideProgramDark);
 append(init,{Load{name("special.bin")},Prepare{name("box.vga")},Palette{name("endgame.pal")},Raw{name("scene1.raw")},Level{0}});
 add("Darkside scene 1",std::move(init),[](const Context &)->Frame{return {};});
 fade("scene 1 fade in",true);
 auto &firstHold=add("scene 1 hold",{},[](const Context &)->Frame{return {};},1,30);firstHold.reset=R::Entry;firstHold.continuousKeyPoll=true;
 fade("scene 1 fade out",false);
 background("throne background","scene2-b.raw");
 add("throne face",{Prepare{name("face.end")}},[=](const Context &)->Frame{auto s=sprite("face.end",0,29,76);s.bottom=171;return {s};});
 fade("throne fade in",true);
 // The one-tick check at 0xd7b6 shares the reset before the preceding fade.
 auto &rise=add("Alamar rises",{},[=](const Context &c)->Frame {
  const int y=74-2*int(c.iteration);auto s=sprite("face.end",0,29,y);s.bottom=171;
  Frame f{Restore{"background"},s};if(y==60)f.push_back(A{A::FadeSong,{},207});if(y==22)f.push_back(A{A::FadeSong,{},0});return f;
 },27,1);rise.entering=[=](const Context &c){select(21,c);};
 auto &come=tail("come to me",false,27);come.enter=audio("come2.voc",XeenSceneArchive::DarksideProgramDark);
 come.input=I::Ignore;come.continuousKeyPoll=true;
 background("throne room","mainback.raw",false);
 add("throne room hold",{Prepare{name("sc03a.end")}},[=](const Context &)->Frame{return {sprite("sc03a.end",0,250),Store{"background"}};},1,30).continuousKeyPoll=true;
 auto &door=loop("door opens",{"sc03b.end"},6,6,4,72,125);append(door.enter,audio("door.voc"));
 auto &party=loop("party enters",{"sc03c.end"},19,19,4,72,125);
 const auto partyDraw=party.compose;party.compose=[=](const Context &c){auto f=partyDraw(c);if(c.iteration==3 || c.iteration==11)f.push_back(A{A::Fm,{},7});if(c.iteration==7 || c.iteration==16)f.push_back(A{A::Fm,{},8});return f;};
 split("disk background",nullptr);
 auto &flight=add("disk flight",{Prepare{name("disk.end")}},[=](const Context &c) {
  const auto i=c.iteration;const int x=2*i,y=101-int(std::min(i,50u))+int(i>76?i-76:0);
  Frame f{HorizontalMerge{"left","right",unsigned(x)},sprite("disk.end",((i+1)/2)%10,i,y)};
  if(x<44)f.push_back(sprite("disk.end",11,-x,22,0x800));f.push_back(sprite("disk.end",10,0,134));
  if(x%22==0)append(f,audio("whoosh.voc"));return f;
 },160,1);
 (void)flight;
 add("disk flight tail",{},[](const Context &)->Frame{return {};},1,10).input=I::Unpolled;
 auto &thud=add("disk lands",audio("thud.voc"),[](const Context &)->Frame{return {};},0,0);thud.input=I::Unpolled;thud.finished=[](const Context &c){return !c.digitalBusy;};
 // 0xdb81 resets before loading the next RAW, but the current image is
 // retained until the 20-tick hold and fade-out have finished.
 auto &diskHold=add("disk opening hold",{Store{"disk-flight"},Raw{name("sc050001.raw")},Store{"disk-opening"},Restore{"disk-flight"}},[](const Context &)->Frame{return {};},1,20);
 diskHold.reset=R::Entry;diskHold.resetBeforeEnter=true;
 diskHold.continuousKeyPoll=true;
 fade("disk fade out",false);
 add("disk opening background",{Restore{"disk-opening"},Store{"background"}},[](const Context &)->Frame{return {};});fade("disk fade in",true);
 add("disk opening minimum",{},[](const Context &)->Frame{return {};},1,30).continuousKeyPoll=true;
 auto &open=loop("disk opens",{"diska.end","diskb.end","diskc.end","diskd.end","diske.end"},8,34,3,28,11);
 open.afterHold=[=](const Context &c)->Frame {
  if(c.iteration==0 || c.iteration==2 || c.iteration==4 || c.iteration==6)return audio("click.voc");
  if(c.iteration>=9 && c.iteration<=12)return {A{A::Fm,{},30}};
  if(c.iteration==13)return {A{A::Fm,{},14}};if(c.iteration==18)return audio("explosio.voc");return {};
 };
 background("beam background","mainback.raw",false);
 add("beam first",audio("alien2.voc",XeenSceneArchive::DarksideProgramDark),[=](const Context &)->Frame{return {sprite("sc06a.end")};});
 auto &beam=loop("beam",{"sc06a.end","sc06b.end"},10,20,3);
 const auto beamDraw=beam.compose;beam.compose=[=](const Context &c){auto f=beamDraw(c);for(auto i:{6u,8u,9u,10u,13u,15u,16u})if(c.iteration==i)f.insert(f.begin(),A{A::Fm,{},3});return f;};
 fade("beam fade out",false);
 background("Corak background","sc070001.raw");fade("Corak fade in",true);
 add("Corak settle",{},[](const Context &)->Frame{return {};},1,2).input=I::Unpolled;
 auto &arrival=loop("Corak arrives",{"sc07a.end","sc07b.end","sc07c.end","sc07d.end","sc07e.end","sc07f.end","sc07g.end","sc07h.end"},6,45,2,61,12);
 const auto arrivalDraw=arrival.compose;arrival.compose=[=](const Context &c){auto f=arrivalDraw(c);if(c.iteration>40)caption(f,c);return f;};
 arrival.entering=[=](const Context &c){select(22,c,false);};
 arrival.afterHold=[=](const Context &c)->Frame {if(c.iteration==40)state->captionEnabled=true;if(c.iteration==1 || c.iteration==19)return audio("windstor.voc");return {};};
 auto &question=tail("Corak question",false,30);question.enter=audio("corak2.voc");
 question.input=I::Ignore;question.continuousKeyPoll=true;
 auto &prelude=loop("Corak answer prelude",{"sc08.end"},3,2,3);prelude.enter.push_back(A{A::StopDigital});
 prelude.entering=[=](const Context &c){select(23,c);};
 prelude.compose=[=](const Context &c){Frame f{sprite("sc08.end",0)};caption(f,c);return f;};
 add("Corak yes",audio("yes1.voc"),[=](const Context &c){const auto mouth=c.digitalBusy?random->range(0,2):0;observed->endingMouths[0].push_back(mouth);Frame f{sprite("sc08.end",mouth)};caption(f,c);return f;},13,3);
 tail("Corak yes tail",false).compose=[=](const Context &c){Frame f{sprite("sc08.end")};caption(f,c);return f;};
 background("nowhere background","sc090001.raw");
 auto &nowhere=add("nowhere to run",{Prepare{name("sc09.end")}},[=](const Context &c){auto f=Frame{Restore{"background"}};if(c.iteration==2)append(f,audio("nowre1.voc"));const auto mouth=random->range(0,8);observed->endingMouths[1].push_back(mouth);f.push_back(sprite("sc09.end",mouth));caption(f,c);return f;},15,3);
 nowhere.entering=[=](const Context &c){select(24,c);};tail("nowhere tail",false).compose=[=](const Context &c){Frame f{Restore{"background"}};caption(f,c);return f;};
 auto &nor=add("nor do you",{Prepare{name("sc10.end")}},[=](const Context &c){Frame f;if(c.iteration==3)append(f,audio("nordo2.voc"));const auto mouth=random->range(0,1);observed->endingMouths[2].push_back(mouth);f.push_back(sprite("sc10.end",mouth));caption(f,c);return f;},15,3);
 nor.entering=[=](const Context &c){select(25,c);};tail("nor tail",false).compose=[=](const Context &c){Frame f{sprite("sc10.end")};caption(f,c);return f;};
 Frame helmetMusic{A{A::FadeSong,{},0},A{A::Mixer,{},95}};append(helmetMusic,song("dngon2.m",XeenSceneArchive::DarksideProgramDark));
 fade("helmet fade out",false,std::move(helmetMusic));
 background("helmet background","blank.raw",false);
 auto &helmet=loop("helmet closeup",{"sc11.end"},18,15,3,100,44);helmet.fadeAfterFirst=4;helmet.firstFadeOrder=Segment::FirstFadeOrder::AfterPublish;
 helmet.afterHold=[=](const Context &c)->Frame {if(c.iteration==3)return audio("click.voc");if(c.iteration==5)return audio("airmon.voc",XeenSceneArchive::DarksideProgramDark);return {};};
 fade("helmet closeup fade out",false);
 auto &helmetRise=add("helmet rises",{},[=](const Context &c)->Frame{return {Restore{"background"},sprite("sc11.end",15,60,80),sprite("sc11.end",16,61,80-2*int(c.iteration)),sprite("sc11.end",17)};},30,3);helmetRise.fadeAfterFirst=4;helmetRise.firstFadeOrder=Segment::FirstFadeOrder::AfterPublish;
 fade("helmet rise fade out",false);background("Sheltem revealed","scene12.raw");fade("Sheltem fade in",true);
 add("Sheltem hold",{},[](const Context &)->Frame{return {};},1,30).continuousKeyPoll=true;fade("Sheltem fade out",false);
 auto &ready=add("ready",{Prepare{name("sc13.end")},A{A::Mixer,{},48}},[=](const Context &c){Frame f;if(c.iteration==1)append(f,audio("ready2.voc"));const auto mouth=random->range(0,3);observed->endingMouths[3].push_back(mouth);f.push_back(sprite("sc13.end",mouth));caption(f,c);return f;},15,3);
 ready.entering=[=](const Context &c){select(26,c);};ready.fadeAfterFirst=4;ready.firstFadeOrder=Segment::FirstFadeOrder::AfterPublish;
 auto &readyTail=tail("ready tail",false);readyTail.compose=[=](const Context &c){Frame f{sprite("sc13.end")};caption(f,c);return f;};
 fade("fight fade out",false);background("fight background","blank.raw",false);
 add("fight first",{Prepare{name("sc14.end")}},[=](const Context &)->Frame{return {sprite("sc14.end",0,37,37)};});fade("fight fade in",true);
 auto &fight=add("fight challenge",{},[=](const Context &c){Frame f{Restore{"background"}};if(c.iteration==3)append(f,audio("fight2.voc"));f.push_back(sprite("sc14.end",c.iteration%10,37,37));caption(f,c);return f;},20,3);
 fight.entering=[=](const Context &c){select(27,c);};auto &fightTail=tail("fight challenge tail",false);if(digital)fightTail.compose={};
 background("fight room","mainback.raw");
 auto &battle=loop("fight",{"sc15.end"},21,21,3,52,84);
 battle.afterHold=[=](const Context &c)->Frame {if(c.iteration==4)return audio("disint.voc",XeenSceneArchive::DarksideProgramDark);if(c.iteration==12)return audio("explosio.voc");return {};};
 split("force background","sc170001.raw");
 Frame forceVoice{A{A::StopDigital}};append(forceVoice,audio("fail1.voc"));
 auto &force=add("cannot fail",std::move(forceVoice),[=](const Context &c){Frame f{HorizontalMerge{"left","right",0},sprite("sc16.end",c.iteration,7,29)};caption(f,c);return f;},5,4);
 force.entering=[=](const Context &c){select(28,c);};
 auto &forcePan=add("force travels",{},[=](const Context &c){const auto i=c.iteration;Frame f{HorizontalMerge{"left","right",16*i},sprite("sc16.end",std::min(11u,5+i),7-16*int(i),29)};if(i>=5)f.push_back(sprite("sc16.end",12,273,70));caption(f,c);return f;},20,2);
 forcePan.afterHold=[=](const Context &c)->Frame {return c.iteration==3?audio("alien2.voc",XeenSceneArchive::DarksideProgramDark):Frame{};};
 auto &cloak=loop("cloak",{"sc17xa.end","sc17xb.end"},8,16,3,44,19);
 cloak.afterHold=[=](const Context &c)->Frame {return c.iteration==9?audio("thud1.voc",XeenSceneArchive::DarksideProgramDark):Frame{};};
 fade("cloak fade out",false);background("fire background","sc180001.raw");fade("fire fade in",true);
 auto &fire=loop("Sheltem fires",{"sc18a.end","sc18b.end","sc18c.end","sc18d.end"},8,31,3,29,12);
 fire.afterHold=[=](const Context &c)->Frame {return c.iteration==2?audio("gascompr.voc"):Frame{};};
 background("blood background","sc190001.raw");
 auto &blood=loop("Corak bleeding",{"sc19.end"},28,28,3,33,10);const auto bloodDraw=blood.compose;
 blood.compose=[=](const Context &c){auto f=bloodDraw(c);if(c.iteration==20)f.insert(f.begin(),A{A::FadeSong,{},207});return f;};
 fade("blood fade out",false,{A{A::FadeSong,{},0}});background("wave background","blank.raw");Frame waveMusic{A{A::Mixer,{},95}};append(waveMusic,song("sf17.m"));fade("wave fade in",true,std::move(waveMusic));
 auto &wave=add("Corak waves",{Prepare{name("sc20.end")}},[=](const Context &c){Frame f{Restore{"background"}};if(c.iteration%9==0)append(f,audio("cast.voc"));f.push_back(sprite("sc20.end",c.iteration%9,80,18));return f;},18,3);
 (void)wave;background("bolts background","mainback.raw");
 auto &bolts=add("bolts",{Prepare{name("sc21.end")}},[=](const Context &c){const auto i=c.iteration;const auto frame=i<13?i:3+(i-13)%10;Frame f{Restore{"background"}};if(i==0 || (i>=13 && (i-13)%10==0))append(f,audio("alien2.voc",XeenSceneArchive::DarksideProgramDark));f.push_back(sprite("sc21.end",frame,133,81));return f;},43,2);
 (void)bolts;add("Sheltem laughs",audio("laff1.voc",XeenSceneArchive::DarksideProgramDark),[=](const Context &)->Frame{return {Restore{"background"},sprite("sc21.end",0,133,81),Store{"sphere-wait"}};});split("sphere background","sc230001.raw");pending.push_back(Restore{"sphere-wait"});
 auto &sphereTail=add("sphere voice tail",{},[](const Context &)->Frame{return {};},0,0);sphereTail.input=I::Unpolled;sphereTail.finished=[](const Context &c){return !c.digitalBusy;};
 auto &sphere=add("sphere forms",audio("cast.voc"),[=](const Context &c)->Frame{return {HorizontalMerge{"left","right",320},sprite("sc22.end",c.iteration,156,28)};},5,4);(void)sphere;
 add("sphere travels",audio("gascompr.voc"),[=](const Context &c){const auto i=c.iteration;Frame f{HorizontalMerge{"left","right",320-16*i},sprite("sc22.end",std::min(7u,5+i),156+16*i,28,0x800)};if(i>=2)f.push_back(sprite("sc22.end",8,136,64));return f;},21,2);
 auto &shield=loop("shield",{"sc23a.end","sc23b.end"},12,19,3,72,4);append(shield.enter,audio("cast.voc"));
 shield.afterHold=[=](const Context &c)->Frame {if(c.iteration==5)return audio("padspell.voc");if(c.iteration==11)return {A{A::Fm,{},9}};return {};};
 fade("shield fade out",false);background("ricochet background","mainback.raw",false);
 add("ricochet first",{},[=](const Context &)->Frame{return {sprite("sc24a.end",0,148)};});fade("ricochet fade in",true);
 auto &ricochet=loop("ricochet",{"sc24a.end","sc24b.end"},15,30,3,148,0);append(ricochet.enter,audio("cast.voc"));
 ricochet.afterHold=[=](const Context &c)->Frame {if(c.iteration==6)return audio("padspell.voc");if(c.iteration==11 || c.iteration==14)return audio("click.voc");if(c.iteration==17)return audio("thud1.voc",XeenSceneArchive::DarksideProgramDark);return {};};
 background("defeat background","sc250001.raw");
 // 0xf328..0xf3a6 loads the voice and both pages before publishing the
 // initial pose. Reads are preparation, not part of a later speech hold.
 auto &defeatBackground=scene.segments.back();
 append(defeatBackground.enter,{A{A::RetainDigital,name("admit2.voc")},Prepare{name("sc25a.end")},Prepare{name("sc25b.end")}});
 defeatBackground.compose=[=](const Context &)->Frame{return {sprite("sc25a.end",0,27,30)};};
 auto &defeat=add("admit defeat",{},[=](const Context &c){Frame f{Restore{"background"}};if(c.iteration==2)f.push_back(A{A::Digital,name("admit2.voc")});f.push_back(sprite("sc25a.end",c.iteration%9,27,30));caption(f,c);return f;},18,3);
 defeat.entering=[=](const Context &c){select(29,c);};
 auto &defeatTail=tail("admit defeat tail",false);if(digital)defeatTail.compose={};
 background("I do background","sc260001.raw");
 // 0xf462..0xf4f8: retain voice, read all four pages, publish, start voice.
 append(scene.segments.back().enter,{A{A::RetainDigital,name("ido2.voc")},Prepare{name("sc26a.end")},Prepare{name("sc26b.end")},Prepare{name("sc26c.end")},Prepare{name("sc26d.end")}});
 auto &ido=add("I do",{A{A::Digital,name("ido2.voc")}},[=](const Context &c){auto i=c.iteration<32?c.iteration:22+(c.iteration-32)%10;const char *r[]{"sc26a.end","sc26b.end","sc26c.end","sc26d.end"};Frame f{Restore{"background"},sprite(r[i/8],i%8,58,25)};caption(f,c);return f;},0,2);
 ido.entering=[=](const Context &c){select(30,c);};ido.finished=idle;
 background("what background","sc270001.raw");
 append(scene.segments.back().enter,{A{A::RetainDigital,name("what3.voc")},Prepare{name("sc27.end")}});
 auto &what=add("what no",{},[=](const Context &c){Frame f{Restore{"background"}};if(c.iteration==1)f.push_back(A{A::Digital,name("what3.voc")});f.push_back(sprite("sc27.end",c.iteration,65,22));caption(f,c);return f;},12,3);
 what.entering=[=](const Context &c){select(31,c);};
 auto &whatTail=tail("what tail",false);if(digital)whatTail.compose={};
 fade("vortex fade out",false,song("dungeon2.m"));background("vortex room","mainback.raw",false);
 add("vortex first",{},[=](const Context &)->Frame{return {sprite("sca28.end",0,74)};});fade("vortex fade in",true);
 auto &vortex=loop("vortex",{"sca28.end","scb28.end","scc28.end","scd28.end","sce28.end","scf28.end","scg28.end","sch28.end","sci28.end","scj28.end","sck28.end"},4,44,3,74,0);
 vortex.afterHold=[=](const Context &c)->Frame {if(c.iteration==0 || c.iteration==10 || c.iteration==18)return audio("rumble.voc");if(c.iteration==27)return audio("airmon.voc",XeenSceneArchive::DarksideProgramDark);if(c.iteration==38)return audio("explosio.voc");return {};};
 fade("castle fade out",false);background("castle background","sc290001.raw");fade("castle fade in",true);
 auto &castle=loop("castle explodes",{"sc29a.end","sc29b.end","sc29c.end","sc29d.end","sc29e.end","sc29f.end"},10,74,3,92,10);
 castle.compose=[=](const Context &c){const auto i=c.iteration<54?c.iteration:49+(c.iteration-54)%5;const char *r[]{"sc29a.end","sc29b.end","sc29c.end","sc29d.end","sc29e.end","sc29f.end"};Frame f{Restore{"background"},sprite(r[i/10],i%10,92,10)};if(c.iteration==40)f.insert(f.begin(),A{A::FadeSong,{},207});return f;};
 castle.afterHold=[=](const Context &c)->Frame {for(auto i:{1u,5u,11u,19u,34u,35u})if(c.iteration==i)return audio("explosio.voc");if(c.iteration==53)return audio("crash.voc");return {};};
 fade("castle final fade out",false,{A{A::FadeSong,{},0}});
 // Shared Pharaoh pages, EXE 0x6c91. The eight 32-word tables already
 // belong to the installed Darkside data; the score has no random reveal.
 const auto pharaoh=[=]()->Frame {
  const auto p=state->phase;state->phase=(p+1)%32;const auto &t=d->darkside.ball;
  const auto &claw=d->darkside.resources[D::Claw];const auto &dragon=d->darkside.resources[D::Pharaoh];
  return {Restore{"score-room"},Sprite{claw,5,t[2][p],t[3][p],0,0x800},Sprite{claw,6,145,185},
   Sprite{dragon,unsigned(t[1][p]),139,t[0][p],0,0x800},Sprite{claw,p%5,t[4][p],t[5][p],0,0x800}};
 };
 Frame scoreEnter{A{A::Mixer,{},95}};append(scoreEnter,song("outday3.m"));
 append(scoreEnter,{Palette{d->darkside.resources[D::Palette]},Raw{d->darkside.resources[D::Room3]},Store{"score-room"}});
 auto &scoreRoom=add("Darkside score room",std::move(scoreEnter),[=](const Context &)->Frame {
  // Initial composition uses the same tables but fixed claw 6 at (149,184).
  auto f=pharaoh();std::get<Sprite>(f[2]).x=149;std::get<Sprite>(f[2]).y=184;return f;
 });(void)scoreRoom;fade("score fade in",true);
 char digits[11];std::snprintf(digits,sizeof(digits),"%010u",unsigned(score));
 auto page1=d->darksideEnding.scorePages[0];const auto format=page1.find("%010lu");
 if(format==std::string::npos)throw std::runtime_error("Invalid Darkside score format");page1.replace(format,6,digits);
 for(unsigned page=0;page<2;++page) {
  const auto text=page?d->darksideEnding.scorePages[1]:page1;
  auto &s=add("Darkside score page "+std::to_string(page+1),{},[=](const Context &){auto f=pharaoh();
   for(const auto &format:d->darksideEnding.scoreFormats)f.push_back(Text{xeenDialogFormat(format,{text}),12,8,12,8,162,198});return f;
  },0,3);s.input=I::Keyboard;s.pollAfterHold=true;s.keyboardGroup=2;
 }
 // 0xd696 disables the shared scroll, just like Darkside startup. The
 // final call at 0xfad3 returns through 0x6f28..0x6f35 without drawing.
 add("Darkside ending shutdown",{A{A::StopDigital},A{A::FadeSong,{},0}},[](const Context &)->Frame{return {Level{0}};});
 scene.closing=scene.segments.size();return scene;
}
}
