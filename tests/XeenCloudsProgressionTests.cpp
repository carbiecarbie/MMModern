#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "tools/XeenTestSave.h"
#include "TestTreeRemoval.h"
#include <chrono>
#include <filesystem>
#include <iostream>
using namespace training_test;
namespace mmodern {
struct XeenMonsterTestAccess {
 static std::optional<XeenPresentationRequest> request(const XeenEventFlow &flow) {
  if(flow._pending && flow._pending->state.pendingPresentation)return flow._pending->state.pendingPresentation->request;
  return {};
 }
};
}
namespace {
namespace fs=std::filesystem;
struct Directory {
 fs::path path=fs::temp_directory_path()/("mmodern-m90-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 Directory(){check(fs::create_directory(path),"M90 temporary directory");}
 ~Directory(){removeTestTree(path);}
};
void table(Inputs &in,XeenSaveSnapshot &snapshot,XeenMapIdentity map) {
 if(snapshot.find(map) && snapshot.find(map)->monsters)return;
 const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,map),in.statistics);
 auto &saved=snapshot.map(map).monsters;saved.emplace(XeenSaveMonsterTable{std::uint16_t(actors.size()),{}});
 for(const auto &a:actors)saved->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});
}
void press(Fixture &f,PlayerAction action) {f.now+=125;f.act(action);f.flow->completeInputHandoff(f.flow->frame().presentation());}
void tick(Fixture &f) {
 f.now+=125;f.flow->beginCycle(++f.cycle);
 if(auto image=f.flow->updatePresentation())f.present(*image);
 f.flow->completeInputHandoff(f.flow->frame().presentation());
}
std::vector<XeenPresentationRequest> settle(Fixture &f,const std::string &word={}) {
 std::vector<XeenPresentationRequest> requests;
 for(unsigned n=0;n<4000;++n) {
  const auto request=XeenMonsterTestAccess::request(*f.flow);
  if(request) {
   requests.push_back(*request);
   check(request->text.find("not supported")==std::string::npos && request->refusal.find("not supported")==std::string::npos,
    ("M90 unsupported Event: "+request->text+request->refusal).c_str());
   switch(request->response) {
   case XeenPresentationResponseRequirement::Acknowledgment:press(f,AcknowledgeAction{});break;
   case XeenPresentationResponseRequirement::YesNo:press(f,YesAction{});break;
   case XeenPresentationResponseRequirement::CharacterSelection:press(f,SelectMemberAction{0});break;
   case XeenPresentationResponseRequirement::WordInput:
    check(!word.empty(),"Unexpected M90 word prompt");press(f,TextInputAction{word});press(f,DialogKeyAction{13});break;
   default:break;
   }
  } else if(const auto *combat=f.flow->encounter()->combat();combat && combat->phase()==XeenCombatPhase::PlayerReady)press(f,AttackAction{});
  else if(f.flow->presenter().blocksGameplay())press(f,AcknowledgeAction{});
  tick(f);
  check(f.flow->encounter()->notice().find("not supported")==std::string::npos,("M90 unsupported: "+f.flow->encounter()->notice()).c_str());
  if(f.flow->canSave())return requests;
 }
 throw std::runtime_error("M90 Event did not return to Quiet at map "+std::to_string(f.c.mapId.number)+": "+f.flow->encounter()->notice());
}
XeenSaveSnapshot event(Inputs &in,XeenSaveSnapshot snapshot,XeenCamera camera,
 std::vector<XeenPresentationRequest> *requests=nullptr,const std::string &word={}) try {
 snapshot.camera=camera;table(in,snapshot,camera.mapId);
 Fixture f(in,snapshot,false,nullptr,{},{},[&](auto &image,auto portrait,auto phase){in.assets.drawNpc(image,portrait,phase);});
 press(f,InteractionAction{});auto seen=settle(f,word);if(requests)*requests=std::move(seen);
 return f.snapshot();
} catch(const std::exception &e) {
 throw std::runtime_error("Event "+std::to_string(camera.mapId.number)+" ("+std::to_string(camera.x)+","+std::to_string(camera.y)+"): "+e.what());
}
void megacredits(Inputs &in,XeenSaveSnapshot source) {
 auto credit=source;credit.questItems[26]=0;
 credit=event(in,credit,{79,27,22,XeenDirection::South});
 check(credit.questItems[26]==1,"Megacredit WhoWill grant");
 const auto *map=credit.find(79);
 check(map && !map->events.empty(),"Megacredit Remove persists");
 const auto creditWall=[](const XeenSaveSnapshot &snapshot) {
  const auto *state=snapshot.find(79);check(state,"Megacredit map state exists");
  check(state->wallItems.size()==1 && state->wallItems[0].record==9 &&
   state->wallItems[0].x==100 && state->wallItems[0].y==100,
   "Megacredit wall-item record 9 must move to (100,100)");
 };
 creditWall(credit);
 const auto encoded=XeenSaveFormat::encode(credit);
 Fixture restored(in,XeenSaveFormat::decode(encoded));
 check(XeenSaveFormat::encode(restored.snapshot())==encoded,"Megacredit restore must be exact");
 creditWall(restored.snapshot());
 credit=event(in,restored.snapshot(),{79,27,22,XeenDirection::South});
 check(credit.questItems[26]==1,"Removed credit must not be granted again after reload");
 auto s=event(in,source,{12,11,12,XeenDirection::South});
 check(s.journey->purse->gold==0 && s.gameFlags[45] && s.questItems[0]==1,"Newcastle deed costs 50000 gold");
 auto poor=s;poor.questItems[26]=4;std::vector<XeenPresentationRequest> seen;
 poor=event(in,poor,{64,2,7,XeenDirection::North},&seen);
 bool refused=false;for(const auto &r:seen)refused|=r.text.find("enough Megacredits")!=std::string::npos;
 check(refused && poor.questItems[26]==4 && !poor.gameFlags[43],"Payment helper refuses four credits without a wall grant");
 s=event(in,s,{64,2,7,XeenDirection::North});
 check(s.questItems[26]==10 && s.gameFlags[43],"Walls take exactly five credits");
 s=event(in,s,{12,11,12,XeenDirection::South});
 check(s.questItems[5]==1 && s.camera.mapId==72 && s.camera.x==7 && s.camera.y==15,"Walls Taxman stone and teleport");
 s=event(in,s,{64,2,7,XeenDirection::North});
 check(s.questItems[26]==5 && s.gameFlags[44],"Keep takes exactly five credits");
 s=event(in,s,{12,11,12,XeenDirection::South});
 check(s.questItems[6]==1 && s.camera.mapId==73 && s.camera.x==7 && s.camera.y==15,"Keep Taxman stone and teleport");
 s=event(in,s,{64,1,4,XeenDirection::North});
 check(s.questFlags[22] && !s.questItems[27],"Artemus opens Crodo quest");
 s=event(in,s,{64,1,4,XeenDirection::North});
 check(!s.questFlags[22] && s.questItems[27]==1,"Artemus accepts award 13 and grants permit");
 s=event(in,s,{64,2,7,XeenDirection::North});
 check(s.questItems[26]==0 && s.gameFlags[54],"Dungeon takes final five credits");
 s=event(in,s,{12,11,12,XeenDirection::South},&seen);
 bool dungeonDialogue=false;
 for(const auto &r:seen)dungeonDialogue|=r.mapId==12 && r.source.line==42 && r.source.opcode==0x05;
 check(dungeonDialogue,"Third Taxman visit must reach the dungeon-completion dialogue");
 const auto taxmanEvents=in.events.load(12);bool completedVisit=false;
 for(unsigned i=0;i<taxmanEvents.records.size();++i) {
  const auto &r=taxmanEvents.records[i];if(r.x!=11 || r.y!=12 || r.line!=39)continue;
  for(const auto &change:s.map(12).events)completedVisit|=change.record==i && change.opcode==0;
 }
 check(completedVisit,"Dungeon Taxman line 43 disables the first-visit branch");
 check(s.camera.mapId==73 && s.camera.x==7 && s.camera.y==15,"Dungeon Taxman completion and third teleport");
 s=event(in,s,{73,12,14,XeenDirection::North},nullptr,"Laboratory");
 check(s.camera.mapId==71 && s.camera.x==2 && s.camera.y==14,"Laboratory password reaches dungeon");
 s=event(in,s,{71,7,4,XeenDirection::North});
 bool sword=false;for(auto id:s.activeRosterIds)for(const auto &item:s.characters[id].weapons)sword|=item.id==34;
 check(sword,"Newcastle pedestal grants the Slayer Sword");
}
void mainLine(Inputs &in,XeenSaveSnapshot s) {
 s=event(in,s,{45,7,14,XeenDirection::North});
 check(s.questItems[3]==1 && s.journey->purse->gems==0,"Skull sells the High Magic key for 300 gems");
 const auto before=s;
 s=event(in,s,{61,7,6,XeenDirection::North});
 check(s.questFlags[23],"Crodo starts the Lord Xeen quest");
 for(auto id:s.activeRosterIds)check(s.characters[id].awards[13]==1 &&
  s.journey->supplements[id].inputs.experience==before.journey->supplements[id].inputs.experience+1000000,
  "Crodo gives every active member award 13 and one million XP");
 // The four original games draw RNG. Isolate the deterministic trade with their dolls.
 for(unsigned item:{29u,30u,31u,32u})s.questItems[item]=1;
 s=event(in,s,{27,12,29,XeenDirection::North});
 check(s.questItems[28]==1,"Slick gives the Cupie Doll");
 for(unsigned item:{29u,30u,31u,32u})check(s.questItems[item]==0,"Slick consumes each game doll");
 s=event(in,s,{27,16,29,XeenDirection::North});
 check(s.camera.mapId==75 && s.camera.x==6 && s.camera.y==0,"Cupie gate reaches Castle Xeen");
 s=event(in,s,{75,1,2,XeenDirection::West});
 check(s.camera.mapId==76 && s.camera.x==1 && s.camera.y==2 && s.camera.direction==XeenDirection::East,"Castle level 1 stairs");
 s=event(in,s,{76,3,2,XeenDirection::East});
 check(s.camera.mapId==77 && s.camera.x==3 && s.camera.y==2 && s.camera.direction==XeenDirection::West,"Castle level 2 stairs");
 s=event(in,s,{77,9,8,XeenDirection::North});
 check(s.camera.mapId==78 && s.camera.x==9 && s.camera.y==8 && s.camera.direction==XeenDirection::South,"Castle level 3 stairs reach Lord Xeen's map");
}
void winterkill(Inputs &in,XeenSaveSnapshot s) {
 const XeenCamera randon{32,8,13,XeenDirection::North},bell{32,13,1,XeenDirection::North};
 s=event(in,s,randon);
 check(s.questFlags[18] && !s.gameFlags[1],"Randon opens Winterkill quest");
 const auto originalEvents=in.events.load(32);
 bool liveBellPredicate=false;
 for(unsigned i=0;i<originalEvents.records.size();++i) {
  const auto &r=originalEvents.records[i];
  if(r.x!=13 || r.y!=1 || r.line!=5)continue;
  check(r.opcode==0x2a && r.parameters==std::vector<std::uint8_t>{255,27},"Original bell line 5 is IfMapFlag 255");
  for(const auto &change:s.map(32).events)if(change.record==i)
   check(change.opcode==r.opcode,"Bell monster predicate must remain unaltered");
  liveBellPredicate=true;
 }
 check(liveBellPredicate,"Original bell monster predicate exists");
 bool firstWaveAlive=false;
 for(unsigned slot=0;slot<20;++slot)firstWaveAlive|=s.map(32).monsters->slots.at(slot).lifecycle==XeenActorLifecycle::Present;
 check(firstWaveAlive && !s.gameFlags[0],"Original first wave remains before ringing the bell");
 s=event(in,s,bell);check(!s.gameFlags[0],"Live IfMapFlag 255 must not clear the first wave");
 const auto wave=[&](unsigned firstSlot,unsigned firstLine) {
  // maze0032.evt: Randon lines 22-41 bind slots 20-39; 69-88 bind 40-59.
  // Check every original Spawn operand, then its saved presence and restored MOB/MON binding.
  const auto mob=in.maps.loadObjects(in.assets,32);
  Fixture restored(in,s);const auto &actors=restored.w.sessionState().regionalActors(32);
  unsigned count=0;
  for(const auto &r:originalEvents.records) {
   if(r.x!=8 || r.y!=13 || r.direction!=0 || r.line<firstLine || r.line>=firstLine+20)continue;
   const unsigned slot=firstSlot+r.line-firstLine;
   check(r.opcode==0x10 && r.parameters.size()==4 && r.parameters[0]==slot,"Original Randon wave Spawn slot range");
   const auto &saved=s.map(32).monsters->slots.at(slot);
   check(saved.lifecycle==XeenActorLifecycle::Present && saved.x>=0 && saved.x<32 && saved.y>=0 && saved.y<32,
    ("Randon wave slot "+std::to_string(slot)+" must be present before clearing").c_str());
   const auto &actor=actors.at(slot);const auto &binding=mob.entities.monsters.at(slot);
   check(actor.id==XeenMonsterIdentity{32,slot} && actor.original.resourceId==binding.resourceId &&
    actor.original.tableIndex==binding.tableIndex && actor.statistics &&
    actor.statistics->raw==in.statistics.at(binding.resourceId).raw,"Randon wave preserves original MOB slot and MON binding");
   ++count;
  }
  check(count==20,"Each original Randon wave contains twenty Spawn instructions");
 };
 // Original MOB tables with completed fights supplied at the checkpoints:
 // this case exercises Randon and the bell, not the already tested kill rules.
 const auto defeated=[&] {
  for(auto &a:s.map(32).monsters->slots)if(a.lifecycle==XeenActorLifecycle::Present) {
   a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
  }
 };
 defeated();s=event(in,s,bell);check(s.gameFlags[0],"Bell records first cleared wave");
 s=event(in,s,randon);check(s.gameFlags[72],"IfMapFlag 255 admits Randon's second wave");
 wave(20,22);
 defeated();s=event(in,s,bell);check(s.gameFlags[3],"Bell records second cleared wave");
 s=event(in,s,randon);check(s.gameFlags[73],"Randon spawns final wave");
 wave(40,69);
 defeated();s=event(in,s,bell);check(s.gameFlags[8],"Bell records final cleared wave");
 const auto before=s;s=event(in,s,randon);
 check(s.gameFlags[1] && !s.questFlags[18],"Randon completes Winterkill");
 for(auto id:s.activeRosterIds)check(s.characters[id].awards[16]==1 &&
  s.journey->supplements[id].inputs.experience==before.journey->supplements[id].inputs.experience+500000,
  "Randon grants award 16 and 500000 XP");
 const auto after=s;s=event(in,s,randon);
 for(auto id:s.activeRosterIds)check(s.journey->supplements[id].inputs.experience==after.journey->supplements[id].inputs.experience,"Winterkill reward does not repeat");
}
}
int main(int argc,char **argv){try {
 check(argc==2,"usage: clouds-progression <installation>");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation missing");
 Directory directory;Inputs in(*installation);
 const auto preset=[&](const char *name){const auto path=directory.path/(std::string(name)+".mmsave");developer::generateTestSave(*installation,name,path);return XeenSaveFile::read(path);};
 std::cout<<"Megacredits chain\n";megacredits(in,preset("m90-newcastle"));
 std::cout<<"Main line\n";mainLine(in,preset("m90-illusion"));
 std::cout<<"Winterkill\n";winterkill(in,preset("m90-winterkill"));
 std::cout<<"Clouds progression original-data tests passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
