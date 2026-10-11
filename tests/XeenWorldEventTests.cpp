#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenObjectVisual.h"
#include "games/xeen/XeenStateEquality.h"
#include <iostream>
#include <queue>
#include <sstream>
namespace mmodern {
struct XeenCityEventTestAccess {
 static void composer(XeenEventFlow &flow,XeenEventFlow::TransitionCompose compose){flow._transitionCompose=std::move(compose);}
 static void presenter(XeenEventFlow &flow,XeenEventFlow::EncounterCompose compose){flow._encounterCompose=std::move(compose);}
};
}
using namespace training_test;
namespace {
const char *stage="rules";
struct Fixture : training_test::Fixture {
 Fixture(Inputs &in,const XeenSaveSnapshot &source,bool animated=false,const XeenEventFile *city=nullptr):
  training_test::Fixture(in,source,animated,city,{},{},[&in](auto &f,auto portrait,auto phase){in.assets.drawNpc(f,portrait,phase);}) {
  flow->loadRestDream=[]{return frame();};
  flow->drawTavernArt=[&in](auto &image){in.assets.drawTavern(image);};
  flow->tavernFoodOffer=[&in]{return in.assets.tavernFoodOffer();};
 }
};
XeenEventRecord record(unsigned line,unsigned opcode,std::vector<std::uint8_t> params={}) {
 XeenEventRecord r;r.x=20;r.y=20;r.direction=4;r.line=line;r.opcode=opcode;
 r.parameters=std::move(params);r.lengthField=5+r.parameters.size();r.fileOffset=line*16;return r;
}
XeenSaveSnapshot at(Inputs &in,int x,int y,XeenDirection direction=XeenDirection::North) {
 auto s=in.service();s.camera={28,x,y,direction};
 for(auto &actor:*s.journey->vertigoActors)if(actor.lifecycle==XeenActorLifecycle::Present)actor.activated=true;
 return s;
}
void settle(Fixture &f) {
 for(unsigned n=0;n<10000 && !f.flow->canSave();++n) {
  if(const auto combat=f.flow->encounter()->combat()) {
   if(combat->phase()==XeenCombatPhase::Defeat || combat->phase()==XeenCombatPhase::Failed ||
    combat->phase()==XeenCombatPhase::SupportStopped)throw std::runtime_error("Original route combat cannot continue: "+f.flow->encounter()->notice());
   if(combat->phase()==XeenCombatPhase::PlayerReady)f.act(AttackAction{});
  } else {
   const auto input=f.flow->inputContext(f.flow->frame().presentation());
   const auto pending=f.flow->presentationGeneration()?std::optional<XeenEventExecutionState>{XeenTrainingTestAccess::serviceState(*f.flow)}:std::nullopt;
   if((pending && pending->pendingPresentation->request.response==XeenPresentationResponseRequirement::YesNo) ||
    (input.dialog && input.dialog->key('y') && input.dialog->key('n')))f.act(YesAction{});
   else if((input.dialog && input.dialog->anyKey) || f.w.sessionState().journeyActivity()==XeenJourneyActivity::Event ||
     f.w.sessionState().journeyActivity()==XeenJourneyActivity::Reward)f.act(AcknowledgeAction{});
  }
  f.now+=125;f.flow->beginCycle(++f.cycle);
  if(auto next=f.flow->updatePresentation())f.present(*next);
 }
 if(!f.flow->canSave())throw std::runtime_error("Event settlement bound: "+f.flow->encounter()->notice());
}
void interact(Fixture &f,bool select=false) {
 f.act(InteractionAction{});
 if(select && f.flow->canCancelInteraction())f.act(SelectMemberAction{0});
 settle(f);
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
 check(f.flow->canSave() && before==XeenSaveFormat::encode(f.snapshot()),"Cancelled candidate published rewards/object");
 f.flow->handle(SelectMemberAction{0},selectorInput,selector);
 check(before==XeenSaveFormat::encode(f.snapshot()),"Retired selector published staged rewards");
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
 auto turned=saved;turned.objectOverrides[0].flipped=!bool(turned.objectOverrides[0].flipped);Fixture turnedRestore(in,turned,false,&city);
 check(XeenSaveFormat::encode(turnedRestore.snapshot())==XeenSaveFormat::encode(turned),"Loading reset a retained object flip");
 f.w.discardMapCache();check(f.w.effectiveObject({28,0}).x==20,"Map cache discarded durable object move");
 for(unsigned field=0;field<7;++field) {
  Fixture tampered(in,s,false,&city);bool injected=false;
  XeenCityEventTestAccess::composer(*tampered.flow,[&](auto &world,const auto &party,const auto &,auto,auto) {
   injected=true;auto &p=const_cast<XeenPartyState &>(party);
   // Write/restore attacks must remain observable even with identical values.
   if(field==0){auto &a=p.roster.at(0).awards[6];const auto before=std::uint8_t(a);a=1;a=before;}
   if(field==1){auto &g=p.monsterTreasure->gold;const auto before=std::uint32_t(g);++g;g=before;}
   if(field==2){auto &o=const_cast<XeenObjectOverride &>(world.sessionState().objectOverrides().front());const auto x=int(o.x);o.x=0;o.x=x;}
   if(field==3){auto &c=p.roster.at(0).currentHp;const auto hp=int(c);++c;c=hp;}
   if(field==4){auto &xp=const_cast<XeenCombatInputs &>(*p.roster.combatInputs(0)).experience;const auto value=std::uint32_t(xp);++xp;xp=value;}
   if(field==5){auto &id=p.roster.at(0).weapons[0].id;const auto value=std::uint8_t(id);id=2;id=value;}
   if(field==6){auto &rng=const_cast<XeenJourneyRandomState &>(*world.sessionState().journeyRandom());const auto value=std::uint32_t(rng.state);++rng.state;rng.state=value;}
   return XeenEventFlow::Composition{frame(),false};
  });
  tampered.act(InteractionAction{});save_test::rejects([&]{tampered.act(SelectMemberAction{0});settle(tampered);});
  check(injected && !tampered.flow->canSave() && !tampered.p.questFlags.isSet(29) &&
   tampered.p.monsterTreasure->gold==gold && tampered.w.sessionState().objectOverrides().empty(),"Stale candidate published world rewards");
 }
 Fixture retry(in,s,false,&city);unsigned attempts=0;
 XeenCityEventTestAccess::composer(*retry.flow,[&](auto &,const auto &,const auto &,auto,auto) {
  if(++attempts==1)throw std::runtime_error("Synthetic recoverable presentation failure");
  return XeenEventFlow::Composition{frame(),false};
 });
 retry.act(InteractionAction{});retry.act(SelectMemberAction{0});
 check(!retry.flow->canSave() && retry.p.monsterTreasure->gold==gold && !retry.p.questFlags.isSet(29),"Failed presentation lost atomicity");
 settle(retry);check(attempts==2 && retry.p.monsterTreasure->gold==gold+4000 &&
  retry.p.roster.combatInputs(0)->experience==xp+5000,"Presentation retry repeated or lost fixed rewards");
 const auto once=XeenSaveFormat::encode(retry.snapshot());retry.act(AcknowledgeAction{});retry.act(AcknowledgeAction{});
 check(once==XeenSaveFormat::encode(retry.snapshot()),"Repeated receipt regranted fixed rewards");
 // Unsupported dependencies in called scripts must refuse the whole group.
 for(const auto suffix:std::vector<XeenEventRecord>{record(9,0x14,{101,1,0}),record(9,0x0c,{34,0,0,0,0,0,0}),
   record(9,0x0c,{16,1,0,0,0,16,1,0,0,0}),record(9,0x0c,{20,0,0,0}),record(9,0x0f,{7}),record(9,0x0b,{255,0,0})}) {
  city.records={record(0,0x0c,{0,0,15,6}),record(1,0x0b,{0,20,20}),record(2,0x19,{20,20,9}),record(3,0x12),suffix,record(10,0x1a)};
  Fixture refused(in,s,false,&city);const auto preimage=XeenSaveFormat::encode(refused.snapshot());
  interact(refused);check(preimage==XeenSaveFormat::encode(refused.snapshot()),"Unsupported dependent call published prefix");
 }
 auto candidate=f.w.transitionCandidate();
 XeenPartyState grants;candidate->copyEventParty(f.p,grants);
 const auto selected=grants.party.activeRosterIds()[0],other=grants.party.activeRosterIds()[1];
 grants.roster.at(selected).awards[127]=15;
 candidate->applyEventGrant(grants,15,81,0,false,true);
 check(grants.roster.at(selected).awards[127]==1 && grants.roster.at(other).awards[127]==f.p.roster.at(other).awards[127],"Selected award count/remapping failed");
 candidate->applyEventGrant(grants,15,81,0,false,false);
 check(grants.roster.at(selected).awards[127]==0,"Award clear failed");
 const auto beforeXp=std::uint32_t(grants.roster.combatInputs(selected)->experience);
 candidate->applyEventGrant(grants,16,0xffffffffu,0,false,true);
 check(grants.roster.combatInputs(selected)->experience==std::uint32_t(beforeXp-1),"XP arithmetic did not wrap as uint32");
 const auto beforeGold=std::uint32_t(grants.monsterTreasure->gold);
 candidate->applyEventGrant(grants,34,0xffffffffu,0,true,true);
 check(grants.monsterTreasure->gold==std::uint32_t(beforeGold-1),"All-member currency applied more than once or saturated");
 const auto original=candidate->effectiveObject({28,0});
 candidate->applyMoveObject({28,0},original.x,original.y);const bool flip=candidate->effectiveObjectFlip({28,0});
 candidate->applyMoveObject({28,0},original.x,original.y);
 check(candidate->effectiveObjectFlip({28,0})!=flip,"Same-position MoveObj did not toggle flip");
 candidate->applyMoveObject({28,0},-128,-1);check(candidate->isObjectDisabled({28,0}),"Signed off-map object stayed active");
 const auto resolver=XeenObjectVisualResolver::load(in.assets);
 XeenIndoorScene().build(*candidate,s.camera,&resolver,nullptr,0);
 check(candidate->effectiveObjectFlip({28,0})==resolver.resolve(candidate->objectFile(28),0,s.camera.direction,0).horizontalFlip,
  "Scene draw did not apply reference flip reset to off-screen object");
 auto malformed=saved;malformed.objectOverrides[0].id.recordIndex=99999;
 save_test::rejects([&]{Fixture invalid(in,malformed,false,&city);});
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
 for(bool flag:{false,true}) {
  stage=flag?"well flag 9 on":"well flag 9 off";
  auto s=at(in,14,17);s.gameFlags[9]=flag;Fixture well(in,s);const auto before=XeenSaveFormat::encode(well.snapshot());
  unsigned errors=0;well.flow->reportManual=[&](const auto &r){if(std::holds_alternative<XeenEventExecutionError>(r))++errors;};
  interact(well);check(errors==1 && before==XeenSaveFormat::encode(well.snapshot()) &&
   well.flow->encounter()->notice().find("not supported yet")!=std::string::npos,"Well branch did not visibly refuse before every effect");
 }
 stage="proof";auto s=at(in,9,22);Fixture proof(in,s);const auto rng=proof.w.sessionState().journeyRandom();
 proof.act(InteractionAction{});check(proof.flow->canCancelInteraction(),"Proof WhoWill is absent");
 proof.act(CancelInteractionAction{});check(!proof.f.isSet(9),"Cancelled proof changed flag 9");
 interact(proof,true);check(proof.f.isSet(9) && proof.w.effectiveObject({28,0}).x==9 && proof.w.effectiveObject({28,0}).y==22 &&
  proof.w.sessionState().journeyRandom()==rng,"Original proof flag/object or fixed RNG diverged");
 stage="Gunther";auto next=proof.snapshot();next.camera={28,14,5,XeenDirection::West};Fixture gunther(in,next);
 const auto gold=gunther.p.monsterTreasure->gold,gems=gunther.p.monsterTreasure->gems;
 std::array<std::uint32_t,30> xp{};for(unsigned owner=0;owner<30;++owner)xp[owner]=gunther.p.roster.combatInputs(owner)->experience;
 interact(gunther);check(gunther.f.isSet(10) && gunther.p.monsterTreasure->gold==gold+4000 && gunther.p.monsterTreasure->gems==gems+50 &&
  !gunther.p.questFlags.isSet(29) && gunther.p.questFlags.isSet(1),"Original Gunther rewards/next quest diverged");
 for(auto owner:gunther.p.party.activeRosterIds())check(gunther.p.roster.at(owner).awards[6]==1 &&
  gunther.p.roster.combatInputs(owner)->experience==xp[owner]+5000,"Original Gunther XP/award lost member");
 const auto awarded=gunther.snapshot();interact(gunther);
 check(XeenSaveFormat::encode(gunther.snapshot())==XeenSaveFormat::encode(awarded),"Repeated Gunther regranted reward or erased next quest");
 Fixture reload(in,awarded);interact(reload);
 check(XeenSaveFormat::encode(reload.snapshot())==XeenSaveFormat::encode(awarded),"Reload replayed Gunther rewards");
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
   f.w.sessionState().journeyRandom()==oracle.w.sessionState().journeyRandom(),"Reward page published candidate early");
  unsigned attempts=0;
  XeenCityEventTestAccess::composer(*f.flow,[&](auto &,const auto &,const auto &,auto,auto) {
   if(++attempts==1)throw std::runtime_error("Recoverable generated reward composition failure");
   return XeenEventFlow::Composition{frame(),false};
  });
  settle(f);
  check(presentationAttempts>=2 && attempts==2 && f.f.isSet(199) && f.w.effectiveObject({28,8}).x==20 &&
   f.w.sessionState().journeyRandom()==random.continuation(),"Generated reward retry rerolled or lost world group");
  check(f.p.monsterTreasure->gold==oldGold+(full?0:*gold) && f.p.monsterTreasure->gems==oldGems+(full?0:5),"Ordered currency/loss differs");
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
 city.records.push_back(record(4,0x14,{101,1,0}));city.records[3]=record(3,0x19,{20,20,4});city.records.push_back(record(5,0x1a));
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
  check(once==XeenSaveFormat::encode(f.snapshot()),"Original tree repeated payment");
  Fixture reload(in,f.snapshot());interact(reload);
  check(once==XeenSaveFormat::encode(reload.snapshot()),"Restored tree repeated payment");
 }
 check(paid.count()==26 && paid[57] && paid[58] && paid[232] && paid[255],"Original tree family incomplete");
}
XeenSaveSnapshot fresh(Inputs &in) {
 auto party=XeenPartyLoader().loadFromResources(in.chr,in.pty);
 auto camera=XeenCharacterFormat::parsePartyLocation(in.pty);XeenGameFlags flags;
 XeenWorld world(in.mapLoader(),in.objectLoader());
 XeenJourneySetup setup{in.chr,XeenGameplayContextFormat::parse(in.pty),in.statistics,in.city,7,in.regional()};
 setup.prepared=false;setup.purse=XeenCharacterFormat::parseMonsterPurse(in.pty);
 setup.bank=XeenCharacterFormat::parseBankBalances(in.pty);setup.regionalRecovery=XeenQuestFlagFormat::parseRegionalRecovery(in.pty);
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
void originalJourney(Inputs &in) {
 stage="New Game route";
 Fixture f(in,fresh(in));walk(f,14,5);face(f,XeenDirection::West);interact(f);
 check(f.p.questFlags.isSet(29),"New Game Gunther quest missing");
 walk(f,8,18);interact(f);walk(f,9,22);interact(f,true);
 check(f.f.isSet(9),"New Game proof did not advance Joe quest");
 walk(f,14,5);face(f,XeenDirection::West);const auto gold=f.p.monsterTreasure->gold;
 interact(f);check(f.f.isSet(10) && f.p.monsterTreasure->gold==gold+4000 && f.p.questFlags.isSet(1),"New Game Gunther reward missing");
 const auto awarded=XeenSaveFormat::encode(f.snapshot());interact(f);
 check(awarded==XeenSaveFormat::encode(f.snapshot()),"New Game repeat Gunther regranted rewards");
 stage="New Game crate";walk(f,9,28);
 check(f.w.effectiveObject({28,8}).resourceId==95 && f.w.effectiveObject({28,8}).x==-128 &&
  f.w.effectiveObject({28,8}).y==-128,"Original open-crate overlay source differs");
 const auto items=itemCount(f.p);interact(f,true);
 check(itemCount(f.p)==items+1 && f.w.effectiveObject({28,8}).x==9 && f.w.effectiveObject({28,8}).y==28,
  "Original crate did not deliver item/reveal open appearance");
 const auto opened=XeenSaveFormat::encode(f.snapshot());interact(f);
 check(opened==XeenSaveFormat::encode(f.snapshot()),"Repeated crate changed inventory/RNG");
 f.act(RestAction{});settle(f);
 walk(f,8,18);interact(f);walk(f,11,18);face(f,XeenDirection::West);interact(f);
 walk(f,15,0);face(f,XeenDirection::South);
 const std::vector<XeenActor> actors=f.w.sessionState().regionalActors(28);interact(f);
 check(f.c.mapId==XeenMapIdentity(23) && f.c.direction==XeenDirection::South,"Original exit facing/transfer failed");
 // Snapshot and restore the earned town effects while outside the town.
 const auto saved=f.snapshot();Fixture reload(in,saved);
 check(XeenSaveFormat::encode(reload.snapshot())==XeenSaveFormat::encode(saved),"Earned route exact reload failed");
 const auto entry=std::find_if(in.mainland.records.begin(),in.mainland.records.end(),[](const auto &r){return r.opcode==0x07 && r.parameters.size()==3 && r.parameters[0]==28;});
 check(entry!=in.mainland.records.end(),"Original town entrance absent");
 walk(reload,entry->x,entry->y);if(entry->direction<4)face(reload,XeenDirection(entry->direction));interact(reload);
 check(reload.c.mapId==XeenMapIdentity(28),"Original town re-entry failed");
 const auto retained=reload.w.sessionState().regionalActors(28);
 check(retained.size()==actors.size(),"Flag 9 town exit changed actor count");
 for(unsigned i=0;i<actors.size();++i)check(xeen_state::sameActor(retained[i],actors[i]),"Flag 9 town exit respawned actors");
 walk(reload,14,5);face(reload,XeenDirection::West);const auto purse=reload.p.monsterTreasure->gold;
 interact(reload);check(reload.p.monsterTreasure->gold==purse && reload.p.questFlags.isSet(1),"Reload/re-entry repeated reward");
 stage="earned crate reload";walk(reload,9,28);const auto openedAgain=XeenSaveFormat::encode(reload.snapshot());interact(reload);
 check(openedAgain==XeenSaveFormat::encode(reload.snapshot()) && reload.w.effectiveObject({28,8}).x==9,
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
 check(afterTavern==XeenSaveFormat::encode(reload.snapshot()),"Tavern minted another tree payment");
 walk(reload,15,0);face(reload,XeenDirection::South);interact(reload);Fixture treeReload(in,reload.snapshot());
 walk(treeReload,entry->x,entry->y);if(entry->direction<4)face(treeReload,XeenDirection(entry->direction));interact(treeReload);
 walk(treeReload,13,2);const auto afterTravel=XeenSaveFormat::encode(treeReload.snapshot());interact(treeReload);
 check(afterTravel==XeenSaveFormat::encode(treeReload.snapshot()),"Departure/reload minted another tree payment");
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
  try {rules(in);treasure(in);originalVisualRegressions(in);originalEvents(in);treeFlags(in);originalJourney(in);}catch(...){std::cout.rdbuf(saved);throw;}
  std::cout.rdbuf(saved);
  std::cout<<"World effects, shared treasure, bounded generation, cancellation, retry and original scripts passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<stage<<": "<<e.what()<<'\n';return 1;}
}
