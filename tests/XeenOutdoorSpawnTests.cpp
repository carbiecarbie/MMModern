#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "formats/xeen/XeenMapFormat.h"
#include <cstdio>
#include <iostream>

using namespace training_test;
namespace {
void inventory(Inputs &in) {
 unsigned uses=0,unbound=0;
 for(unsigned number=1;number<=129;++number) {
  const XeenMapIdentity id{XeenSide::Darkside,std::uint16_t(number)};
  char name[32];std::snprintf(name,sizeof(name),number>=100?"mazex%03u.mob":"maze%04u.mob",number);
  if(!in.assets.hasInitialResource(name,id.side))continue;
  const auto entities=XeenMapFormat::parseMob(in.assets.readInitialResource(name,id.side));
  std::snprintf(name,sizeof(name),number>=100?"mazex%03u.evt":"maze%04u.evt",number);
  if(!in.assets.hasInitialResource(name,id.side))continue;
  const auto records=XeenEventFormat::parse(in.assets.readInitialResource(name,id.side));
  for(const auto &r:records)if(r.opcode==16) {
   ++uses;check(r.parameters.size()==4,"Unexpected original Spawn operands");
   const unsigned slot=r.parameters[0];
   if(slot>=entities.monsters.size())continue; // DOS writes an inert slot.
   const auto &m=entities.monsters[slot];
   if(!m.hasResource()){++unbound;std::cout<<"Unbound "<<number<<" @"<<r.fileOffset<<" slot "<<slot<<'\n';}
  }
 }
 std::cout<<"Darkside Spawn uses="<<uses<<" unbound="<<unbound<<'\n';
 check(uses==965,"Original Darkside Spawn inventory changed; revisit slot bindings");
 check(!unbound,"Darkside Spawn uses an unbound slot: DOS investigation required");
}
void original(Inputs &in,unsigned number,unsigned line,int x,int y) {
 const XeenMapIdentity id{XeenSide::Darkside,std::uint16_t(number)};
 auto snapshot=in.base();snapshot.camera={id,x,y,XeenDirection::North};snapshot.darksideSong=0;
 Fixture f(in,snapshot);
 auto world=f.w.transitionCandidate();XeenPartyState party;world->copyEventParty(f.p,party);
 const auto file=in.events.load(id);
 XeenEventInterpreter vm;
 const auto result=vm.begin(snapshot.camera,party,f.f,*world,[&](auto map){return XeenEventScript(in.events.load(map));},[&](auto map){return in.texts.load(map);},line);
 if(const auto *error=std::get_if<XeenEventExecutionError>(&result))throw std::runtime_error(error->message);
 check(std::holds_alternative<XeenEventExecutionCompleted>(result),"Original Spawn run did not complete");
 check(world->sessionState().journeyRandom()==snapshot.journey->random,"Original Spawn run changed gameplay RNG");
 XeenActorApproach::validateEnvironment(*world,world->sessionState().monsters(id),file);
 check(world->sessionState().monsters(id)[0].x==x && world->sessionState().monsters(id)[0].y==y,
  "Real original Spawn run did not move the parking slot onto the map");
 // A quiet camera away from the original wave isolates restore validation.
 snapshot.camera={id,15,15,XeenDirection::North};
 auto &table=snapshot.map(id).monsters;const auto &actors=world->sessionState().monsters(id);
 table=XeenSaveMonsterTable{std::uint16_t(actors.size()),{}};
 for(const auto &a:actors)table->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});
 const auto bytes=XeenSaveFormat::encode(snapshot);
 Fixture restored(in,XeenSaveFormat::decode(bytes));
 check(bytes==XeenSaveFormat::encode(restored.snapshot()),"Original outdoor wave changed during save/load");
 check(restored.w.sessionState().journeyRandom()==snapshot.journey->random,"Spawn restore replayed RNG");
 std::cout<<"Original Spawn map "<<number<<" validates and restores exactly\n";
}
}
int main(int argc,char **argv) {
 try {
  check(argc==2,"usage: outdoor-spawn-tests <installation>");
  const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation absent");
  Inputs in(*installation);inventory(in);original(in,7,54,6,12);original(in,89,6,3,3);
  return 0;
 }catch(const std::exception &e){std::cerr<<"Outdoor Spawn: "<<e.what()<<'\n';return 1;}
}
