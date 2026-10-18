#include "games/xeen/XeenEventValues.h"
#include "XeenTestInstallation.h"
#include "formats/xeen/XeenVocDuration.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenObjectVisual.h"
#include "games/xeen/XeenStateEquality.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <queue>
#include <set>
#include <sstream>
namespace mmodern {
struct XeenCityEventTestAccess {
 static const XeenPartyState &visible(const XeenEventFlow &f){return f._transition?f._transition->party:f._party;}
 static const auto &visibleRandom(const XeenEventFlow &f){return f._transition?f._transition->world->sessionState().journeyRandom():f._world.sessionState().journeyRandom();}
 static void composer(XeenEventFlow &flow,XeenEventFlow::TransitionCompose compose){flow._transitionCompose=std::move(compose);}
 static std::uint64_t portraitDeadline(const XeenEventFlow &flow){return PresentationClock::ceilMilliseconds(flow._portraitSequence->deadline);}
 static std::uint64_t npcDeadline(const XeenEventFlow &flow){return PresentationClock::ceilMilliseconds(flow._npcSequence->deadline);}
 static bool handoff(const XeenEventFlow &flow){return flow._handoffPending;}
 static unsigned portraitFrame(const XeenEventFlow &flow){return flow._portraitSequence?flow._portraitSequence->frame:4;}
 static bool portrait(const XeenEventFlow &flow){return bool(flow._portraitSequence);}
 static auto portraitStage(const XeenEventFlow &flow){return flow._portraitSequence->effect.stage;}
 static unsigned countdown(const XeenEventFlow &flow){return flow._encounter->state().pending();}
 static void presenter(XeenEventFlow &flow,XeenEventFlow::EncounterCompose compose){flow._encounterCompose=std::move(compose);}
};
}
using namespace training_test;
namespace {
std::string stage="rules";
struct Fixture : training_test::Fixture {
 Fixture(Inputs &in,const XeenSaveSnapshot &source,bool animated=false,const XeenEventFile *city=nullptr):
  training_test::Fixture(in,source,animated,city,{},{},[&in](auto &f,auto portrait,auto phase){in.assets.drawNpc(f,portrait,phase);}) {
  flow->loadRestDream=[&in]{return in.assets.restDreamImage();};
  // These mechanics fixtures intentionally leave shop samples unavailable;
  // original Rest sequences use their actual archive/header durations.
  flow->loadSampleDuration=[&in](const char *name,XeenSceneArchive archive)->std::uint64_t {
   if(std::string(name)!="dreams2.voc" && std::string(name)!="laff1.voc")return 0;
   return xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,archive));
  };
  flow->drawGuildArt=[&in](auto &image){in.assets.drawGuild(image);};
  flow->drawSmithArt=[&in](auto &image){in.assets.drawSmith(image);};
  flow->drawTavernArt=[&in](auto &image){in.assets.drawTavern(image);};
  flow->tavernFoodOffer=[&in]{return in.assets.tavernFoodOffer();};
 }
 void act(PlayerAction action) {
  for(unsigned n=0;n<100 && !flow->inputContext(flow->frame().presentation()).readyForAction;++n) {
   now+=125;flow->beginCycle(++cycle);
   if(flow->inputContext(flow->frame().presentation()).readyForAction)break;
   if(auto next=flow->updatePresentation())present(*next);
  }
  training_test::Fixture::act(std::move(action));
 }
};
XeenEventRecord record(unsigned line,unsigned opcode,std::vector<std::uint8_t> params={}) {
 XeenEventRecord r;r.x=20;r.y=20;r.direction=4;r.line=line;r.opcode=opcode;
 r.parameters=std::move(params);r.lengthField=5+r.parameters.size();r.fileOffset=line*16;return r;
}
XeenSaveSnapshot at(Inputs &in,int x,int y,XeenDirection direction=XeenDirection::North) {
 auto s=in.service();s.camera={28,x,y,direction};
 return s;
}
void settle(Fixture &f,const std::function<void()> &beforeInput={}) {
 for(unsigned n=0;n<10000 && !f.flow->canSave();++n) {
  f.now+=125;f.flow->beginCycle(++f.cycle);
  if(beforeInput)beforeInput();
  if(const auto combat=f.flow->encounter()->combat()) {
   if(combat->phase()==XeenCombatPhase::Defeat || combat->phase()==XeenCombatPhase::Failed ||
    combat->phase()==XeenCombatPhase::SupportStopped)throw std::runtime_error("Original route combat cannot continue: "+f.flow->encounter()->notice());
   if(combat->phase()==XeenCombatPhase::PlayerReady)f.act(AttackAction{});
  } else {
   const auto input=f.flow->inputContext(f.flow->frame().presentation());
   const auto pending=f.flow->presentationGeneration()?std::optional<XeenEventExecutionState>{XeenTrainingTestAccess::serviceState(*f.flow)}:std::nullopt;
   if(input.readyForAction && ((pending && pending->pendingPresentation->request.response==XeenPresentationResponseRequirement::YesNo) ||
    (input.dialog && !input.dialog->anyKey && input.dialog->key('y') && input.dialog->key('n'))))f.act(YesAction{});
   else if(input.readyForAction && ((input.dialog && input.dialog->anyKey) || f.w.sessionState().journeyActivity()==XeenJourneyActivity::Event ||
     f.w.sessionState().journeyActivity()==XeenJourneyActivity::Reward))f.act(AcknowledgeAction{});
  }
  if(auto next=f.flow->updatePresentation())f.present(*next);
 }
 if(!f.flow->canSave())throw std::runtime_error("Event settlement bound: "+f.flow->encounter()->notice());
}
void interact(Fixture &f,bool select=false) {
 f.act(InteractionAction{});
 if(select) {
  for(unsigned n=0;n<100 && !f.flow->canCancelInteraction() && !f.flow->canSave();++n) {
   f.now+=125;f.flow->beginCycle(++f.cycle);
   const auto input=f.flow->inputContext(f.flow->frame().presentation());
   if(input.readyForAction && input.dialog && input.dialog->anyKey)f.act(AcknowledgeAction{});
   if(auto next=f.flow->updatePresentation())f.present(*next);
  }
  if(f.flow->canCancelInteraction())f.act(SelectMemberAction{0});
 }
 settle(f);
}
void portraitRewards(Inputs &in) {
 stage="M59 reward portrait sparkle";
 struct Grant {unsigned mode,value;bool give,all,sparkle;};
 for(const auto g:std::vector<Grant>{{13,4,true,false,true},{15,0,true,false,true},
   {16,100,true,true,true},{16,0,true,true,true},{15,8,true,true,false},
   {13,4,false,false,false},{15,6,false,false,false},{34,25,true,true,true}}) {
  auto source=at(in,20,20);const unsigned selected=3,owner=source.activeRosterIds[selected];
  XeenWorld geometry(in.mapLoader());const auto cell=geometry.sampleCell(28,20,20);
  for(unsigned d=0;d<4;++d){const auto wall=wallAt(*cell->cell,XeenDirection(d));if(wall!=1 && wall!=6 && wall!=9 && wall!=13){source.camera.direction=XeenDirection(d);break;}}
  std::vector<std::uint8_t> pair{std::uint8_t(g.mode),std::uint8_t(g.value)};
  if(g.mode==16 || g.mode==34)for(unsigned shift:{8u,16u,24u})pair.push_back(std::uint8_t(g.value>>shift));
  std::vector<std::uint8_t> params=g.give?std::vector<std::uint8_t>{0,0}:pair;
  if(g.give)params.insert(params.end(),pair.begin(),pair.end());else {params.push_back(0);params.push_back(0);}
  XeenEventFile city{28,"maze0028.evt",true,{record(0,0x20,{0,0}),
   record(1,g.all?0x0f:0,g.all?std::vector<std::uint8_t>{0}:std::vector<std::uint8_t>{}),record(2,0x0c,params),record(3,0x12)}};
  Fixture f(in,source,false,&city);const auto before=XeenSaveFormat::encode(f.snapshot());
  unsigned frames=0;std::uint32_t owners=0;f.flow->drawDialogSprite=[&](auto &image,const char *name,unsigned frame,int x,int y){
   if(std::string(name)=="spellfx.icn"){
    check(g.sparkle && y==150,"Unexpected reward sparkle");frames|=1u<<frame;
    constexpr int positions[]{10,45,81,117,153,189};const auto found=std::find(std::begin(positions),std::end(positions),x);
    check(found!=std::end(positions),"Reward sparkle has no native portrait slot");owners|=1u<<source.activeRosterIds[found-std::begin(positions)];
   }
   in.assets.drawDialogSprite(image,name,frame,x,y);
  };
  f.act(InteractionAction{});const auto staleInput=f.flow->displayedInput();const auto staleFrame=f.flow->frame().presentation();
  f.act(CancelInteractionAction{});settle(f);f.flow->handle(SelectMemberAction{selected},staleInput,staleFrame);
  check(!frames && !f.w.scenePresentation().feedbackActive() && before==XeenSaveFormat::encode(f.snapshot()),"Cancelled/stale reward started sparkle");
  unsigned attempts=0;
  if(g.mode==16 && g.value==100)XeenCityEventTestAccess::composer(*f.flow,[&](auto &,const auto &,const auto &,auto,auto){
   if(!attempts++){
    for(auto id:source.activeRosterIds)check(XeenCityEventTestAccess::visible(*f.flow).roster.combatInputs(id)->experience==source.journey->supplements[id].inputs.experience+100 &&
     f.p.roster.combatInputs(id)->experience==source.journey->supplements[id].inputs.experience,
     "Final composition preceded the original recipient XP/FX prefixes");
    throw std::runtime_error("Reward sparkle publication retry");
   }
   return XeenEventFlow::Composition{frame(),false};
  });
  f.act(InteractionAction{});f.act(SelectMemberAction{selected});settle(f);
  std::uint32_t expectedOwners=0;
  // A party gold grant ends the member loop after its first success (0x277f9..0x2790d), so one member sparkles.
  if(g.sparkle)for(auto id:source.activeRosterIds)if(g.mode==34?id==source.activeRosterIds.front():g.all || id==owner)expectedOwners|=1u<<id;
  check(owners==expectedOwners,"Reward sparkle used the wrong retained roster recipients");
  for(const auto &effect:f.w.scenePresentation().portraits)check(effect.spellFrame==4,"Quiet boundary preceded reward sparkle completion");
  const auto saved=f.snapshot();Fixture restored(in,saved,false,&city);
  check(!restored.w.scenePresentation().feedbackActive() && XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(restored.snapshot()),"Loading replayed reward sparkle or changed save");
  const auto effects=f.w.scenePresentation().portraits;
  f.act(AcknowledgeAction{});f.act(AcknowledgeAction{});
  for(unsigned id=0;id<30;++id)check(f.w.scenePresentation().portraits[id].spellDeadline==effects[id].spellDeadline,"Repeated acknowledgment restarted sparkle");
  for(unsigned n=0;n<4;++n){f.now+=110;f.flow->beginCycle(++f.cycle);if(auto image=f.flow->updatePresentation())f.present(*image);}
  check(!f.w.scenePresentation().feedbackActive() && frames==(g.sparkle?15u:0u),"Reward did not draw the original four sparkle frames exactly through completion");
 }
}
void npcBeforeDeadline(Inputs &in) {
 stage="Part B NPC idle before its deadline";
 auto source=at(in,8,18);source.gameFlags[9]=false;Fixture f(in,source);
 f.act(InteractionAction{});check(f.flow->presentationGeneration().has_value(),"Original NPC did not suspend");
 const auto frame=f.flow->frame().presentation();const auto input=f.flow->displayedInput();
 const auto random=f.w.sessionState().journeyRandom();const auto deadline=XeenCityEventTestAccess::npcDeadline(*f.flow);
 f.now=deadline-1;f.flow->beginCycle(++f.cycle);
 check(!f.flow->updatePresentation(),"Pre-deadline NPC poll invented a draw");
 check(!XeenCityEventTestAccess::handoff(*f.flow) && f.flow->acceptsFrame(frame) && f.flow->displayedInput()==input &&
  f.w.sessionState().journeyRandom()==random,"No-frame NPC poll invalidated acquired input lease or consumed random");
 settle(f);check(f.flow->canSave(),"NPC stopped accepting the acquired input after a no-frame poll");
}
void originalPortraitOrder(Inputs &in) {
 stage="Part B original synchronous portrait effects";
 auto source=at(in,20,20);
 for(auto id:source.activeRosterIds)source.characters[id].skills[4]=0;
 XeenEventFile city{28,"maze0028.evt",true,{record(0,0x0f,{0}),record(1,0x0c,{0,0,13,4}),record(2,0x12)}};
 Fixture f(in,source,false,&city);std::vector<std::pair<unsigned,unsigned>> frames;
 constexpr int positions[]{10,45,81,117,153,189};
 f.flow->drawDialogSprite=[&](auto &image,const char *name,unsigned frameIndex,int x,int y) {
  if(std::string(name)=="spellfx.icn") {
   const auto found=std::find(std::begin(positions),std::end(positions),x);check(found!=std::end(positions) && y==150,"Sparkle portrait position");
   const unsigned member=found-std::begin(positions);
   for(unsigned n=0;n<6;++n)check(XeenCityEventTestAccess::visible(*f.flow).roster.at(source.activeRosterIds[n]).skills[4]==unsigned(n<=member) && f.p.roster.at(source.activeRosterIds[n]).skills[4]==0,"Later recipient grant preceded earlier synchronous FX or leaked the staged prefix");
   const auto value=std::make_pair(member,frameIndex);if(frames.empty() || frames.back()!=value)frames.push_back(value);
  }
  in.assets.drawDialogSprite(image,name,frameIndex,x,y);
 };
 f.act(InteractionAction{});check(XeenCityEventTestAccess::portrait(*f.flow),"Grant omitted original synchronous FX");
 const auto countdown=XeenCityEventTestAccess::countdown(*f.flow);
 check(!f.flow->canSave() && !f.flow->respond(*f.flow->presentationGeneration(),XeenPresentationResponse::Presented),"External response skipped sparkle");
 // Escape and redraw cannot progress the helper, and later recipients remain untouched.
 f.present(f.flow->handle(DialogKeyAction{InputKey::Escape},f.flow->displayedInput(),f.flow->frame().presentation()));
 check(XeenCityEventTestAccess::portraitFrame(*f.flow)==0,"Escape skipped sparkle");
 f.now=XeenCityEventTestAccess::portraitDeadline(*f.flow);f.flow->beginCycle(++f.cycle);auto held=f.flow->updatePresentation();check(bool(held),"Two-tick sparkle draw absent");
 check(XeenCityEventTestAccess::portraitFrame(*f.flow)==1,"Sparkle advanced more than one frame");
 f.now+=1000;f.flow->beginCycle(++f.cycle);check(!f.flow->updatePresentation(),"Unacquired sparkle frame advanced");
 f.present(*held);settle(f);
 check(frames.size()==24,"Sequential sparkle frame count");
 for(unsigned n=0;n<24;++n)check(frames[n]==std::make_pair(n/4,n%4),"Sparkles did not serialize recipients 0..3");
 check(XeenCityEventTestAccess::countdown(*f.flow)==countdown && !f.w.scenePresentation().feedbackActive(),"Sparkle changed suspended actor countdown or survived quiet boundary");
 // The original sparkle latch (DAT 0x210bc/0x2117f) skips a member already
 // sparkled in this Event run; only Event start/end clear it (0x267a6/0x291e0).
 XeenEventFile repeated{28,"maze0028.evt",true,{record(0,0x0f,{0}),
  record(1,0x0c,{0,0,16,1,0,0,0}),record(2,0x0c,{0,0,16,2,0,0,0}),record(3,0x0c,{0,0,15,6}),record(4,0x12)}};
 Fixture twice(in,source,false,&repeated);frames.clear();
 twice.flow->drawDialogSprite=[&](auto &image,const char *name,unsigned frameIndex,int x,int y) {
  if(std::string(name)=="spellfx.icn") {
   const auto found=std::find(std::begin(positions),std::end(positions),x);
   check(found!=std::end(positions) && y==150,"Repeated grant portrait position");
   const auto value=std::make_pair(unsigned(found-std::begin(positions)),frameIndex);
   if(frames.empty() || frames.back()!=value)frames.push_back(value);
  }
  in.assets.drawDialogSprite(image,name,frameIndex,x,y);
 };
 twice.act(InteractionAction{});settle(twice);
 check(frames.size()==24,"A later grant in the same Event sparkled a recipient again");
 for(unsigned n=0;n<24;++n)check(frames[n]==std::make_pair(n/4,n%4),"Repeated grants lost recipient/frame order");
 for(auto owner:source.activeRosterIds)check(twice.p.roster.combatInputs(owner)->experience==source.journey->supplements[owner].inputs.experience+3 &&
  twice.p.roster.at(owner).awards[6]==1,"Latched grants were not applied");
 frames.clear();twice.act(InteractionAction{});settle(twice);
 check(frames.size()==24,"A new Event run did not clear the sparkle latch");
 // The generic Event path draws each overlay before its HP store, then has
 // exactly one two-tick final hold shared by all six recipients (DAT 0x23f73).
 XeenEventFile hurt{28,"maze0028.evt",true,{record(0,0x0f,{0}),record(1,0x16,{1,0,0}),record(2,0x12)}};
 Fixture d(in,source,false,&hurt);std::vector<unsigned> overlays;
 d.flow->drawDialogSprite=[&](auto &image,const char *name,unsigned frameIndex,int x,int y) {
  if(std::string(name)=="charpow.icn") {
   const unsigned member=std::find(std::begin(positions),std::end(positions),x)-std::begin(positions);
   check(member<6 && y==150,"Damage portrait position");
   if(overlays.empty() || overlays.back()!=member)overlays.push_back(member);
  }
  in.assets.drawDialogSprite(image,name,frameIndex,x,y);
 };
 d.act(InteractionAction{});
 const auto retained=XeenTrainingTestAccess::serviceState(*d.flow);
 XeenEventContinuation bound(retained,true);auto tampered=retained;
 tampered.portraitDamage->injuryAcknowledged=true;
 save_test::rejects([&]{bound.check(tampered);});bound.check(retained);
 tampered=retained;tampered.portraitProgress->next=99;
 save_test::rejects([&]{bound.check(tampered);});bound.check(retained);
 for(unsigned member=0;member<6;++member) {
  check(XeenCityEventTestAccess::portraitStage(*d.flow)==XeenEventPortraitStage::DamageOverlay,"Per-recipient damage introduced a timed hold");
  for(unsigned n=0;n<6;++n)check(XeenCityEventTestAccess::visible(*d.flow).roster.at(source.activeRosterIds[n]).currentHp==source.characters[source.activeRosterIds[n]].currentHp-int(n<member) &&
   d.p.roster.at(source.activeRosterIds[n]).currentHp==source.characters[source.activeRosterIds[n]].currentHp,"Event HP preceded its overlay or leaked the staged prefix");
  d.flow->beginCycle(++d.cycle);const auto image=d.flow->updatePresentation();check(bool(image),"Acquired damage overlay did not resume injury");d.present(*image);
 }
 check(XeenCityEventTestAccess::portraitStage(*d.flow)==XeenEventPortraitStage::DamageHold,"Generic damage omitted shared final hold");
 check(!d.flow->canSave(),"Damage became quiet before final hold");
 const auto finalDeadline=XeenCityEventTestAccess::portraitDeadline(*d.flow);
 d.now=finalDeadline-1;d.flow->beginCycle(++d.cycle);check(!d.flow->updatePresentation(),"Damage final hold was shorter than two ticks");
 d.now=finalDeadline;d.flow->beginCycle(++d.cycle);if(auto image=d.flow->updatePresentation())d.present(*image);settle(d);
 for(auto id:source.activeRosterIds)check(d.p.roster.at(id).currentHp==source.characters[id].currentHp-1,"Damage recipient was lost or applied twice");
}
void partA(Inputs &in) {
 stage="M59 Part A staged rules";
 auto source=at(in,20,20);XeenWorld query(in.mapLoader());const auto tile=query.sampleCell(28,20,20);
 for(unsigned d=0;d<4;++d) {const auto wall=wallAt(*tile->cell,XeenDirection(d));if(wall!=1 && wall!=6 && wall!=9 && wall!=13){source.camera.direction=XeenDirection(d);break;}}
 const auto owner=source.activeRosterIds[1];
 source.characters[owner].skills[0]=0;
 source.journey->context->effects[0]=1;source.journey->context->effects[3]=1;source.journey->context->lightAndResistances[2]=13;
 source.journey->context->lightAndResistances[1]=17;source.journey->context->newDay=true;
 source.characters[owner].temporaryLevel=2;source.characters[owner].temporaryAge=3;
 source.characters[owner].intellect.temporary=9;source.journey->supplements[owner].inputs.might.temporary=7;
 for(int flag:{55,155,222,231})source.gameFlags[flag]=true;
 auto arrival=record(0,0x0c,{0,0,103,24});arrival.x=27;arrival.y=11;
 XeenEventFile script{28,"maze0028.evt",true,{record(0,0x24,{0,1}),record(1,0x20,{0,0}),
  record(2,0x0a,{102,0,8}),record(3,0x0c,{0,0,78,0}),record(4,0x0c,{0,0,103,23}),record(5,0x12),
  record(8,0x0c,{0,0,85,1}),record(9,0x0f,{0}),record(10,0x0c,{0,0,15,8}),record(11,0x1f,{28,27,11}),arrival}};
 Fixture cancelled(in,source,false,&script);const auto before=XeenSaveFormat::encode(cancelled.snapshot());
 std::vector<unsigned> headFrames;cancelled.flow->drawDialogSprite=[&](auto &,const auto &name,unsigned frame,int,int){if(std::string(name)=="border.icn")headFrames.push_back(frame);};
 cancelled.act(InteractionAction{});check(!cancelled.flow->canSave(),"Saving during staged heads/selection");
 check(headFrames.size()>=2 && headFrames[headFrames.size()-2]>=4 && headFrames[headFrames.size()-2]<=7 && headFrames.back()==headFrames[headFrames.size()-2]+8,"WhoWill did not present candidate HED indicators");
 const unsigned initialHead=headFrames[headFrames.size()-2];
 headFrames.clear();cancelled.now+=110;cancelled.flow->beginCycle(++cancelled.cycle);if(auto next=cancelled.flow->updatePresentation())cancelled.present(*next);
 check(headFrames.size()>=2 && headFrames[headFrames.size()-2]==4+(initialHead-4+1)%4 && headFrames.back()==headFrames[headFrames.size()-2]+8,"WhoWill HED animation lost its ordinary redraw");
 const auto oldFrame=cancelled.flow->frame().presentation();const auto oldInput=cancelled.flow->displayedInput();
 cancelled.act(CancelInteractionAction{});
 check(before==XeenSaveFormat::encode(cancelled.snapshot()),"Cancel published heads/calendar");
 cancelled.flow->handle(SelectMemberAction{1},oldInput,oldFrame);
 check(before==XeenSaveFormat::encode(cancelled.snapshot()),"Stale selection committed cancelled effects");
 XeenEventFile resetHeads{28,"maze0028.evt",true,{record(0,0x0c,{0,0,85,1}),record(1,0x24,{0,0}),record(2,0x20,{0,0}),record(3,0x12)}};
 Fixture resetPresentation(in,source,false,&resetHeads);unsigned renderedHeads=0;
 resetPresentation.flow->drawDialogSprite=[&](auto &,const auto &name,unsigned,int,int){if(std::string(name)=="border.icn")++renderedHeads;};
 resetPresentation.act(InteractionAction{});check(renderedHeads==0,"Year-reset dialog used live clairvoyance instead of staged context");
 resetPresentation.act(CancelInteractionAction{});check(before==XeenSaveFormat::encode(resetPresentation.snapshot()),"Cancelled year reset published");
 for(unsigned mode:{0u,1u}) {
  XeenEventFile confirmation{28,"maze0028.evt",true,{record(0,0x24,{0,1}),record(1,0x20,{0,0}),record(2,0x09,{44,std::uint8_t(mode),3}),record(3,0x12)}};
  Fixture dialog(in,source,false,&confirmation);unsigned heads=0;
  dialog.flow->drawDialogSprite=[&](auto &,const auto &name,unsigned,int,int){if(std::string(name)=="border.icn")++heads;};
  dialog.act(InteractionAction{});heads=0;dialog.act(SelectMemberAction{1});
  check(mode==0?heads>=2:heads==0,"YesNo versus acknowledgment HED activation differs from reference");
  settle(dialog);
 }
 Fixture jail(in,source,false,&script);const auto rng=jail.w.sessionState().journeyRandom();const auto economy=jail.p.serviceEconomy;
 const auto clock=*jail.p.encounterContext;const auto age=XeenCharacterRules::sheetAge(jail.p.roster.at(owner),{clock.year});
 jail.act(InteractionAction{});jail.act(SelectMemberAction{1});settle(jail);
 check(jail.c.x==27 && jail.c.y==11 && jail.c.direction==source.camera.direction && jail.p.worldFlags->values[24],"Teleport continuation did not restart destination or preserve facing");
 check(jail.p.encounterContext->year==clock.year+1 && jail.p.encounterContext->day==clock.day &&
  jail.p.encounterContext->minutes==clock.minutes+1 && jail.p.encounterContext->ctr24==(clock.ctr24+1)%24 &&
  !jail.p.encounterContext->newDay && !jail.p.encounterContext->rested && jail.p.encounterContext->lightAndResistances[1]==17 &&
  jail.p.encounterContext->effects[3]==0 && jail.p.encounterContext->lightAndResistances[2]==0 && jail.p.serviceEconomy==economy && jail.w.sessionState().journeyRandom()==rng,
  "Direct year jump simulated time/economy/RNG or lost retained clock fields");
 check(XeenCharacterRules::sheetAge(jail.p.roster.at(owner),{jail.p.encounterContext->year})==age+1 &&
  jail.p.roster.at(owner).temporaryLevel==0 && jail.p.roster.at(owner).temporaryAge==3 &&
  jail.p.roster.at(owner).intellect.temporary==0 && jail.p.roster.combatInputs(owner)->might.temporary==0,"Year age/temporary reset");
 for(int flag:{55,155,222,231})check(!jail.f.isSet(flag),"Yearly game flag survived");
 for(auto id:jail.p.party.activeRosterIds())check(jail.p.roster.at(id).awards[8]==1,"Jail active-party award missing");
 check(!jail.p.roster.at(29).awards[8],"Jail awarded inactive roster");
 check(jail.w.headIndicators({28,4,4,XeenDirection::North})==XeenHeadIndicators{0,1},"Primary HED megamap alias lost");
 const auto saved=jail.snapshot();Fixture restored(in,saved,false,&script);
 check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(saved),"Current-format jail/heads/flags/RNG round-trip");
 const auto once=XeenSaveFormat::encode(jail.snapshot());jail.act(AcknowledgeAction{});jail.act(AcknowledgeAction{});
 check(XeenSaveFormat::encode(jail.snapshot())==once,"Repeated acknowledgment reapplied jail year");

 Fixture yearRetry(in,source,false,&script);unsigned yearAttempts=0;
 XeenCityEventTestAccess::composer(*yearRetry.flow,[&](auto &,const auto &,const auto &,auto,auto) {
  if(++yearAttempts==1)throw std::runtime_error("Year composition retry");
  return XeenEventFlow::Composition{frame(),false};
 });
 yearRetry.act(InteractionAction{});yearRetry.act(SelectMemberAction{1});
 check(!yearRetry.flow->canSave() && yearRetry.p.encounterContext->year==clock.year,"Failed year composition published");
 settle(yearRetry);check(yearAttempts==2 && XeenSaveFormat::encode(yearRetry.snapshot())==once,"Year retry repeated or lost staged changes");
 const auto timeBefore=*restored.p.encounterContext;XeenConsequenceCharacters timeCharacters{};XeenConsequenceInputs timeInputs{};
 for(unsigned i=0;i<restored.p.party.size();++i) {const auto id=restored.p.party.activeRosterIds()[i];timeCharacters[i]=restored.p.roster.at(id);timeInputs[i]=*restored.p.roster.combatInputs(id);}
 XeenConditionTimeCandidate timeOracle(timeBefore,1,timeCharacters,timeInputs,&*restored.p.serviceEconomy);
 XeenCombatRandom timeRandom(*restored.w.sessionState().journeyRandom());
 for(;;){XeenConsequenceDraw draw{timeRandom,64,{}};if(timeOracle.service(draw))break;}
 timeOracle.context.ctr24=(timeBefore.ctr24+1)%24;
 restored.act(WaitAction{});settle(restored);
 check(restored.p.encounterContext==std::optional<XeenGameplayContext>{timeOracle.context} && restored.p.serviceEconomy==timeOracle.economy &&
  restored.w.sessionState().journeyRandom()==timeRandom.continuation(),"Post-jail ordinary time/economy/RNG processing");
 for(unsigned field=0;field<3;++field) {
  Fixture tampered(in,source,false,&script);bool injected=false;
  XeenCityEventTestAccess::composer(*tampered.flow,[&](auto &world,const auto &party,const auto &,auto,auto) {
   injected=true;
   if(field==0) {auto &flag=const_cast<XeenPartyState &>(party).worldFlags->values[23];const bool value=flag;flag=!value;flag=value;}
   else if(field==1) {auto &head=const_cast<XeenHeadCache &>(*world.sessionState().headCache()).overrides[68]->left;const auto value=std::uint8_t(head);head=2;head=value;}
   else {auto &heads=const_cast<XeenMap &>(world.map(28)).heads;const auto saved=*heads;heads.reset();heads.emplace(saved);}
   return XeenEventFlow::Composition{frame(),false};
  });
  tampered.act(InteractionAction{});save_test::rejects([&]{tampered.act(SelectMemberAction{1});settle(tampered);});
  check(injected && !tampered.flow->canSave() && tampered.p.encounterContext->year==clock.year && !tampered.w.sessionState().headCache(),"Head/flag write-restore escaped publication guard");
 }

 auto success=source;success.characters[owner].skills[0]=1;success.characters[owner].permanentLevel=10;success.characters[owner].currentHp=1;
 Fixture heal(in,success,false,&script);const auto max=XeenCharacterRules::maxHp(heal.p.roster.at(owner),{clock.year});
 heal.act(InteractionAction{});heal.act(SelectMemberAction{1});settle(heal);
 check(heal.p.roster.at(owner).currentHp==max && heal.p.worldFlags->values[23] && heal.c.x==20 && heal.p.encounterContext->minutes==success.journey->context->minutes+1 && heal.p.encounterContext->ctr24==(success.journey->context->ctr24+1)%24 && heal.w.sessionState().journeyRandom()==rng,"Selected Thievery/heal introduced rolls/year");
 for(unsigned i=0;i<heal.p.party.size();++i)if(i!=1)check(heal.p.roster.at(heal.p.party.activeRosterIds()[i]).currentHp==success.characters[success.activeRosterIds[i]].currentHp,"Heal affected another member");
 // Typed damage is checked against the shared combat candidate, including RNG.
 XeenEventFile damageScript{28,"maze0028.evt",true,{record(0,0x20,{0,0}),record(1,0x16,{5,0,5}),record(2,0x0c,{0,0,103,23}),record(3,0x12)}};
 auto damageSource=source;damageSource.characters[owner].currentHp=2;damageSource.journey->context->lightAndResistances[2]=0;
 Fixture damage(in,damageSource,false,&damageScript);XeenConsequenceCharacters characters{};XeenConsequenceInputs inputs{};
 for(unsigned n=0;n<damage.p.party.size();++n) {const auto id=damage.p.party.activeRosterIds()[n];characters[n]=damage.p.roster.at(id);inputs[n]=*damage.p.roster.combatInputs(id);}
 XeenDamageProtection protection;for(unsigned n=0;n<4;++n)protection.resistances[n]=damage.p.encounterContext->lightAndResistances[n+2];
 protection.powerShield=damage.p.encounterContext->effects[6];
 XeenTypedDamageCandidate oracle(characters,inputs,5,XeenDamageType::Poison,clock.year,2,protection);XeenCombatRandom random(*damage.w.sessionState().journeyRandom());
 for(;;){XeenConsequenceDraw draw{random,64,{}};if(oracle.service(draw))break;}
 XeenConditionTimeCandidate damageSpace(*damage.p.encounterContext,1,oracle.characters,inputs,&*damage.p.serviceEconomy);
 XeenCombatRandom damageEnding(random.continuation());
 for(;;){XeenConsequenceDraw draw{damageEnding,64,{}};if(damageSpace.service(draw))break;}
 damage.act(InteractionAction{});damage.act(SelectMemberAction{1});settle(damage);
 check(damage.p.roster.at(owner).currentHp==damageSpace.characters[1].currentHp && damage.p.roster.at(owner).conditions==damageSpace.characters[1].conditions &&
  damage.p.roster.at(owner).conditions[3]==characters[1].conditions[3] && damage.w.sessionState().journeyRandom()==damageEnding.continuation() && damage.p.worldFlags->values[23],"Typed Event damage/resistance/death/RNG differs from combat plus its Space charge");

 Fixture damageRetry(in,damageSource,false,&damageScript);unsigned damageAttempts=0;
 XeenCityEventTestAccess::composer(*damageRetry.flow,[&](auto &,const auto &,const auto &,auto,auto) {
  if(++damageAttempts==1)throw std::runtime_error("Damage composition retry");
  return XeenEventFlow::Composition{frame(),false};
 });
 damageRetry.act(InteractionAction{});damageRetry.act(SelectMemberAction{1});
 check(!damageRetry.flow->canSave() && XeenCityEventTestAccess::visibleRandom(*damageRetry.flow)==random.continuation() && damageRetry.w.sessionState().journeyRandom()==rng && damageRetry.p.roster.at(owner).currentHp==2,"Damage overlay must retain its save RNG before HP injury without publishing live state");
 settle(damageRetry);check(damageAttempts==2 && XeenSaveFormat::encode(damageRetry.snapshot())==XeenSaveFormat::encode(damage.snapshot()),"Damage retry consumed a second draw or reapplied damage");

}

void rules(Inputs &in) {
 auto s=at(in,20,20);
 XeenWorld query(in.mapLoader());const auto tile=query.sampleCell(28,20,20);
 for(unsigned d=0;d<4;++d) {const auto wall=wallAt(*tile->cell,XeenDirection(d));
  if(wall!=1 && wall!=6 && wall!=9 && wall!=13){s.camera.direction=XeenDirection(d);break;}}
 XeenEventFile city{28,"maze0028.evt",true,{record(0,0x0b,{0,20,20}),record(1,0x0c,{0,0,16,0x88,0x13,0,0}),
  record(2,0x20,{0,0}),record(3,0x0f,{0}),record(4,0x0c,{0,0,15,73}),
  record(5,0x14,{34,0xa0,0x0f,0,0,35,50,0,0,0}),record(6,0x0c,{0,0,104,29}),record(7,0x12)}};
 Fixture f(in,s,false,&city);const auto before=XeenSaveFormat::encode(f.snapshot());
 f.act(InteractionAction{});const auto selector=f.flow->frame().presentation();const auto selectorInput=f.flow->displayedInput();
 f.act(CancelInteractionAction{});
 auto cancelledPrefix=XeenSaveFormat::decode(before);
 cancelledPrefix.journey->supplements[s.activeRosterIds[0]].inputs.experience+=5000;
 const auto cancelledBytes=XeenSaveFormat::encode(cancelledPrefix);
 check(f.flow->canSave() && cancelledBytes==XeenSaveFormat::encode(f.snapshot()),"Cancellation lost observed XP prefix or published later rewards/object");
 f.flow->handle(SelectMemberAction{0},selectorInput,selector);
 check(cancelledBytes==XeenSaveFormat::encode(f.snapshot()),"Retired selector published staged rewards");
 const auto rng=f.w.sessionState().journeyRandom();
 const auto xp=f.p.roster.combatInputs(0)->experience;
 const auto gold=f.p.monsterTreasure->gold,gems=f.p.monsterTreasure->gems;
 interact(f,true);
 check(f.p.roster.combatInputs(0)->experience==std::uint32_t(xp+5000) && f.p.monsterTreasure->gold==gold+4000 &&
  f.p.monsterTreasure->gems==gems+50 && f.p.questFlags.isSet(29) && f.w.sessionState().journeyRandom()==rng,
  "Detached fixed rewards lost XP/purse/quest/RNG ordering");
 for(auto owner:f.p.party.activeRosterIds())check(f.p.roster.at(owner).awards[126]==1,"SetChar/award remapping lost party member");
 check(!f.p.roster.at(29).awards[126] && f.w.effectiveObject({28,0}).x==20,"Inactive award owner or MoveObj identity changed");
 const auto saved=f.snapshot();Fixture restored(in,saved,false,&city);
 check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(saved),"World reward exact round-trip");
 auto turned=saved;auto &moved=turned.map(28).objects.front();check(moved.record==0 && moved.x==20,"MoveObj record retained");moved.flipped=!moved.flipped;Fixture turnedRestore(in,turned,false,&city);
 check(XeenSaveFormat::encode(turnedRestore.snapshot())==XeenSaveFormat::encode(turned),"Loading reset a retained object flip");
 f.w.discardMapCache();check(f.w.effectiveObject({28,0}).x==20,"Map cache discarded durable object move");
 for(unsigned field=0;field<8;++field) {
  Fixture tampered(in,s,false,&city);bool injected=false;
  std::optional<XeenPartyState> authorizedPrefix;
  std::optional<XeenJourneyRandomState> prefixRandom;
  XeenCityEventTestAccess::composer(*tampered.flow,[&](auto &world,const auto &party,const auto &,auto,auto) {
   // The final destination composer follows all original reward/FX prefixes.
   // Retain that independently copied live value, not the private final candidate.
   check(!world.sessionState().objectOverrides().empty(),"Final candidate lost its staged MoveObj");
   authorizedPrefix.emplace();world.copyEventParty(tampered.p,*authorizedPrefix);
   prefixRandom=tampered.w.sessionState().journeyRandom();
   check(!tampered.p.questFlags.isSet(29) && party.questFlags.isSet(29) && tampered.p.monsterTreasure->gold==gold &&
    tampered.w.sessionState().objectOverrides().empty(),"Pre-composer reward prefix crossed currency/world settlement");
   injected=true;auto &p=const_cast<XeenPartyState &>(party);
   // Write/restore attacks must remain observable even with identical values.
   if(field==0){auto &a=p.roster.at(0).awards[6];const auto before=std::uint8_t(a);a=1;a=before;}
   if(field==1){auto &g=p.monsterTreasure->gold;const auto before=std::uint32_t(g);++g;g=before;}
   if(field==2){auto &o=const_cast<XeenObjectOverride &>([&]()->const XeenObjectOverride & {
    for(const auto &entry:world.sessionState().mapStates())if(!entry.second.objectOverrides.empty())return entry.second.objectOverrides.front();
    throw std::logic_error("No live object override");}());const auto x=int(o.x);o.x=0;o.x=x;}
   if(field==3){auto &c=p.roster.at(0).currentHp;const auto hp=int(c);++c;c=hp;}
   if(field==4){auto &xp=const_cast<XeenCombatInputs &>(*p.roster.combatInputs(0)).experience;const auto value=std::uint32_t(xp);++xp;xp=value;}
   if(field==5){auto &id=p.roster.at(0).weapons[0].id;const auto value=std::uint8_t(id);id=2;id=value;}
   if(field==7){auto &skill=p.roster.at(0).skills[4];const auto value=std::uint8_t(skill);++skill;skill=value;}
   if(field==6){auto &rng=const_cast<XeenJourneyRandomState &>(*world.sessionState().journeyRandom());const auto value=std::uint32_t(rng.state);++rng.state;rng.state=value;}
   return XeenEventFlow::Composition{frame(),false};
  });
  tampered.act(InteractionAction{});save_test::rejects([&]{tampered.act(SelectMemberAction{0});settle(tampered);});
  check(injected && authorizedPrefix && !tampered.flow->canSave() &&
   tampered.p.questFlags.values()==authorizedPrefix->questFlags.values() && tampered.p.questItems.counts()==authorizedPrefix->questItems.counts() &&
   tampered.p.monsterTreasure==authorizedPrefix->monsterTreasure && tampered.p.encounterContext==authorizedPrefix->encounterContext &&
   tampered.p.worldFlags==authorizedPrefix->worldFlags && tampered.p.serviceEconomy==authorizedPrefix->serviceEconomy &&
   tampered.p.food==authorizedPrefix->food && tampered.p.party.activeRosterIds()==authorizedPrefix->party.activeRosterIds() &&
   tampered.w.sessionState().journeyRandom()==prefixRandom && tampered.w.sessionState().objectOverrides().empty(),
   "Stale candidate changed its already authorized prefix or published final world/currency");
  for(unsigned id=0;id<30;++id)check(xeen_state::sameCharacter(tampered.p.roster.at(id),authorizedPrefix->roster.at(id)) &&
   xeen_state::sameInputs(*tampered.p.roster.combatInputs(id),*authorizedPrefix->roster.combatInputs(id)),
   "Stale candidate changed an authorized character/XP prefix");
 }
 Fixture retry(in,s,false,&city);unsigned attempts=0;
 XeenCityEventTestAccess::composer(*retry.flow,[&](auto &,const auto &,const auto &,auto,auto) {
  if(++attempts==1)throw std::runtime_error("Synthetic recoverable presentation failure");
  return XeenEventFlow::Composition{frame(),false};
 });
 retry.act(InteractionAction{});retry.act(SelectMemberAction{0});
 for(unsigned n=0;!attempts && n<8;++n)retry.act(AcknowledgeAction{});
 check(attempts==1,"Final composition retry fixture did not reach its retained failure boundary");
 check(!retry.flow->canSave() && retry.p.monsterTreasure->gold==gold && !retry.p.questFlags.isSet(29) && XeenCityEventTestAccess::visible(*retry.flow).questFlags.isSet(29) && retry.w.sessionState().objectOverrides().empty(),"Failed final presentation lost its retained prefix or published currency/world");
 settle(retry);check(attempts==2 && retry.p.monsterTreasure->gold==gold+4000 &&
  retry.p.roster.combatInputs(0)->experience==s.journey->supplements[0].inputs.experience+5000,"Presentation retry repeated or lost fixed rewards");
 const auto once=XeenSaveFormat::encode(retry.snapshot());retry.act(AcknowledgeAction{});retry.act(AcknowledgeAction{});
 check(once==XeenSaveFormat::encode(retry.snapshot()),"Repeated receipt regranted fixed rewards");
 // Unsupported dependencies in called scripts must refuse the whole group.
 for(const auto suffix:std::vector<XeenEventRecord>{record(9,0x14,{67,1,0}),record(9,0x0c,{34,0,0,0,0,0,0}),
   record(9,0x0c,{0,0,82,5}),record(9,0x0c,{20,0,0,0}),record(9,0x0f,{8}),record(9,0x0b,{255,0,0})}) {
  city.records={record(0,0x0c,{0,0,15,6}),record(1,0x0b,{0,20,20}),record(2,0x19,{20,20,9}),record(3,0x12),suffix,record(10,0x1a)};
  Fixture refused(in,s,false,&city);const auto preimage=XeenSaveFormat::encode(refused.snapshot());
  interact(refused);check(preimage==XeenSaveFormat::encode(refused.snapshot()),"Unsupported dependent call published prefix");
 }
 auto candidate=f.w.transitionCandidate();
 XeenPartyState grants;candidate->copyEventParty(f.p,grants);
 const auto selected=grants.party.activeRosterIds()[0],other=grants.party.activeRosterIds()[1];
 XeenGameFlags grantFlags;XeenCamera grantCamera=f.c;XeenEventScriptValues grantScript;
 const XeenEventValueOwners owners{grants,grantFlags,grantCamera,grantScript};
 // Award 81 is byte 63's high nibble: a grant ORs 1, a removal ORs 0xf into the low nibble.
 grants.roster.at(selected).awards[127]=14;
 xeenEventGive(owners,15,81,0);
 check(grants.roster.at(selected).awards[127]==15 && grants.roster.at(other).awards[127]==f.p.roster.at(other).awards[127],"Selected award count/remapping failed");
 xeenEventTake(owners,15,81,0);
 check(grants.roster.at(selected).awards[127]==15 && grants.roster.at(selected).awards[63]==15,"Award removal did not follow the original nibble defect");
 const auto beforeXp=std::uint32_t(grants.roster.combatInputs(selected)->experience);
 xeenEventGive(owners,16,0xffffffffu,0);
 check(grants.roster.combatInputs(selected)->experience==std::uint32_t(beforeXp-1),"XP arithmetic did not wrap as uint32");
 const auto beforeGold=std::uint32_t(grants.monsterTreasure->gold);
 xeenEventGive(owners,34,0xffffffffu,0);
 check(grants.monsterTreasure->gold==std::uint32_t(beforeGold-1),"Purse arithmetic did not wrap as uint32");
 grants.roster.at(selected).skills[4]=254;
 for(const auto expected:{255,0}) {
  xeenEventGive(owners,13,4,0);
  check(grants.roster.at(selected).skills[4]==expected,"Generic skill grant did not wrap its byte");
  auto skillSave=f.snapshot();skillSave.characters[selected].skills=grants.roster.at(selected).skills;
  Fixture restoredSkill(in,XeenSaveFormat::decode(XeenSaveFormat::encode(skillSave)),false,&city);
  check(restoredSkill.p.roster.at(selected).skills[4]==expected,"Skill byte changed on exact restore");
 }
 xeenEventGive(owners,13,4,0);
 xeenEventTake(owners,13,4,0);
 check(!grants.roster.at(selected).skills[4] && grants.roster.at(29).skills==f.p.roster.at(29).skills,"Generic skill removal changed inactive owner");
 const auto original=candidate->effectiveObject({28,0});
 candidate->applyMoveObject({28,0},original.x,original.y);const bool flip=candidate->effectiveObjectFlip({28,0});
 candidate->applyMoveObject({28,0},original.x,original.y);
 check(candidate->effectiveObjectFlip({28,0})!=flip,"Same-position MoveObj did not toggle flip");
 candidate->applyMoveObject({28,0},-128,-1);check(candidate->isObjectDisabled({28,0}),"Signed off-map object stayed active");
 const auto resolver=XeenObjectVisualResolver::load(in.assets);
 XeenIndoorScene().build(*candidate,s.camera,&resolver,nullptr,0);
 check(candidate->effectiveObjectFlip({28,0})==resolver.resolve(candidate->objectFile(28),0,s.camera.direction,0).horizontalFlip,
  "Scene draw did not apply reference flip reset to off-screen object");
 auto malformed=saved;malformed.map(28).objects.back().record=65535; // Beyond the MOB records.
 save_test::rejects([&]{Fixture invalid(in,malformed,false,&city);});
}
void teacherPrompt(Fixture &f) {
 f.act(InteractionAction{});
 for(unsigned n=0;n<20 && !f.flow->canCancelInteraction() && !f.flow->canSave();++n) {
  f.now+=125;f.flow->beginCycle(++f.cycle);if(auto image=f.flow->updatePresentation())f.present(*image);
 }
}
bool npcRandomOnly(const XeenJourneyRandomState &before,const XeenJourneyRandomState &after) {
 if(after.count<before.count || after.count-before.count>10000)return false;
 XeenCombatRandom expected(before);
 while(expected.continuation().count<after.count)if(!expected.draw(0,3))return false;
 return expected.continuation()==after;
}
bool sameExceptNpcRandom(const std::vector<std::uint8_t> &bytes,XeenSaveSnapshot after) {
 const auto before=XeenSaveFormat::decode(bytes);
 if(!npcRandomOnly(*before.journey->random,*after.journey->random))return false;
 after.journey->random=before.journey->random;
 return bytes==XeenSaveFormat::encode(after);
}
bool sameAfterSpace(Inputs &in,const std::vector<std::uint8_t> &bytes,const XeenSaveSnapshot &after,bool npc=false) {
 Fixture expected(in,XeenSaveFormat::decode(bytes));expected.act(WaitAction{});settle(expected);
 const auto prepared=XeenSaveFormat::encode(expected.snapshot());
 return npc?sameExceptNpcRandom(prepared,after):prepared==XeenSaveFormat::encode(after);
}
void teachers(Inputs &in) {
 stage="M58 original teachers";
 for(const auto teacher:std::array<std::array<unsigned,4>,2>{{{{8,16,4,100}},{{25,26,11,2000}}}}) {
  const auto x=teacher[0],y=teacher[1],skill=teacher[2],price=teacher[3];
  auto source=at(in,x,y);const unsigned selected=3,owner=source.activeRosterIds.at(selected);
  source.characters[owner].skills[skill]=0;source.journey->treasure->gold=price*3;
   if(skill==4) {save_test::clearExplored(source);for(auto &c:source.characters)c.skills[4]=0;}
  Fixture f(in,source);const auto before=XeenSaveFormat::encode(f.snapshot());
  const auto rng=f.w.sessionState().journeyRandom();const auto time=f.p.encounterContext;
  teacherPrompt(f);
  check(f.flow->canCancelInteraction() && f.flow->presenter().hasNpcOffer(),"Teacher mode2 did not continue directly to WhoWill");
  const auto pending=XeenTrainingTestAccess::serviceState(*f.flow);
  check(pending.pendingPresentation->request.verbIndex==6 && pending.pendingPresentation->request.textIndex==(skill==4?54:43),"Original teacher WhoWill text/verb changed");
  const auto deadline=f.flow->presenter().npcTiming().deadline;
  f.now=deadline;f.flow->beginCycle(++f.cycle);if(auto image=f.flow->updatePresentation())f.present(*image);
  check(f.flow->presenter().npcTiming().deadline>deadline && npcRandomOnly(*rng,*f.w.sessionState().journeyRandom()),"Teacher WhoWill stopped portrait animation or changed gameplay RNG");
  const auto staleInput=f.flow->displayedInput();const auto staleFrame=f.flow->frame().presentation();
  f.act(CancelInteractionAction{});check(f.flow->canSave() && sameExceptNpcRandom(before,f.snapshot()),"Cancelled teacher published state");
  f.flow->handle(SelectMemberAction{selected},staleInput,staleFrame);
  check(sameExceptNpcRandom(before,f.snapshot()),"Retired teacher selection spent gold or taught");
  for(unsigned lesson=1;lesson<=2;++lesson) {
   teacherPrompt(f);check(f.flow->canCancelInteraction(),"Repeated teacher WhoWill unavailable");
   f.act(SelectMemberAction{selected});settle(f);
   check(f.w.scenePresentation().portraits[owner].spellFrame==4,"Original teacher did not sparkle selected portrait");
   check(f.p.roster.at(owner).skills[skill]==lesson && f.p.monsterTreasure->gold==price*(3-lesson),"Original paid lesson debit/increment changed");
   check(npcRandomOnly(*rng,*f.w.sessionState().journeyRandom()) && f.p.encounterContext->minutes==time->minutes+lesson && f.p.encounterContext->ctr24==(time->ctr24+lesson)%24,"Teacher changed RNG or omitted the completed Space suffix");
   check(f.p.roster.at(owner).currentHp==source.characters[owner].currentHp &&
    f.p.roster.at(owner).currentSp==source.characters[owner].currentSp &&
    f.p.roster.at(owner).permanentLevel==source.characters[owner].permanentLevel,"Learning refilled HP/SP or trained a level");
   for(auto other:f.p.party.activeRosterIds())if(other!=owner)
    check(f.p.roster.at(other).skills[skill]==source.characters[other].skills[skill],"Teacher taught unselected owner");
  }
  const auto saved=f.snapshot();Fixture reload(in,saved);
  check(XeenSaveFormat::encode(reload.snapshot())==XeenSaveFormat::encode(saved),"Teacher exact reload changed skill/purse");
   if(skill==4) {
    const auto taughtCell=reload.w.sampleCell(28,x,y);
    check(taughtCell && taughtCell->cell->stepped && save_test::exploredTiles(saved).size()==1,
     "Completed teacher Space records only the occupied physical cell");
    reload.act(NavigationAction::TurnRight);settle(reload);
    const auto occupied=reload.w.sampleCell(28,x,y);
    check(occupied && occupied->cell->stepped && reload.w.sessionState().exploredTiles().size()==1,
     "Later stepped action after Mylo does not reveal another physical cell");
   }
  for(const auto gold:{price-1,price}) {
   auto boundary=source;boundary.journey->treasure->gold=gold;Fixture b(in,boundary);
   const auto preimage=XeenSaveFormat::encode(b.snapshot());
   teacherPrompt(b);b.act(SelectMemberAction{selected});settle(b);
   check(b.p.roster.at(owner).skills[skill]==(gold==price?1:0) && b.p.monsterTreasure->gold==(gold==price?0:gold),"Teacher gold boundary changed");
   if(gold<price) {
    Fixture expected(in,XeenSaveFormat::decode(preimage));expected.act(WaitAction{});settle(expected);
    check(sameExceptNpcRandom(XeenSaveFormat::encode(expected.snapshot()),b.snapshot()),"Insufficient lesson changed more than the completed Space suffix");
   }
  }
  auto blocked=in.city;
  // Skill 18 is past the skill table; every skill inside it is granted.
  for(auto &r:blocked.records)if(r.x==x && r.y==y && r.line==6)r.parameters.back()=18;
  Fixture unsupported(in,source,false,&blocked);const auto unchanged=XeenSaveFormat::encode(unsupported.snapshot());
  interact(unsupported);
  check(sameExceptNpcRandom(unchanged,unsupported.snapshot()) && unsupported.flow->canSave(),"Unsupported dependent skill spent gold or changed world/time/RNG");
 }
}
void originalVisualRegressions(Inputs &in) {
 stage="original wall at 12,20 north";
 XeenWorld world(in.mapLoader());const auto walls=XeenIndoorScene().build(world,{28,12,20,XeenDirection::North});
 const auto hasOrder=[&](int order){return std::any_of(walls.begin(),walls.end(),[&](const auto &c){return c.originalOrder==order;});};
 check(hasOrder(120) && hasOrder(107),"Original front wall incorrectly hid the adjacent left side wall");
 stage="original proof bottom window after recomposition";
 Fixture proof(in,at(in,9,22),true);proof.act(InteractionAction{});proof.act(SelectMemberAction{0});
 std::optional<XeenPresentationRequest> bottom;
 for(unsigned n=0;n<20 && proof.flow->presentationGeneration();++n) {
  const auto state=XeenTrainingTestAccess::serviceState(*proof.flow);
  if(state.pendingPresentation && state.pendingPresentation->request.kind==XeenPresentationKind::BottomWindowMessage) {
   bottom=state.pendingPresentation->request;break;
  }
  proof.act(AcknowledgeAction{});
 }
 check(bottom.has_value(),"Original proof bottom window absent");
 XeenEventPresenter oracle(in.font);const auto expected=oracle.present(frame(),*bottom).frame;
 const auto intact=[&](const IndexedFrame &image) {
  for(int y=143;y<199;++y)for(int x=0;x<320;++x)
   if(image.pixels[y*320+x]!=expected.pixels[y*320+x])return false;
  return true;
 };
 check(intact(proof.flow->frame()),"Original proof bottom window initially obscured");
 proof.present(proof.flow->refresh(false));
 check(intact(proof.flow->frame()),"Party redraw obscured the bottom window during recomposition");
 proof.now+=500;proof.prepare();
 check(intact(proof.flow->frame()),"Idle redraw obscured the bottom window");
 settle(proof);check(!intact(proof.flow->frame()),"Acknowledgment failed to restore the party panel");
}
void originalEvents(Inputs &in) {
 // Original records, synthetic placement/flag fixtures: isolate each script branch.
 stage="proof";auto s=at(in,9,22);Fixture proof(in,s);const auto rng=proof.w.sessionState().journeyRandom();
 proof.act(InteractionAction{});check(proof.flow->canCancelInteraction(),"Proof WhoWill is absent");
 proof.act(CancelInteractionAction{});check(!proof.f.isSet(9),"Cancelled proof changed flag 9");
 interact(proof,true);check(proof.f.isSet(9) && proof.w.effectiveObject({28,0}).x==9 && proof.w.effectiveObject({28,0}).y==22 &&
  proof.w.sessionState().journeyRandom()==rng,"Original proof flag/object or fixed RNG diverged");
 stage="Gunther";auto next=proof.snapshot();next.camera={28,14,5,XeenDirection::West};Fixture gunther(in,next);
 const auto gold=gunther.p.monsterTreasure->gold,gems=gunther.p.monsterTreasure->gems;
 std::array<std::uint32_t,30> xp{};for(unsigned owner=0;owner<30;++owner)xp[owner]=gunther.p.roster.combatInputs(owner)->experience;
 // DOS capture: one sparkle pass over portraits 1-6 (XP, line 15), then the
 // "+ 5,000 experience" side-panel text (line 18); award 6 (line 20) is latched.
 std::vector<std::string> sequence;
 gunther.flow->drawDialogSprite=[&](auto &image,const char *name,unsigned frameIndex,int x,int y) {
  if(std::string(name)=="spellfx.icn") {
   constexpr int positions[]{10,45,81,117,153,189};
   const auto found=std::find(std::begin(positions),std::end(positions),x);check(found!=std::end(positions) && y==150,"Gunther sparkle position");
   const auto entry="S"+std::to_string(found-std::begin(positions))+std::to_string(frameIndex);
   if(sequence.empty() || sequence.back()!=entry)sequence.push_back(entry);
  }
  in.assets.drawDialogSprite(image,name,frameIndex,x,y);
 };
 gunther.flow->reportManual=[&](const auto &result){if(const auto *pending=std::get_if<XeenEventExecutionSuspended>(&result))
  if(pending->request.textIndex==59)sequence.push_back((pending->request.kind==XeenPresentationKind::CenteredMessage?"T":"?")+pending->request.text);};
 interact(gunther);
 check(gunther.p.party.size()==6 && sequence.size()==25,"Gunther reward did not sparkle exactly once per member before its text");
 for(unsigned n=0;n<24;++n)check(sequence[n]=="S"+std::to_string(n/4)+std::to_string(n%4),"Gunther sparkle order");
 check(sequence[24]=="T+ 5,000 experience","Gunther experience text is not the side-panel window 38 message");
 check(gunther.f.isSet(10) && gunther.p.monsterTreasure->gold==gold+4000 && gunther.p.monsterTreasure->gems==gems+50 &&
  !gunther.p.questFlags.isSet(29) && gunther.p.questFlags.isSet(1),"Original Gunther rewards/next quest diverged");
 for(auto owner:gunther.p.party.activeRosterIds())check(gunther.p.roster.at(owner).awards[6]==1 &&
  gunther.p.roster.combatInputs(owner)->experience==xp[owner]+5000,"Original Gunther XP/award lost member");
 const auto awarded=gunther.snapshot();interact(gunther);
 check(sameAfterSpace(in,XeenSaveFormat::encode(awarded),gunther.snapshot(),true),"Repeated Gunther regranted reward or erased next quest");
 Fixture reload(in,awarded);interact(reload);
 check(sameAfterSpace(in,XeenSaveFormat::encode(awarded),reload.snapshot(),true),"Reload replayed Gunther rewards");
 for(const auto xy:std::vector<std::pair<int,int>>{{8,18},{11,18}}) {
  auto saved=awarded;saved.camera={28,xy.first,xy.second,XeenDirection::West};Fixture joe(in,saved);
  bool absent=false;const auto text=in.texts.load(28).strings[xy.first==8?49:15];
  joe.flow->reportManual=[&](const auto &result){if(const auto *pending=std::get_if<XeenEventExecutionSuspended>(&result))
   if(pending->request.text==text && pending->request.kind!=XeenPresentationKind::NpcAcknowledgment)absent=true;};
  interact(joe);check(absent && joe.f.isSet(9) && joe.flow->canSave(),"Joe absent/closed branch failed");
 }
 auto first=at(in,14,5,XeenDirection::West);Fixture request(in,first);interact(request);
 check(request.p.questFlags.isSet(29) && !request.f.isSet(10),"Gunther first-talk quest flag missing");
}
bool sameItem(XeenItem a,XeenItem b) {
 return std::tie(a.material,a.id,a.state,a.frame)==std::tie(b.material,b.id,b.state,b.frame);
}
unsigned itemCount(const XeenPartyState &);
// Review finding 1: GiveMulti draws from the one shared Journey stream
// (0x23786..0x237e3, helper 0x45b9a), like SelectRandomChar (0x287d8..0x287eb)
// and every other Event draw; no reward generator outlives its instruction.
struct RandomRun {XeenJourneyRandomState random;std::uint32_t gold=0,gems=0;bool quiet=false;};
RandomRun randomRun(Inputs &in,std::vector<XeenEventRecord> records,XeenJourneyRandomState start) {
 auto source=at(in,20,20);source.journey->random=start;
 XeenEventFile city{28,"maze0028.evt",true,std::move(records)};
 Fixture f(in,source,false,&city);f.act(InteractionAction{});settle(f);
 return {*f.w.sessionState().journeyRandom(),std::uint32_t(f.p.monsterTreasure->gold),std::uint32_t(f.p.monsterTreasure->gems),f.flow->canSave()};
}
XeenJourneyRandomState afterDraws(XeenJourneyRandomState state,const std::vector<std::pair<std::uint32_t,std::uint32_t>> &draws) {
 XeenCombatRandom random(state);
 for(const auto &d:draws)while(!random.draw(d.first,d.second)){}
 return random.continuation();
}
void rewardRandomOwnership(Inputs &in) {
 stage="GiveMulti shared random stream";
 const XeenJourneyRandomState seed{1,7,0};
 const auto base=at(in,20,20);
 const auto gems=std::uint32_t(base.journey->treasure->gems),gold=std::uint32_t(base.journey->treasure->gold);
 check(base.activeRosterIds.size()==6,"Shared stream regression needs six members");
 const auto gems1=record(0,0x14,{101,1,0});
 // GiveMulti(101,1), SelectRandomChar, GiveMulti(101,1), Exit: three draws in order.
 {
  const auto run=randomRun(in,{gems1,record(1,0x2b),record(2,0x14,{101,1,0}),record(3,0x12)},seed);
  check(run.quiet && run.gems==gems+2,"Interleaved GiveMulti rewards differ");
  check(run.random==afterDraws(seed,{{1,1},{1,6},{1,1}}) && run.random.count==3,"GiveMulti reused a stale reward generator");
 }
 // The same ownership for every other Event draw between two rewards. Each middle
 // instruction is measured alone from the state after the first reward.
 const auto first=afterDraws(seed,{{1,1}});
 using Middle=XeenEventRecord(*)(unsigned);
 for(const Middle middle:std::vector<Middle>{
   [](unsigned line){return record(line,0x17,{4,1,std::uint8_t(line+1)});}, // JumpRnd to the next line either way
   [](unsigned line){return record(line,0x21,{0,5});},                         // RndDamage
   [](unsigned line){return record(line,0x0f,{7});},                           // SetChar 7
   [](unsigned line){return record(line,0x0c,{0,0,100,3,0,0,0});}}) {         // give 100: a take/give draw
  const auto alone=randomRun(in,{middle(0),record(1,0x12)},first);
  const auto run=randomRun(in,{gems1,middle(1),record(2,0x14,{101,1,0}),record(3,0x12)},seed);
  check(alone.quiet && run.quiet && alone.random.count>first.count,"Middle Event draw did not run");
  check(run.random==afterDraws(alone.random,{{1,1}}) && run.random.count==alone.random.count+1 && run.gems==gems+2,
   "A later GiveMulti overwrote or repeated an intervening Journey draw");
 }
 // A suspension during item generation keeps its generator; the next draws follow it.
 {
  std::vector<XeenEventRecord> records;
  for(unsigned line=0;line<21;++line)records.push_back(record(line,0x14,{100,1,0,0,0,100,1,0,0,0,100,1,0,0,0}));
  records.push_back(record(21,0x14,{66,7,0,0,0,0}));records.push_back(record(22,0x2b));
  records.push_back(record(23,0x14,{101,1,0}));records.push_back(record(24,0x12));
  XeenCombatRandom expected(seed);XeenConsequenceDraw many{expected,256,{}};
  for(unsigned i=0;i<63;++i)check(many.draw(1,1)==1,"Fixed random-gold oracle");
  XeenItemCandidate item(7);check(item.service(many),"Bounded item oracle");
  const auto run=randomRun(in,records,seed);
  check(run.quiet && run.gold==gold+63 && run.gems==gems+1 &&
   run.random==afterDraws(expected.continuation(),{{1,6},{1,1}}),"Suspended item generation lost its order with later draws");
 }
 // Review finding 3: the currency and food draws take the operand's low word
 // (0x23770..0x237de). 65537 draws 1..1 once; 0xffff draws 1..1 (signed word divisor).
 {
  const auto run=randomRun(in,{record(0,0x14,{100,1,0,1,0}),record(1,0x12)},seed);
  check(run.quiet && run.gold==gold+1 && run.random==afterDraws(seed,{{1,1}}) && run.random.count==1,"GiveMulti 100 used the decoded width");
  const auto high=randomRun(in,{record(0,0x14,{101,0xff,0xff}),record(1,0x12)},seed);
  check(high.quiet && high.gems==gems+1 && high.random==afterDraws(seed,{{1,1}}),"GiveMulti 101 bit-15 word differs");
  const auto half=randomRun(in,{record(0,0x14,{101,0x00,0x80}),record(1,0x12)},seed);
  check(half.quiet && half.random==afterDraws(seed,{{1,0x8000}}),"GiveMulti 101 0x8000 word differs");
  for(const auto &zero:std::vector<XeenEventRecord>{record(0,0x14,{100,0,0,1,0}),record(0,0x14,{101,0,0})}) {
   const auto refused=randomRun(in,{zero,record(1,0x12)},seed);
   check(refused.quiet && refused.random==seed && refused.gold==gold && refused.gems==gems,"A zero word divisor was not refused before drawing");
  }
 }
}
void treasure(Inputs &in) {
 stage="shared Event treasure";
 auto s=at(in,20,20);
 for(auto owner:s.activeRosterIds) {
  s.characters[owner].weapons={};s.characters[owner].armor={};
  s.characters[owner].accessories={};s.characters[owner].miscellaneous={};
 }
 XeenEventFile city{28,"maze0028.evt",true,{record(0,0x20,{0,0}),record(1,0x0b,{8,20,20}),
  record(2,0x14,{66,2,100,10,0,0,0,35,5,0}),record(3,0x0c,{0,0,20,199}),record(4,0x12)}};
 Fixture oracle(in,s,false,&city);XeenCombatRandom random(*oracle.w.sessionState().journeyRandom());
 XeenItemCandidate generated(2);XeenConsequenceDraw draw{random,64,{}};
 check(generated.service(draw),"Treasure oracle incomplete");const auto gold=draw.draw(1,10);check(bool(gold),"Gold oracle rejected");
 for(bool full:{false,true}) {
  auto source=s;
  if(full)for(auto owner:source.activeRosterIds)for(unsigned c=0;c<4;++c)
   xeenRewardItems(source.characters[owner],XeenRewardCategory(c)).fill({1,1,0,0});
  Fixture f(in,source,false,&city);const auto before=XeenSaveFormat::encode(f.snapshot());
  const auto oldGold=std::uint32_t(f.p.monsterTreasure->gold),oldGems=std::uint32_t(f.p.monsterTreasure->gems);
  f.act(InteractionAction{});const auto selector=f.flow->frame().presentation();const auto input=f.flow->displayedInput();
  unsigned presentationAttempts=0;
  XeenCityEventTestAccess::presenter(*f.flow,[&](auto,auto) {
   if(f.flow->presentationGeneration()) {
    const auto state=XeenTrainingTestAccess::serviceState(*f.flow);
    if(state.pendingPresentation->request.kind==XeenPresentationKind::RewardReceipt && ++presentationAttempts==1 && !full)
     throw std::runtime_error("Recoverable receipt presentation failure");
   }
   return XeenEventFlow::Composition{frame(),false};
  });
  unsigned reports=0;
  f.flow->reportManual=[&](const auto &result) {
   if(const auto *pending=std::get_if<XeenEventExecutionSuspended>(&result))
    if(full && pending->request.kind==XeenPresentationKind::RewardReceipt && ++reports==1)
     throw std::runtime_error("Recoverable receipt reporting failure");
  };
  f.act(SelectMemberAction{1});
  check(!f.flow->canSave() && !XeenSaveState::canCapture(f.p,f.c,f.w) &&
   f.p.monsterTreasure->gold==oldGold && f.p.monsterTreasure->gems==oldGems &&
   !f.f.isSet(199) && f.w.sessionState().objectOverrides().empty() &&
   XeenCityEventTestAccess::visibleRandom(*f.flow)==random.continuation(),"Reward prefix lost its retained generation RNG or published final world/currency early");
  unsigned attempts=0;
  XeenCityEventTestAccess::composer(*f.flow,[&](auto &,const auto &,const auto &,auto,auto) {
   if(++attempts==1)throw std::runtime_error("Recoverable generated reward composition failure");
   return XeenEventFlow::Composition{frame(),false};
  });
  bool finalSeen=false;
  settle(f,[&] {
   if(!f.flow->presentationGeneration())return;
   check(f.p.monsterTreasure->gold==oldGold && f.p.monsterTreasure->gems==oldGems,"Every Event treasure page keeps live currency private until final acknowledgment");
   const auto state=XeenTrainingTestAccess::serviceState(*f.flow);
   if(state.rewardPhase==XeenRewardPhase::Receipt && state.pendingPresentation->request.kind==XeenPresentationKind::RewardReceipt &&
    f.flow->presenter().blocksGameplay() && f.flow->presenter().pageIndex()+1==f.flow->presenter().pageCount())finalSeen=true;
  });
  check(finalSeen,"Generated Event reward reaches a separate final acknowledgment");
  check(presentationAttempts>=2 && attempts==2 && f.f.isSet(199) && f.w.effectiveObject({28,8}).x==20 &&
   f.w.sessionState().journeyRandom()==random.continuation(),"Generated reward retry rerolled or lost world group");
  check(f.p.monsterTreasure->gold==oldGold+*gold && f.p.monsterTreasure->gems==oldGems+5,"Full packs preserve generated currency and RNG order");
  if(!full)check(sameItem(xeenRewardItems(f.p.roster.at(f.p.party.activeRosterIds()[1]),
   XeenRewardCategory(generated.category()))[0],generated.item()),"Generated category or selected recipient differs");
  const auto paid=XeenSaveFormat::encode(f.snapshot());
  f.flow->handle(SelectMemberAction{0},input,selector);f.act(AcknowledgeAction{});f.act(AcknowledgeAction{});
  check(paid==XeenSaveFormat::encode(f.snapshot()),"Stale input replayed generated reward");
  Fixture reload(in,f.snapshot(),false,&city);check(paid==XeenSaveFormat::encode(reload.snapshot()),"Generated inventory/purse/RNG restore differs");
 }
 // Generation before a later selector remains private and cancellation drops its RNG too.
 city.records={record(0,0x14,{66,7,100,10,0,0,0,0,0}),record(1,0x20,{0,0}),record(2,0x0c,{0,0,20,199}),record(3,0x12)};
 Fixture cancelled(in,s,false,&city);const auto preimage=XeenSaveFormat::encode(cancelled.snapshot());
 cancelled.act(InteractionAction{});check(cancelled.flow->canCancelInteraction(),"Post-generation selector absent");
 cancelled.act(CancelInteractionAction{});check(cancelled.flow->canSave() && preimage==XeenSaveFormat::encode(cancelled.snapshot()),"Cancellation leaked item/RNG/flag");
 city.records.push_back(record(4,0x14,{107,1}));city.records[3]=record(3,0x19,{20,20,4});city.records.push_back(record(5,0x1a));
 Fixture refused(in,s,false,&city);interact(refused);
 check(preimage==XeenSaveFormat::encode(refused.snapshot()),"Dependent unsupported suffix consumed treasure RNG");
 // Sixty-three raw draws leave one draw for makeItem, forcing a partial item continuation.
 city.records.clear();
 for(unsigned line=0;line<21;++line)city.records.push_back(record(line,0x14,{100,1,0,0,0,100,1,0,0,0,100,1,0,0,0}));
 city.records.push_back(record(21,0x14,{66,7,0,0,0,0}));
 city.records.push_back(record(22,0x0c,{0,0,20,199}));city.records.push_back(record(23,0x12));
 Fixture bounded(in,s,false,&city);XeenCombatRandom expected(*bounded.w.sessionState().journeyRandom());
 const auto initialPosition=expected.position();
 XeenConsequenceDraw many{expected,256,{}};
 for(unsigned i=0;i<63;++i)check(many.draw(1,1)==1,"Fixed random-gold oracle");
 XeenItemCandidate item(7);check(item.service(many),"Bounded item oracle");
 bounded.act(InteractionAction{});auto pending=XeenTrainingTestAccess::serviceState(*bounded.flow);
 check(pending.generatingReward && pending.rewardItem && pending.rewardRandom && pending.rewardRandom->position()==initialPosition+64 &&
  pending.instructionCount==22 && pending.pendingRewards.gold()==63 && !bounded.flow->canSave(),"Raw draw budget or instruction continuation differs");
 XeenEventContinuation guard(pending,true);
 for(unsigned field=0;field<5;++field) {
  auto altered=pending;
  if(field==0)altered.rewardPair++;
  if(field==1)altered.rewardRandom->draw(1,1);
  if(field==2)altered.rewardItem.emplace(1);
  if(field==3)altered.pendingRewards.credit(34,1);
  if(field==4)altered.pendingRewards.enqueue({1,1,0,0},XeenRewardCategory::Weapons);
  save_test::rejects([&]{guard.check(altered);});
 }
 check(!XeenSaveState::canCapture(bounded.p,bounded.c,bounded.w) && !bounded.f.isSet(199) &&
  bounded.w.sessionState().journeyRandom()==oracle.w.sessionState().journeyRandom() &&
  bounded.p.monsterTreasure->gold==s.journey->treasure->gold,"Suspended generator published live state");
 settle(bounded);
 check(bounded.f.isSet(199) && bounded.w.sessionState().journeyRandom()==expected.continuation() &&
  bounded.p.monsterTreasure->gold==s.journey->treasure->gold+63,"Suspended generation replayed a pair or instruction");
 // A full ten-item category still consumes makeItem, then exits the script before later pairs/effects.
 city.records.clear();for(unsigned line=0;line<40;++line)
  city.records.push_back(record(line,0x14,{34,1,0,0,0,66,1,35,1,0}));
 city.records.push_back(record(40,0x0c,{0,0,20,199}));city.records.push_back(record(41,0x12));
 Fixture overflow(in,s,false,&city);XeenCombatRandom overflowRandom(*overflow.w.sessionState().journeyRandom());
 std::array<unsigned,4> counts{};unsigned accepted=0,attempted=0;
 for(;attempted<40;++attempted) {
  XeenItemCandidate next(1);XeenConsequenceDraw one{overflowRandom,64,{}};check(next.service(one),"Overflow oracle incomplete");
  if(counts[next.category()]==10){++attempted;break;}
  ++counts[next.category()];++accepted;
 }
 interact(overflow);
 check(attempted==accepted+1 && !overflow.f.isSet(199) && itemCount(overflow.p)==accepted &&
  overflow.p.monsterTreasure->gold==s.journey->treasure->gold+attempted &&
  overflow.p.monsterTreasure->gems==s.journey->treasure->gems+accepted &&
  overflow.w.sessionState().journeyRandom()==overflowRandom.continuation(),"Category overflow changed termination, pair order or RNG");
}
unsigned itemCount(const XeenPartyState &p) {
 unsigned n=0;for(const auto &c:p.roster.characters())for(const auto *items:{&c.weapons,&c.armor,&c.accessories,&c.miscellaneous})
  for(const auto &item:*items)n+=item.id!=0;
 return n;
}
void treeFlags(Inputs &in) {
 stage="all original tree flags";std::bitset<256> paid;
 for(const auto &r:in.city.records)if(r.opcode==0x0c && r.parameters.size()==4 &&
  r.parameters[0]==0 && r.parameters[1]==0 && r.parameters[2]==20 &&
  (r.parameters[3]>=232 || r.parameters[3]==57 || r.parameters[3]==58)) {
  const auto flag=r.parameters[3];if(paid[flag])continue;paid.set(flag);
  auto s=at(in,r.x,r.y);s.gameFlags[231]=true;
  Fixture f(in,s);const auto gold=std::uint32_t(f.p.monsterTreasure->gold);
  XeenCombatRandom random(*f.w.sessionState().journeyRandom());XeenConsequenceDraw draw{random,64,{}};
  const auto amount=draw.draw(1,10);check(bool(amount),"Tree gold oracle");
  interact(f,true);check(f.f.isSet(flag) && f.p.monsterTreasure->gold==gold+*amount &&
   f.w.sessionState().journeyRandom()==random.continuation(),"Original tree flag/payment/RNG differs");
  const auto once=XeenSaveFormat::encode(f.snapshot());interact(f);
  check(sameAfterSpace(in,once,f.snapshot()),"Original tree repeated payment beyond its Space suffix");
  Fixture reload(in,f.snapshot());interact(reload);
  check(sameAfterSpace(in,XeenSaveFormat::encode(f.snapshot()),reload.snapshot()),"Restored tree repeated payment beyond its Space suffix");
 }
 check(paid.count()==26 && paid[57] && paid[58] && paid[232] && paid[255],"Original tree family incomplete");
}
XeenSaveSnapshot fresh(Inputs &in) {
 auto party=XeenPartyLoader().loadFromResources(in.chr,in.pty);
 auto camera=XeenCharacterFormat::parsePartyLocation(in.pty);XeenGameFlags flags;
 XeenWorld world(in.mapLoader(),in.objectLoader());
 XeenJourneySetup setup{in.chr,XeenGameplayContextFormat::parse(in.pty),in.statistics,in.city,7,in.regional()};
 setup.prepared=false;setup.purse=XeenCharacterFormat::parseMonsterPurse(in.pty);
 setup.bank=XeenCharacterFormat::parseBankBalances(in.pty);setup.worldFlags=XeenQuestFlagFormat::parseWorldFlags(in.pty);
 setup.regionalText=in.texts.load(23);setup.learnedNames=in.names;setup.learnedNamesProvider=[&in]{return in.names;};
 setup.cityEventsProvider=[&in]{return in.city;};
 setup.mainlandEventsProvider=[&in]{return in.mainland;};
 XeenEncounterFlow flow(world,party,camera,flags,[]{return 0;},setup);
 check(flow.prepareJourneyFrame(flow.ticket(),[]{}) && flow.presentJourney(flow.ticket()),"Original New Game frame");
 check(party.food==90 && party.monsterTreasure->gold==800 && camera.mapId==XeenMapIdentity(28),"Original New Game state");
 return XeenSaveState::capture(in.signature,party,camera,flags,world);
}
void face(Fixture &f,XeenDirection direction) {
 for(unsigned n=0;n<4 && f.c.direction!=direction;++n){f.act(NavigationAction::TurnRight);settle(f);}
 check(f.c.direction==direction,"Original route failed to turn");
}
void walk(Fixture &f,int x,int y) {
 const auto map=f.c.mapId;
 std::array<int,1024> parent;parent.fill(-1);std::array<XeenDirection,1024> directions{};
 const int start=f.c.y*32+f.c.x,end=y*32+x;parent[start]=start;std::queue<int> pending;pending.push(start);
 while(!pending.empty() && parent[end]<0) {
  const int cell=pending.front();pending.pop();
  for(unsigned d=0;d<4;++d) {
   XeenCamera next{map,cell%32,cell/32,XeenDirection(d)};
   if(XeenMovement().apply(f.w,next,NavigationAction::MoveForward)!=XeenMovementResult::Moved) {
    if(map!=XeenMapIdentity(28))continue;
    const auto source=f.w.sampleCell(28,cell%32,cell/32);const auto wall=wallAt(*source->cell,XeenDirection(d));
    if(wall!=9 && wall!=13)continue;
    constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};next.x+=dx[d];next.y+=dy[d];
    if(!xeenIndoorCoordinate(next.x,next.y))continue;
    const auto target=f.w.sampleCell(28,next.x,next.y);if(!target || target->cell->surfaceIndex==4)continue;
   }
   const int index=next.y*32+next.x;if(parent[index]>=0)continue;
   parent[index]=cell;directions[index]=XeenDirection(d);pending.push(index);
  }
 }
 if(parent[end]<0)throw std::runtime_error("Original Joe route unreachable at "+std::to_string(x)+","+std::to_string(y));
 std::vector<XeenDirection> route;for(int cell=end;cell!=start;cell=parent[cell])route.push_back(directions[cell]);
 for(auto it=route.rbegin();it!=route.rend();++it) {
  face(f,*it);
  for(unsigned attempt=0;attempt<10;++attempt) {
   auto query=f.c;if(XeenMovement().apply(f.w,query,NavigationAction::MoveForward)==XeenMovementResult::Moved)break;
   unsigned thief=0;for(unsigned i=1;i<f.p.party.size();++i)
    if(XeenCharacterRules::thievery(f.p.party.member(f.p.roster,i))>XeenCharacterRules::thievery(f.p.party.member(f.p.roster,thief)))thief=i;
   f.act(InteractionAction{});if(f.flow->canCancelInteraction())f.act(SelectMemberAction{thief});settle(f);
  }
  f.act(NavigationAction::MoveForward);settle(f);
  bool tired=false;for(auto owner:f.p.party.activeRosterIds())tired=tired ||
   f.p.roster.at(owner).currentHp<XeenCharacterRules::maxHp(f.p.roster.at(owner),{f.p.encounterContext->year})/2;
  if(tired && f.p.food>=6){f.act(RestAction{});settle(f);}
 }
 check(f.c.x==x && f.c.y==y,"Original Joe route failed to reach target");
}
void partAOriginal(Inputs &in) {
 stage="M59 original display cases and jail";
 auto source=at(in,12,12);const auto owner=source.activeRosterIds[0];source.characters[owner].skills[0]=0;
 Fixture jail(in,source);const auto year=jail.p.encounterContext->year;const auto day=jail.p.encounterContext->day;const auto minutes=jail.p.encounterContext->minutes;
 jail.act(InteractionAction{});check(jail.flow->canCancelInteraction(),"Case WhoWill absent");jail.act(SelectMemberAction{0});settle(jail);
 check(jail.c.mapId==XeenMapIdentity(28) && jail.c.x==27 && jail.c.y==11 && jail.p.encounterContext->year==year+1 &&
  jail.p.encounterContext->day==day && jail.p.encounterContext->minutes==minutes+1,"Original jail year/destination/Space time");
 for(auto id:jail.p.party.activeRosterIds())check(jail.p.roster.at(id).awards[8]==1,"Original conviction missing");
 const auto convicted=jail.snapshot();Fixture restored(in,convicted);check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(convicted),"Original jail reload");
 walk(restored,25,13);check(restored.flow->canSave() && restored.c.x==25 && restored.c.y==13,"Jail ordinary walk-out failed");
 auto success=source;success.characters[owner].skills[0]=1;Fixture thief(in,success);
 const auto xp=thief.p.roster.combatInputs(owner)->experience;
 interact(thief,true);check(thief.c.x==12 && thief.c.y==12 && thief.p.encounterContext->year==year &&
  thief.p.roster.combatInputs(owner)->experience==xp && !thief.p.roster.at(owner).awards[8],"Original case invented lockpick roll/XP or jail");
 stage="M59 original well before proof";source=at(in,14,17);source.gameFlags[9]=false;
 Fixture poison(in,source);const auto selected=poison.p.party.activeRosterIds()[1];
 poison.act(InteractionAction{});check(poison.flow->canCancelInteraction(),"Well WhoWill absent");
 poison.act(SelectMemberAction{1});settle(poison);
 check(poison.flow->canSave() && !poison.flow->presentationGeneration(),"Before-proof well showed a message");
 check(poison.p.roster.at(selected).conditions[3]==source.characters[selected].conditions[3],"Poison damage applied Poison condition");
 for(unsigned n=0;n<poison.p.party.size();++n)if(n!=1)check(poison.p.roster.at(poison.p.party.activeRosterIds()[n]).currentHp==source.characters[source.activeRosterIds[n]].currentHp,"Well damaged another member");
 stage="M59 original well healing boundaries";
 for(int boundary:{-1,0,1}) {
  auto healed=source;healed.gameFlags[9]=true;const auto max=XeenCharacterRules::maxHp(healed.characters[selected],{healed.journey->context->year});
  healed.characters[selected].currentHp=max+boundary;Fixture well(in,healed);
  std::vector<unsigned> messages;std::string refreshText;well.flow->reportManual=[&](const auto &r){if(const auto *p=std::get_if<XeenEventExecutionSuspended>(&r))if(p->request.textIndex==52 || p->request.textIndex==62){messages.push_back(*p->request.textIndex);refreshText=p->request.text;}};
  well.act(InteractionAction{});well.act(SelectMemberAction{1});
  while(XeenCityEventTestAccess::portrait(*well.flow)) {
   well.now+=125;well.flow->beginCycle(++well.cycle);if(auto image=well.flow->updatePresentation())well.present(*image);
  }
  check(!well.flow->canSave() && well.flow->presentationGeneration(),"Well success/alternate text absent");
  const auto pending=XeenTrainingTestAccess::serviceState(*well.flow);
  if(messages!=std::vector<unsigned>{unsigned(boundary<=0?52:62)} || pending.pendingPresentation->request.response!=XeenPresentationResponseRequirement::Acknowledgment) {
   std::string diagnostic="Well boundary="+std::to_string(boundary)+" source max="+std::to_string(max)+" restored max="+std::to_string(XeenCharacterRules::maxHp(well.p.roster.at(selected),{well.p.encounterContext->year}))+" pending text="+std::to_string(pending.pendingPresentation->request.textIndex.value_or(255))+" response="+std::to_string(unsigned(pending.pendingPresentation->request.response))+" displays=";
   for(auto message:messages)diagnostic+=std::to_string(message)+",";
   throw std::runtime_error(diagnostic);
  }
  if(boundary<=0)check(refreshText.find("You feel refreshed")!=std::string::npos,"Well original refreshed text differs");
  check(!well.p.worldFlags->values[23] && XeenCityEventTestAccess::visible(*well.flow).roster.at(selected).currentHp==(boundary<=0?max:max+1) &&
   well.p.roster.at(selected).currentHp==healed.characters[selected].currentHp,"Well HP prefix or deferred flag preceded its original point or leaked live state");
  settle(well);
  check(well.p.roster.at(selected).currentHp==(boundary<=0?max:max+1) && bool(well.p.worldFlags->values[23])==(boundary<=0),"Well healing cap/alternate flag order");
  for(unsigned n=0;n<well.p.party.size();++n)if(n!=1)check(well.p.roster.at(well.p.party.activeRosterIds()[n]).currentHp==healed.characters[healed.activeRosterIds[n]].currentHp,"Well healed another member");
  const auto saved=well.snapshot();Fixture roundtrip(in,saved);check(XeenSaveFormat::encode(roundtrip.snapshot())==XeenSaveFormat::encode(saved),"Well flags/heads/HP/RNG exact restore");
 }
}

void originalJourney(Inputs &in) {
 stage="New Game route";
 Fixture f(in,fresh(in));walk(f,14,5);face(f,XeenDirection::West);interact(f);
 check(f.p.questFlags.isSet(29),"New Game Gunther quest missing");
 walk(f,8,18);interact(f);walk(f,9,22);interact(f,true);
 check(f.f.isSet(9),"New Game proof did not advance Joe quest");
 walk(f,14,5);face(f,XeenDirection::West);const auto gold=f.p.monsterTreasure->gold;
 std::set<int> questPortraits;const auto draw=f.flow->drawDialogSprite;
 f.flow->drawDialogSprite=[&](auto &image,const char *name,unsigned frame,int x,int y){if(std::string(name)=="spellfx.icn")questPortraits.insert(x);draw(image,name,frame,x,y);};
 interact(f);f.flow->drawDialogSprite=draw;
 check(questPortraits.size()==f.p.party.size(),"Original quest XP did not sparkle every party portrait");
 check(f.f.isSet(10) && f.p.monsterTreasure->gold==gold+4000 && f.p.questFlags.isSet(1),"New Game Gunther reward missing");
 const auto awarded=XeenSaveFormat::encode(f.snapshot());interact(f);
 check(sameAfterSpace(in,awarded,f.snapshot(),true),"New Game repeat Gunther regranted rewards");
 stage="New Game crate";walk(f,9,28);
 check(f.w.effectiveObject({28,8}).resourceId==95 && f.w.effectiveObject({28,8}).x==-128 &&
  f.w.effectiveObject({28,8}).y==-128,"Original open-crate overlay source differs");
 const auto beforeLoot=f.snapshot();const auto items=itemCount(f.p);interact(f,true);
 check(itemCount(f.p)==items+1 && f.w.effectiveObject({28,8}).x==9 && f.w.effectiveObject({28,8}).y==28,
  "Original crate did not deliver item/reveal open appearance");
 const auto opened=XeenSaveFormat::encode(f.snapshot());interact(f);
 check(sameAfterSpace(in,opened,f.snapshot()),"Repeated crate changed inventory/RNG beyond its Space suffix");
 stage="M61 earned crate Identify and Sell";
 unsigned lootMember=6,lootCategory=0,lootSlot=0;
 for(unsigned member=0;member<f.p.party.size();++member)for(unsigned category=0;category<4;++category) {
  const auto owner=f.p.party.activeRosterIds()[member];
  const auto &old=*xeenInventoryItems(beforeLoot.characters[owner],XeenInventoryCategory(category));
  const auto &current=*xeenInventoryItems(f.p.roster.at(owner),XeenInventoryCategory(category));
  for(unsigned slot=0;slot<9;++slot)if(!old[slot].id && current[slot].id){lootMember=member;lootCategory=category;lootSlot=slot;}
 }
 check(lootMember<6,"Earned crate item was not found in inventory");
 const auto lootOwner=f.p.party.activeRosterIds()[lootMember];
 const auto loot=(*xeenInventoryItems(f.p.roster.at(lootOwner),XeenInventoryCategory(lootCategory)))[lootSlot];
 walk(f,8,4);face(f,XeenDirection::West);f.act(InteractionAction{});f.prepare();
 const auto serviceGold=std::uint32_t(f.p.monsterTreasure->gold);
 const auto serviceMinutes=f.p.encounterContext->minutes;
 const auto identifyPrice=*xeenItemPrice(XeenInventoryCategory(lootCategory),loot,XeenItemService::Identify);
 const auto sellPrice=*xeenItemPrice(XeenInventoryCategory(lootCategory),loot,XeenItemService::Sell,f.p.roster.at(lootOwner).skills[8]);
 f.act(DialogKeyAction{'b'});f.act(SelectMemberAction{lootMember});
 constexpr unsigned tabs[]{'w','a','c','m'};f.act(DialogKeyAction{tabs[lootCategory]});f.act(DialogKeyAction{'i'});
 f.act(SelectInventorySlotAction{lootSlot});f.act(YesAction{});
 check(f.flow->inputContext(f.flow->frame().presentation()).dialog->anyKey &&
  f.p.monsterTreasure->gold==serviceGold-identifyPrice &&
  xeenSameItem((*xeenInventoryItems(f.p.roster.at(lootOwner),XeenInventoryCategory(lootCategory)))[lootSlot],loot),"Earned loot Identify changed bytes or lost detail/payment");
 f.act(AcknowledgeAction{});f.act(DialogKeyAction{'s'});f.act(SelectInventorySlotAction{lootSlot});f.act(YesAction{});
 check(f.p.monsterTreasure->gold==serviceGold-identifyPrice+sellPrice && f.p.encounterContext->minutes==serviceMinutes,"Earned loot Sell credit or transaction timing differs");
 for(unsigned n=0;n<100 && !f.flow->canSave();++n)f.act(CancelInteractionAction{});
 check(f.flow->canSave(),"Earned loot Ironworks departure did not settle");
 f.act(RestAction{});settle(f);
 walk(f,8,18);interact(f);walk(f,11,18);face(f,XeenDirection::West);interact(f);
 walk(f,14,17);interact(f,true);check(bool(f.w.sessionState().headCache()),"Well did not retain HED cache");
 walk(f,15,0);face(f,XeenDirection::South);
 const std::vector<XeenActor> actors=f.w.sessionState().regionalActors(28);interact(f);
 check(f.c.mapId==XeenMapIdentity(23) && f.c.direction==XeenDirection::South,"Original exit facing/transfer failed");
 check(!f.w.sessionState().headCache(),"Primary-map reload retained town HED overrides");
 // Snapshot and restore the earned town effects while outside the town.
 const auto saved=f.snapshot();Fixture reload(in,saved);
 check(XeenSaveFormat::encode(reload.snapshot())==XeenSaveFormat::encode(saved),"Earned route exact reload failed");
 const auto entry=std::find_if(in.mainland.records.begin(),in.mainland.records.end(),[](const auto &r){return r.opcode==0x07 && r.parameters.size()==3 && r.parameters[0]==28;});
 check(entry!=in.mainland.records.end(),"Original town entrance absent");
 walk(reload,entry->x,entry->y);if(entry->direction<4)face(reload,XeenDirection(entry->direction));interact(reload);
 check(reload.c.mapId==XeenMapIdentity(28),"Original town re-entry failed");
 check(!reload.w.sessionState().headCache(),"Town re-entry resurrected old HED overrides");
 const auto retained=reload.w.sessionState().regionalActors(28);
 check(retained.size()==actors.size(),"Flag 9 town exit changed actor count");
 // Decision 8: re-entry is a map load. Positions, identities and lifecycles
 // survive; HP returns to MON and only the arrival view wakes monsters.
 for(unsigned i=0;i<actors.size();++i) {
  auto expected=actors[i];xeenRefillLoadedMonster(expected);expected.activated=retained[i].activated;
  check(xeen_state::sameActor(retained[i],expected),"Flag 9 town exit respawned actors");
 }
 walk(reload,14,5);face(reload,XeenDirection::West);const auto purse=reload.p.monsterTreasure->gold;
 interact(reload);check(reload.p.monsterTreasure->gold==purse && reload.p.questFlags.isSet(1),"Reload/re-entry repeated reward");
 stage="earned crate reload";walk(reload,9,28);const auto openedAgain=XeenSaveFormat::encode(reload.snapshot());interact(reload);
 check(sameAfterSpace(in,openedAgain,reload.snapshot()) && reload.w.effectiveObject({28,8}).x==9,
  "Travel/reload reopened paid crate");
 stage="earned tree cycle";check(reload.f.isSet(231),"Town departure did not initialize tree cycle");
 walk(reload,13,2);const auto beforeTree=std::uint32_t(reload.p.monsterTreasure->gold);interact(reload,true);
 check(reload.f.isSet(232) && reload.p.monsterTreasure->gold>beforeTree && reload.p.monsterTreasure->gold<=beforeTree+10,"Earned tree payment absent");
 walk(reload,24,5);const auto tavern=std::find_if(in.city.records.begin(),in.city.records.end(),[](const auto &r){return r.x==24 && r.y==5 && r.line==0;});
 check(tavern!=in.city.records.end(),"Original Tavern entry missing");if(tavern->direction<4)face(reload,XeenDirection(tavern->direction));
 reload.act(InteractionAction{});
 for(unsigned n=0;n<100 && !reload.flow->canSave();++n) {
  reload.now+=125;reload.flow->beginCycle(++reload.cycle);if(auto frame=reload.flow->updatePresentation())reload.present(*frame);
  reload.act(CancelInteractionAction{});
 }
 settle(reload);check(reload.f.isSet(231) && reload.f.isSet(232),"Tavern reset initialized tree payment");
 walk(reload,13,2);const auto afterTavern=XeenSaveFormat::encode(reload.snapshot());interact(reload);
 check(sameAfterSpace(in,afterTavern,reload.snapshot()),"Tavern minted another tree payment");
 walk(reload,15,0);face(reload,XeenDirection::South);interact(reload);Fixture treeReload(in,reload.snapshot());
 walk(treeReload,entry->x,entry->y);if(entry->direction<4)face(treeReload,XeenDirection(entry->direction));interact(treeReload);
 walk(treeReload,13,2);const auto afterTravel=XeenSaveFormat::encode(treeReload.snapshot());interact(treeReload);
 check(sameAfterSpace(in,afterTravel,treeReload.snapshot()),"Departure/reload minted another tree payment");
}
void partBVernGuild(Inputs &in) {
 stage="M59 Vern selected payment";
 auto source=at(in,16,11,XeenDirection::East);const unsigned selected=3,owner=source.activeRosterIds[selected];
 for(auto &c:source.characters)c.awards[0]=0;
 source.journey->treasure->gold=75;
 Fixture f(in,source);const auto initial=XeenSaveFormat::encode(f.snapshot());const auto random=f.w.sessionState().journeyRandom();const auto time=f.p.encounterContext;
 teacherPrompt(f);check(f.flow->canCancelInteraction(),"Vern WhoWill unavailable");
 auto pending=XeenTrainingTestAccess::serviceState(*f.flow);
 check(pending.pendingPresentation->request.verbIndex==21 && pending.pendingPresentation->request.textIndex==41,"Vern selection text changed");
 const auto &request=pending.pendingPresentation->request;
 XeenTextRenderOptions who;who.originalControls=true;who.drawWindow=true;who.stopAtBottom=true;
 who.bounds={233,82,312,146};who.windowBounds={225,74,320,154};who.x=233;who.y=82;
 const auto originalWho=XeenTextRenderer(in.font).render(f.flow->frame(),xeenDialogFormat(in.assets.uiText().scalar("WHO_WILL"),
  {request.text,in.assets.uiText().table("WHO_ACTIONS").at(*request.verbIndex),std::to_string(request.members.size())}),who);
 check(originalWho.diagnostics.empty(),"Vern original WhoWill control stream");
 for(int y=74;y<154;++y)for(int x=225;x<320;++x) {
  // The explicit audio deferral is window 6 in front of the selector.
  if(x<268 && y>=149)continue;
  check(originalWho.pages.front().pixels[y*320+x]==f.flow->frame().pixels[y*320+x],
      "Vern selection did not use original WhoWill window/text");
 }
 if(const auto *path=std::getenv("MMODERN_SERVICE_PREVIEW")) {
  std::ofstream out(std::string(path)+"/guild-who-will.ppm",std::ios::binary);out<<"P6\n320 200\n255\n";
  const auto palette=in.assets.readArchiveResource("mm4.pal");
  for(auto pixel:f.flow->frame().pixels)for(unsigned channel=0;channel<3;++channel){const unsigned char c=palette[pixel*3+channel]*4;out.write(reinterpret_cast<const char *>(&c),1);}
 }
 const auto staleFrame=f.flow->frame().presentation();const auto staleInput=f.flow->displayedInput();
 f.act(NoAction{});check(f.p.monsterTreasure->gold==75 && f.p.roster.at(owner).awards[0]==0,"No charged Vern dues");
 f.act(DialogKeyAction{InputKey::Escape});settle(f);check(sameExceptNpcRandom(initial,f.snapshot()),"Vern cancellation changed state");
 f.flow->handle(SelectMemberAction{selected},staleInput,staleFrame);check(sameExceptNpcRandom(initial,f.snapshot()),"Stale Vern selection charged dues");
 for(unsigned n=1;n<=2;++n) {
  teacherPrompt(f);const auto controls=f.flow->inputContext(f.flow->frame().presentation()).dialog;
  check(controls && controls->hits.size()==6 && controls->key(InputKey::F1+selected),"WhoWill lacks original portrait/keyboard input");
  const auto choice=n==1?controls->click(118,151):controls->key(InputKey::F1+selected);
  check(bool(choice),"WhoWill portrait hit absent");f.act(*choice);settle(f);
  check(f.w.scenePresentation().portraits[owner].spellFrame==4,"Vern reached quiet boundary before its sparkle finished");
  check(f.p.monsterTreasure->gold==75-25*n && f.p.roster.at(owner).awards[0]==1,"Vern repeat payment/award differs");
  for(auto id:f.p.party.activeRosterIds())if(id!=owner)check(f.p.roster.at(id).awards[0]==0,"Vern enrolled an unselected owner");
  check(f.p.encounterContext->minutes==time->minutes+n && f.p.encounterContext->ctr24==(time->ctr24+n)%24,"Vern omitted the completed Space suffix");
  check(npcRandomOnly(*random,*f.w.sessionState().journeyRandom()),"Vern consumed random requests beyond original NPC animation");
 }
 for(unsigned gold:{24u,25u}) {
  auto boundary=source;boundary.journey->treasure->gold=gold;Fixture b(in,boundary);const auto before=XeenSaveFormat::encode(b.snapshot());
  teacherPrompt(b);b.act(SelectMemberAction{selected});settle(b);
  check(b.p.monsterTreasure->gold==(gold==25?0:24) && b.p.roster.at(owner).awards[0]==(gold==25),"Vern exact/insufficient gold boundary");
  if(gold==24) {
   Fixture expected(in,XeenSaveFormat::decode(before));expected.act(WaitAction{});settle(expected);
   check(sameExceptNpcRandom(XeenSaveFormat::encode(expected.snapshot()),b.snapshot()),"Insufficient Vern dues changed more than the completed Space suffix");
  }
 }
 auto disabled=source;disabled.characters[owner].conditions[8]=1;Fixture blocked(in,disabled);
 const auto unchanged=XeenSaveFormat::encode(blocked.snapshot());teacherPrompt(blocked);
 blocked.act(DialogKeyAction{InputKey::F1+selected});
 check(blocked.flow->inputContext(blocked.flow->frame().presentation()).dialog->anyKey && !blocked.flow->canSave(),"Disabled WhoWill selection lacks original refusal modal");
 check(!blocked.flow->respond(*blocked.flow->presentationGeneration(),SelectedCharacter{0}),"Direct response bypassed WhoWill refusal acknowledgment");
 blocked.act(DialogKeyAction{InputKey::Escape});
 check(blocked.flow->canCancelInteraction() && !blocked.flow->canSave() && blocked.flow->inputContext(blocked.flow->frame().presentation()).dialog->key(InputKey::F1),"Refusal acknowledgment cancelled WhoWill or selected a member");
 blocked.act(DialogKeyAction{InputKey::Escape});settle(blocked);
 check(sameExceptNpcRandom(unchanged,blocked.snapshot()),"Disabled WhoWill selection spent time/gold/RNG or enrolled a member");
 // Fail composition at publication; payment and membership must remain one unit.
 Fixture retry(in,source);teacherPrompt(retry);unsigned failures=0;
 XeenCityEventTestAccess::composer(*retry.flow,[&](auto &,const auto &,const auto &,auto,auto){
  check((retry.p.monsterTreasure->gold==75 && retry.p.roster.at(owner).awards[0]==0) ||
   (retry.p.monsterTreasure->gold==50 && retry.p.roster.at(owner).awards[0]==1),"Vern published only half a transaction");
  if(!failures++)throw std::runtime_error("Vern frame retry");return XeenEventFlow::Composition{frame(),false};});
 retry.act(SelectMemberAction{selected});settle(retry);check(failures && retry.p.monsterTreasure->gold==50 && retry.p.roster.at(owner).awards[0]==1,"Vern retry double-paid or lost award");
 stage="M59 mixed-membership Guild";
 auto guild=f.snapshot();guild.camera={28,20,13,XeenDirection::North};guild.journey->context->day=10;
 Fixture g(in,guild);XeenServiceDayCandidate expected(*guild.journey->context,*guild.journey->serviceEconomy,*guild.journey->random);
 while(!expected.service(64)){}expected.validateComplete();
 const auto input=[&]{return g.flow->inputContext(g.flow->frame().presentation()).dialog;};
 g.act(InteractionAction{});
 for(unsigned n=0;n<1000 && (!input() || !input()->key('b'));++n){g.flow->beginCycle(++g.cycle);if(auto image=g.flow->updatePresentation())g.present(*image);}
 check(g.w.sessionState().journeyActivity()==XeenJourneyActivity::Service && input() && input()->key('b'),("Guild terminal service unavailable: "+g.flow->encounter()->notice()).c_str());
 check(!g.flow->canSave() && !XeenSaveState::canCapture(g.p,g.c,g.w),"Guild allowed a transient save");
 const auto text=[&]{return XeenTrainingTestAccess::templeText(*g.flow);};
 check(text().find("You have to be a member to shop here")!=std::string::npos,"Guild admitted non-member");
 const auto purse=g.p.monsterTreasure->gold;g.act(DialogKeyAction{'b'});g.act(DialogKeyAction{'s'});
 check(text().find("You have to be a member")!=std::string::npos && g.p.monsterTreasure->gold==purse,"Non-member Guild action changed state");
 const auto choose=[&](unsigned key,bool mouse) {
  if(!mouse){g.act(DialogKeyAction{key});return;}
  for(const auto &hit:input()->hits)if(hit.key==key){const auto action=input()->click((hit.left+hit.right)/2,(hit.top+hit.bottom)/2);check(bool(action),"Guild mouse hit absent");g.act(*action);return;}
  throw std::runtime_error("Guild command has no mouse hit");
 };
 choose(InputKey::F1+selected,true);
 check(text().find("uy Spells")!=std::string::npos && text().find("pell Info")!=std::string::npos && text().find(g.p.roster.at(owner).name)!=std::string::npos,"Guild did not use selected member's award");
 for(unsigned key:{unsigned('b'),unsigned('s')}) {
  choose(key,key=='s');check(input()->key('1') && input()->key(InputKey::Escape),"Guild original spell list absent");g.act(CancelInteractionAction{});
 }
 choose(InputKey::F1,false);check(text().find("You have to be a member")!=std::string::npos,"Guild retained previous member's membership");
 choose(InputKey::F1+selected,false);choose(InputKey::Escape,true);
 for(unsigned n=0;n<1000 && !g.flow->canSave();++n){g.act(CancelInteractionAction{});g.flow->beginCycle(++g.cycle);if(auto image=g.flow->updatePresentation())g.present(*image);}
 check(g.flow->canSave() && g.p.monsterTreasure->gold==purse && *g.p.encounterContext==expected.context() && *g.p.serviceEconomy==expected.economy() && *g.w.sessionState().journeyRandom()==expected.continuation(),"Guild departure did not settle exactly once");
 Fixture restored(in,g.snapshot());check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(g.snapshot()),"Guild membership/day restore is not exact");

 // Original Vern -> Guild input path with a synthetic purse large enough to buy.
 stage="Vern membership and original Protection from Elements purchase";
 auto funded=source;funded.journey->treasure->gold=1000;const unsigned caster=4,casterOwner=funded.activeRosterIds[caster];
 Fixture enrollment(in,funded);teacherPrompt(enrollment);enrollment.act(SelectMemberAction{caster});settle(enrollment);
 check(enrollment.p.roster.at(casterOwner).awards[0]==1 && enrollment.p.monsterTreasure->gold==975,"Vern did not enroll funded caster");
 auto enrolled=enrollment.snapshot();enrolled.camera={28,20,13,XeenDirection::North};Fixture shop(in,enrolled);shop.act(InteractionAction{});shop.prepare();
 shop.act(DialogKeyAction{InputKey::F1+caster});shop.act(DialogKeyAction{'b'});
 const auto offered=XeenLearnedSpellRules::rows(shop.p.roster.at(casterOwner),true);
 const auto category=*XeenLearnedSpellRules::categoryForClass(shop.p.roster.at(casterOwner).characterClass);
 const auto purchase=std::find(offered.begin(),offered.end(),*XeenLearnedSpellRules::slotForSpell(category,55));
 check(purchase!=offered.end() && purchase-offered.begin()<10,"original Protection from Elements offer missing");const auto bought=*purchase;
 const auto spell=*XeenLearnedSpellRules::spellForSlot(*XeenLearnedSpellRules::categoryForClass(shop.p.roster.at(casterOwner).characterClass),bought);
 check(spell==55 && XeenLearnedSpellRules::supported(spell),"original Protection from Elements purchase identity/support");
 const auto price=XeenLearnedSpellRules::purchasePrice(spell,shop.p.roster.at(casterOwner).characterClass);
 shop.act(DialogKeyAction{'1'+unsigned(purchase-offered.begin())});shop.act(DialogKeyAction{'y'});
 check(shop.p.roster.at(casterOwner).learnedSpells->at(bought)==1 && shop.p.monsterTreasure->gold==975-price,"original Guild purchase did not learn/debit selected book");
 shop.act(CancelInteractionAction{});shop.act(CancelInteractionAction{});
 for(unsigned n=0;n<1000 && !shop.flow->canSave();++n){shop.flow->beginCycle(++shop.cycle);if(auto image=shop.flow->updatePresentation())shop.present(*image);}
 check(shop.flow->canSave(),"Purchased Guild departure did not settle");
 Fixture purchased(in,shop.snapshot());check(XeenSaveFormat::encode(purchased.snapshot())==XeenSaveFormat::encode(shop.snapshot()),"purchased book did not restore exactly");
}
void partBSearches(Inputs &in) {
 struct Family {const char *name;unsigned items;bool crates,monster;std::vector<std::pair<int,int>> sites;};
 const std::vector<Family> families{
  {"monster crates",0,true,true,{{6,25},{6,26},{8,26},{8,25},{8,23},{8,22},{9,23}}},
  {"item crates",1,true,false,{{9,28},{8,28},{9,26},{9,25},{3,25}}},
  {"empty crates",0,true,false,{{7,2},{7,6},{5,10},{5,14},{6,16},{6,17},{6,18},{3,26},{3,28},{4,28},{5,25},{5,26},{25,13},{27,15}}},
  {"empty rubbish",0,false,false,{{1,12},{1,10},{3,11},{3,13},{3,9},{4,12}}},
  {"item rubbish",1,false,false,{{3,12}}},
  {"item beds",1,false,false,{{3,17},{28,7},{3,5},{28,3}}},
  {"empty beds",0,false,false,{{3,21},{3,7},{3,3},{28,4},{28,6},{28,12},{27,12},{22,11},{18,11}}}
 };
 auto base=at(in,20,20);
 // Isolate original scripts from unrelated roaming actors. Spawn still revives its real MOB slot.
 for(auto &a:base.map(28).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 const auto resolver=XeenObjectVisualResolver::load(in.assets);
 unsigned sites=0;const auto nothing=in.texts.load(28).strings[28];
 for(const auto &family:families)for(const auto [x,y]:family.sites) {
  const std::string site=std::string(family.name)+" "+std::to_string(x)+","+std::to_string(y);stage=site;auto source=base;source.camera={28,x,y,XeenDirection::North};
  // Search records apply to every facing; choose one without a door taking
  // precedence over Space, just as a player facing the searched object would.
  XeenWorld geometry(in.mapLoader());const auto cell=geometry.sampleCell(28,x,y);
  for(unsigned d=0;d<4;++d){const auto wall=wallAt(*cell->cell,XeenDirection(d));if(wall!=1 && wall!=6 && wall!=9 && wall!=13){source.camera.direction=XeenDirection(d);break;}}
  Fixture f(in,source);
  const auto before=XeenSaveFormat::encode(f.snapshot());const auto objects=save_test::movedObjects(f.snapshot());const auto items=itemCount(f.p);
  std::vector<unsigned> moves;for(const auto &r:in.city.records)if(r.x==x && r.y==y && r.opcode==0x0b)moves.push_back(r.parameters[0]);
  f.act(InteractionAction{});check(f.flow->canCancelInteraction(),"Search WhoWill absent");f.act(CancelInteractionAction{});settle(f);
  check(before==XeenSaveFormat::encode(f.snapshot()),"Cancelled search changed objects/actors/items/RNG");
  const auto actorId=f.w.sessionState().regionalActors(28).at(2).id;
  bool combat=false;f.flow->reportManual=[&](const auto &result){if(const auto *error=std::get_if<XeenEventExecutionError>(&result))throw std::runtime_error(error->message);};
  f.act(InteractionAction{});f.act(SelectMemberAction{3});
  for(unsigned n=0;n<100 && !f.flow->canSave() && !f.flow->encounter()->combat();++n){f.now+=125;f.flow->beginCycle(++f.cycle);if(auto image=f.flow->updatePresentation())f.present(*image);}
  if(family.monster){combat=f.flow->encounter()->combat()!=nullptr;const auto &actor=f.w.sessionState().regionalActors(28).at(2);check(combat && actor.id==actorId && actor.x==x && actor.y==y && actor.lifecycle==XeenActorLifecycle::Present,"Monster crate lost slot identity or normal combat");}
  settle(f);
  if(!family.monster)check(itemCount(f.p)==items+family.items,"Search item delivery differs");
  if(family.crates){check(moves.size()==1,"Original crate MoveObj missing");const auto open=f.w.effectiveObject({28,moves[0]});check(open.resourceId==95 && open.x==x && open.y==y,"Crate did not reveal original open object");}
  else check(save_test::movedObjects(f.snapshot())==objects,"Bed/rubbish changed its appearance");
  bool empty=false;f.flow->reportManual=[&](const auto &result){if(const auto *p=std::get_if<XeenEventExecutionSuspended>(&result))if(p->request.text==nothing)empty=true;};
  // The synthetic frame compositor does not draw objects. Apply the normal
  // scene draw, which resets same-position MoveObj flip metadata (known quirk).
  XeenIndoorScene().build(f.w,f.c,&resolver,nullptr,0);
  const auto once=XeenSaveFormat::encode(f.snapshot());interact(f,true);
  XeenIndoorScene().build(f.w,f.c,&resolver,nullptr,0);
  check(empty,"Repeat search did not say Nothing here");
  check(sameAfterSpace(in,once,f.snapshot()),"Repeat search changed more than its Space suffix");
  if(family.monster && sites==0){const auto repeated=XeenSaveFormat::encode(f.snapshot());Fixture reload(in,f.snapshot());check(repeated==XeenSaveFormat::encode(reload.snapshot()),"Monster crate exact restore changed state");interact(reload,true);XeenIndoorScene().build(reload.w,reload.c,&resolver,nullptr,0);check(sameAfterSpace(in,repeated,reload.snapshot()),"Restored crate respawned monster beyond its Space suffix");}
  ++sites;
 }
 check(sites==46,"Search family coverage incomplete");
 stage="M59 signs and talk";unsigned labels=0;
 for(const auto &r:in.city.records)if(r.line==0 && ((r.fileOffset>=455 && r.fileOffset<514) || (r.fileOffset>=715 && r.fileOffset<834) || (r.fileOffset>=957 && r.fileOffset<1037) || (r.fileOffset>=1101 && r.fileOffset<1108) || (r.fileOffset>=1295 && r.fileOffset<1449) || (r.fileOffset>=4677 && r.fileOffset<4726) || (r.fileOffset>=7190 && r.fileOffset<7204))) {
  for(bool proof:{false,true}){auto source=base;source.camera={28,r.x,r.y,r.direction<4?XeenDirection(r.direction):XeenDirection::North};source.gameFlags[9]=proof;Fixture f(in,source);bool shown=false;
   f.flow->reportManual=[&](const auto &result){if(const auto *p=std::get_if<XeenEventExecutionSuspended>(&result))shown|=!p->request.text.empty();if(const auto *e=std::get_if<XeenEventExecutionError>(&result))throw std::runtime_error(e->message);};
   // The proof-shop range also contains the pest-proof crate's WhoWill.
   interact(f,true);check(shown,"Original sign/talk text absent");}
  ++labels;
 }
 check(labels>=12,"Sign/talk family coverage incomplete");
}

}
int main(int argc,char **argv) {
 try {
  check(argc>=2,"usage: world-event <installation>");const auto installation=xeenTestInstallationDetector().detect(argv[1]);
  check(bool(installation),"Original installation absent");Inputs in(*installation);
  if(argc==3) {
   for(const auto &r:in.city.records)if((r.x==9 && r.y==22) || (r.x==14 && (r.y==5 || r.y==17)) ||
    (r.x==8 && r.y==18) || (r.x==11 && r.y==18) || (r.x==15 && r.y==0)) {
    std::cout<<unsigned(r.x)<<','<<unsigned(r.y)<<','<<unsigned(r.direction)<<" line "<<unsigned(r.line)<<" op "<<unsigned(r.opcode)<<" :";
    for(auto p:r.parameters)std::cout<<' '<<unsigned(p);std::cout<<'\n';
   }
   return 0;
  }
  std::ostringstream output;auto *saved=std::cout.rdbuf(output.rdbuf());
  // Each section reports its wall time on stderr, so a slow run shows where it went.
  const auto timed=[&](const char *name,void (*section)(Inputs &)) {
   const auto began=std::chrono::steady_clock::now();section(in);
   std::cerr<<name<<": "<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count()<<" ms\n";
  };
  try {
   timed("portraitRewards",portraitRewards);timed("originalPortraitOrder",originalPortraitOrder);timed("npcBeforeDeadline",npcBeforeDeadline);
   timed("partBVernGuild",partBVernGuild);timed("partBSearches",partBSearches);timed("partA",partA);timed("rules",rules);timed("teachers",teachers);
   timed("treasure",treasure);timed("rewardRandomOwnership",rewardRandomOwnership);timed("originalVisualRegressions",originalVisualRegressions);timed("originalEvents",originalEvents);
   timed("partAOriginal",partAOriginal);timed("treeFlags",treeFlags);timed("originalJourney",originalJourney);
  }catch(...){std::cout.rdbuf(saved);throw;}
  std::cout.rdbuf(saved);
  std::cout<<"World effects, shared treasure, bounded generation, cancellation, retry and original scripts passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<stage<<": "<<e.what()<<'\n';return 1;}
}
