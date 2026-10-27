#include "games/xeen/XeenCutscene.h"
#include "games/xeen/XeenDialogView.h"
#include <algorithm>
#include <cstdio>
namespace mmodern {
XeenCutscene XeenCutscene::cloudsEnding(const XeenCutsceneData &data,const XeenCutsceneData::Captions &captions,XeenWallClock clock,std::uint32_t score) {
 using E=XeenCutsceneData::Ending;using Reset=Segment::Reset;using Input=Segment::Input;
 auto d=std::make_shared<XeenCutsceneData>(data);const bool digital=data.digitalDevice>=2;
 auto random=std::make_shared<XeenPresentationRandom>(std::move(clock),d->ending.randomWords);
 struct State {
  unsigned vortex=1,offset=0,stride=1,cursor=0;
  unsigned sky=0,flying=0;
  std::array<unsigned,10> digits{};
  int people=117,crodo=320;
  bool laugh=false;
  std::uint64_t captionTick=0;
  std::string caption,strip;
 };
 auto state=std::make_shared<State>();XeenCutscene result;auto observed=result.observations;
 state->flying=d->ending.initialFlying;
 const auto resource=[d](E::Resource r){return d->ending.resources[r];};
 const auto sprite=[resource](E::Resource r,unsigned frame=0,int x=0,int y=0,int scale=0,unsigned flags=0) {
  return Sprite{resource(r),frame,x,y,scale,flags};
 };
 const auto add=[&](std::string name,Frame enter,std::function<Frame(const Context &)> compose,unsigned frames=1,unsigned ticks=0)->Segment & {
  result.segments.push_back({});auto &s=result.segments.back();s.name=std::move(name);s.enter=std::move(enter);
  s.compose=std::move(compose);s.frames=frames;s.ticks=ticks;s.input=Input::Ignore;s.audioBeforeFlip=true;return s;
 };
 const auto fade=[&](std::string name,bool in,Frame enter=Frame{},unsigned step=4) {
  enter.push_back(Store{"fade"});auto &s=add(std::move(name),std::move(enter),[=](const Context &c)->Frame {
   return {Restore{"fade"},Level{in?step*c.iteration:128-step*c.iteration}};
  },128/step+1);s.reset=Reset::Entry;
 };
 const auto castle=[=](unsigned index)->Frame {
  return {Compressed{d->ending.prec.at(index)},sprite(E::Prec),sprite(E::Prec,1,160)};
 };
 Frame loading{Palette{resource(E::StormPalette)},Raw{resource(E::Blank)},Prepare{resource(E::Prec)}};
 for(const auto &name:d->ending.prec)loading.push_back(PrepareCompressed{name});
 add("ending castle",std::move(loading),[=](const Context &){auto f=castle(0);f.push_back(Level{0});return f;});
 fade("castle fade in",true);
 auto &hold=add("castle initial hold",{},[=](const Context &){return castle(0);},1,15);hold.reset=Reset::AfterFlip;
 auto &storm=add("castle storm",{Audio{Audio::Fm,{},1},Audio{Audio::Fm,{},34}},[=](const Context &c) {
  auto f=castle(c.iteration);if(auto cue=d->ending.stormFm[c.iteration+1])f.push_back(Audio{Audio::Fm,{},cue});return f;
 },41,3);storm.holdBeforeFlip=true;
 Frame vortexLoads;
 for(const auto &name:d->ending.vortex)vortexLoads.push_back(PrepareCompressed{name});
 vortexLoads.push_back(Audio{Audio::Fm,{},34});
 for(const auto &name:d->ending.castleTop)vortexLoads.push_back(Prepare{name});
 for(const auto &name:d->ending.castleBottom)vortexLoads.push_back(Prepare{name});
 for(const auto &name:d->ending.darkLord)vortexLoads.push_back(Prepare{name});
 vortexLoads.push_back(Audio{Audio::RetainDigital,resource(E::Dark1)});
 vortexLoads.push_back(Audio{Audio::RetainDigital,resource(E::XeenLaugh)});
 const auto vortex=[=](int table)->Frame {
  const auto index=state->vortex;state->vortex=index%20+1;
  Frame f{Compressed{d->ending.vortex[index-1]}};
  // The EXE increments/wraps DI before indexing every cue table. The
  // destruction table uses DI; appearance, speech and disappearance use DI-1.
  if(table>=0){const auto slot=state->vortex-(table!=0);if(slot<20)if(auto cue=d->ending.vortexFm[table][slot])f.push_back(Audio{Audio::Fm,{},cue});}
  return f;
 };
 auto &pyramid=add("ending pyramid",std::move(vortexLoads),[=](const Context &){auto f=vortex(-1);
  f.push_back(Sprite{d->ending.castleTop[0]});f.push_back(Sprite{d->ending.castleBottom[0],0,0,100});return f;
 },15,3);pyramid.holdBeforeFlip=true;
 fade("pyramid palette",true,{Palette{resource(E::AlamarPalette)}},128);
 for(unsigned actor=0;actor<6;++actor) {
  auto &destroy=add("castle destruction "+std::to_string(actor),{},[=](const Context &c) {
   auto f=vortex(0);
   if(state->laugh && !c.digitalBusy){state->laugh=false;f.push_back(Audio{Audio::Fm,{},34});}
   if(!state->laugh && actor==0 && c.iteration==6){state->laugh=true;f.push_back(Audio{Audio::Digital,resource(E::XeenLaugh)});}
   f.push_back(Sprite{d->ending.castleTop[actor],c.iteration});f.push_back(Sprite{d->ending.castleBottom[actor],c.iteration,0,100});return f;
  },d->ending.castleFrames[actor],3);destroy.holdBeforeFlip=true;
 }
 const auto alamar=[=](unsigned frame) {
  const unsigned which=frame<7?0:frame<11?1:2,base=which==0?0:which==1?7:11;
  return Sprite{d->ending.darkLord[which],frame-base};
 };
 auto &appearing=add("Alamar appears",{},[=](const Context &c){auto f=vortex(1);f.push_back(alamar(c.iteration));return f;},16,3);
 appearing.holdBeforeFlip=true;
 const auto startCaption=[=](unsigned record,const std::string &text,const Context &c) {
  state->caption=text;state->captionTick=c.tick;state->cursor=!digital?1:0;state->strip.clear();
  if(state->cursor)observed->captionRecords.push_back(record);
 };
 const auto caption=[=](Frame &f,const Context &c) {
  if(digital)return;
  if(state->cursor && c.tick-state->captionTick>=2) {
   state->captionTick=c.tick;state->cursor=(state->cursor+1)%state->caption.size();
   observed->captionAdvances.push_back(c.tick);
   const auto start=state->cursor>40?state->cursor-40:0;
   state->strip=xeenDialogFormat(d->captionFormat,{state->caption.substr(start,state->cursor-start)});
  }
  observed->captionWidths.push_back(std::min(40u,state->cursor));f.push_back(sprite(E::Box,0,36,189));f.push_back(Text{state->strip});
 };
 for(unsigned voice=0;voice<3;++voice) {
  const auto name=resource(E::Resource(E::Dark1+voice));
  Frame enter{Audio{Audio::RetainDigital,name},Audio{Audio::Digital,name}};
  if(!voice)enter.insert(enter.begin(),Audio{Audio::Mixer,{},75});
  auto &speech=add("Alamar voice "+std::to_string(voice+1),std::move(enter),[=](const Context &c) {
   auto f=vortex(2);f.push_back(Sprite{d->ending.darkLord[2],random->range(2,6)});caption(f,c);return f;
  },0,3); // Caption helper 0x1191b flips before the caller's three-tick hold.
  if(!voice)speech.entering=[=](const Context &c){startCaption(8,captions.record(8,300),c);};
  speech.finished=[=](const Context &c){return !c.digitalBusy && state->cursor==0;};
  if(voice && !digital)speech.compose={};
 }
 auto &vanishing=add("Alamar disappears",{Audio{Audio::RetainDigital,resource(E::DarkLaugh)},Audio{Audio::Digital,resource(E::DarkLaugh)},Audio{Audio::Mixer,{},95}},
  [=](const Context &c){auto f=vortex(3);f.push_back(alamar(12-c.iteration));return f;},13,3);vanishing.holdBeforeFlip=true;
 fade("vortex fade out",false,{Audio{Audio::StopDigital},Audio{Audio::Song,resource(E::Song)}},1);
 add("ending Later",{Palette{resource(E::Palette)},Raw{resource(E::Later)}},[](const Context &)->Frame{return {Level{0}};});
 fade("Later fade in",true);
 Frame throneLoads;
 for(auto r:{E::EndText,E::King,E::Room,E::Mirror,E::Sky,E::People,E::Crodo,E::KingCrodo,E::MirrorBack})throneLoads.push_back(Prepare{resource(r)});
 throneLoads.push_back(Audio{Audio::RetainDigital,resource(E::King4)});
 auto &later=add("Later minimum",std::move(throneLoads),[](const Context &)->Frame{return {};},1,100);
 later.reset=Reset::Entry;later.resetBeforeEnter=true;
 fade("Later fade out",false);
 auto &throne=add("ending throne",{Raw{resource(E::Throne1)},Store{"throne0"},Raw{resource(E::Throne2)},Store{"throne1"}},
  [=](const Context &)->Frame {
   observed->throneOffsets.push_back(state->offset);
   Frame f{HorizontalMerge{"throne0","throne1",state->offset},sprite(E::People,0,state->people,68,0,0x800)};
   if(state->offset>250)f.push_back(sprite(E::Crodo,0,state->crodo,68,0,0x800));return f;
  },0);
 throne.fadeAfterFirst=4;
 throne.published=[=](const Context &c) {
  if(c.iteration==0){state->stride=stride(c.elapsed);observed->panStride=state->stride;}
  if(state->offset>250)state->crodo=std::max(181,state->crodo-int(2*state->stride));
  state->offset=std::min(320u,state->offset+state->stride);state->people-=2*int(state->stride);
 };
 throne.finished=[=](const Context &){return state->offset>=320;};
 auto &throneHold=add("throne final hold",{},[=](const Context &)->Frame {
  return {HorizontalMerge{"throne0","throne1",320},sprite(E::Crodo,0,state->crodo,68,0,0x800),Store{"throne"}};
 },1,5);throneHold.reset=Reset::AfterFlip;
 for(int scale=12;scale>=0;--scale) {
  auto &king=add("king scale "+std::to_string(scale),{},[=](const Context &)->Frame {
   return {Restore{"throne"},sprite(E::King,0,d->ending.kingLeft[scale],d->ending.kingY[scale],scale),
    sprite(E::King,1,d->ending.kingRight[scale],d->ending.kingY[scale],scale)};
  },0);king.reset=Reset::Entry;king.totalTicks=1;
 }
 const auto king=[=]()->Frame{return {sprite(E::King),sprite(E::King,1,160)};};
 for(unsigned voice=0;voice<3;++voice) {
  const auto name=d->ending.kingVoices[voice];
  auto &speech=add("king voice "+std::to_string(voice+1),{Audio{Audio::RetainDigital,name},Audio{Audio::Digital,name}},[=](const Context &c) {
   auto f=king();const auto mouth=random->range(1,6);observed->kingMouths.push_back(mouth);if(mouth>1)f.push_back(sprite(E::King,mouth));caption(f,c);return f;
  },0,3); // EXE 0x942f publishes, then 0x943c waits for three elapsed ticks.
  // DOS selects record 10, even though king1..3 speak the record-9 text.
  if(!voice)speech.entering=[=](const Context &c){startCaption(10,captions.record(10,200),c);};
  speech.finished=[=](const Context &c){return !c.digitalBusy && state->cursor==0;};
  if(voice && !digital)speech.compose={};
  auto &rest=add("king pause "+std::to_string(voice+1),{},[=](const Context &){return king();},1,1);rest.reset=Reset::AfterFlip;
  if(voice && !digital)rest.compose={};
 }
 fade("throne fade out",false);
 // Mirror room: the two horizontal pages initially contain the same RAW.
 add("mirror room",{Palette{resource(E::MirrorPalette)},Raw{resource(E::MirrorRaw)},Store{"mirror0"},Store{"mirror1"}},[=](const Context &)->Frame {
  return {sprite(E::Room),sprite(E::Room,1,160),Level{0}};
 });
 fade("mirror fade in",true);
 auto &room=add("mirror room pan",{},[=](const Context &c)->Frame {
  return {HorizontalMerge{"mirror0","mirror1",std::min(82u,c.iteration*state->stride)},sprite(E::Room),sprite(E::Room,1,160)};
 },0,1);room.finished=[=](const Context &c){return (c.iteration+1)*state->stride>=83;};
 add("mirror pan saved",{},[](const Context &)->Frame{return {Store{"mirror-room"}};});
 auto &zoom=add("mirror zoom",{},[=](const Context &c)->Frame {
  const auto n=8-c.iteration;const auto x=d->ending.mirrorX[n],y=d->ending.mirrorY[n];
  return {Restore{"mirror-room"},sprite(E::Sky,0,x,y,n),sprite(E::MirrorBack,0,x,y,n),sprite(E::Mirror,0,x,y,n)};
 },9);
 (void)zoom;
 add("mirror sky saved",{},[=](const Context &)->Frame{return {sprite(E::Sky),Store{"sky"}};});
 const auto sky=[=]()->Frame {
  const auto column=state->sky;state->sky=(column+1)%320;observed->skyColumns.push_back(column);
  return {Rotate{"sky",column},sprite(E::MirrorBack)};
 };
 auto &entrance=add("mirror entrance",{},[=](const Context &c) {
  auto f=sky();const auto offset=std::min(115u,c.iteration*4*state->stride);
  f.push_back(sprite(E::Mirror));f.push_back(sprite(E::KingCrodo,0,int(offset)-115,200-int(offset),0,0x800));
  f.push_back(sprite(E::KingCrodo,1,335-int(offset),200-int(offset),0,0x800));return f;
 },0);
 entrance.finished=[=](const Context &c){return (c.iteration+1)*4*state->stride>=115;};
 const auto people=[=](Frame &f) {
  f.push_back(sprite(E::Mirror));f.push_back(sprite(E::KingCrodo,0,0,85,0,0x800));f.push_back(sprite(E::KingCrodo,1,220,85,0,0x800));
 };
 add("parade entrance complete",{},[=](const Context &){auto f=sky();people(f);return f;});
 for(unsigned actor=0;actor<d->ending.parade.size();++actor) {
  const auto row=d->ending.parade[actor];const auto prefix="parade "+std::to_string(actor)+" ";
  const auto compose=[=](bool attack,unsigned frame) {
   auto f=sky();state->flying=(state->flying+1)%8;
   const int x=31+(row.metadata[0]?d->ending.flyingX[state->flying]:0),y=10+(row.metadata[0]?d->ending.flyingY[state->flying]:0);
   f.push_back(Sprite{attack?row.attack:row.idle,frame,x,y});people(f);return f;
  };
  const auto phase=[&](const std::string &name,bool attack,unsigned frame,unsigned ticks,Frame enter=Frame{}) ->Segment & {
   auto &s=add(prefix+name,std::move(enter),[=](const Context &){return compose(attack,frame);},0);
   s.totalTicks=ticks;s.reset=Reset::Entry;return s;
  };
  for(unsigned i=0;i<8;++i) {
   Frame enter;if(!i)enter={Prepare{row.idle},Prepare{row.attack},Audio{Audio::StopDigital},Audio{Audio::RetainDigital,row.voice}};
   auto &s=phase("idle "+std::to_string(i),false,i,1,std::move(enter));
   if(!i)s.entering=[=](const Context &){observed->paradeSelectors.push_back(row.selector);};
  }
  for(unsigned i=0;i<3;++i)phase("attack "+std::to_string(i),true,i,1,i==2?Frame{Audio{Audio::Digital,row.voice}}:Frame{});
  phase("attack hold",true,2,15);
  auto &power=phase("power hold",true,3,7);
  power.entering=[=](const Context &){state->offset=random->range(0,5);};
  power.compose=[=](const Context &c){auto f=compose(true,3);if(!c.iteration){const auto name=d->ending.powers[state->offset];f.insert(f.begin(),{Audio{Audio::StopDigital},Audio{Audio::RetainDigital,name},Audio{Audio::Digital,name}});}return f;};
 }
 const auto scroll=[&](const char *name,bool close) {
  auto &s=add(name,{Store{"scroll-base"}},[=](const Context &c)->Frame{return {Scroll{close?22-c.iteration:c.iteration}};},22,1);
  s.input=Input::Scroll;s.holdBeforeFlip=true;s.terminal={Scroll{close?0u:22u}};
 };
 scroll("parade closing scroll",true);
 add("score tableau",{},[=](const Context &){auto f=sky();f.push_back(sprite(E::Mirror));return f;});
 scroll("score opening scroll",false);
 const auto scoreFrame=[=](unsigned heading,int y,bool digits)->Frame {
  auto f=sky();f.push_back(sprite(E::Mirror));f.push_back(sprite(E::EndText,heading));
  if(digits)for(unsigned n=0;n<10;++n)f.push_back(sprite(E::EndText,state->digits[n],64+19*n,y));return f;
 };
 for(unsigned frame=0;frame<20;++frame) {
  auto &heading=add(frame?"score heading "+std::to_string(frame):"score heading",{},[=](const Context &){return scoreFrame(frame,73,false);},0);
  heading.reset=Reset::Entry;heading.totalTicks=1;
 }
 add("score random digits",{},[=](const Context &){for(auto &digit:state->digits)digit=random->range(20,29);
  observed->scoreFrames.push_back(state->digits);return scoreFrame(19,73,true);
 },30,2);
 char formatted[11];std::snprintf(formatted,sizeof(formatted),"%010u",unsigned(score));const std::string finalScore=formatted;
 add("score settling digits",{},[=](const Context &c) {
  for(auto &digit:state->digits)digit=random->range(20,29);
  for(unsigned n=9-c.iteration;n<10;++n)state->digits[n]=20+finalScore[n]-'0';
  observed->settledDigits.push_back(9-c.iteration);observed->scoreFrames.push_back(state->digits);return scoreFrame(19,73,true);
 },10,2);
 add("score downward",{},[=](const Context &c){return scoreFrame(19,73+c.iteration,true);},38);
 for(unsigned page=0;page<2;++page) {
  auto &s=add("score page "+std::to_string(page+1),{},[=](const Context &){auto f=scoreFrame(19,110,true);
   // EXE 0xa353/0x1d51f sets window 28; 0x19540 insets its eight-pixel margin.
   f.push_back(Text{d->ending.scorePages[page],71,68,71,68,246,152});return f;
  },0);s.input=Input::Keyboard;
 }
 scroll("score closing scroll",true);fade("score fade out",false);
 add("last king",{},[=](const Context &){auto f=king();f.push_back(Level{0});return f;});
 fade("last king fade in",true);
 auto &last=add("king voice 4",{Audio{Audio::RetainDigital,resource(E::King4)},Audio{Audio::Digital,resource(E::King4)}},[=](const Context &c) {
  auto f=king();const auto mouth=random->range(1,6);observed->kingMouths.push_back(mouth);if(mouth>1)f.push_back(sprite(E::King,mouth));caption(f,c);return f;
 },0,3);
 last.entering=[=](const Context &c){startCaption(10,captions.record(10,200),c);};last.finished=[=](const Context &c){return !c.digitalBusy && state->cursor==0;};
 auto &pause=add("last king pause",{},[=](const Context &){return king();},1,1);pause.reset=Reset::AfterFlip;
 scroll("ending closing scroll",true);
 // The EXE's ending mixer sites are only 0x8e42 (75) and 0x8f91 (95).
 add("ending shutdown",{Audio{Audio::StopDigital},Audio{Audio::FadeSong,{},0}},[](const Context &)->Frame{return {Level{0}};});
 result.closing=result.segments.size();return result;
}
}
