#include "games/xeen/XeenCutscene.h"
#include "games/xeen/XeenDialogView.h"
#include <algorithm>
namespace mmodern {
XeenCutscene XeenCutscene::cloudsEnding(const XeenCutsceneData &data,const XeenCutsceneData::Captions &captions,XeenWallClock clock) {
 using E=XeenCutsceneData::Ending;using Reset=Segment::Reset;using Input=Segment::Input;
 auto d=std::make_shared<XeenCutsceneData>(data);const bool digital=data.digitalDevice>=2;
 auto random=std::make_shared<XeenPresentationRandom>(std::move(clock),d->ending.randomWords);
 struct State {
  unsigned vortex=1,offset=0,stride=1,cursor=0;
  int people=117,crodo=320;
  bool laugh=false;
  std::uint64_t captionTick=0;
  std::string caption,strip;
 };
 auto state=std::make_shared<State>();XeenCutscene result;auto observed=result.observations;
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
 add("C1 return",{Audio{Audio::StopDigital},Audio{Audio::Mixer,{},95},Audio{Audio::FadeSong,{},0}},[](const Context &)->Frame{return {Level{0}};});
 result.closing=result.segments.size();return result;
}
}
