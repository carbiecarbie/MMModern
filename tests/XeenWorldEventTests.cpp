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
};
}
using namespace training_test;
namespace {
const char *stage="rules";
struct Fixture : training_test::Fixture {
 Fixture(Inputs &in,const XeenSaveSnapshot &source,bool animated=false,const XeenEventFile *city=nullptr):
  training_test::Fixture(in,source,animated,city,{},{},[&in](auto &f,auto portrait,auto phase){in.assets.drawNpc(f,portrait,phase);}) {
  flow->loadRestDream=[]{return frame();};
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
 for(unsigned field=0;field<5;++field) {
  Fixture tampered(in,s,false,&city);bool injected=false;
  XeenCityEventTestAccess::composer(*tampered.flow,[&](auto &world,const auto &party,const auto &,auto,auto) {
   injected=true;auto &p=const_cast<XeenPartyState &>(party);
   // Write/restore attacks must remain observable even with identical values.
   if(field==0){auto &a=p.roster.at(0).awards[6];const auto before=std::uint8_t(a);a=1;a=before;}
   if(field==1){auto &g=p.monsterTreasure->gold;const auto before=std::uint32_t(g);++g;g=before;}
   if(field==2){auto &o=const_cast<XeenObjectOverride &>(world.sessionState().objectOverrides().front());const auto x=int(o.x);o.x=0;o.x=x;}
   if(field==3){auto &c=p.roster.at(0).currentHp;const auto hp=int(c);++c;c=hp;}
   if(field==4){auto &xp=const_cast<XeenCombatInputs &>(*p.roster.combatInputs(0)).experience;const auto value=std::uint32_t(xp);++xp;xp=value;}
   return XeenEventFlow::Composition{frame(),false};
  });
  tampered.act(InteractionAction{});save_test::rejects([&]{tampered.act(SelectMemberAction{0});});
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
 for(const auto suffix:std::vector<XeenEventRecord>{record(9,0x14,{66,1}),record(9,0x0c,{34,0,0,0,0,0,0}),
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
  try {rules(in);originalEvents(in);originalJourney(in);}catch(...){std::cout.rdbuf(saved);throw;}
  std::cout.rdbuf(saved);
  std::cout<<"Part A world effects, awards, fixed rewards, cancellations, dependent refusal and original scripts passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<stage<<": "<<e.what()<<'\n';return 1;}
}
