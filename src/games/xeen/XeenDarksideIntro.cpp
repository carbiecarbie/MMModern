#include "games/xeen/XeenCutscene.h"
#include "games/xeen/XeenDialogView.h"
#include <algorithm>
namespace mmodern {
XeenCutscene XeenCutscene::darkside(const XeenCutsceneData &data,const XeenCutsceneData::Captions &captions,XeenWallClock clock,DarksidePath path) {
 using D=XeenCutsceneData::Darkside;using A=Audio;using I=Segment::Input;using R=Segment::Reset;
 auto d=std::make_shared<XeenCutsceneData>(data);auto lines=std::make_shared<XeenCutsceneData::Captions>(captions);auto random=std::make_shared<XeenPresentationRandom>(std::move(clock),d->ending.randomWords);
 const bool digital=d->digitalDevice>=2;XeenCutscene scene;auto observed=scene.observations;
 struct State {unsigned frame=0,offset=0,stride=1,phase=0,pos=0,lookup=0,sub=0,voice=0,cursor=0;std::uint64_t captionTick=0,roomTick=0;bool captionStartsAtZero=false,roomDone=false;std::string caption,strip;};
 auto state=std::make_shared<State>();
 const auto name=[d](D::Resource id){return d->darkside.resources[id];};
 const auto sprite=[name](D::Resource id,unsigned frame=0,int x=0,int y=0,int scale=0,unsigned flags=0){return Sprite{name(id),frame,x,y,scale,flags};};
 const auto cue=[](const auto &table,unsigned frame) {for(const auto &item:table)if(item.key==frame)return unsigned(item.target);return 0u;};
 const auto add=[&](std::string n,Frame enter,std::function<Frame(const Context &)> compose,unsigned frames=1,unsigned ticks=0)->Segment & {
  scene.segments.push_back({});auto &s=scene.segments.back();s.name=std::move(n);s.enter=std::move(enter);s.compose=std::move(compose);s.frames=frames;s.ticks=ticks;s.audioBeforeFlip=true;return s;
 };
 const auto fade=[&](std::string n,bool in,unsigned step=4,Frame enter={}) {
  enter.push_back(Store{"fade"});auto &s=add(std::move(n),std::move(enter),[=](const Context &c)->Frame {
   const auto level=std::min(128u,c.iteration*step);return {Restore{"fade"},Level{in?level:128-level}};
  },(128+step-1)/step+1);s.input=I::Unpolled;s.reset=R::Entry;
 };
 const auto selectCaption=[=](unsigned index,unsigned seed,const Context &c) {
  state->caption=lines->record(index,1000);if(seed>=state->caption.size())throw std::runtime_error("Invalid Darkside caption seed");
  state->cursor=digital?0:seed;state->captionStartsAtZero=!digital && !seed;state->captionTick=c.tick;state->strip.clear();observed->captionRecords.push_back(index);
 };
 const auto caption=[=](Frame &f,const Context &c) {
  if(digital)return;
  if((state->cursor || state->captionStartsAtZero) && c.tick-state->captionTick>=2) {
   state->captionStartsAtZero=false;state->captionTick=c.tick;state->cursor=(state->cursor+1)%state->caption.size();
   auto begin=state->cursor>40?state->cursor-40:0;
   state->strip=xeenDialogFormat(d->captionFormat,{state->caption.substr(begin,state->cursor-begin)});
   observed->captionAdvances.push_back(c.tick);
  }
  f.push_back(sprite(D::Box,0,36,189));f.push_back(Text{state->strip});
 };
 Frame init{Archive{XeenSceneArchive::DarksideProgramDark},Load{d->darkside.startupScratch},Archive{XeenSceneArchive::DarksideProgram},
  Load{d->darkside.pyramidScratch},Palette{name(D::Palette)},Raw{name(D::NwcRaw1)},Store{"nwc0"},Raw{name(D::NwcRaw2)},Store{"nwc1"}};
 for(auto r:{D::Nwc1,D::Nwc2,D::Nwc3,D::Nwc4})init.push_back(Prepare{name(r)});
 for(auto r:{D::Dragon1,D::Dragon2,D::Dragon3})init.push_back(A{A::RetainDigital,name(r)});
 add("NWC background",std::move(init),[](const Context &)->Frame{return {Merge{"nwc0","nwc1",0},Level{0}};});
 fade("NWC fade in",true);
 auto &nwc=add("NWC",{A{A::Fm,{},1}},[=](const Context &c)->Frame {
  auto which=D::Resource(D::Nwc1+state->pos);
  Frame f{Merge{"nwc0","nwc1",0},sprite(which,state->frame)};
  switch(cue(d->darkside.nwcCues,c.iteration)) {
   case 0xab35:f.push_back(A{A::Digital,name(D::Dragon1)});break;
   case 0xab3d:f.push_back(A{A::Digital,name(D::Dragon2)});break;
   case 0xab2e:state->frame=0;++state->pos;break;default:++state->frame;break;
  }
  return f;
 },55,2);nwc.audioBeforeFlip=false;
 auto &nwcTail=add("NWC tail",{},[=](const Context &c)->Frame {
  Frame f{Merge{"nwc0","nwc1",200},sprite(D::Nwc4,c.iteration)};
  switch(cue(d->darkside.nwcTailCues,c.iteration)){case 0xabb7:f.push_back(A{A::Fm,{},40});break;case 0xabbb:f.push_back(A{A::Fm,{},0});break;
   case 0xabbf:f.push_back(A{A::Fm,{},3});break;case 0xabc3:f.push_back(A{A::Digital,name(D::Dragon3)});break;case 0xabd2:f.push_back(A{A::Fm,{},2});break;}
  return f;
 },42,2);
 nwcTail.audioBeforeFlip=false;
 auto &nwcHold=add("NWC final hold",{},[](const Context &)->Frame{return {};},1,10);nwcHold.reset=R::Previous;nwcHold.pollAfterHold=true;
 fade("JVC fade out",false,8,{A{A::Mixer,{},95},Store{"nwc-final"},Raw{name(D::Jvc)},Store{"jvc"},Restore{"nwc-final"}});
 add("JVC",{},[](const Context &)->Frame{return {Restore{"jvc"},Level{0}};});
 fade("JVC fade in",true,4,{path==DarksidePath::Full?A{A::Song,d->darkside.theme(d->musicDevice)}:A{A::FadeSong,{},0}});
 Frame loading{Raw{name(D::PyramidRaw2)},Store{"pyr0"},Raw{name(D::PyramidRaw3)},Store{"pyr1"}};
 for(auto r:{D::Title,D::PyramidTop,D::Pyramid})loading.push_back(Prepare{name(r)});
 for(auto r:{D::Pharaoh1a,D::Pharaoh1b})loading.push_back(A{A::RetainDigital,name(r)});
 auto &jvcHold=add("JVC hold",std::move(loading),[](const Context &)->Frame{return {Restore{"jvc"}};},1,60);jvcHold.reset=R::Entry;jvcHold.resetBeforeEnter=true;
 if(path==DarksidePath::Full) {
  fade("pyramid fade out",false,8);
  auto &pan=add("pyramid approach",{},[=](const Context &)->Frame {
   Frame f{Merge{"pyr0","pyr1",state->offset},sprite(D::Title)};
   auto frame=(state->sub/4)%10;if(frame)f.push_back(sprite(D::Title,frame));++state->sub;return f;
  },0,1);
  pan.fadeAfterFirst=4;pan.published=[=](const Context &c){if(!c.iteration){state->stride=stride(c.elapsed?c.elapsed-1:0);observed->panStride=state->stride;}state->offset=std::min(200u,state->offset+state->stride);};
  pan.finished=[=](const Context &){return state->offset>=200;};
  auto &panHold=add("pyramid hold",{},[](const Context &)->Frame{return {Merge{"pyr0","pyr1",200},Store{"pyramid"}};},1,30);panHold.reset=R::AfterFlip;
  auto &zoom=add("pyramid zoom",{},[=](const Context &c)->Frame {
   const auto i=14-c.iteration;Frame f{Restore{"pyramid"},sprite(D::PyramidTop,0,d->darkside.pyramid[0][i],d->darkside.pyramid[2][i],i),sprite(D::PyramidTop,1,d->darkside.pyramid[1][i],d->darkside.pyramid[2][i],i)};
   if(i==2)f.push_back(A{A::Mixer,{},48});return f;
  },15,2);zoom.audioBeforeFlip=false;
  auto &speech=add("pyramid voice",{Store{"pyramid"},A{A::Digital,name(D::Pharaoh1a)}},[=](const Context &c)->Frame {
   Frame f{Restore{"pyramid"},sprite(D::Pyramid,c.iteration,132,62)};caption(f,c);
   if(!state->voice && !c.digitalBusy){state->voice=1;f.push_back(A{A::Digital,name(D::Pharaoh1b)});}return f;
  },19,4);speech.entering=[=](const Context &c){selectCaption(0,8,c);};
  if(!digital){speech.holdInterval=2;speech.duringHold=[=](const Context &c)->Frame{Frame f;caption(f,c);return f;};}
  auto &speechTail=add("pyramid voice tail",{},[=](const Context &c)->Frame{Frame f;caption(f,c);return f;},0);
  speechTail.input=I::Unpolled;speechTail.finished=[=](const Context &c){return !c.digitalBusy && !state->cursor;};
  fade("room transition",false);
  Frame roomLoads{Raw{name(D::Room2)},Store{"room1"},Raw{name(D::Room3)},Store{"room0"},Raw{name(D::Room1)},Prepare{name(D::Dragon)}};
  for(unsigned i=0;i<5;++i)roomLoads.push_back(A{A::RetainDigital,name(D::Resource(D::Pharaoh2a+i))});
  auto &roomStart=add("room setup",std::move(roomLoads),[=](const Context &c)->Frame {
   Frame f{HorizontalMerge{"room0","room1",320},sprite(D::Dragon,9,d->darkside.room[0][0],d->darkside.room[1][0],0,0x800),Level{0}};caption(f,c);return f;
  });roomStart.resetBeforeEnter=true;roomStart.entering=[=](const Context &c){state->roomTick=c.tick;state->offset=320;state->voice=0;state->pos=state->lookup=state->sub=0;state->roomDone=false;selectCaption(1,25,c);};
  fade("room fade in",true);
  auto &room=add("room",{},[=](const Context &c)->Frame {
   Frame f{HorizontalMerge{"room0","room1",state->offset},sprite(D::Dragon,d->darkside.roomFrames[state->lookup],d->darkside.room[0][state->pos],d->darkside.room[1][state->pos],0,0x800)};
   caption(f,c);if(state->voice<5 && (!state->voice || !c.digitalBusy))f.push_back(A{A::Digital,name(D::Resource(D::Pharaoh2a+state->voice++))});
   if(++state->sub==3){state->sub=0;if(state->pos==5)f.push_back(A{A::Fm,{},7});else if(!state->pos)f.push_back(A{A::Fm,{},8});state->pos=(state->pos+1)%10;state->lookup=(state->lookup+1)%36;}
   state->roomDone=state->offset==0;state->offset=state->offset>2*state->stride?state->offset-2*state->stride:0;return f;
  },0,1);
  room.finished=[=](const Context &c){return state->roomDone || (state->voice==5 && !c.digitalBusy && !state->cursor);};
  auto &roomTail=add("room tail",{},[=](const Context &c)->Frame {
   Frame f{HorizontalMerge{"room0","room1",0},sprite(D::Dragon,0,d->darkside.room[0][state->pos],d->darkside.room[1][state->pos],0,0x800)};caption(f,c);return f;
  },0,1);roomTail.input=I::Unpolled;
  roomTail.finished=[=](const Context &c){return !c.digitalBusy && !state->cursor && (digital || c.tick-state->roomTick>=220);};
  fade("ball transition",false,4,{A{A::StopDigital}});
  Frame ballLoads{Raw{name(D::BallRaw)},Store{"ball"}};
  for(auto r:{D::Ball,D::Pharaoh,D::DarkLord,D::Goon,D::Queen,D::Wizard,D::Fizzle,D::Claw,D::WizardMouth,D::Box})ballLoads.push_back(Prepare{name(r)});
  for(auto r:{D::ScratchVoice,D::Pharaoh3,D::Queen1,D::Queen2,D::QueenHiss,D::Pharaoh4,D::Dark3,D::Pharaoh5a,D::Pharaoh5b,D::Pharaoh6w,D::Alamar1,D::Pharaoh7t,D::Pharaoh8,D::Ellinger,D::Pharaoh9a,D::Pharaoh9b})ballLoads.push_back(A{A::RetainDigital,name(r)});
  const auto ball=[=](bool rubbing)->Frame {
   const auto p=state->phase;state->phase=(p+1)%32;const auto &t=d->darkside.ball;
   Frame f{Restore{"ball"}};if(p)f.push_back(sprite(D::Ball,p-1));
   f.push_back(sprite(D::Claw,5,t[2][p],t[3][p],0,0x800));f.push_back(sprite(D::Claw,6,149,184));
   f.push_back(sprite(D::Pharaoh,t[1][p],139,t[0][p],0,0x800));
   f.push_back(sprite(D::Claw,p%5,t[rubbing?6:4][p],t[rubbing?7:5][p],0,0x800));return f;
  };
  auto &rub=add("ball rub",std::move(ballLoads),[=](const Context &c)->Frame{auto f=ball(true);caption(f,c);return f;},32,3);rub.fadeAfterFirst=4;
  auto &queenIn=add("queen fizzle",{A{A::Fm,{},42}},[=](const Context &c)->Frame {
   auto f=ball(false);if(c.iteration>3)f.push_back(sprite(D::Queen,0,9,57));f.push_back(sprite(D::Fizzle,c.iteration,9,57));
   if(c.iteration==6){f.push_back(A{A::Digital,name(D::Pharaoh3)});f.push_back(A{A::Fm,{},0});}if(c.iteration>6)caption(f,c);return f;
  },11,3);queenIn.entering=[=](const Context &c){selectCaption(9,1,c);state->voice=0;state->frame=0;};
  auto &queen=add("queen",{},[=](const Context &c)->Frame {
   auto f=ball(false);if(!c.digitalBusy && !state->cursor && state->voice<3) {
    auto r=D::Resource(D::Queen1+state->voice);if(!state->voice)selectCaption(10,1,c);else {state->cursor=0;state->strip.clear();state->captionStartsAtZero=false;}
    f.push_back(A{A::Digital,name(r)});++state->voice;if(state->voice==3)state->frame=7;
   }
   unsigned frame=0;if(state->voice==3){if(state->frame<=16)frame=state->frame++;}else if(state->voice){frame=random->range(0,5);observed->darksideQueenFrames.push_back(frame);}
   f.push_back(sprite(D::Queen,frame,9,57));caption(f,c);return f;
  },0,3);queen.finished=[=](const Context &c){return state->voice==3 && !c.digitalBusy && !state->cursor;};
  auto &queenReply=add("queen reply",{A{A::Digital,name(D::Pharaoh4)}},[=](const Context &c)->Frame{auto f=ball(false);f.push_back(sprite(D::Queen,0,9,57));caption(f,c);return f;},0,3);
  queenReply.entering=[=](const Context &c){selectCaption(11,1,c);};queenReply.finished=[=](const Context &c){return !c.digitalBusy && !state->cursor;};
  auto &lord=add("Dark Lord",{Archive{XeenSceneArchive::DarksideProgramDark},A{A::RetainDigital,name(D::Laugh)},Archive{XeenSceneArchive::DarksideProgram},A{A::Digital,name(D::Dark3)}},[=](const Context &c)->Frame {
   auto f=ball(false);if(!c.digitalBusy && !state->cursor && state->voice<3) {
    auto r=state->voice==0?D::Pharaoh5a:state->voice==1?D::Pharaoh5b:D::Laugh;
    f.push_back(A{A::Digital,name(r)});++state->voice;
   }
   unsigned frame=0;if(!state->voice){frame=random->range(0,3);observed->darksideLordFrames.push_back(frame);}
   f.push_back(sprite(D::DarkLord,frame,9,57));caption(f,c);return f;
  },0,3);lord.entering=[=](const Context &c){state->voice=0;selectCaption(12,1,c);};
  lord.finished=[=](const Context &){return state->voice>=3;};
  auto &lordOut=add("Dark Lord fizzle",{A{A::Fm,{},42}},[=](const Context &c)->Frame {
   const auto i=10-c.iteration;auto f=ball(false);if(i>3){auto frame=random->range(0,3);observed->darksideLordFrames.push_back(frame);f.push_back(sprite(D::DarkLord,frame,9,57));}
   f.push_back(sprite(D::Fizzle,i,9,57));if(!c.digitalBusy && !state->voice){f.push_back(A{A::Digital,name(D::Laugh)});state->voice=1;}caption(f,c);return f;
  },11,3);lordOut.entering=[=](const Context &c){state->voice=0;selectCaption(13,1,c);};
  auto &lordTail=add("Dark Lord tail",{A{A::Fm,{},0}},[=](const Context &c)->Frame{auto f=ball(false);caption(f,c);return f;},0,3);
  lordTail.finished=[=](const Context &c){return !c.digitalBusy && !state->cursor;};
  Frame goonLoads{A{A::StopDigital}};for(auto r:{D::Pharaoh6w,D::Alamar1,D::Pharaoh7t,D::Pharaoh8,D::Ellinger,D::Pharaoh9a,D::Pharaoh9b})goonLoads.push_back(A{A::RetainDigital,name(r)});
  auto &goonRub=add("Alamar rub",std::move(goonLoads),[=](const Context &c)->Frame{auto f=ball(true);caption(f,c);return f;},32,3);
  goonRub.entering=[=](const Context &){state->phase=0;};
  auto &goonIn=add("Alamar fizzle",{A{A::Fm,{},42}},[=](const Context &c)->Frame {
   auto f=ball(false);if(c.iteration>3)f.push_back(sprite(D::Goon,0,9,57));f.push_back(sprite(D::Fizzle,c.iteration,9,57));
   if(c.iteration==6)f.push_back(A{A::Digital,name(D::Pharaoh6w)});if(c.iteration>6)caption(f,c);return f;
  },11,3);goonIn.entering=[=](const Context &c){selectCaption(14,1,c);state->voice=0;state->frame=0;};
  auto &goon=add("Alamar",{},[=](const Context &c)->Frame {
   auto f=ball(false);if(!state->voice && !c.digitalBusy && !state->cursor){state->voice=1;selectCaption(15,1,c);f.push_back(A{A::Digital,name(D::Alamar1)});f.push_back(A{A::Fm,{},0});}
   f.push_back(sprite(D::Goon,state->voice?state->frame++%17:0,9,57));caption(f,c);return f;
  },0,3);goon.finished=[=](const Context &c){return state->voice && !c.digitalBusy && !state->cursor;};
  auto &goonOut=add("Alamar reverse fizzle",{A{A::Fm,{},42}},[=](const Context &c)->Frame {
   auto i=10-c.iteration;auto f=ball(false);if(i>4)f.push_back(sprite(D::Goon,0,9,57));f.push_back(sprite(D::Fizzle,i,9,57));
   if(!c.digitalBusy && !state->voice){state->voice=1;f.push_back(A{A::Digital,name(D::Pharaoh7t)});}caption(f,c);return f;
  },11,3);goonOut.entering=[=](const Context &c){state->voice=0;selectCaption(16,1,c);};
  auto &wizardRub=add("Ellinger rub",{A{A::Fm,{},0}},[=](const Context &c)->Frame{auto f=ball(true);caption(f,c);return f;},32,3);
  wizardRub.entering=[=](const Context &){state->phase=0;};
  auto &wizardIn=add("Ellinger fizzle",{A{A::Fm,{},42}},[=](const Context &c)->Frame {
   auto f=ball(false);if(c.iteration>3)f.push_back(sprite(D::Wizard,0,9,57));f.push_back(sprite(D::Fizzle,c.iteration,9,57));
   if(c.iteration==5)f.push_back(A{A::Digital,name(D::Pharaoh8)});if(c.iteration>6)caption(f,c);return f;
  },11,3);wizardIn.entering=[=](const Context &c){state->voice=0;selectCaption(17,1,c);};
  auto &wizard=add("Ellinger",{},[=](const Context &c)->Frame {
   auto f=ball(false);if(!c.digitalBusy && !state->cursor && state->voice<2) {
    if(!state->voice){selectCaption(18,1,c);f.push_back(A{A::Digital,name(D::Ellinger)});f.push_back(A{A::Fm,{},0});}
    else {state->cursor=0;state->strip.clear();state->captionStartsAtZero=false;f.push_back(A{A::Digital,name(D::Pharaoh9a)});}++state->voice;
   }
   unsigned frame=0;if(state->voice==1){frame=random->range(0,3);observed->darksideWizardFrames.push_back(frame);}
   f.push_back(sprite(D::Wizard,frame,9,57));caption(f,c);return f;
  },0,3);wizard.finished=[=](const Context &c){return state->voice==2 && !c.digitalBusy && !state->cursor;};
  add("Ellinger reverse fizzle",{A{A::Fm,{},42}},[=](const Context &c)->Frame {
   auto i=10-c.iteration;auto f=ball(false);if(i>4)f.push_back(sprite(D::Wizard,0,9,57));f.push_back(sprite(D::Fizzle,i,9,57));return f;
  },11,3);
  auto &orbDecision=add("orb decision",{},[=](const Context &c)->Frame {
   auto f=ball(false);if(!state->voice && !c.digitalBusy){state->voice=1;f.push_back(A{A::Digital,name(D::Pharaoh9b)});}caption(f,c);return f;
  },0,3);orbDecision.entering=[=](const Context &c){state->voice=0;selectCaption(19,1,c);};
  orbDecision.finished=[=](const Context &c){return state->voice && !c.digitalBusy && !state->cursor;};
  fade("release transition",false,4,{A{A::Fm,{},0}});
  Frame flyLoads{Palette{name(D::FlyPalette)},Raw{name(D::FlyRaw)},Store{"fly"},Prepare{name(D::Fly)},A{A::RetainDigital,name(D::Pharaoh11)}};
  auto &flyBackground=add("release background",std::move(flyLoads),[=](const Context &c)->Frame {
   Frame f{Restore{"fly"},Level{0}};caption(f,c);return f;
  });flyBackground.entering=[=](const Context &c){selectCaption(6,0,c);};
  fade("release fade in",true);
  auto &fly=add("release",{A{A::Digital,name(D::Pharaoh11)}},[=](const Context &c)->Frame {
   Frame f{Restore{"fly"},sprite(D::Fly,c.iteration)};caption(f,c);
   if(c.iteration==16 || c.iteration==28)f.push_back(A{A::Fm,{},43});return f;
  },35,2);fly.audioBeforeFlip=false;
  fade("bird transition",false,4,{A{A::Fm,{},0}});
  Frame birdLoads{A{A::Mixer,{},95},Raw{name(D::BirdRaw)},Store{"bird"},A{A::RetainDigital,name(D::Comet)}};
  for(unsigned i=1;i<=11;++i)birdLoads.push_back(Prepare{d->darkside.bird(i)});
  for(unsigned i=1;i<=4;++i)birdLoads.push_back(Prepare{d->darkside.drop(i)});
  birdLoads.push_back(Archive{XeenSceneArchive::DarksideProgramDark});
  for(auto r:{D::Screech,D::Disintegrate,D::Laugh})birdLoads.push_back(A{A::RetainDigital,name(r)});
  birdLoads.push_back(Archive{XeenSceneArchive::DarksideProgram});
  for(auto r:{D::Vine,D::Thud})birdLoads.push_back(A{A::RetainDigital,name(r)});
  add("bird background",std::move(birdLoads),[](const Context &)->Frame{return {Restore{"bird"},Level{0}};});
  fade("bird fade in",true);
  auto &bird=add("bird",{},[=](const Context &c)->Frame {
   Frame f{Restore{"bird"},Sprite{d->darkside.bird(c.iteration/8+1),c.iteration%8}};
   switch(cue(d->darkside.birdCues,c.iteration)){case 0xcff3:f.push_back(A{A::Digital,name(D::Comet)});f.push_back(A{A::Fm,{},43});break;
    case 0xd001:f.push_back(A{A::Fm,{},43});break;case 0xd00b:f.push_back(A{A::Digital,name(D::Comet)});break;
    case 0xd013:f.push_back(A{A::Digital,name(D::Screech)});break;case 0xd01a:f.push_back(A{A::Digital,name(D::Disintegrate)});break;case 0xd021:f.push_back(A{A::Digital,name(D::Laugh)});break;}return f;
  },81,2);bird.audioBeforeFlip=false;
  auto &fall=add("fall",{A{A::Fm,{},0},Raw{name(D::DropRaw)},Store{"drop"},Archive{XeenSceneArchive::DarksideProgramDark},A{A::RetainDigital,name(D::Branch)},Archive{XeenSceneArchive::DarksideProgram}},[=](const Context &c)->Frame {
   Frame f{Restore{"drop"},Sprite{d->darkside.drop(c.iteration/12+1),c.iteration%12}};
   if(c.iteration==9)f.push_back(A{A::Digital,name(D::Branch)});if(c.iteration==22)f.push_back(A{A::Digital,name(D::Vine)});return f;
  },39,2);fall.audioBeforeFlip=false;
  fade("fall fade out",false,24,{A{A::Digital,name(D::Thud)}});
  Frame endLoads{Raw{name(D::EndRaw)},Store{"end"}};for(auto r:{D::End1,D::End2,D::End3})endLoads.push_back(Prepare{name(r)});
  add("orb background",std::move(endLoads),[](const Context &)->Frame{return {Restore{"end"},Level{0}};});
  fade("orb fade in",true);
  auto &end=add("orb",{A{A::FadeSong,{},16}},[=](const Context &c)->Frame {
   Frame f{Restore{"end"},sprite(D::Resource(D::End1+c.iteration/8),c.iteration%8)};
   return f;
  },19,4);end.audioBeforeFlip=false;
  auto &titleHold=add("orb title hold",{Store{"end-title"},Prepare{name(D::EndTitle)}},[](const Context &)->Frame{return {Restore{"end-title"}};},1,20);
  titleHold.reset=R::Entry;titleHold.resetBeforeEnter=true;
  add("orb title",{},[=](const Context &c)->Frame {
   auto i=11-c.iteration;return {Restore{"end-title"},sprite(D::EndTitle,0,d->darkside.title[0][i],d->darkside.title[2][i],i),sprite(D::EndTitle,1,d->darkside.title[1][i],d->darkside.title[2][i],i)};
  },12,2);
 }
 add("seen point",{},[](const Context &)->Frame{return {};}).entering=[=](const Context &){observed->darksideSeenPoint=true;};
 fade("World transition",false);
 Frame world{Palette{name(D::FirePalette)},Raw{name(D::FireRaw)},Store{"world"}};
 for(auto r:{D::Explosion,D::Rumble,D::Thud})world.push_back(A{A::RetainDigital,name(r)});
 add("World background",std::move(world),[](const Context &)->Frame{return {Restore{"world"},Level{0}};}).input=I::Ignore;
 fade("World fade in",true);
 Frame fireLoads;for(unsigned i=1;i<=7;++i)fireLoads.push_back(Prepare{d->darkside.fire(i)});
 auto &worldHold=add("World first hold",std::move(fireLoads),[](const Context &)->Frame{return {Restore{"world"}};},1,10);
 worldHold.input=I::BoundaryHold;worldHold.reset=R::Entry;worldHold.resetBeforeEnter=true;
 auto &reveal=add("World reveal",{},[=](const Context &c)->Frame {
  Frame f{Restore{"world"},Sprite{d->darkside.fire(c.iteration/5+1),c.iteration%5,0,45}};
  if(c.iteration==17)f.push_back(A{A::Digital,name(D::Explosion)});
  else if(c.iteration<17 && !c.digitalBusy)f.push_back(A{A::Digital,name(D::Rumble)});return f;
 },28,2);reveal.input=I::Ignore;
 for(unsigned cycle=0;cycle<2;++cycle) {
  auto &flames=add("World flames "+std::to_string(cycle),cycle?Frame{}:Frame{Store{"revealed"}},[=](const Context &c)->Frame {
   Frame f{Restore{"revealed"},Sprite{d->darkside.fire(7),c.iteration,0,45}};
   if(c.iteration==0 || c.iteration==11)f.push_back(A{A::Digital,name(D::Thud)});
   if(c.iteration==3)f.push_back(A{A::Fm,{},60});return f;
  },21,2);flames.input=I::Ignore;
 }
 auto &worldFinal=add("World final hold",{},[](const Context &)->Frame{return {};},1,10);
 worldFinal.input=I::BoundaryHold;worldFinal.reset=R::Previous;
 scene.closing=scene.segments.size();
 add("cleanup",{A{A::Fm,{},0},A{A::StopDigital},A{A::FadeSong,{},0}},[](const Context &)->Frame{return {};}).input=I::Ignore;
 return scene;
}
}
