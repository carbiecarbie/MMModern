#include "games/xeen/XeenCutscene.h"
#include "games/xeen/XeenDialogView.h"
#include <algorithm>
#include <limits>
namespace mmodern {
unsigned XeenCutscene::stride(std::uint64_t measured) {return unsigned(std::min<std::uint64_t>(std::numeric_limits<unsigned>::max(),std::max<std::uint64_t>(1,measured)));}
XeenCutscene XeenCutscene::clouds(const XeenCutsceneData &data,std::string caption) {
 using R=XeenCutsceneData;using Reset=Segment::Reset;using Input=Segment::Input;
 XeenCutscene result;
 struct State {
  int offset=0,remaining=200,burst=-1,descent=0;unsigned flight=0;
  unsigned king=0,xeen=0,pet=0,peasants=0,total=0,mouth=0,lookup=0,cursor=1;
  std::optional<std::uint64_t> captionTick;
  std::string captionText;
 };
 auto state=std::make_shared<State>();auto d=std::make_shared<XeenCutsceneData>(data);
 auto story=std::make_shared<std::string>(std::move(caption));auto observed=result.observations;
 const auto resource=[d](R::Resource r){return d->resources[r];};
 const auto sprite=[resource](R::Resource r,unsigned f=0,int x=0,int y=0,int scale=0,unsigned flags=0) {
  return Sprite{resource(r),f,x,y,scale,flags};
 };
 const auto add=[&](std::string name,Frame enter,std::function<Frame(const Context &)> compose,unsigned frames=1,unsigned ticks=0,Reset reset=Reset::BeforeCompose,Input input=Input::Escape)->Segment & {
  result.segments.push_back({});auto &s=result.segments.back();s.name=std::move(name);s.enter=std::move(enter);
  s.compose=std::move(compose);s.frames=frames;s.ticks=ticks;s.reset=reset;s.input=input;return s;
 };
 const auto fade=[&](std::string name,unsigned step,bool in,Frame enter=Frame{}) {
  // EXE 0x14a69..0x14adb: endpoints included, one write per retrace.
  enter.push_back(Store{"fade"});
  add(std::move(name),std::move(enter),[step,in](const Context &c)->Frame {
   return {Restore{"fade"},Level{in?c.iteration*step:128-c.iteration*step}};
  },128/step+1,0,Reset::Entry,Input::Ignore);
 };
 add("logo background",{Palette{resource(R::LogoPalette)},Raw{resource(R::LogoBack)},Store{"logo"}},
  [](const Context &)->Frame{return {Restore{"logo"},Level{0}};});
 fade("logo fade in",128,true,{Restore{"logo"}});
 add("NWC logo",{Prepare{resource(R::Logo)},Prepare{resource(R::LogoTail)},
  Audio{Audio::RetainDigital,resource(R::Fire)},Audio{Audio::RetainDigital,resource(R::Growl)},Audio{Audio::Fm,{},1}},
  [=](const Context &c)->Frame {
   Frame f{Restore{"logo"},sprite(c.iteration<65?R::Logo:R::LogoTail,c.iteration%65)};
   // EXE 0x7da7..0x7e6b: cues follow publication and the scene poll.
   switch(c.iteration) {
    case 37:f.push_back(Audio{Audio::Fm,{},0});break;
    case 52:case 60:f.push_back(Audio{Audio::Fm,{},3});break;
    case 53:f.push_back(Audio{Audio::Digital,resource(R::Fire)});break;
    case 64:f.push_back(Audio{Audio::Fm,{},2});break;
    case 66:f.push_back(Audio{Audio::Fm,{},52});f.push_back(Audio{Audio::Digital,resource(R::Growl)});break;
   }return f;
  },80,2);
 auto &logoHold=add("logo final hold",{},[](const Context &)->Frame{return {Restore{"logo"}};},1,30,Reset::AfterFlip);
 logoHold.pollAfterHold=true; // 0x7e84..0x7e96: no inner key poll.
 fade("JVC fade out",8,false,{Audio{Audio::Song,resource(R::Song)},Raw{resource(R::Jvc)},Store{"jvc"},Restore{"logo"}});
 add("JVC publication",{Palette{resource(R::JvcPalette)}},[](const Context &)->Frame{return {Restore{"jvc"},Level{0}};});
 fade("JVC fade in",4,true);
 Frame panLoads;
 for(auto r:{R::Stars,R::Title,R::Lake,R::Xeen,R::Tower,R::Wizard,R::LakeFlight,R::LakeDescent,R::Burst})panLoads.push_back(Prepare{resource(r)});
 auto &jvc=add("JVC hold",std::move(panLoads),[](const Context &)->Frame{return {Restore{"jvc"}};},1,60,Reset::Entry);
 jvc.resetBeforeEnter=true; // 0x7eff: loading consumes the splash hold.
 fade("JVC fade out to stars",8,false);
 auto &stars=add("stars descent",{Palette{resource(R::ScenePalette)},Raw{resource(R::Blank)},Store{"blank"},
  sprite(R::Stars,0),sprite(R::Stars,1,160),Store{"page0"},Restore{"blank"},sprite(R::Title),Store{"page1"}},
  [=](const Context &)->Frame {
   observed->starsOffsets.push_back(unsigned(state->offset));
   Frame f{Merge{"page0","page1",unsigned(state->offset)}};
   if(state->remaining<160)f.push_back(sprite(R::Xeen));
   if(state->remaining<100 && state->burst<14) {
    f.push_back(sprite(R::Xeen));if(++state->burst<14)f.push_back(sprite(R::Burst,unsigned(state->burst)));
   }return f;
  },0,1);
 stars.fadeAfterFirst=4;
 stars.published=[=](const Context &c) {
  // 0x7fc1..0x80d9: measure before the first fade, then latch, never tune.
  if(c.iteration==0)observed->panStride=stride(c.elapsed);
  state->offset+=int(observed->panStride);state->remaining-=int(observed->panStride);
 };
 stars.finished=[=](const Context &){return state->remaining<=0;};
 auto &lake=add("lake descent",{Restore{"blank"},sprite(R::Title),Store{"page0"},Restore{"blank"},sprite(R::Lake),Store{"page1"}},
  [=](const Context &c)->Frame {
   if(c.iteration==0){state->offset=0;state->remaining=200;state->descent=0;}
   state->remaining-=int(observed->panStride);state->offset=std::min(200,state->offset+int(observed->panStride));
   observed->lakeOffsets.push_back(unsigned(state->offset));Frame f{Merge{"page0","page1",unsigned(state->offset)}};
   if(state->remaining<95 && state->descent<44 && ++state->descent<44)
    f.push_back(sprite(R::LakeDescent,unsigned(state->descent),0,state->remaining,0,0x800));
   f.push_back(sprite(R::Xeen));return f;
  },0,1);
 lake.finished=[=](const Context &){return state->remaining<=0;};
 const auto flight=[=](Frame f) {
  f.push_back(sprite(R::LakeFlight,state->flight,0,state->remaining,0,0x800));state->flight=(state->flight+1)%43;return f;
 };
 auto &flying=add("lake flight",{Restore{"blank"},sprite(R::Lake,0,0,0,0,0x800),Store{"lake"}},
  [=](const Context &)->Frame {return flight({Restore{"lake"}});},0,0,Reset::Entry);
 flying.totalTicks=100; // 0x8244: one reset for the complete retrace-limited loop.
 for(int scale=15;scale>=0;--scale) {
  auto &tower=add("tower scale "+std::to_string(scale),{},[=](const Context &)->Frame {
   auto f=flight({Restore{"lake"}});f.push_back(sprite(R::Tower,0,d->towerLeft[scale],d->towerY[scale],scale));
   f.push_back(sprite(R::Tower,1,d->towerRight[scale],d->towerY[scale],scale));return f;
  },0,0,Reset::Entry);tower.totalTicks=1;
 }
 add("wizard",{Restore{"blank"},sprite(R::Tower,0),sprite(R::Tower,1,160),Store{"tower"}},
  [=](const Context &c)->Frame{return {Restore{"tower"},sprite(R::Wizard,c.iteration)};},39,2,Reset::AfterFlip); // 0x838e..0x840f
 fade("tableau fade out",4,false);
 Frame tableau{Restore{"blank"},sprite(R::GroupBack,0),sprite(R::GroupBack,1,160),sprite(R::Crodo,0,0,-5),Text{d->labels},Store{"tableau"},Store{"scroll-base"}};
 for(unsigned i=0;i<16;++i)tableau.push_back(Prepare{d->hand(i)});
 for(unsigned i=1;i<=4;++i)tableau.push_back(Prepare{d->marble(i)});
 auto &opening=add("opening scroll",std::move(tableau),[](const Context &c)->Frame{return {Scroll{c.iteration+1}};},22,1,Reset::BeforeCompose,Input::Scroll);
 opening.holdBeforeFlip=true;opening.fadeAfterFirst=2;opening.terminal={Restore{"tableau"}};
 add("tableau",{Restore{"tableau"}},[](const Context &)->Frame{return {Restore{"tableau"}};});
 for(unsigned slot=0;slot<14;++slot) {
  Frame enter;
  if(slot!=6 && slot!=7)enter.push_back(Audio{Audio::RetainDigital,d->voices[slot]});
  if(slot==5){enter.push_back(Audio{Audio::RetainDigital,d->voices[6]});enter.push_back(Audio{Audio::RetainDigital,d->voices[7]});}
  enter.push_back(Audio{Audio::Digital,d->voices[slot]});
  const bool mouth=slot!=2 && slot!=4 && slot!=8 && slot!=9 && slot!=12 && slot!=13;
  auto &voice=add("narration "+std::to_string(slot),std::move(enter),[=](const Context &c)->Frame {
   if(c.iteration==0){state->mouth=0;state->lookup=0;}
   Frame f{Restore{"blank"},sprite(R::GroupBack,0),sprite(R::GroupBack,1,160)};
   switch(slot) {
    case 2:state->king=(state->king+1)%5;f.push_back(sprite(R::Group,state->king));state->peasants=(state->peasants+1)%9;break;
    case 4:state->peasants=(state->peasants+1)%9+9;break;
    case 8:case 12:state->xeen=(state->xeen+1)%3;state->peasants=(state->peasants+1)%9;state->pet=(state->pet+1)%6+3;break;
    case 9:case 13:state->pet=(state->pet+1)%3;f.push_back(sprite(R::Group,state->pet+43,178,134));
     state->peasants=(state->peasants+1)%9;state->xeen=(state->xeen+1)%15+3;break;
    default:state->peasants=(state->peasants+1)%9;state->xeen=(state->xeen+1)%15+3;state->pet=(state->pet+1)%6+3;break;
   }
   f.push_back(sprite(R::Group,state->peasants+5,0,99));f.push_back(sprite(R::Group,state->xeen+24,202,12));
   if(++state->total%30==0)f.push_back(sprite(R::Group,43,178,134));
   f.push_back(sprite(R::Crodo,mouth?state->mouth:0,0,-5));
   if(mouth){state->lookup=std::min(30u,state->lookup);state->mouth=d->mouths[d->rows[slot]*31+state->lookup];}
   f.push_back(Text{d->labels});
   if(d->digitalDevice<2) {
    if(!state->captionTick)state->captionTick=c.tick;
    if(c.tick-*state->captionTick>1) {
     state->captionTick=c.tick;state->cursor=(state->cursor+1)%story->size();
     observed->captionAdvances.push_back(c.tick);
     const auto begin=state->cursor>40?state->cursor-40:0;
     state->captionText=xeenDialogFormat(d->captionFormat,{story->substr(begin,state->cursor-begin)});
    }
    const auto begin=state->cursor>40?state->cursor-40:0;
    observed->captionWidths.push_back(state->cursor-begin);
    f.push_back(sprite(R::Box,0,36,189));f.push_back(Text{state->captionText});
   }else state->cursor=0;
   return f;
  },0,0,Reset::Entry);
  voice.untilDigitalIdle=true;
  if(slot==0)voice.entering=[=](const Context &c){state->captionTick=c.tick;};
  voice.threshold=[=](const Context &){return mouth?unsigned(d->waits[d->rows[slot]*31+state->lookup]):0u;};
  voice.published=[=](const Context &) {if(mouth){++state->lookup;if(d->digitalDevice<2 && state->lookup>30)state->lookup=0;}};
  voice.finished=[=](const Context &c){return !c.digitalBusy && state->cursor==0;};
  // No-digital configurations finish the single caption record in slot zero.
  if(slot>0 && d->digitalDevice<2)voice.compose={};
 }
 result.closing=result.segments.size();
 auto &closing=add("closing scroll",{Audio{Audio::Fm,{},0},Audio{Audio::FadeSong,{},50},Store{"scroll-base"}},
  [](const Context &c)->Frame{return {Scroll{22-c.iteration}};},22,1,Reset::BeforeCompose,Input::Scroll);
 closing.holdBeforeFlip=true;closing.terminal={Scroll{0}};
 add("black",{Audio{Audio::StopDigital},Audio{Audio::FadeSong,{},0}},[](const Context &)->Frame{return {Level{0}};},1,0,Reset::Entry,Input::Ignore);
 return result;
}
}
