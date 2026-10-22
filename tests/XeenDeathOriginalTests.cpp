#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "formats/xeen/XeenVoc.h"
#include "app/XeenTitleFlow.h"
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include <iostream>
using namespace training_test;
namespace mmodern {
// Retained-state observations only. The sequence enters through original Rest
// condition death; duration overrides test the bounded no-sound-tail rule.
struct XeenRestTestAccess {
 using Phase=XeenEventFlow::DeathContinuation::Phase;
 static bool confirmRest(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Confirm;}
 static bool completeRest(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Complete;}
 static bool required(const XeenEventFlow &f){return f.partyDeathRequired();} static bool active(const XeenEventFlow &f){return f._death.has_value();}
 static auto phase(const XeenEventFlow &f){return f._death->phase;}
 static auto fire(const XeenEventFlow &f){return f._death->fire;}
 static auto laugh(const XeenEventFlow &f){return f._death->laugh;}
 static auto restarts(const XeenEventFlow &f){return f._death->restarts;}
 static auto scroll(const XeenEventFlow &f){return f._death->scroll;}
 static auto fade(const XeenEventFlow &f){return f._death->fadeLevel;}
 static auto deadline(const XeenEventFlow &f){return f._death->deadline;}
 static auto end(const XeenEventFlow &f){return f._death->laughEnd;}
 static const auto &background(const XeenEventFlow &f){return f._death->gameplay;}
 static const auto &final(const XeenEventFlow &f){return f._death->final;}
};
}
namespace {
using Access=XeenRestTestAccess;
using P=Access::Phase;
void wire(Fixture &f,std::uint64_t overrideDuration=0) {
 f.flow->loadSampleDuration=[&f,overrideDuration](const char *name,XeenSceneArchive archive){
  const auto duration=xeenVocDurationNanoseconds(f.in.assets.readArchiveResource(name,archive));
  return overrideDuration && std::string(name)=="xeenlaff.voc"?overrideDuration:duration;
 };
}
IndexedFrame scroll(const IndexedFrame &base,unsigned index,const XeenDialogSpriteDraw &draw) {
 auto frame=base;constexpr int l[]{29,23,15,-5,-11,-23,-49,-71};
 constexpr int r[]{165,171,198,218,228,245,264,281};
 const auto hand=[&](unsigned n,int x=0) {const auto name="hand"+std::string(n<10?"0":"")+std::to_string(n)+".vga";draw(frame,name.c_str(),0,x,0);};
 if(index>14){hand(14,l[index-15]);hand(15,r[index-15]);}else hand(index?index-1:0);
 if(index<=20){const auto n=index?index-1:0;const auto name="marb0"+std::to_string(n/5+1)+".vga";draw(frame,name.c_str(),n%5,0,0);}
 return frame;
}
XeenSaveSnapshot conditionDeathSource(Inputs &in) {
 auto source=in.service();source.journey->context->minutes=1439;
 for(auto owner:source.activeRosterIds)source.characters[owner].conditions[2]=255;
 for(auto &actor:source.map(28).monsters->slots){actor.x=actor.y=-128;actor.lifecycle=XeenActorLifecycle::Defeated;actor.accounted=true;}
 return source;
}void sequence(Inputs &in,bool longSample,bool skipScroll,bool efx=true) {
 auto source=conditionDeathSource(in);source.soundOptions.efx=efx;
 Fixture f(in,source);wire(f,longSample?10000000000ULL:0);
 f.flow->loadRestDream=[&]{return in.assets.restDreamImage();};
 f.act(RestAction{});check(Access::confirmRest(*f.flow),"Original all-member Rest danger confirmation");f.act(YesAction{});
 for(unsigned n=0;!Access::completeRest(*f.flow);++n){check(n<500,"Original condition-death Rest completion");f.now+=100;f.flow->beginCycle(++f.cycle);if(const auto next=f.flow->updatePresentation())f.present(*next);}
 unsigned sampleReads=0;const auto duration=f.flow->loadSampleDuration;
 f.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){++sampleReads;return duration(name,archive);};
 const auto random=f.w.sessionState().journeyRandom();const auto context=f.p.encounterContext;const auto food=f.p.food;
 f.act(AcknowledgeAction{});
 check(Access::active(*f.flow) && Access::phase(*f.flow)==P::Fire,"All noncombat terminal death routes enter original sequence");
 // The final scene is the completed fire (all seventeen pairs) plus pair 34/35,
 // never the gameplay with only the top pair (pin WorldOfXeenEngine::death).
 {
  const auto &draw=f.flow->drawDialogSprite;const auto &gameplay=Access::background(*f.flow);
  check(Access::final(*f.flow).pixels==drawXeenDeathFinal(drawXeenDeathFire(gameplay,34,draw),draw).pixels &&
   Access::final(*f.flow).pixels!=drawXeenDeathFinal(gameplay,draw).pixels,"Death final scene is built on the completed fire");
 }
 unsigned fires=0,finals=0,laughs=0,restores=0,scrolls=0,closed=0,fades=0,retries=0;
 bool sawRestart=false;std::uint64_t sampleEnd=0;
 const auto advance=[&](std::uint64_t ms,bool acquire=true) {
  f.now=ms;f.flow->beginCycle(++f.cycle);const auto next=f.flow->updatePresentation();if(next && acquire)f.present(*next);return next;
 };
 for(unsigned n=0;n<150 && !f.flow->partyDeathComplete();++n) {
  const auto phase=Access::phase(*f.flow);const auto input=f.flow->inputContext(f.flow->frame().presentation());
  check(!f.flow->canSave() && input.mainScreen==MainScreen::None && input.hideCursor,"Death keeps save/gameplay/cursor fence");
  check(f.w.sessionState().journeyRandom()==random && f.p.encounterContext==context && f.p.food==food,"Death presentation changed gameplay state/RNG");
  IndexedFrame expected;
  switch(phase) {
  case P::Fire:++fires;expected=drawXeenDeathFire(Access::background(*f.flow),Access::fire(*f.flow),f.flow->drawDialogSprite);break;
  case P::Final:++finals;expected=Access::final(*f.flow);break;
  case P::Laugh:
   ++laughs;expected=drawXeenDeathLaugh(Access::final(*f.flow),Access::laugh(*f.flow)?std::optional<unsigned>{Access::laugh(*f.flow)-1}:std::nullopt,f.flow->drawDialogSprite);
   check(Access::laugh(*f.flow)<=22,"Stored death1 frame22 must not execute");
   if(!sampleEnd)sampleEnd=Access::end(*f.flow);else check(sampleEnd==Access::end(*f.flow),"Laugh restart/repaint restarted sample interval");
   sawRestart=sawRestart || Access::restarts(*f.flow)==1;break;
  case P::Restore:++restores;expected=Access::final(*f.flow);break;
  case P::Scroll:++scrolls;expected=scroll(Access::final(*f.flow),Access::scroll(*f.flow),f.flow->drawDialogSprite);break;
  case P::Closed:++closed;expected=scroll(Access::final(*f.flow),0,f.flow->drawDialogSprite);break;
  case P::FadeOut:
   ++fades;expected=scroll(Access::final(*f.flow),0,f.flow->drawDialogSprite);
   for(auto &c:expected.palette)c=std::uint8_t((unsigned(c)*Access::fade(*f.flow)*2)>>8);break;
  case P::Done:throw std::runtime_error("Done acquired without loop exit");
  }
  check(expected.pixels==f.flow->frame().pixels && expected.palette==f.flow->frame().palette,"Original death resource/frame composition");
  const auto frame=f.flow->frame().presentation();const auto generation=f.flow->displayedInput();
  const auto deadline=Access::deadline(*f.flow);
  check(f.flow->nextPresentationDeadline()==deadline,"Death platform deadline mismatch");
  const auto now=PresentationClock::fromMilliseconds(f.now);
  const auto timing=phase==P::Fire || phase==P::FadeOut?PresentationClock::retraceAfter(now):
   phase==P::Laugh?PresentationClock::after(now,2):phase==P::Scroll?PresentationClock::after(now,1):now;
  check(deadline==timing,"Death fire/retrace, laughter2tick or scroll1tick timing");
  if(phase!=P::Scroll) {
   f.act(CancelInteractionAction{});f.act(AcknowledgeAction{});
   check(Access::phase(*f.flow)==phase,"Fire/laughter/fade must not have local Escape skip");
  } else if(skipScroll) {
   check(input.readyForAction && input.dialog->anyKey,"Original closing scroll must poll input");
   f.act(CancelInteractionAction{});check(Access::phase(*f.flow)==P::Closed,"Original scroll input closes to final hands");continue;
  }
  if(deadline && (deadline-1)/1000000>=f.now)check(!advance((deadline-1)/1000000),"Death stage advanced before deadline");
  if(phase==P::Fire && Access::fire(*f.flow)==12)f.flow->beforeEncounterFrameCopy=[&]{++retries;throw std::runtime_error("Synthetic death upload failure");};
  const auto due=PresentationClock::ceilMilliseconds(deadline);
  const auto next=advance(due,false);f.flow->beforeEncounterFrameCopy={};
  if(f.flow->partyDeathComplete()){check(!next,"Death completion invented a final sound tail/frame");break;}
  check(bool(next),"Death deadline did not publish next frame");
  const auto retainedPhase=Access::phase(*f.flow);const auto retainedFire=Access::fire(*f.flow);const auto retainedLaugh=Access::laugh(*f.flow);
  check(!advance(due,false) && Access::phase(*f.flow)==retainedPhase && Access::fire(*f.flow)==retainedFire && Access::laugh(*f.flow)==retainedLaugh,
   "Unacquired death frame advanced");
  f.present(*next);
  // The previous input origin cannot skip the new sequence frame.
  f.flow->handle(CancelInteractionAction{},generation,frame);
 }
 check(f.flow->partyDeathComplete() && fires==18 && finals==1 && restores==1 && closed==1 && fades==33 && retries==1,
  "Original death fire/final/restore/fade frame counts");
 check(efx?(sampleReads==2 && sawRestart && (longSample?laughs==34:laughs<34)):(sampleReads==0 && !sawRestart && laughs==1),"Original sample gate/restart/bounded laugh loop");
 check(scrolls==(skipScroll?1u:22u),"Original closing hand sequence count");
 if(longSample)check(PresentationClock::fromMilliseconds(f.now)<sampleEnd,"Bounded laugh loop invented a final sample-wait tail");
 std::cout<<"Clouds death fire="<<fires<<" laugh="<<laughs<<" scroll="<<scrolls<<" fade="<<fades<<" passed\n";
}
void refusal(Inputs &in) {
 Fixture f(in,in.service());wire(f);
 check(!Access::required(*f.flow) && !Access::active(*f.flow) && !f.flow->partyDeathComplete(),"Living party entered death sequence");
}
void applicationOutcome(Inputs &in) {
    mmodern::XeenProgramState programState;
 std::uint64_t now=0;XeenEventFlow *flow=nullptr;XeenSessionOutcome outcome;
 SdlWindow::FrameUpdateHandler retired;SdlWindow::IdleFrameHandler retiredIdle;
 XeenGameplayServices services{programState,in.resources(),{},in.mapLoader(),in.objectLoader(),
  [&](auto id){return in.texts.load(id);},in.font,
  [](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{frame(),false};},{},
  [&](XeenEventFlow &f,const auto &) {
   flow=&f;f.dialogText=&in.assets.uiText();
   f.drawDialogSprite=[&](auto &image,const char *name,unsigned index,int x,int y){in.assets.drawDialogSprite(image,name,index,x,y);};
   f.loadRestDream=[&]{return in.assets.restDreamImage();};
   f.loadSampleDuration=[&](const char *name,XeenSceneArchive archive){return xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,archive));};
  },
  [&](const IndexedFrame &first,const auto &handler,const auto &,const auto &idle,const auto &) {
   retired=handler;retiredIdle=idle;auto displayed=first;std::uint64_t cycle=0;
   const auto present=[&](const IndexedFrame &value){displayed=value;handler.framePresented(value.presentation());handler.completeInputHandoff(value.presentation());};
   const auto action=[&](const PlayerAction &a) {handler.beginCycle(++cycle);if(const auto value=handler.withPresentedInput(a,*handler.displayedInput(),displayed.presentation()))present(*value);};
   present(first);action(RestAction{});
   for(unsigned n=0;!handler.finished();++n) {
    check(n<1000,"Application death outcome bounded");
    if(Access::confirmRest(*flow)){action(YesAction{});continue;}
    if(Access::completeRest(*flow)){action(AcknowledgeAction{});continue;}
    now+=100;handler.beginCycle(++cycle);if(const auto value=idle())present(*value);
   }
   check(flow->partyDeathComplete(),"Application returned before acquiring death completion");
   return true;
  }};
 services.clock=[&]{return now;};services.outcome=&outcome;
 services.composeEncounter=[](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{frame(),false};};
 services.validateEncounterSprite=[](auto){};services.validateCombatSprite=[](auto){};
 services.restoreSnapshot=std::make_shared<const XeenSaveSnapshot>(conditionDeathSource(in));
 const auto status=Application(services.program).playGameplay(services,{},std::filesystem::path("synthetic-death-slot"),true);
 check(status==0 && outcome.kind==XeenSessionOutcome::Kind::Title,"Completed death must emit Title outcome instead of Exit/Load");
 check(retired.finished() && !retired.displayedInput() && !retiredIdle(),"Retired death callbacks touched destroyed gameplay owners");
 retired.closed();
}void titleReturn(Inputs &in) {
 const auto path=[](unsigned n){return std::filesystem::path("synthetic-slot-"+std::to_string(n));};
 auto services=XeenTitleFlow::original(in.assets,in.font,[]{return std::array<XeenSaveFile::Slot,10>{};},path);
 XeenTitleFlow title(services);title.newSlots();const auto stale=title.frame().presentation();title.returnToTitle();
 check(title.screen()==XeenTitleFlow::Screen::Background && !title.entry() && !title.acceptsInput(stale),"Death returns to original World title idle, not slots");
 title.presented(title.frame().presentation());title.completeInput(title.frame().presentation());
 check(!title.inputContext(title.frame().presentation()).readyForAction,"Death-return title initial fade missing");
 unsigned writes=1;std::uint64_t ns=0;title.animateAt(ns);
 for(unsigned n=0;n<32;++n) {
  ns=PresentationClock::retraceAfter(ns);check(bool(title.animateAt(ns)),"Death-return fade palette level missing");++writes;
  check(!title.animateAt(ns),"Unacquired fade advanced twice");
  title.presented(title.frame().presentation());title.completeInput(title.frame().presentation());
 }
 check(writes==33 && title.frame().pixels==services.background.pixels && !title.inputContext(title.frame().presentation()).readyForAction,
  "Death return retains all33 background fade levels before first idle poll");
 const auto first=title.animateAt(ns);check(first && first->pixels==services.animation[1].pixels,"First title phase must publish immediately after final fade acquisition");
 check(!title.animateAt(ns+1000000000),"Unacquired first title phase advanced during a host stall");
 title.presented(first->presentation());title.completeInput(first->presentation());
 check(title.inputContext(first->presentation()).readyForAction,"Title input poll missing after acquired first phase");
 const auto deadline=PresentationClock::after(ns,4);
 check(!title.animateAt(deadline-1),"First title phase lacks its original four-tick hold");
 const auto second=title.animateAt(deadline);check(second && second->pixels==services.animation[2].pixels,"First title phase skipped or gained an extra four-tick wait");
}
}
int main(int argc,char **argv){try {
 check(argc==2,"Death original usage: installation");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation absent");
 Inputs in(*installation);sequence(in,false,false);sequence(in,true,false);sequence(in,false,true);sequence(in,false,false,false);refusal(in);titleReturn(in);applicationOutcome(in);
 std::cout<<"Original death timing, restart, input, frame retry and title-return tests passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
